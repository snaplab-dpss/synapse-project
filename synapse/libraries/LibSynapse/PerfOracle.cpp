#include <LibSynapse/PerfOracle.h>
#include <LibCore/Debug.h>

#include <cmath>
#include <iomanip>

namespace LibSynapse {

namespace {

constexpr const int NEWTON_MAX_ITERATIONS{10};
constexpr const double NEWTON_PRECISION{1e-3};

// Coefficients are in increasing order: x^0, x^1, x^2, ...
// min and max are inclusive
double newton_root_finder(const std::vector<double> &coefficients, u64 min, u64 max) {
  double x = (min + max) / 2.0;

  double f     = 0;
  double df_dx = 0;

  for (int i = 0; i < NEWTON_MAX_ITERATIONS; i++) {
    // It's very important to compute each term in this order, otherwise we don't converge (precision loss).
    for (int c = coefficients.size() - 1; c >= 0; c--) {
      f += coefficients[c] * pow(x, c);

      if (c > 0) {
        df_dx += c * coefficients[c] * pow(x, c - 1);
      }
    }

    if (std::abs(f) <= NEWTON_PRECISION) {
      break;
    }

    x -= f / df_dx;
  }

  assert(f <= NEWTON_PRECISION && "Newton's method did not converge");

  return x;
}

std::unordered_map<u16, bps_t> get_front_panel_port_capacities(const Tofino::tna_config_t tna_config) {
  std::unordered_map<u16, bps_t> capacities;
  for (const Tofino::tofino_port_t &port : tna_config.ports) {
    capacities.insert({port.nf_device, port.capacity});
  }
  return capacities;
}

std::unordered_map<u16, bps_t> get_recirculation_port_capacities(const Tofino::tna_config_t tna_config) {
  std::unordered_map<u16, bps_t> capacities;
  for (const Tofino::tofino_recirculation_port_t &port : tna_config.recirculation_ports) {
    capacities.insert({port.dev_port, port.capacity});
  }
  return capacities;
}

} // namespace

port_ingress_t::port_ingress_t() : global(0), controller(0) {}

port_ingress_t::port_ingress_t(const port_ingress_t &other) : global(other.global), controller(other.controller), recirc(other.recirc) {}

port_ingress_t::port_ingress_t(port_ingress_t &&other)
    : global(std::move(other.global)), controller(std::move(other.controller)), recirc(std::move(other.recirc)) {}

port_ingress_t &port_ingress_t::operator=(const port_ingress_t &other) {
  if (this == &other) {
    return *this;
  }

  global     = other.global;
  controller = other.controller;
  recirc     = other.recirc;

  return *this;
}

port_ingress_t &port_ingress_t::operator+=(const port_ingress_t &other) {
  assert(other.global >= 0 && "Global hit rate must be non-negative");
  assert(other.global <= 1 && "Global hit rate must be at most 1");

  global     = global + other.global;
  controller = controller + other.controller;

  for (const auto &[rport_depth_pair, hr] : other.recirc) {
    const int rport = rport_depth_pair.first;
    const int depth = rport_depth_pair.second;

    assert(rport >= 0 && "Recirculation port must be non-negative");
    assert(depth >= 1 && "Recirculation depth must be at least 1");

    if (recirc.find(rport_depth_pair) == recirc.end()) {
      recirc.insert({rport_depth_pair, hr});
    } else {
      hit_rate_t &recirc_hr = recirc.at(rport_depth_pair);
      recirc_hr             = recirc_hr + hr;
    }
  }

  return *this;
}

hit_rate_t port_ingress_t::get_total_hr() const {
  hit_rate_t total_hr = global + controller;

  for (const auto &[rport_depth_pair, hr] : recirc) {
    total_hr = total_hr + hr;
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
  assert(depth > 0 && "Recirculation depth must be at least 1");

  hit_rate_t total_hr = 0_hr;

  for (const auto &[rport_depth_pair, hr] : recirc) {
    if (rport_depth_pair.second == depth) {
      total_hr = total_hr + hr;
    }
  }

  return total_hr;
}

PerfOracle::PerfOracle(const targets_config_t &targets_config, bytes_t _avg_pkt_size)
    : front_panel_ports_capacities(get_front_panel_port_capacities(targets_config.tofino_config)),
      recirculation_ports_capacities(get_recirculation_port_capacities(targets_config.tofino_config)),
      controller_capacity(targets_config.controller_capacity), avg_pkt_size(_avg_pkt_size), unaccounted_ingress(1), dropped_ingress(0),
      controller_dropped_ingress(0) {
  for (auto [port, capacity] : front_panel_ports_capacities) {
    ports_ingress[port] = port_ingress_t();
  }

  for (auto [dev_port, capacity] : recirculation_ports_capacities) {
    recirc_ports_ingress[dev_port] = port_ingress_t();
  }
}

PerfOracle::PerfOracle(const PerfOracle &other)
    : front_panel_ports_capacities(other.front_panel_ports_capacities),
      recirculation_ports_capacities(other.recirculation_ports_capacities), controller_capacity(other.controller_capacity),
      avg_pkt_size(other.avg_pkt_size), unaccounted_ingress(other.unaccounted_ingress), ports_ingress(other.ports_ingress),
      recirc_ports_ingress(other.recirc_ports_ingress), controller_ingress(other.controller_ingress),
      dropped_ingress(other.dropped_ingress), controller_dropped_ingress(other.controller_dropped_ingress) {}

PerfOracle::PerfOracle(PerfOracle &&other)
    : front_panel_ports_capacities(std::move(other.front_panel_ports_capacities)),
      recirculation_ports_capacities(std::move(other.recirculation_ports_capacities)),
      controller_capacity(std::move(other.controller_capacity)), avg_pkt_size(std::move(other.avg_pkt_size)),
      unaccounted_ingress(std::move(other.unaccounted_ingress)), ports_ingress(std::move(other.ports_ingress)),
      recirc_ports_ingress(std::move(other.recirc_ports_ingress)), controller_ingress(std::move(other.controller_ingress)),
      dropped_ingress(std::move(other.dropped_ingress)), controller_dropped_ingress(std::move(other.controller_dropped_ingress)) {}

PerfOracle &PerfOracle::operator=(const PerfOracle &other) {
  if (this == &other) {
    return *this;
  }

  front_panel_ports_capacities   = other.front_panel_ports_capacities;
  recirculation_ports_capacities = other.recirculation_ports_capacities;
  controller_capacity            = other.controller_capacity;
  avg_pkt_size                   = other.avg_pkt_size;
  unaccounted_ingress            = other.unaccounted_ingress;
  ports_ingress                  = other.ports_ingress;
  recirc_ports_ingress           = other.recirc_ports_ingress;
  controller_ingress             = other.controller_ingress;
  dropped_ingress                = other.dropped_ingress;
  controller_dropped_ingress     = other.controller_dropped_ingress;

  return *this;
}

void PerfOracle::add_fwd_traffic(u16 port, const port_ingress_t &ingress) {
  if (ingress.get_total_hr() == 0) {
    return;
  }

  assert(front_panel_ports_capacities.find(port) != front_panel_ports_capacities.end() && "Invalid port");
  ports_ingress[port] += ingress;
  unaccounted_ingress = unaccounted_ingress - ingress.get_total_hr();
}

void PerfOracle::add_dropped_traffic(hit_rate_t hr) {
  dropped_ingress     = dropped_ingress + hr;
  unaccounted_ingress = unaccounted_ingress - hr;
}

void PerfOracle::add_controller_dropped_traffic(hit_rate_t hr) {
  add_dropped_traffic(hr);
  controller_dropped_ingress = controller_dropped_ingress + hr;
}

void PerfOracle::add_fwd_traffic(u16 port, hit_rate_t hr) {
  if (hr == 0_hr) {
    return;
  }

  assert(front_panel_ports_capacities.find(port) != front_panel_ports_capacities.end() && "Invalid port");
  port_ingress_t ingress;
  ingress.global = hr;
  add_fwd_traffic(port, ingress);
}

void PerfOracle::add_controller_traffic(const port_ingress_t &ingress) { controller_ingress += ingress; }

void PerfOracle::add_controller_traffic(hit_rate_t hr) {
  port_ingress_t ingress;
  ingress.global = hr;
  add_controller_traffic(ingress);
}

void PerfOracle::add_recirculated_traffic(u16 port, const port_ingress_t &ingress) {
  assert(recirc_ports_ingress.find(port) != recirc_ports_ingress.end() && "Invalid port");
  recirc_ports_ingress[port] += ingress;
}

void PerfOracle::add_recirculated_traffic(u16 port, hit_rate_t hr) {
  assert(recirc_ports_ingress.find(port) != recirc_ports_ingress.end() && "Invalid port");
  port_ingress_t ingress;
  ingress.global = hr;
  add_recirculated_traffic(port, ingress);
}

std::vector<pps_t> PerfOracle::get_recirculated_egress(u16 port, pps_t global_ingress) const {
  assert(recirc_ports_ingress.find(port) != recirc_ports_ingress.end() && "Invalid port");
  assert(recirculation_ports_capacities.find(port) != recirculation_ports_capacities.end() && "Invalid port");

  const port_ingress_t &usage = recirc_ports_ingress.at(port);

  const bps_t Tin = LibCore::pps2bps(global_ingress * usage.global.value, avg_pkt_size);
  const bps_t Cr  = recirculation_ports_capacities.at(port);

  if (Tin == 0) {
    return {0};
  }

  const int max_depth = usage.get_max_recirc_depth();

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

    const double s = usage.get_hr_at_recirc_depth(1) / usage.global;
    const pps_t Ts = (-Tin + sqrt(Tin * Tin + 4 * Cr * (Tin * s))) / 2;

    Tout[0] = (Tin / static_cast<double>(Tin + Ts)) * Cr * (1.0 - s);
    Tout[1] = (Ts / static_cast<double>(Tin + Ts)) * Cr;
  } break;
  case 2: {
    // s1 is relative to s0
    // s0 is relative to the first recirculation

    const hit_rate_t hr_depth_1 = usage.get_hr_at_recirc_depth(1);
    const hit_rate_t hr_depth_2 = usage.get_hr_at_recirc_depth(2);

    if (hr_depth_1 == 0) {
      // Nothing to recirculate actually
      Tout[0] = 0;
      Tout[1] = 0;
      Tout[2] = 0;
      break;
    }

    const double s0 = hr_depth_1 / usage.global;
    const double s1 = hr_depth_2 / hr_depth_1;

    const double a = (s1 / s0) * (1.0 / Tin);
    const double b = 1;
    const double c = Tin;
    const double d = -1.0 * Tin * Cr * s0;

    const std::vector<double> Ts0_coefficients = {d, c, b, a};
    const double Ts0                           = newton_root_finder(Ts0_coefficients, 0, Cr);
    const double Ts1                           = Ts0 * Ts0 * (1.0 / Tin) * (s1 / s0);

    Tout[0] = (Tin / static_cast<double>(Tin + Ts0 + Ts1)) * Cr * (1.0 - s0);
    Tout[1] = (Ts0 / static_cast<double>(Tin + Ts0 + Ts1)) * Cr * (1.0 - s1);
    Tout[2] = (Ts1 / static_cast<double>(Tin + Ts0 + Ts1)) * Cr;
  } break;
  default: {
    panic("TODO: arbitrary recirculation depth");
  }
  }

