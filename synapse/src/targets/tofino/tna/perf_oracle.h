#pragma once

#include <cstdint>
#include <vector>
#include <optional>

#include "../../../types.h"

namespace tofino {

struct TNAProperties;

struct RecircPortUsage {
  int port;

  // One for each consecutive recirculation.
  // E.g.:
  //    Index 0 contains the fraction of traffic recirculated once.
  //    Index 1 contains the fraction of traffic recirculated twice.
  //    Etc.
  std::vector<hit_rate_t> fractions;

  // Fraction of traffic being steered to another recirculation port.
  // This avoids us hit_rate_t counting the contribution of recirculation
  // traffic to the final throughput estimation.
  hit_rate_t steering_fraction;
};

class PerfOracle {
private:
  int total_ports;
  u64 port_capacity_bps;
  int total_recirc_ports;
  u64 recirc_port_capacity_bps;
  int avg_pkt_bytes;

  std::vector<RecircPortUsage> recirc_ports_usage;
  hit_rate_t non_recirc_traffic;

  u64 throughput_pps;

public:
  PerfOracle(const TNAProperties *properties, int avg_pkt_bytes);
  PerfOracle(const PerfOracle &other);

  void add_recirculated_traffic(int port, int port_recirculations,
                                hit_rate_t fraction,
                                std::optional<int> prev_recirc_port);
  u64 estimate_throughput_pps() const;
  void log_debug() const;

private:
  void steer_recirculation_traffic(int source_port, int destination_port,
                                   hit_rate_t fraction);
  void update_estimate_throughput_pps();
};

} // namespace tofino
