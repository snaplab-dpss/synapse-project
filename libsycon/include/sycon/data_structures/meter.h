#pragma once

#include <unordered_set>

#include "synapse_ds.h"
#include "../config.h"
#include "../primitives/table.h"
#include "../time.h"

namespace sycon {

// A per-key token bucket implemented by a TNA DirectMeter: the bucket is the table entry, so it is
// born with the entry and dies with it. The dataplane charges and checks it inside the same table
// apply that answers whether the key is tracked at all, which is why there is no read operation
// here -- the controller only ever starts tracking a key, or stops.
class Meter : public SynapseDS {
private:
  std::unordered_set<buffer_t, buffer_hash_t> tracked;

  Table table;
  const meter_spec_t spec;

public:
  Meter(const std::string &_name, const std::string &table_name, u64 rate, u64 burst, std::optional<time_ms_t> timeout = std::nullopt)
      : SynapseDS(_name), table(table_name), spec({rate, burst}) {
    if (timeout.has_value()) {
      table.set_notify_mode(timeout.value(), this, Meter::expiration_callback, true);
    }
  }

  bool is_tracking(const buffer_t &k) const { return tracked.find(k) != tracked.end(); }

  // The entry carries the bucket's rate and burst, and the hardware starts it full: the first
  // packet of a flow therefore passes uncharged, which the C charges instead.
  bool trace(const buffer_t &k) {
    if (table.get_usage() >= table.get_effective_capacity()) {
      LOG_DEBUG("WARNING: Attempted to track a key, but the meter table is full");
      return false;
    }

    const table_action_t &action = table.get_actions().at(0);
    table.add_entry(k, action.name, {}, spec);
    tracked.insert(k);

    return true;
  }

  void del(const buffer_t &k) {
    if (tracked.find(k) == tracked.end()) {
      return;
    }

    table.del_entry(k);
    tracked.erase(k);
  }

  void dump() const {
    std::stringstream ss;
    dump(ss);
    LOG("%s", ss.str().c_str());
  }

  void dump(std::ostream &os) const {
    os << "================================================\n";
    os << "Meter tracking " << tracked.size() << " keys\n";
    os << "================================================\n";
    table.dump(os);
  }

private:
  static void expiration_callback(const bf_rt_target_t &dev_tgt, const bfrt::BfRtTableKey *key, void *cookie) {
    cfg.begin_dataplane_notification_transaction();

    Meter *meter = reinterpret_cast<Meter *>(cookie);
    assert(meter && "Invalid cookie");

    const buffer_t key_buffer = meter->table.get_key_value(key);
    meter->del(key_buffer);

    cfg.commit_dataplane_notification_transaction();
  }
};

} // namespace sycon
