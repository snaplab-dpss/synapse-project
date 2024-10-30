#pragma once

#include <cstdint>
#include <vector>
#include <optional>

#include "../../../types.h"

namespace tofino {

struct TNAProperties;

class PerfOracle {
private:
  // One for each consecutive recirculation.
  // E.g.:
  //    Index 0 contains the fraction of traffic recirculated once.
  //    Index 1 contains the fraction of traffic recirculated twice.
  //    Etc.
  typedef std::vector<hit_rate_t> recirc_port_usage_t;

  int total_ports;
  bps_t port_capacity_bps;
  int total_recirc_ports;
  bps_t recirc_port_capacity_bps;
  int avg_pkt_bytes;

  std::unordered_map<int, hit_rate_t> ports_usage;
  std::vector<recirc_port_usage_t> recirc_ports_usage;
  hit_rate_t controller_port_usage;

public:
  PerfOracle(const TNAProperties *properties, int avg_pkt_bytes);
  PerfOracle(const PerfOracle &other);

  void add_fwd_traffic(int port, hit_rate_t fraction);
  hit_rate_t get_fwd_traffic(int port) const;

  void add_recirculated_traffic(int port, int recirculations,
                                hit_rate_t fraction);
  hit_rate_t get_recirculated_traffic(int port, int recirculations) const;
  pps_t get_recirculated_traffic_pps(int port, int recirculations,
                                     pps_t ingress) const;

  void add_controller_traffic(hit_rate_t fraction);
  hit_rate_t get_controller_traffic() const;

  pps_t get_port_capacity_pps() const;
  bps_t get_port_capacity_bps() const;

  pps_t get_max_input_pps() const;
  bps_t get_max_input_bps() const;

  void debug() const;
};

} // namespace tofino
