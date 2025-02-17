#pragma once

#include <bf_rt/bf_rt_learn.hpp>

#include <array>
#include <optional>
#include <unordered_set>
#include <vector>

#include "../primitives/register.h"
#include "table.h"

namespace sycon {

template <size_t K, size_t V> class TransientCachedTableMap : public Table<K, V> {
  static_assert(K > 0);
  static_assert(V > 0);

  struct digest_t {
    u64 cache_index;
    table_key_t<K> key;
    table_value_t<V> value;
  };

private:
  PrimitiveRegister cache_timer;

  std::string digest_control_name;
  std::string digest_name;

  const bfrt::BfRtLearn *learn_obj;
  std::vector<bf_rt_id_t> learn_obj_fields_ids;

public:
  TransientCachedTableMap(const std::string &_control_name, const std::string &_table_name, time_ms_t _timeout,
                          const std::string &_cache_timer_name, const std::string &_digest_control_name, const std::string &_digest_name)
      : Table<K, V>(_control_name, _table_name, _timeout), cache_timer(_control_name, _cache_timer_name),
        digest_control_name(_digest_control_name), digest_name(_digest_name) {
    init_digest();
    register_digest_callback();
  }

private:
  void init_digest() {
    bf_status_t bf_status = cfg.info->bfrtLearnFromNameGet(PrimitiveTable::append_control(digest_control_name, digest_name), &learn_obj);
    ASSERT_BF_STATUS(bf_status)

    bf_status = learn_obj->learnFieldIdListGet(&learn_obj_fields_ids);
    ASSERT_BF_STATUS(bf_status)

    if (learn_obj_fields_ids.size() != K + V + 1) {
      ERROR("Digest learn object should be composed of a cache index value, "
            "followed by the same keys and values stored on the table map (has "
            "%lu total fields)",
            learn_obj_fields_ids.size());
    }
  }

  void register_digest_callback() {
    bf_status_t bf_status = learn_obj->bfRtLearnCallbackRegister(cfg.session, cfg.dev_tgt, internal_digest_callback, (void *)this);
    ASSERT_BF_STATUS(bf_status)
  }

  digest_t build_digest(const bfrt::BfRtLearnData *data) const {
    digest_t digest;

    assert(1 + K + V == learn_obj_fields_ids.size());

    bf_rt_id_t id         = learn_obj_fields_ids[0];
    bf_status_t bf_status = data->getValue(id, &digest.cache_index);
    ASSERT_BF_STATUS(bf_status)

    for (size_t i = 1; i < learn_obj_fields_ids.size(); i++) {
      id = learn_obj_fields_ids[i];

      if (i - 1 < K) {
        bf_status = data->getValue(id, &digest.key.values[i - 1]);
      } else {
        assert(i - K - 1 < V);
        bf_status = data->getValue(id, &digest.value.values[i - K - 1]);
      }

      ASSERT_BF_STATUS(bf_status)
    }

    return digest;
  }

  void migrate_entry(const digest_t &digest) {
    // Note: there's no worry here about atomicity. This is all done inside a
    // transaction, so the register and table updates are actually seen at the
    // same time.

    // Expire the cache entry (allows other flows to use it). There's no need to
    // change the key and value registers, just expiring their entries is
    // enough, they can have stale data as long as it's expired.
    cache_timer.set(digest.cache_index, 0);

    // Add entry to the table.
    Table<K, V>::put(digest.key, digest.value);
  }

  static bf_status_t internal_digest_callback(const bf_rt_target_t &bf_rt_tgt, const std::shared_ptr<bfrt::BfRtSession> session,
                                              std::vector<std::unique_ptr<bfrt::BfRtLearnData>> data,
                                              bf_rt_learn_msg_hdl *const learn_msg_hdl, const void *cookie) {
    DEBUG("RECEIVED %lu DIGESTS!", data.size());

    TransientCachedTableMap *tctm = (TransientCachedTableMap *)(cookie);

    cfg.begin_transaction();
    for (const auto &d : data) {
      tctm->migrate_entry(tctm->build_digest(d.get()));
    }
    cfg.end_transaction();

    // Don't forget to ACK back! This tells the driver to release the allocated
    // resources.
    bf_status_t ack_status = tctm->learn_obj->bfRtLearnNotifyAck(session, learn_msg_hdl);
    ASSERT_BF_STATUS(ack_status)

    return BF_SUCCESS;
  }
};

} // namespace sycon