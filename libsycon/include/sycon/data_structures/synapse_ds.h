#pragma once

#include <string>
#include <vector>

#include "../primitives/table.h"

namespace sycon {

class SynapseDS {
protected:
  const std::string name;

public:
  SynapseDS(const std::string _name) : name(_name) {}

  virtual ~SynapseDS() = default;
};

// A key kept as one copy per lookup site is refreshed only at the site a packet takes, so one
// copy expiring says nothing about the others: they may have been hit since they last expired
// themselves. The key has expired only once every copy is idle right now.
inline bool expired_everywhere(std::vector<Table> &tables, const buffer_t &k) {
  for (Table &table : tables) {
    if (table.get_entry_ttl(k) > 0) {
      return false;
    }
  }
  return true;
}

} // namespace sycon