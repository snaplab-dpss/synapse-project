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
// themselves. The key has expired once the copy that reports is joined by every other copy being
// idle right now. The reporting copy is not asked: its report is the fact, even if traffic has
// resumed by the time it is delivered (a flow idle for a timeout expired, as in the C, and is
// re-created by its next packet), and the other structures of the same flow act on their own
// reports, so refuting this one would leave the flow half-installed.
inline bool expired_everywhere(std::vector<Table> &tables, const buffer_t &k, const std::string &reporting_table) {
  for (Table &table : tables) {
    if (table.get_full_name() != reporting_table && table.get_entry_ttl(k) > 0) {
      return false;
    }
  }
  return true;
}

} // namespace sycon