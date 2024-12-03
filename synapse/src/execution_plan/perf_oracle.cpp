#include <cmath>

#include "perf_oracle.h"
#include "../log.h"

#define EPSILON 1e-6

static void clamp_fraction(hit_rate_t &fraction) {
  fraction = std::max(0.0, fraction);
  fraction = std::min(1.0, fraction);
  if (fraction < EPSILON) {
    fraction = 0;
  }
}

port_ingress_t::port_ingress_t() : global(0), controller(0) {}

port_ingress_t::port_ingress_t(const port_ingress_t &other)
    : global(other.global), controller(other.controller), recirc(other.recirc) {
}

port_ingress_t::port_ingress_t(port_ingress_t &&other)
    : global(std::move(other.global)), controller(std::move(other.controller)),
      recirc(std::move(other.recirc)) {}

port_ingress_t &port_ingress_t::operator=(const port_ingress_t &other) {
  if (this == &other) {
    return *this;
  }

  global = other.global;
  controller = other.controller;
  recirc = other.recirc;

  return *this;
}

port_ingress_t &port_ingress_t::operator+=(const port_ingress_t &other) {
  assert(other.global >= 0);
  assert(other.global <= 1);

  global += other.global;
  clamp_fraction(global);

  controller += other.controller;
  clamp_fraction(controller);

  for (auto [rport_depth_pair, hr] : other.recirc) {
    int rport = rport_depth_pair.first;
    int depth = rport_depth_pair.second;

    assert(rport >= 0);
    assert(depth >= 1);
    assert(hr >= 0);
    assert(hr <= 1);

    if (recirc.find(rport_depth_pair) == recirc.end()) {
      recirc.insert({rport_depth_pair, hr});
    } else {
      recirc.at(rport_depth_pair) += hr;
    }

    clamp_fraction(recirc.at(rport_depth_pair));
  }

  return *this;
}

hit_rate_t port_ingress_t::get_total_hr() const {
  hit_rate_t total_hr = global + controller;

  for (const auto &[rport_depth_pair, hr] : recirc) {
    total_hr += hr;
  }

  return total_hr;
}

int port_ingress_t::get_max_recirc_depth() const {
  int max_depth = 0;

  for (const auto &[rport_depth_pair, hr] : recirc) {
    max_depth = std::max(max_depth, rport_depth_pair.second);
  }

  return max_depth;
}

hit_rate_t port_ingress_t::get_hr_at_recirc_depth(int depth) const {
  assert(depth > 0);

  hit_rate_t total_hr = 0;

  for (const auto &[rport_depth_pair, hr] : recirc) {
    if (rport_depth_pair.second == depth) {
      total_hr += hr;
    }
  }

  return total_hr;
}

static std::vector<bps_t> parse_front_panel_ports(const toml::table &config) {
  std::vector<bps_t> ports;

  for (const auto &port : *config["switch"]["front_panel_ports"].as_array()) {
    bps_t capacity = *port.as_table()->at("capacity_bps").value<bps_t>();
    ports.push_back(capacity);
  }

  return ports;
}

static std::vector<bps_t> parse_recirculation_ports(const toml::table &config) {
  std::vector<bps_t> ports;

  for (const auto &port : *config["switch"]["recirculation_ports"].as_array()) {
    bps_t capacity = *port.as_table()->at("capacity_bps").value<bps_t>();
    ports.push_back(capacity);
  }

  return ports;
}

PerfOracle::PerfOracle(const toml::table &config, int _avg_pkt_bytes)
    : front_panel_ports_capacities(parse_front_panel_ports(config)),
      recirculation_ports_capacities(parse_recirculation_ports(config)),
      controller_capacity(*config["controller"]["capacity_pps"].value<pps_t>()),
      avg_pkt_bytes(_avg_pkt_bytes), unaccounted_ingress(1),
      dropped_ingress(0) {
  for (size_t port = 0; port < front_panel_ports_capacities.size(); port++) {
    ports_ingress[port] = port_ingress_t();
  }

  for (size_t rport = 0; rport < recirculation_ports_capacities.size();
       rport++) {
    recirc_ports_ingress[rport] = port_ingress_t();
  }
}