  std::vector<pps_t> Tout_pps(Tout.size());
  for (size_t i = 0; i < Tout.size(); i++) {
    Tout_pps[i] = LibCore::bps2pps(Tout[i], avg_pkt_size);
  }

  return Tout_pps;
}

bps_t PerfOracle::get_max_input_bps() const {
  bps_t Tin_max = 0;
  for (auto [_, capacity] : front_panel_ports_capacities)
    Tin_max += capacity;
  return Tin_max;
}

pps_t PerfOracle::get_max_input_pps() const { return LibCore::bps2pps(get_max_input_bps(), avg_pkt_size); }

pps_t PerfOracle::estimate_tput(pps_t ingress) const {
  // 1. First we calculate the recirculation egress for each recirculation port and recirculation depth.
  // Recirculation traffic can only come from global ingress and other recirculation ports.
  std::unordered_map<int, std::vector<pps_t>> recirc_egress;
  for (auto [rport, _] : recirc_ports_ingress) {
    recirc_egress[rport] = get_recirculated_egress(rport, ingress);
  }

  // 2. Then we calculate the controller throughput (as it can be a bottleneck).
  // The controller can receive traffic from both global ingress and recirculation ports.
  pps_t controller_tput = ingress * controller_ingress.global.value;
  for (const auto &[port_depth_pair, hr] : controller_ingress.recirc) {
    int rport = port_depth_pair.first;
    int depth = port_depth_pair.second;
    assert(rport >= 0 && "Invalid recirculation port");
    assert(depth > 0 && "Invalid recirculation depth");
    assert(depth <= (int)recirc_egress[rport].size() && "Invalid recirculation depth");
    controller_tput += recirc_egress[rport][depth - 1] * hr.value;
  }
  controller_tput = std::min(controller_tput, controller_capacity);

  // 3. Finally we calculate the egress throughput for each front-panel port.
  pps_t tput = 0;

  hit_rate_t unaccounted_controller_hr = controller_ingress.get_total_hr() - controller_dropped_ingress;
  const hit_rate_t total_controller_hr = controller_ingress.get_total_hr();

  for (const auto &[fwd_port, port_ingress] : ports_ingress) {
    pps_t port_tput = ingress * port_ingress.global.value;

    if (total_controller_hr > 0) {
      const double rel_ctrl_hr = port_ingress.controller / total_controller_hr;
      port_tput += controller_tput * rel_ctrl_hr;
      unaccounted_controller_hr = unaccounted_controller_hr - port_ingress.controller;
    }

    for (const auto &[port_depth_pair, hr] : port_ingress.recirc) {
      int rport = port_depth_pair.first;
      int depth = port_depth_pair.second;
      assert(rport >= 0 && "Invalid recirculation port");
      assert(depth > 0 && "Invalid recirculation depth");
      assert(depth <= (int)recirc_egress[rport].size() && "Invalid recirculation depth");
      port_tput += recirc_egress[rport][depth - 1] * hr.value;
    }

    const pps_t port_capacity = LibCore::bps2pps(front_panel_ports_capacities.at(fwd_port), avg_pkt_size);
    tput += std::min(port_tput, port_capacity);
  }

  // There's controller traffic which has not yet been forwarded to any front-panel port.
  // Let's optimistically steer it to a mock port and count it as throughput.
  if (unaccounted_controller_hr > 0) {
    const double rel_ctrl_hr = unaccounted_controller_hr / total_controller_hr;
    tput += controller_tput * rel_ctrl_hr;
  }

  const hit_rate_t remaining_hr = unaccounted_ingress - unaccounted_controller_hr;
  tput += ingress * remaining_hr.value;

  assert(tput <= ingress);
  return tput;
}

