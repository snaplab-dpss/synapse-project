#pragma once

#include <optional>
#include <unordered_set>

#include "../bytes.h"
#include "../primitives/table.h"
#include "../time.h"

namespace sycon {

class SimpleTable;
struct cookie_t {
  SimpleTable *table;
};

typedef uint64_t field_value_t;

struct fields_values_t {
  uint32_t size;
  field_value_t *values;

  fields_values_t();
  fields_values_t(uint32_t _size);
  fields_values_t(fields_values_t &&other);
  fields_values_t(const fields_values_t &other);

  ~fields_values_t();

  bool operator==(const fields_values_t &other);

  struct hash {
    std::size_t operator()(const fields_values_t &k) const;
  };
};

typedef fields_values_t table_key_t;
typedef fields_values_t table_data_t;

std::ostream &operator<<(std::ostream &os,
                         const fields_values_t &fields_values);

struct table_entry_t {
  table_key_t key;
  table_data_t data;

  table_entry_t(const table_key_t &_key);
  table_entry_t(const table_key_t &_key, const table_data_t &_data);

  struct hash {
    std::size_t operator()(const table_entry_t &entry) const;
  };
};

bool operator==(const table_entry_t &lhs, const table_entry_t &rhs);

struct entries_t {
  std::unordered_set<table_entry_t, table_entry_t::hash> entries;

  void clear();
  void set(const entries_t &other);
  void assert_entries() const;

  void add(const table_entry_t &entry);
  void add(const table_key_t &key, const table_data_t &data);
  void add(const table_key_t &key);

  void erase(const table_key_t &key);
  bool contains(const table_key_t &key) const;
};

class SimpleTable : public Table {
 private:
  entries_t added_entries;

  bf_rt_id_t populate_action_id;

  cookie_t cookie;
  std::optional<time_ms_t> timeout;
  std::optional<bfrt::BfRtIdleTmoExpiryCb> callback;

 public:
  SimpleTable(const std::string &_control_name, const std::string &_table_name);
  SimpleTable(const std::string &_control_name, const std::string &_table_name,
              time_ms_t _timeout, const bfrt::BfRtIdleTmoExpiryCb &_callback);

  void enable_expirations();
  void disable_expirations();

  void set(const bytes_t &key_bytes, std::vector<bytes_t> params_bytes);
  void set(const bytes_t &key_bytes);

  void del(const bytes_t &key_bytes);
  void del(const bfrt::BfRtTableKey *_key);

  static std::unique_ptr<SimpleTable> build(const std::string &_control_name,
                                            const std::string &_table_name);
  static std::unique_ptr<SimpleTable> build(
      const std::string &_control_name, const std::string &_table_name,
      time_ms_t _timeout, const bfrt::BfRtIdleTmoExpiryCb &_callback);

 private:
  void key_setup();
  void key_setup(const table_key_t &key_field_values);

  void data_setup();
  void data_setup(const table_data_t &param_field_values);

  table_data_t get(const table_key_t &key_fields_values);

  void add(const table_key_t &key_fields_values,
           const table_data_t &param_fields_values);

  void add(const table_key_t &key_fields_values);

  void mod(const table_key_t &key_fields_values,
           const table_data_t &param_fields_values);

  void del(const table_key_t &key_fields_values);

  void clear();

 public:
  table_key_t key_to_fields_values(const bfrt::BfRtTableKey *key);

  table_data_t data_to_fields_values(const bfrt::BfRtTableData *data);

  static fields_values_t bytes_to_fields_values(
      const bytes_t &values, const std::vector<table_field_t> &fields);

  // one to one relation
  static fields_values_t bytes_to_fields_values(
      const std::vector<bytes_t> &values,
      const std::vector<table_field_t> &fields);
};

}  // namespace sycon