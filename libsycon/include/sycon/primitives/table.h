#pragma once

#include <bf_rt/bf_rt_init.hpp>
#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_table_data.hpp>
#include <bf_rt/bf_rt_table_key.hpp>

#include <iomanip>
#include <memory>
#include <string>
#include <unordered_map>
#include <optional>

#include "../time.h"
#include "../buffer.h"

namespace sycon {

struct table_field_t {
  std::string name;
  bf_rt_id_t id;
  bits_t size;
};

struct table_action_t {
  std::string name;
  bf_rt_id_t action_id;
  std::vector<table_field_t> data_fields;
};

class Table {
protected:
  // Actually, we usually only get around 90% of usage from the dataplane tables.
  // Higher than that and we start getting collisions, and errors trying to insert new entries.
  constexpr const static double CAPACITY_EFFICIENCY{0.9};

  const std::string name;

  const bf_rt_target_t dev_tgt;
  const bfrt::BfRtInfo *info;
  std::shared_ptr<bfrt::BfRtSession> session;

  const bfrt::BfRtTable *table;
  const size_t capacity;

  const std::vector<table_field_t> key_fields;
  const std::vector<table_action_t> actions;
  const std::optional<table_action_t> no_action;

  std::unique_ptr<bfrt::BfRtTableKey> key;
  std::unique_ptr<bfrt::BfRtTableData> data;

  bool time_aware;
  bf_rt_id_t entry_ttl_data_id;
  time_ms_t entry_ttl;

public:
  Table(const std::string &name);
  Table(const Table &other);
  Table(Table &&other) = delete;

  void set_session(const std::shared_ptr<bfrt::BfRtSession> &_session);

  std::string get_name() const;
  std::string get_full_name() const;
  size_t get_capacity() const;
  size_t get_effective_capacity() const;
  size_t get_usage() const;

  const std::vector<table_field_t> &get_key_fields() const;
  const std::vector<table_action_t> &get_actions() const;

  table_field_t get_key_field(const std::string &name) const;
  table_action_t get_action(const std::string &name) const;

  void set_notify_mode(time_ms_t timeout_value, void *cookie, const bfrt::BfRtIdleTmoExpiryCb &callback, bool enable);

  void add_entry(const buffer_t &k);
  void add_entry(const buffer_t &k, const std::string &action_name, const std::vector<buffer_t> &params);
  bool try_add_entry(const buffer_t &k);
  bool try_add_entry(const buffer_t &k, const std::string &action_name, const std::vector<buffer_t> &params);
  void mod_entry(const buffer_t &k);
  void mod_entry(const buffer_t &k, const std::string &action_name, const std::vector<buffer_t> &params);
  void add_or_mod_entry(const buffer_t &k);
  void add_or_mod_entry(const buffer_t &k, const std::string &action_name, const std::vector<buffer_t> &params);
  void del_entry(const buffer_t &k);

  buffer_t get_key_value(const bfrt::BfRtTableKey *key) const;

  void dump_data_fields() const;
  void dump_data_fields(std::ostream &) const;

  void dump() const;
  virtual void dump(std::ostream &) const;

  static void dump_table_names(const bfrt::BfRtInfo *bfrtInfo);

protected:
  void set_key(const buffer_t &k);
  void set_data();
  void set_data(const std::string &action_name, const std::vector<buffer_t> &params);
};

} // namespace sycon