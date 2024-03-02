#pragma once

#include <array>
#include <optional>
#include <unordered_set>

#include "table_map.h"

namespace sycon {

template <size_t K, size_t V>
class CachedTableMap : public TableMap<K, V> {
  static_assert(K > 0);
  static_assert(V > 0);

 public:
  CachedTableMap(const std::string &_control_name,
                 const std::string &_table_name, time_ms_t _timeout)
      : TableMap<K, V>(_control_name, _table_name, _timeout) {}
};

}  // namespace sycon