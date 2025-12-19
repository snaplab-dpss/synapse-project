#pragma once

#include <array>
#include <optional>
#include <unordered_set>

#include "synapse_ds.h"
#include "../primitives/table.h"
#include "../primitives/register.h"
#include "../primitives/digest.h"
#include "../config.h"
#include "../constants.h"
#include "../time.h"
#include "../field.h"

namespace sycon {

class IntegerAllocator : public SynapseDS {
private:
  std::unordered_set<u32> free_indexes;
  std::unordered_map<u32, std::unordered_set<std::string>> expirations_per_index;
  u32 tail;

  std::vector<Table> tables;
  Register reg_integer_allocator_head;
  Register reg_integer_allocator_tail;
  Register reg_integer_allocator_indexes;
  Register reg_integer_allocator_pending;
  Digest digest;

  const u32 capacity;

public:
  IntegerAllocator(const std::string &_name, const std::vector<std::string> &_table_names, time_ms_t timeout,
                   const std::string &reg_integer_allocator_head_name, const std::string &reg_integer_allocator_tail_name,
                   const std::string &reg_integer_allocator_indexes_name, const std::string &reg_integer_allocator_pending_name,
                   const std::string &digest_name)
      : SynapseDS(_name), tables(build_tables(_table_names)), reg_integer_allocator_head(reg_integer_allocator_head_name),
        reg_integer_allocator_tail(reg_integer_allocator_tail_name), reg_integer_allocator_indexes(reg_integer_allocator_indexes_name),
        reg_integer_allocator_pending(reg_integer_allocator_pending_name), digest(digest_name),
        capacity(get_capacity(reg_integer_allocator_indexes, reg_integer_allocator_pending, tables)) {
    init_tail();

    // If you pay close attention, you'll see that we leave one index unused (capacity-1).
    // This is because we use a circular buffer approach, and we need a way to distinguish between available and full states.
    for (u32 i = 0; i < tail; i++) {
      free_indexes.insert(i);
      reg_integer_allocator_indexes.set(i, i);
    }

    for (Table &table : tables) {
      table.set_notify_mode(timeout, this, IntegerAllocator::expiration_callback, true);
    }

    digest.register_callback(IntegerAllocator::digest_callback, this);
  }

private:
  void init_tail() {
    tail = capacity - 1;
    reg_integer_allocator_tail.set(0, tail);
  }

  void advance_tail(u32 new_index) {
    LOG_DEBUG("Storing index (%u) at tail (%u) and incrementing tail", new_index, tail);
    reg_integer_allocator_indexes.set(tail, new_index);
    tail = (tail + 1) % capacity;
    reg_integer_allocator_tail.set(0, tail);
  }

  void migrate_index_to_tables(const buffer_t &index) {
    const u32 index_value = index.get();

    free_indexes.erase(index_value);

    for (Table &table : tables) {
      LOG_DEBUG("[%s] Populating with index %s (%u)", table.get_name().c_str(), index.to_string().c_str(), index_value);
      table.add_or_mod_entry(index);
    }

    reg_integer_allocator_pending.set(index_value, 0);
  }

  static void expiration_callback(const bf_rt_target_t &dev_tgt, const bfrt::BfRtTableKey *key, void *cookie) {
    cfg.begin_dataplane_notification_transaction();

    IntegerAllocator *integer_allocator = reinterpret_cast<IntegerAllocator *>(cookie);
    assert(integer_allocator && "Invalid cookie");

    const bfrt::BfRtTable *table;
    bf_status_t status = key->tableGet(&table);
    ASSERT_BF_STATUS(status);

    std::string table_name;
    status = table->tableNameGet(&table_name);
    ASSERT_BF_STATUS(status);

    bool target_table_found = false;
    table_field_t key_field;
    for (const Table &target_table : integer_allocator->tables) {
      if (target_table.get_full_name() == table_name) {
        target_table_found = true;
        assert(target_table.get_key_fields().size() == 1);
        key_field = target_table.get_key_fields()[0];
        break;
      }
    }

    if (!target_table_found) {
      ERROR("Target table %s not found", table_name.c_str());
    }

    u64 key_value;
    status = key->getValue(key_field.id, &key_value);
    ASSERT_BF_STATUS(status);

    const u32 index = static_cast<u32>(key_value);

    integer_allocator->expirations_per_index[index].insert(table_name);
    if (integer_allocator->expirations_per_index[index].size() == integer_allocator->tables.size()) {
      integer_allocator->expirations_per_index.erase(index);
      integer_allocator->free_indexes.insert(index);
      integer_allocator->advance_tail(index);
    }

    cfg.commit_dataplane_notification_transaction();
  }

  static bf_status_t digest_callback(const bf_rt_target_t &bf_rt_tgt, const std::shared_ptr<bfrt::BfRtSession> session,
                                     std::vector<std::unique_ptr<bfrt::BfRtLearnData>> learn_data, bf_rt_learn_msg_hdl *const learn_msg_hdl,
                                     const void *cookie) {
    // Ugh... But what choice do we have? Why the hell have they decided to enforce const on the cookie?
    IntegerAllocator *integer_allocator = const_cast<IntegerAllocator *>(reinterpret_cast<const IntegerAllocator *>(cookie));
    assert(integer_allocator && "Invalid cookie");

    cfg.begin_dataplane_notification_transaction();

    for (const std::unique_ptr<bfrt::BfRtLearnData> &data_entry : learn_data) {
      const buffer_t diggest_buffer = integer_allocator->digest.get_value(data_entry.get());

      const u32 index = diggest_buffer.get();

      LOG_DEBUG("[%s] Digest callback invoked (data=%s index=%u)", integer_allocator->digest.get_name().c_str(),
                diggest_buffer.to_string(true).c_str(), index);

      if (!integer_allocator->free_indexes.contains(index)) {
        continue;
      }

      integer_allocator->migrate_index_to_tables(diggest_buffer);
    }

    integer_allocator->digest.notify_ack(learn_msg_hdl);

    cfg.commit_dataplane_notification_transaction();

    return BF_SUCCESS;
  }

  static std::vector<Table> build_tables(const std::vector<std::string> &table_names) {
    assert(!table_names.empty() && "Table name must not be empty");

    std::vector<Table> tables;
    for (const std::string &table_name : table_names) {
      tables.emplace_back(table_name);
    }

    return tables;
  }

  static u32 get_capacity(const Register &reg_integer_allocator_indexes, const Register &reg_integer_allocator_pending,
                          const std::vector<Table> &tables) {
    const u32 capacity = reg_integer_allocator_indexes.get_capacity();

    assert(reg_integer_allocator_pending.get_capacity() == capacity);
    for (const Table &table : tables) {
      assert(table.get_effective_capacity() >= capacity);
    }

    return capacity;
  }
};

} // namespace sycon