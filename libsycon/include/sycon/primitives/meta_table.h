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

class MetaTable {
protected:
  const bf_rt_target_t dev_tgt;
  const bfrt::BfRtInfo *info;
  std::shared_ptr<bfrt::BfRtSession> session;

  const std::string control;
  const std::string name;
  const bfrt::BfRtTable *table;
  const size_t capacity;

  std::unique_ptr<bfrt::BfRtTableKey> key;
  std::unique_ptr<bfrt::BfRtTableData> data;

public:
  MetaTable(const std::string &_control_name, const std::string &_table_name);
  MetaTable(const MetaTable &other);
  MetaTable(MetaTable &&other) = delete;

  void set_session(const std::shared_ptr<bfrt::BfRtSession> &_session);

  const std::string &get_name() const;
  size_t get_capacity() const;
  size_t get_usage() const;

  void dump() const;
  virtual void dump(std::ostream &) const;

protected:
  void init_key(const std::unordered_map<std::string, bf_rt_id_t *> &fields);
  void init_data(const std::unordered_map<std::string, bf_rt_id_t *> &fields);

  void init_action(const std::string &name, bf_rt_id_t *action_id);
  void init_action(bf_rt_id_t *action_id);
  void init_actions(const std::unordered_map<std::string, bf_rt_id_t *> &actions);
};

} // namespace sycon