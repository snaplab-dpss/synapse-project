#include <LibSynapse/PerfOracle.h>
#include <LibCore/Debug.h>
#include <LibCore/Math.h>

#include <cmath>
#include <iomanip>

namespace LibSynapse {

using LibCore::bps2pps;
using LibCore::newton_root_finder;
using LibCore::pps2bps;
using LibCore::tput2str;

namespace {

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
  global     = global + other.global;
  controller = controller + other.controller;

  for (const auto &[recirc_depth, hr] : other.recirc) {
    if (!recirc.contains(recirc_depth)) {
      recirc.insert({recirc_depth, hr});
    } else {
      hit_rate_t &recirc_hr = recirc.at(recirc_depth);
      recirc_hr             = recirc_hr + hr;
    }
  }

  return *this;
}

hit_rate_t port_ingress_t::get_total_hr() const {
  hit_rate_t total_hr = global + controller;
  for (const auto &[_, hr] : recirc) {
    total_hr = total_hr + hr;
  }
  return total_hr;
}

hit_rate_t port_ingress_t::get_hr_at_recirc_depth(u8 depth) const {
  hit_rate_t total_hr = 0_hr;

  for (const auto &[rport_depth, hr] : recirc) {
    if (rport_depth == depth) {
      total_hr = total_hr + hr;
    }
  }

  return total_hr;
}

PerfOracle::PerfOracle(const targets_config_t &targets_config, bytes_t _avg_pkt_size)
    : front_panel_ports_capacities(get_front_panel_port_capacities(targets_config.tofino_config)),
      recirculation_ports_capacities(get_recirculation_port_capacities(targets_config.tofino_config)),
      max_switch_capacity(targets_config.tofino_config.properties.max_capacity), controller_capacity(targets_config.controller_capacity),
      avg_pkt_size(_avg_pkt_size), unaccounted_ingress(1), dropped_ingress(0), controller_dropped_ingress(0) {
  for (auto [port, capacity] : front_panel_ports_capacities) {
    ports_ingress[port] = port_ingress_t();
  }
}

PerfOracle::PerfOracle(const PerfOracle &other)
    : front_panel_ports_capacities(other.front_panel_ports_capacities), recirculation_ports_capacities(other.recirculation_ports_capacities),
      max_switch_capacity(other.max_switch_capacity), controller_capacity(other.controller_capacity), avg_pkt_size(other.avg_pkt_size),
      unaccounted_ingress(other.unaccounted_ingress), ports_ingress(other.ports_ingress), recirc_ports_ingress(other.recirc_ports_ingress),
      controller_ingress(other.controller_ingress), dropped_ingress(other.dropped_ingress),
      controller_dropped_ingress(other.controller_dropped_ingress) {}

PerfOracle::PerfOracle(PerfOracle &&other)
    : front_panel_ports_capacities(std::move(other.front_panel_ports_capacities)),
      recirculation_ports_capacities(std::move(other.recirculation_ports_capacities)), max_switch_capacity(std::move(other.max_switch_capacity)),
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
  max_switch_capacity            = other.max_switch_capacity;
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
  assert_or_panic(front_panel_ports_capacities.contains(port), "Unknown port %u", port);

  if (ingress.get_total_hr() == 0) {
    return;
  }

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

