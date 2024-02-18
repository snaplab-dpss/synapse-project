#pragma once

#include <bf_rt/bf_rt_init.hpp>
#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_table_data.hpp>
#include <bf_rt/bf_rt_table_key.hpp>
#include <iomanip>
#include <memory>
#include <string>
#include <unordered_map>

#include "../config.h"
#include "../log.h"

namespace sycon {

class Table {
 protected:
  std::string table_name;
  const bfrt::BfRtTable *table;

  std::unique_ptr<bfrt::BfRtTableKey> key;
  std::unique_ptr<bfrt::BfRtTableData> data;

 protected:
  Table(const std::string &_table_name)
      : table_name(_table_name), table(nullptr) {
    assert(cfg.info);
    assert(cfg.session);

    bf_status_t bf_status = cfg.info->bfrtTableFromNameGet(table_name, &table);
    ASSERT_BF_STATUS(bf_status)

    // Allocate key and data once, and use reset across different uses
    bf_status = table->keyAllocate(&key);
    ASSERT_BF_STATUS(bf_status)

    bf_status = table->dataAllocate(&data);
    ASSERT_BF_STATUS(bf_status)
  }

  void init_key(std::unordered_map<std::string, bf_rt_id_t *> fields) {
    for (const auto &field : fields) {
      bf_status_t bf_status = table->keyFieldIdGet(field.first, field.second);
      ASSERT_BF_STATUS(bf_status)
    }
  }

  void init_data(std::unordered_map<std::string, bf_rt_id_t *> fields) {
    for (const auto &field : fields) {
      bf_status_t bf_status = table->dataFieldIdGet(field.first, field.second);
      ASSERT_BF_STATUS(bf_status)
    }
  }

  void init_data_with_actions(
      std::unordered_map<std::string, std::pair<bf_rt_id_t, bf_rt_id_t *>>
          fields) {
    for (const auto &field : fields) {
      bf_status_t bf_status = table->dataFieldIdGet(
          field.first, field.second.first, field.second.second);
      ASSERT_BF_STATUS(bf_status)
    }
  }

  void init_actions(std::unordered_map<std::string, bf_rt_id_t *> actions) {
    for (const auto &action : actions) {
      bf_status_t bf_status = table->actionIdGet(action.first, action.second);
      ASSERT_BF_STATUS(bf_status)
    }
  }

  size_t get_size() const {
    size_t size;
    auto bf_status = table->tableSizeGet(*cfg.session, cfg.dev_tgt, &size);
    ASSERT_BF_STATUS(bf_status)
    return size;
  }

  size_t get_usage() const {
    uint32_t usage;
    auto bf_status = table->tableUsageGet(
        *cfg.session, cfg.dev_tgt,
        bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW, &usage);
    ASSERT_BF_STATUS(bf_status)
    return usage;
  }

 public:
  void dump();
  void dump(std::ostream &);

  static void dump_table_names(const bfrt::BfRtInfo *bfrtInfo);

 private:
  void dump_data_fields();
  void dump_data_fields(std::ostream &);
};
};  // namespace sycon