PerfOracle::PerfOracle(const PerfOracle &other)
    : front_panel_ports_capacities(other.front_panel_ports_capacities),
      recirculation_ports_capacities(other.recirculation_ports_capacities),
      controller_capacity(other.controller_capacity),
      avg_pkt_bytes(other.avg_pkt_bytes),
      unaccounted_ingress(other.unaccounted_ingress),
      ports_ingress(other.ports_ingress),
      recirc_ports_ingress(other.recirc_ports_ingress),
      controller_ingress(other.controller_ingress),
      dropped_ingress(other.dropped_ingress) {}

PerfOracle::PerfOracle(PerfOracle &&other)
    : front_panel_ports_capacities(
          std::move(other.front_panel_ports_capacities)),
      recirculation_ports_capacities(
          std::move(other.recirculation_ports_capacities)),
      controller_capacity(std::move(other.controller_capacity)),
      avg_pkt_bytes(std::move(other.avg_pkt_bytes)),
      unaccounted_ingress(std::move(other.unaccounted_ingress)),
      ports_ingress(std::move(other.ports_ingress)),
      recirc_ports_ingress(std::move(other.recirc_ports_ingress)),
      controller_ingress(std::move(other.controller_ingress)),
      dropped_ingress(std::move(other.dropped_ingress)) {}

PerfOracle &PerfOracle::operator=(const PerfOracle &other) {
  if (this == &other) {
    return *this;
  }

  front_panel_ports_capacities = other.front_panel_ports_capacities;
  recirculation_ports_capacities = other.recirculation_ports_capacities;
  controller_capacity = other.controller_capacity;
  avg_pkt_bytes = other.avg_pkt_bytes;
  unaccounted_ingress = other.unaccounted_ingress;
  ports_ingress = other.ports_ingress;
  recirc_ports_ingress = other.recirc_ports_ingress;
  controller_ingress = other.controller_ingress;
  dropped_ingress = other.dropped_ingress;

  return *this;
}

void PerfOracle::add_fwd_traffic(int port, const port_ingress_t &ingress) {
  assert(port < (int)front_panel_ports_capacities.size());
  ports_ingress[port] += ingress;
  unaccounted_ingress -= ingress.get_total_hr();
  clamp_fraction(unaccounted_ingress);
}

void PerfOracle::add_dropped_traffic(hit_rate_t hr) {
  dropped_ingress += hr;
  clamp_fraction(dropped_ingress);
  unaccounted_ingress -= hr;
  clamp_fraction(unaccounted_ingress);
}

void PerfOracle::add_fwd_traffic(int port, hit_rate_t hr) {
  assert(port < (int)front_panel_ports_capacities.size());
  port_ingress_t ingress;
  ingress.global = hr;
  add_fwd_traffic(port, ingress);
}

void PerfOracle::add_controller_traffic(const port_ingress_t &ingress) {
  controller_ingress += ingress;
}

void PerfOracle::add_controller_traffic(hit_rate_t hr) {
  port_ingress_t ingress;
  ingress.global = hr;
  add_controller_traffic(ingress);
}

void PerfOracle::add_recirculated_traffic(int port,
                                          const port_ingress_t &ingress) {
  assert(port >= 0);
  assert(port < (int)recirculation_ports_capacities.size());
  recirc_ports_ingress[port] += ingress;
}

void PerfOracle::add_recirculated_traffic(int port, hit_rate_t hr) {
  assert(port >= 0);
  assert(port < (int)recirculation_ports_capacities.size());
  port_ingress_t ingress;
  ingress.global = hr;
  add_recirculated_traffic(port, ingress);
}

// Coefficients are in increasing order: x^0, x^1, x^2, ...
// min and max are inclusive
static hit_rate_t newton_root_finder(hit_rate_t *coefficients,
                                     int n_coefficients, u64 min, u64 max) {
  hit_rate_t x = (min + max) / 2.0;
  int it = 0;

  while (it < NEWTON_MAX_ITERATIONS) {
    hit_rate_t f = 0;
    hit_rate_t df_dx = 0;

    // It's very important to compute each term in this order, otherwise we
    // don't converge (precision loss)
    for (int c = n_coefficients; c >= 0; c--) {
      f += coefficients[c] * pow(x, c);

      if (c > 0) {
        df_dx += c * coefficients[c] * pow(x, c - 1);
      }
    }

    if (std::abs(f) <= NEWTON_PRECISION) {
      break;
    }

    x = x - f / df_dx;
    it++;
  }

  return x;
}