  assert(front_panel_ports_capacities.contains(port) && "Invalid port");
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

void PerfOracle::add_recirculated_traffic(const port_ingress_t &ingress) { recirc_ports_ingress += ingress; }

void PerfOracle::add_recirculated_traffic(hit_rate_t hr) {
  port_ingress_t ingress;
  ingress.global = hr;
  add_recirculated_traffic(ingress);
}

std::vector<pps_t> PerfOracle::get_recirculated_egress(pps_t global_ingress) const {
  // We assume a uniform distribution of traffic throughout pipes.
  // As such, we decompose this into a calculation of recirculation traffic for each pipe, and
  // add them all up at the end.
  // Then, we need to consider the traffic coming from each recirculation depth.
  // For example, if we have an additional recirculation, we will have traffic coming from
  // the first recirculation (depth 0).

  std::optional<u8> max_depth;
  for (auto &[rdepth, hr] : recirc_ports_ingress.recirc) {
    max_depth = std::max(max_depth.value_or(0), static_cast<u8>(rdepth + 1));
  }
  for (const auto &[port, port_ingress] : ports_ingress) {
    for (const auto &[rport_depth, _] : port_ingress.recirc) {
      max_depth = std::max(max_depth.value_or(0), rport_depth);
    }
  }
  for (const auto &[rport_depth, _] : controller_ingress.recirc) {
    max_depth = std::max(max_depth.value_or(0), rport_depth);
  }

  if (!max_depth.has_value()) {
    return {};
  }

  // First element is the Tout component from the first recirculation (i.e.
  // comming directly from global ingress). The rest are the Tout components
  // from the surplus recirculations.
  std::vector<bps_t> Tout(max_depth.value() + 1, 0);

  const double Tin = pps2bps(global_ingress * recirc_ports_ingress.global.value, avg_pkt_size) / recirculation_ports_capacities.size();
  if (Tin == 0) {
    return Tout;
  }

  assert(std::all_of(recirculation_ports_capacities.begin(), recirculation_ports_capacities.end(),
                     [&](const auto &p) { return p.second == recirculation_ports_capacities.begin()->second; }) &&
         "All recirculation ports must have the same capacity");
  const double Cr = recirculation_ports_capacities.begin()->second;

  // std::cerr << "Tin: " << tput2str(Tin, "bps") << "\n";
  // std::cerr << "Cr: " << tput2str(Cr, "bps") << " " << tput2str(bps2pps(Cr, avg_pkt_size), "pps") << "\n";

  hit_rate_t cummulative_recirc_hr = recirc_ports_ingress.global;
  for (auto &[rdepth, hr] : recirc_ports_ingress.recirc) {
    cummulative_recirc_hr = cummulative_recirc_hr + hr;
  }
  // std::cerr << "Commulative recirc HR: " << cummulative_recirc_hr << "\n";
  if (Tin * cummulative_recirc_hr <= Cr) {
    // std::cerr << "No recirculation congestion\n";
    Tout[0] = Tin * recirc_ports_ingress.global.value;
    for (auto &[rdepth, hr] : recirc_ports_ingress.recirc) {
      assert(static_cast<size_t>(rdepth + 1) < Tout.size());
      Tout[rdepth + 1] = Tin * hr.value;
    }
    std::vector<pps_t> Tout_pps(Tout.size());
    for (size_t i = 0; i < Tout.size(); i++) {
      Tout_pps[i] = bps2pps(Tout[i], avg_pkt_size) * recirculation_ports_capacities.size();
    }
    return Tout_pps;
  }

  switch (Tout.size()) {
  case 1: {
    // Single recirculation, easy.
    // Receiving traffic only from global ingress, no surplus.
    Tout[0] = std::min(Tin, Cr);
    // std::cerr << "[Single recirc] Tout[0] = " << tput2str(Tout[0], "bps") << "\n";
  } break;
  case 2: {
    // Recirculation surplus.
    // s is relative to the first recirculation.

    const double s  = recirc_ports_ingress.get_hr_at_recirc_depth(0) / recirc_ports_ingress.global;
    const double Ts = std::min((-Tin + sqrt(Tin * Tin + 4 * Cr * Tin * s)) / 2, Tin);

    Tout[0] = (Tin / (Tin + Ts)) * Cr * (1.0 - s);
    Tout[1] = (Ts / (Tin + Ts)) * Cr;

    // std::cerr << "[Double recirc] s = " << s << "\n";
    // std::cerr << "[Double recirc] Ts = " << tput2str(Ts, "bps") << "\n";
    // std::cerr << "[Double recirc] Tout[0] = " << tput2str(Tout[0], "bps") << "\n";
    // std::cerr << "[Double recirc] Tout[1] = " << tput2str(Tout[1], "bps") << "\n";
  } break;
  case 3: {
    // s1 is relative to s0
    // s0 is relative to the first recirculation

    const hit_rate_t hr_surplus_0 = recirc_ports_ingress.get_hr_at_recirc_depth(0);
    const hit_rate_t hr_surplus_1 = recirc_ports_ingress.get_hr_at_recirc_depth(1);

    if (hr_surplus_0 == 0) {
      // Nothing to recirculate actually
      break;
    }

    const double s0 = hr_surplus_0 / recirc_ports_ingress.global;
    const double s1 = hr_surplus_1 / hr_surplus_0;

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
    // s0 is relative to the first recirculation
    // s1 is relative to s0
    // s2 is relative to s1
    // ...

    bool nothing_to_recirculate = false;
    if (recirc_ports_ingress.global <= 0) {
      nothing_to_recirculate = true;
    }

    for (u8 i = 0; !nothing_to_recirculate && i < Tout.size(); i++) {
      if (recirc_ports_ingress.get_hr_at_recirc_depth(i) <= 0) {
        nothing_to_recirculate = true;
        break;
      }
    }

    if (nothing_to_recirculate) {
      break;
    }

    std::vector<double> s(Tout.size() - 1);
    for (u8 i = 0; i < s.size(); i++) {
      if (i == 0) {
        s[i] = recirc_ports_ingress.get_hr_at_recirc_depth(i + 1) / recirc_ports_ingress.global;
      } else {
        s[i] = recirc_ports_ingress.get_hr_at_recirc_depth(i + 1) / recirc_ports_ingress.get_hr_at_recirc_depth(i);
      }
    }

    std::vector<double> Ts0_coefficients;

    Ts0_coefficients.push_back(-1.0 * Tin * Cr * s[0]);
    Ts0_coefficients.push_back(Tin);
    Ts0_coefficients.push_back(1.0);

    for (u8 i = 1; i < s.size(); i++) {
      double coefficient = 1.0;
      for (u8 j = 1; j <= i; j++) {
        coefficient *= s[j];
      }
      coefficient /= pow(Tin * s[0], i);
      Ts0_coefficients.push_back(coefficient);
    }

    std::vector<double> Ts(s.size());
    for (u8 i = 0; i < s.size(); i++) {
      if (i == 0) {
        Ts[0] = newton_root_finder(Ts0_coefficients, 0, Cr);
      } else {
        Ts[i] = pow(Ts[0], i + 1) * (1.0 / pow(Tin * s[0], i));
        for (u8 j = 1; j <= i; j++) {
          Ts[i] *= s[j];
        }
      }
    }

    double Ts_sum = 0;
    for (u8 i = 0; i < s.size(); i++) {
      Ts_sum += Ts[i];
    }

    for (u8 i = 0; i < s.size() + 1; i++) {
      if (i == 0) {
        Tout[i] = (Tin / (Tin + Ts_sum)) * Cr * (1.0 - s[i]);
      } else if (i < s.size()) {
        Tout[i] = (Ts[i - 1] / (Tin + Ts_sum)) * Cr * (1.0 - s[i]);
      } else {
        Tout[i] = (Tin / (Tin + Ts_sum)) * Cr;
      }
    }
  }
  }

  std::vector<pps_t> Tout_pps(Tout.size());
  for (size_t i = 0; i < Tout.size(); i++) {
    Tout_pps[i] = bps2pps(Tout[i], avg_pkt_size) * recirculation_ports_capacities.size();
  }

  // std::cerr << "Tout (bps): ";
  // for (size_t i = 0; i < Tout.size(); i++) {
  //   std::cerr << "d[" << i << "] = " << tput2str(Tout[i], "bps") << " ";
  // }
  // std::cerr << "\n";

  return Tout_pps;
}

bps_t PerfOracle::get_max_input_bps() const {
  bps_t Tin_max = 0;
  for (auto [_, capacity] : front_panel_ports_capacities)
    Tin_max += capacity;
  return Tin_max;
}

pps_t PerfOracle::get_max_input_pps() const { return bps2pps(get_max_input_bps(), avg_pkt_size); }

pps_t PerfOracle::estimate_tput(pps_t ingress) const {
  // std::cerr << "\n\n\n";
  // debug();
  // std::cerr << "ingress: " << tput2str(ingress, "pps") << "\n";
  // std::cerr << "ingress: " << tput2str(pps2bps(ingress, avg_pkt_size), "bps") << "\n";

  // 1. First we calculate the recirculation egress for each recirculation depth.
  // Recirculation traffic can only come from global ingress and other recirculation ports.
  const std::vector<pps_t> recirc_egress = get_recirculated_egress(ingress);

  // 2. Then we calculate the controller throughput (as it can be a bottleneck).
  // The controller can receive traffic from both global ingress and recirculation ports.
  pps_t controller_tput = ingress * controller_ingress.global.value;
  for (const auto &[recirc_depth, hr] : controller_ingress.recirc) {
    controller_tput += recirc_egress.at(recirc_depth) * hr.value;
  }

  controller_tput = std::min(controller_tput, controller_capacity);

  // 3. Finally we calculate the egress throughput for each front-panel port.
  pps_t tput = 0;

  const hit_rate_t total_controller_hr = controller_ingress.get_total_hr();
  hit_rate_t unaccounted_controller_hr = total_controller_hr - controller_dropped_ingress;

  // std::cerr << "total_controller_hr: " << total_controller_hr << "\n";

  for (const auto &[fwd_port, port_ingress] : ports_ingress) {
    pps_t port_tput = ingress * port_ingress.global.value;

    if (total_controller_hr > 0) {
      const double rel_ctrl_hr = port_ingress.controller / total_controller_hr;
      port_tput += controller_tput * rel_ctrl_hr;
      unaccounted_controller_hr = unaccounted_controller_hr - port_ingress.controller;
    }

    for (const auto &[recirc_depth, hr] : port_ingress.recirc) {
      port_tput += recirc_egress.at(recirc_depth) * hr.value;
    }

    const pps_t port_capacity = bps2pps(front_panel_ports_capacities.at(fwd_port), avg_pkt_size);

    tput += std::min(port_tput, port_capacity);
  }

  // There's controller traffic which has not yet been forwarded to any front-panel port.
  // Let's optimistically steer it to a mock port and count it as throughput.
  if (unaccounted_controller_hr > 0) {
    const double rel_ctrl_hr = unaccounted_controller_hr / total_controller_hr;
    tput += controller_tput * rel_ctrl_hr;
  }

  // std::cerr << "unaccounted_controller_hr: " << unaccounted_controller_hr << "\n";

  const hit_rate_t remaining_hr = unaccounted_ingress - unaccounted_controller_hr;
  tput += ingress * remaining_hr.value;

  // std::cerr << "remaining_hr: " << remaining_hr << "\n";

  // We shouldn't need this here, but sometimes it happens...
  // We should investigate why.
  tput = std::min(tput, ingress);

  // And finally considering the switch bottleneck.
  tput = std::min(tput, max_switch_capacity);

  // std::cerr << "final tput: " << tput2str(tput, "pps") << "\n";
  // dbg_pause();

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

  const hit_rate_t accounted_hr = egress_hr + dropped_ingress;

  const bool perf_oracle_correct_final_state = (unaccounted_ingress == 0) && (unaccounted_controller_hr == 0) && (accounted_hr == 1);

  if (!perf_oracle_correct_final_state) {
    panic("Invalid perf oracle final state\n"
          "  unaccounted_ingress: %s\n"
          "  unaccounted_controller_hr: %s\n"
          "  egress_hr (%s) + dropped_ingress (%s) == %s",
          unaccounted_ingress.to_string().c_str(), unaccounted_controller_hr.to_string().c_str(), egress_hr.to_string().c_str(),
          dropped_ingress.to_string().c_str(), accounted_hr.to_string().c_str());
  }
}

std::ostream &operator<<(std::ostream &os, const port_ingress_t &ingress) {
  os << "(";
  os << std::fixed << std::setprecision(6);
  os << "global=" << ingress.global;
  os << ",ctrl=" << ingress.controller;
  os << ",recirc=[";
  for (const auto &[recirc_depth, hr] : ingress.recirc) {
    os << "(";
    os << "depth=" << static_cast<int>(recirc_depth);
    os << ",hr=" << hr;
    os << ")";
  }
  os << "])";
  return os;
}

void PerfOracle::debug() const {
  std::vector<u16> ports;
  for (const auto &[port, _] : ports_ingress) {
    ports.push_back(port);
  }
  std::sort(ports.begin(), ports.end());

  std::cerr << "======================= PerfOracle =======================\n";
  std::cerr << "Ports:\n";
  for (u16 port : ports) {
    const port_ingress_t &ingress = ports_ingress.at(port);
    std::cerr << "  ";
    std::cerr << std::setw(3) << std::right << port;
    std::cerr << ": ";
    std::cerr << ingress;
    std::cerr << "\n";
  }
  std::cerr << "Dropped ingress: " << dropped_ingress << "\n";
  std::cerr << "Unaccounted ingress: " << unaccounted_ingress << "\n";
  std::cerr << "Controller: " << controller_ingress << "\n";
  std::cerr << "Recirculation ports: " << recirc_ports_ingress << "\n";
  std::cerr << "==========================================================\n";
}

} // namespace LibSynapse