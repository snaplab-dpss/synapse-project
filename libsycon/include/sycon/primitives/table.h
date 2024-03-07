#pragma once

#include <bf_rt/bf_rt_init.hpp>
#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_table_data.hpp>
#include <bf_rt/bf_rt_table_key.hpp>
#include <iomanip>
#include <memory>
#include <string>
#include <unordered_map>

#include "../time.h"

namespace sycon {

struct table_field_t {
  std::string name;
  bf_rt_id_t id;
  bit_len_t size;
};

class Table {
 protected:
  bf_rt_target_t dev_tgt;
  const bfrt::BfRtInfo *info;
  std::shared_ptr<bfrt::BfRtSession> session;

  std::string control;
  std::string name;

  const bfrt::BfRtTable *table;

  std::unique_ptr<bfrt::BfRtTableKey> key;
  std::unique_ptr<bfrt::BfRtTableData> data;

  std::vector<table_field_t> key_fields;
  std::vector<table_field_t> data_fields;

  bf_rt_id_t entry_ttl_data_id;
  bool time_aware;

 public:
  Table(const std::string &_control_name, const std::string &_table_name);

 protected:
  void init_key();
  void init_data();

  void init_key(const std::unordered_map<std::string, bf_rt_id_t *> &fields);
  void init_data(const std::unordered_map<std::string, bf_rt_id_t *> &fields);

  void init_data_with_action(bf_rt_id_t action_id);

  void init_data_with_action(const std::string &name, bf_rt_id_t action_id,
                             bf_rt_id_t *field_id);

  void init_data_with_actions(
      const std::unordered_map<std::string, std::pair<bf_rt_id_t, bf_rt_id_t *>>
          &fields);
  void init_action(const std::string &name, bf_rt_id_t *action_id);
  void init_action(bf_rt_id_t *action_id);

  void init_actions(
      const std::unordered_map<std::string, bf_rt_id_t *> &actions);

  void set_notify_mode(time_ms_t timeout_value, void *cookie,
                       const bfrt::BfRtIdleTmoExpiryCb &callback, bool enable);

 public:
  void set_session(const std::shared_ptr<bfrt::BfRtSession> &_session);

  std::string get_name() const;

  size_t get_size() const;
  size_t get_usage() const;

  const std::vector<table_field_t> &get_key_fields() const;
  const std::vector<table_field_t> &get_data_fields() const;

  void dump_data_fields() const;
  void dump_data_fields(std::ostream &) const;

  void dump() const;
  void dump(std::ostream &) const;

  static void dump_table_names(const bfrt::BfRtInfo *bfrtInfo);
  static std::string append_control(const std::string &control,
                                    const std::string &name);
};

}  // namespace sycon