void PerfOracle::assert_final_state() const {
  hit_rate_t egress_hr = 0_hr;
  for (const auto &[fwd_port, port_ingress] : ports_ingress) {
    egress_hr = egress_hr + port_ingress.get_total_hr();
  }

  hit_rate_t unaccounted_controller_hr = controller_ingress.get_total_hr() - controller_dropped_ingress;
  for (const auto &[fwd_port, port_ingress] : ports_ingress) {
    unaccounted_controller_hr = unaccounted_controller_hr - port_ingress.controller;
  }

  assert(unaccounted_ingress == 0);
  assert(unaccounted_controller_hr == 0);
  assert(egress_hr + dropped_ingress == 1);
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
  std::cerr << "======================= PerfOracle =======================\n";
  std::cerr << "Ports:\n";
  for (const auto &[port, ingress] : ports_ingress) {
    std::cerr << "  ";
    std::cerr << std::setw(2) << std::right << port;
    std::cerr << ": ";
    std::cerr << ingress;
    std::cerr << "\n";
  }
  std::cerr << "Dropped ingress: " << dropped_ingress << "\n";
  std::cerr << "Unaccounted ingress: " << unaccounted_ingress << "\n";
  std::cerr << "Controller: " << controller_ingress << "\n";
  std::cerr << "Recirculation ports:\n";
  for (const auto &[port, ingress] : recirc_ports_ingress) {
    std::cerr << "  ";
    std::cerr << std::setw(2) << std::right << port;
    std::cerr << ": ";
    std::cerr << ingress;
    std::cerr << "\n";
  }
  std::cerr << "==========================================================\n";
}

} // namespace LibSynapse