std::vector<pps_t>
PerfOracle::get_recirculated_egress(int port, pps_t global_ingress) const {
  assert(port < (int)recirculation_ports_capacities.size());
  const port_ingress_t &usage = recirc_ports_ingress.at(port);

  bps_t Tin = global_ingress * usage.global * (avg_pkt_bytes * 8);
  bps_t Cr = recirculation_ports_capacities[port];

  if (Tin == 0) {
    return {0};
  }

  int max_depth = usage.get_max_recirc_depth();

  // First element is the Tout component from the first recirculation (i.e.
  // comming directly from global ingress). The rest are the Tout components
  // from the surplus recirculations (i.e. recirculation depth >= 1).
  std::vector<bps_t> Tout(1 + max_depth);

  switch (max_depth) {
  case 0: {
    // Single recirculation, easy.
    // Receiving traffic only from global ingress, no surplus.
    Tout[0] = std::min(Tin, Cr);
  } break;
  case 1: {
    // Recirculation surplus.
    // s is relative to the first recirculation.

    hit_rate_t s = usage.get_hr_at_recirc_depth(1) / usage.global;
    pps_t Ts = (-Tin + sqrt(Tin * Tin + 4 * Cr * Tin * s)) / 2;

    Tout[0] = (Tin / (hit_rate_t)(Tin + Ts)) * Cr * (1.0 - s);
    Tout[1] = (Ts / (hit_rate_t)(Tin + Ts)) * Cr;
  } break;
  case 2: {
    // s1 is relative to s0
    // s0 is relative to the first recirculation

    hit_rate_t hr_depth_1 = usage.get_hr_at_recirc_depth(1);
    hit_rate_t hr_depth_2 = usage.get_hr_at_recirc_depth(2);

    if (hr_depth_1 == 0) {
      // Nothing to recirculate actually
      Tout[0] = 0;
      Tout[1] = 0;
      Tout[2] = 0;
      break;
    }

    hit_rate_t s0 = hr_depth_1 / usage.global;
    hit_rate_t s1 = hr_depth_2 / hr_depth_1;

    hit_rate_t a = (s1 / s0) * (1.0 / Tin);
    hit_rate_t b = 1;
    hit_rate_t c = Tin;
    hit_rate_t d = -1.0 * Tin * Cr * s0;

    hit_rate_t Ts0_coefficients[] = {d, c, b, a};
    hit_rate_t Ts0 = newton_root_finder(Ts0_coefficients, 4, 0, Cr);
    hit_rate_t Ts1 = Ts0 * Ts0 * (1.0 / Tin) * (s1 / s0);

    Tout[0] = (Tin / (hit_rate_t)(Tin + Ts0 + Ts1)) * Cr * (1.0 - s0);
    Tout[1] = (Ts0 / (hit_rate_t)(Tin + Ts0 + Ts1)) * Cr * (1.0 - s1);
    Tout[2] = (Ts1 / (hit_rate_t)(Tin + Ts0 + Ts1)) * Cr;
  } break;
  default: {
    PANIC("TODO: arbitrary recirculation depth");
  }
  }

  std::vector<pps_t> Tout_pps(Tout.size());
  for (size_t i = 0; i < Tout.size(); i++) {
    Tout_pps[i] = Tout[i] / (avg_pkt_bytes * 8);
  }

  return Tout_pps;
}

bps_t PerfOracle::get_max_input_bps() const {
  bps_t Tin_max = 0;
  for (bps_t capacity : front_panel_ports_capacities)
    Tin_max += capacity;
  return Tin_max;
}

pps_t PerfOracle::get_max_input_pps() const {
  return get_max_input_bps() / (avg_pkt_bytes * 8);
}

