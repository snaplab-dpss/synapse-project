#pragma once

#include <bf_rt/bf_rt_init.hpp>
#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_table_data.hpp>
#include <bf_rt/bf_rt_table_key.hpp>
#include <iomanip>
#include <memory>
#include <string>
#include <unordered_map>

#include "../bytes.h"
#include "../time.h"

namespace sycon {

struct table_field_t {
  std::string name;
  bf_rt_id_t id;
  bits_t size;
};

class Table {
 protected:
  bf_rt_target_t dev_tgt;
  const bfrt::BfRtInfo *info;
  std::shared_ptr<bfrt::BfRtSession> session;

  std::string control_name;
  std::string table_name;

  const bfrt::BfRtTable *table;

  std::unique_ptr<bfrt::BfRtTableKey> key;
  std::unique_ptr<bfrt::BfRtTableData> data;

  std::vector<table_field_t> key_fields;
  std::vector<table_field_t> data_fields;

 public:
  Table(const std::string &_control_name, const std::string &_table_name);

 protected:
  std::string append_control_name(const std::string &name) const;

  void init_key();
  void init_key(const std::string &name, bf_rt_id_t *id);
  void init_key(const std::unordered_map<std::string, bf_rt_id_t *> &fields);

  void init_data(const std::string &name, bf_rt_id_t *id);
  void init_data(const std::unordered_map<std::string, bf_rt_id_t *> &fields);

  void init_data_with_action(bf_rt_id_t action_id);

  void init_data_with_action(const std::string &name, bf_rt_id_t action_id,
                             bf_rt_id_t *field_id);

  void init_data_with_actions(
      const std::unordered_map<std::string, std::pair<bf_rt_id_t, bf_rt_id_t *>>
          &fields);
  void init_action(const std::string &name, bf_rt_id_t *action_id);

  void init_actions(
      const std::unordered_map<std::string, bf_rt_id_t *> &actions);

  void set_notify_mode(time_ms_t timeout_value, void *cookie,
                       const bfrt::BfRtIdleTmoExpiryCb &callback, bool enable);

 public:
  size_t get_size() const;
  size_t get_usage() const;

  const std::vector<table_field_t> &get_key_fields() const;
  const std::vector<table_field_t> &get_data_fields() const;

  void dump_data_fields();
  void dump_data_fields(std::ostream &);

  void dump();
  void dump(std::ostream &);

  static void dump_table_names(const bfrt::BfRtInfo *bfrtInfo);
};

}  // namespace sycon