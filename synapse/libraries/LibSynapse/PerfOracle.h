#pragma once

#include <vector>
#include <optional>
#include <map>
#include <unordered_map>

#include <LibCore/Types.h>
#include <LibSynapse/Target.h>

namespace LibSynapse {

// Depth values start from 0.
// Depth 0 would be coming from the first recirculation, depth 1 the second, etc.
using recirculation_depth_t = u8;

struct port_ingress_t {
  hit_rate_t global;
  hit_rate_t controller;
  std::map<recirculation_depth_t, hit_rate_t> recirc;

  port_ingress_t();

  port_ingress_t(const port_ingress_t &other);
  port_ingress_t(port_ingress_t &&other);

  port_ingress_t &operator=(const port_ingress_t &other);
  port_ingress_t &operator+=(const port_ingress_t &other);

  hit_rate_t get_total_hr() const;
  hit_rate_t get_hr_at_recirc_depth(u8 depth) const;
};

std::ostream &operator<<(std::ostream &os, const port_ingress_t &ingress);

class PerfOracle {
private:
  std::unordered_map<u16, bps_t> front_panel_ports_capacities;
  std::unordered_map<u16, bps_t> recirculation_ports_capacities;
  pps_t max_switch_capacity;
  pps_t controller_capacity;
  bytes_t avg_pkt_size;

  // All traffic is either (1) forwarded or (2) dropped. Even controller traffic
  // is, sooner or later, forwarded throught the switch. As such, unaccounted
  // ingress is all traffic for which no decisions of forwarding or dropping were
  // made yet.
  hit_rate_t unaccounted_ingress;

  // Typical front-panel ports on the switch.
  std::unordered_map<u16, port_ingress_t> ports_ingress;

  // Recirculation ports. These don't count towards the accounted traffic. They
  // serve only to calculate the throughput bottlenecks inside the switch.
  // We simplify by considering a single recirculation ingress, but during the
  // actual calculation we split this into multiple independent pipes.
  port_ingress_t recirc_ports_ingress;

  // Similarly to the recirculation ports, this serves only to calculate the
  // controller bottleneck, and does not count towards the egress traffic.
  port_ingress_t controller_ingress;

  // Dropped traffic is important for the final egress throughput calculation.
  hit_rate_t dropped_ingress;
  hit_rate_t controller_dropped_ingress;

public:
  PerfOracle(const targets_config_t &targest_config, bytes_t avg_pkt_size);

  PerfOracle(const PerfOracle &other);
  PerfOracle(PerfOracle &&other);

  PerfOracle &operator=(const PerfOracle &other);

  void add_fwd_traffic(u16 port, const port_ingress_t &ingress);
  void add_fwd_traffic(u16 port, hit_rate_t hr);

  void add_recirculated_traffic(const port_ingress_t &ingress);
  void add_recirculated_traffic(hit_rate_t hr);

  void add_controller_traffic(const port_ingress_t &ingress);
  void add_controller_traffic(hit_rate_t hr);

  void add_dropped_traffic(hit_rate_t hr);
  void add_controller_dropped_traffic(hit_rate_t hr);

  pps_t get_max_input_pps() const;
  bps_t get_max_input_bps() const;

  hit_rate_t get_dropped_ingress() const { return dropped_ingress; }

  // We optimistically assume the controller traffic will not bottleneck any
  // of the front-panel ports. We do this by forwarding the entire controller
  // traffic to a mock port.
  //
  // We do this optimistic assumption because otherwise sending to the
  // controller has no real immediate impact on the tput unless it is
  // bottlenecked. We use the controller traffic as a way to calculate the
  // tput that will ultimately be passed through a front-panel port. The
  // problem is that aggregate tput requires forwarding decisions to be made,
  // and these are not made at the time of sending to the controller (yet).
  //
  // One other way to solve this would be to lookahead during speculation and
  // speculatively execute all the forwarding operations, but this would be
  // logic very specific to the SendToController module.
  pps_t estimate_tput(pps_t ingress) const;

  void debug() const;
  void assert_final_state() const;

private:
  std::vector<pps_t> get_recirculated_egress(pps_t ingress) const;
};

} // namespace LibSynapse