pps_t PerfOracle::estimate_tput(pps_t ingress) const {
  // 1. First we calculate the recirculation egress for each recirculation
  // port and recirculation depth. Recirculation traffic can only come from
  // global ingress and other recirculation ports.
  std::unordered_map<int, std::vector<pps_t>> recirc_egress;
  for (size_t rport = 0; rport < recirculation_ports_capacities.size();
       rport++) {
    recirc_egress[rport] = get_recirculated_egress(rport, ingress);
  }

  // 2. Then we calculate the controller throughput (as it can be a
  // bottleneck). The controller can receive traffic from both global ingress
  // and recirculation ports.
  pps_t controller_tput = ingress * controller_ingress.global;
  for (const auto &[port_depth_pair, hr] : controller_ingress.recirc) {
    int rport = port_depth_pair.first;
    int depth = port_depth_pair.second;
    assert(rport >= 0);
    assert(depth > 0);
    assert(depth <= (int)recirc_egress[rport].size());
    controller_tput += recirc_egress[rport][depth - 1] * hr;
  }
  controller_tput = std::min(controller_tput, controller_capacity);

  // 3. Finally we calculate the throughput for each front-panel port.
  pps_t tput = 0;

  hit_rate_t unaccounted_controller_hr = controller_ingress.get_total_hr();
  hit_rate_t total_controller_hr = controller_ingress.get_total_hr();

  for (const auto &[fwd_port, port_ingress] : ports_ingress) {
    pps_t port_tput = ingress * port_ingress.global;

    if (total_controller_hr > 0) {
      hit_rate_t rel_ctrl_hr = port_ingress.controller / total_controller_hr;
      port_tput += controller_tput * rel_ctrl_hr;
      unaccounted_controller_hr -= port_ingress.controller;
    }

    for (const auto &[port_depth_pair, hr] : port_ingress.recirc) {
      int rport = port_depth_pair.first;
      int depth = port_depth_pair.second;
      assert(rport >= 0);
      assert(depth > 0);
      assert(depth <= (int)recirc_egress[rport].size());
      port_tput += recirc_egress[rport][depth - 1] * hr;
    }

    pps_t port_capacity =
        front_panel_ports_capacities[fwd_port] / (avg_pkt_bytes * 8);
    tput += std::min(port_tput, port_capacity);
  }

  // There's controller traffic which has not yet been forwarded to any
  // front-panel port. Let's optimistically steer it to a mock port a count it
  // as throughput.
  if (unaccounted_controller_hr > 0) {
    hit_rate_t rel_ctrl_hr = unaccounted_controller_hr / total_controller_hr;
    tput += controller_tput * rel_ctrl_hr;
  }

  tput += (unaccounted_ingress - unaccounted_controller_hr) * ingress;

  return tput;
}

std::ostream &operator<<(std::ostream &os, const port_ingress_t &ingress) {
  os << "(";
  os << std::fixed << std::setprecision(6);
  os << "global=" << ingress.global;
  os << ",ctrl=" << ingress.controller;
  os << ",[";
  for (const auto &[rport_depth_pair, hr] : ingress.recirc) {
    int rport = rport_depth_pair.first;
    int depth = rport_depth_pair.second;
    os << "(";
    os << "rport=" << rport;
    os << ",depth=" << depth;
    os << ",hr=" << hr;
    os << ")";
  }
  os << "])";
  return os;
}

void PerfOracle::debug() const {
  std::stringstream ss;

  ss << "======================= PerfOracle =======================\n";
  ss << "Ports:\n";
  for (const auto &[port, ingress] : ports_ingress) {
    ss << "  ";
    ss << std::setw(2) << std::right << port;
    ss << ": ";
    ss << ingress;
    ss << "\n";
  }
  ss << "Dropped ingress: " << dropped_ingress << "\n";
  ss << "Unaccounted ingress: " << unaccounted_ingress << "\n";
  ss << "Controller: " << controller_ingress << "\n";
  ss << "Recirculation ports:\n";
  for (const auto &[port, ingress] : recirc_ports_ingress) {
    ss << "  ";
    ss << std::setw(2) << std::right << port;
    ss << ": ";
    ss << ingress;
    ss << "\n";
  }
  ss << "==========================================================\n";

  Log::dbg() << ss.str();
}
