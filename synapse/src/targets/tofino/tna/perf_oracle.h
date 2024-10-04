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
  // traffic to the final tput estimation.
  hit_rate_t steering_fraction;
};

class PerfOracle {
private:
  int total_ports;
  bps_t port_capacity_bps;
  int total_recirc_ports;
  bps_t recirc_port_capacity_bps;
  int avg_pkt_bytes;

  std::vector<RecircPortUsage> recirc_ports_usage;
  hit_rate_t non_recirc_traffic;

  pps_t tput_pps;

public:
  PerfOracle(const TNAProperties *properties, int avg_pkt_bytes);
  PerfOracle(const PerfOracle &other);

  void add_recirculated_traffic(int port, int port_recirculations,
                                hit_rate_t fraction,
                                std::optional<int> prev_recirc_port);
  pps_t estimate_tput_pps() const;

  pps_t get_port_capacity_pps() const;
  bps_t get_port_capacity_bps() const;

  pps_t get_max_input_pps() const;
  bps_t get_max_input_bps() const;

  void log_debug() const;

private:
  void steer_recirculation_traffic(int source_port, int destination_port,
                                   hit_rate_t fraction);
  void update_tput_estimate();
};

} // namespace tofino
