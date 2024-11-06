#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <map>
#include <toml++/toml.hpp>

#include "../types.h"

// Depth values start from 1.
// A depth 0 value would be global, and does not come from recirculation.
// Depth 1 would be the first recirculation, depth 2 the second, etc.
typedef std::pair<int, int> rport_depth_pair_t;

struct port_ingress_t {
  hit_rate_t global;
  hit_rate_t controller;
  std::map<rport_depth_pair_t, hit_rate_t> recirc;

  port_ingress_t();

  port_ingress_t(const port_ingress_t &other);
  port_ingress_t(port_ingress_t &&other);

  port_ingress_t &operator=(const port_ingress_t &other);
  port_ingress_t &operator+=(const port_ingress_t &other);

  hit_rate_t get_total_hr() const;
  int get_max_recirc_depth() const;
  hit_rate_t get_hr_at_recirc_depth(int depth) const;
};

std::ostream &operator<<(std::ostream &os, const port_ingress_t &ingress);

class PerfOracle {
private:
  std::vector<bps_t> front_panel_ports_capacities;
  std::vector<bps_t> recirculation_ports_capacities;
  pps_t controller_capacity;
  int avg_pkt_bytes;

  // All traffic is either (1) forwarded or (2) dropped. Even controller traffic
  // is, sooner or later, forwarded throught the switch. As such, unaccounted
  // ingres is all traffic for which no decisions of forwarding or dropping were
  // made yet.
  hit_rate_t unaccounted_ingress;

  // Typical front-panel ports on the switch.
  std::unordered_map<int, port_ingress_t> ports_ingress;

  // Recirculation ports. These don't count towards the accounted traffic. They
  // serve only to calculate the throughput bottlenecks inside the switch.
  std::unordered_map<int, port_ingress_t> recirc_ports_ingress;

  // Similarly to the recirculation ports, this serves only to calculate the
  // controller bottleneck, and does not count towards the egress traffic.
  port_ingress_t controller_ingress;

  // Dropped traffic is important for the final egress throughput calculation.
  hit_rate_t dropped_ingress;

public:
  PerfOracle(const toml::table &config, int avg_pkt_bytes);

  PerfOracle(const PerfOracle &other);
  PerfOracle(PerfOracle &&other);

  PerfOracle &operator=(const PerfOracle &other);

  void add_fwd_traffic(int port, const port_ingress_t &ingress);
  void add_fwd_traffic(int port, hit_rate_t hr);

  void add_recirculated_traffic(int port, const port_ingress_t &ingress);
  void add_recirculated_traffic(int port, hit_rate_t hr);

  void add_controller_traffic(const port_ingress_t &ingress);
  void add_controller_traffic(hit_rate_t hr);

  void add_dropped_traffic(hit_rate_t hr);

  pps_t get_max_input_pps() const;
  bps_t get_max_input_bps() const;

  pps_t estimate_tput(pps_t ingress) const;

  void debug() const;

private:
  std::vector<pps_t> get_recirculated_egress(int port, pps_t ingress) const;
};
