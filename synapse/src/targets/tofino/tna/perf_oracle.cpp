#include <cmath>

#include "perf_oracle.h"

#include "tna.h"

namespace tofino {

PerfOracle::PerfOracle(const TNAProperties *properties, int _avg_pkt_bytes)
    : total_ports(properties->total_ports),
      port_capacity_bps(properties->port_capacity_bps),
      total_recirc_ports(properties->total_recirc_ports),
      recirc_port_capacity_bps(properties->recirc_port_capacity_bps),
      avg_pkt_bytes(_avg_pkt_bytes),
      recirc_ports_usage(properties->total_recirc_ports),
      controller_port_usage(0) {
  for (int port = 0; port < total_ports; port++) {
    ports_usage[port] = 0;
  }
}

PerfOracle::PerfOracle(const PerfOracle &other)
    : total_ports(other.total_ports),
      port_capacity_bps(other.port_capacity_bps),
      total_recirc_ports(other.total_recirc_ports),
      recirc_port_capacity_bps(other.recirc_port_capacity_bps),
      avg_pkt_bytes(other.avg_pkt_bytes), ports_usage(other.ports_usage),
      recirc_ports_usage(other.recirc_ports_usage),
      controller_port_usage(other.controller_port_usage) {}

static bool fractions_le(hit_rate_t f0, hit_rate_t f1) {
  hit_rate_t delta = f0 - f1;
  hit_rate_t epsilon = 1e-8;
  return delta < epsilon;
}

static void clamp_fraction(hit_rate_t &fraction) {
  fraction = std::max(0.0, fraction);
  fraction = std::min(1.0, fraction);
}

void PerfOracle::add_fwd_traffic(int port, hit_rate_t fraction) {
  assert(port < total_ports);
  ports_usage[port] += fraction;
  clamp_fraction(ports_usage[port]);
}

hit_rate_t PerfOracle::get_fwd_traffic(int port) const {
  assert(port < total_ports);
  return ports_usage.at(port);
}

void PerfOracle::add_controller_traffic(hit_rate_t fraction) {
  controller_port_usage += fraction;
  clamp_fraction(controller_port_usage);
}

hit_rate_t PerfOracle::get_controller_traffic() const {
  return controller_port_usage;
}

void PerfOracle::add_recirculated_traffic(int port, int recirculations,
                                          hit_rate_t fraction) {
  assert(port < total_recirc_ports);
  assert(recirculations > 0);

  assert(fraction >= 0);
  assert(fraction <= 1);

  recirc_port_usage_t &usage = recirc_ports_usage[port];
  int curr_recirculations = usage.size();

  assert(recirculations <= curr_recirculations + 1);

  if (recirculations > curr_recirculations) {
    usage.push_back(0);

    assert(curr_recirculations == 0 ||
           fractions_le(fraction, usage[curr_recirculations - 1]));
    assert(recirculations == (int)usage.size());

    if (curr_recirculations > 0) {
      // Hit rate shenanigans to avoid floating point precision issues
      hit_rate_t last_fraction = usage[curr_recirculations - 1];
      fraction = std::min(fraction, last_fraction);
    }
  }

  usage[recirculations - 1] += fraction;
  clamp_fraction(usage[recirculations - 1]);

  // We can only recirculate at most the traffic we have recirculated before.
  assert(recirculations < 2 ||
         fractions_le(usage[recirculations - 1], usage[recirculations - 2]));
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

hit_rate_t PerfOracle::get_recirculated_traffic(int port,
                                                int recirculations) const {
  assert(port < total_recirc_ports);
  assert(recirculations > 0);
  assert(recirculations <= (int)recirc_ports_usage.at(port).size());
  return recirc_ports_usage.at(port)[recirculations - 1];
}

pps_t PerfOracle::get_recirculated_traffic_pps(int port, int recirculations,
                                               pps_t ingress_pps) const {
  assert(port < total_recirc_ports);
  const recirc_port_usage_t &port_usage = recirc_ports_usage.at(port);

  bps_t Tin = ingress_pps * (avg_pkt_bytes * 8);
  bps_t Cr = recirc_port_capacity_bps;
  std::vector<bps_t> Tout(port_usage.size());

  if (port_usage.empty()) {
    return 0;
  }

  switch (port_usage.size()) {
  case 1: {
    // Single recirculation, easy
    Tout[0] = std::min(Tin, Cr);
  } break;
  case 2: {
    // Recirculation surplus
    // s is relative to the first recirculation

    if (port_usage[0] == 0) {
      // Nothing to recirculate actually
      Tout[0] = 0;
      Tout[1] = 0;
      break;
    }

    hit_rate_t s = port_usage[1] / port_usage[0];
    pps_t Ts = (-Tin + sqrt(Tin * Tin + 4 * Cr * Tin * s)) / 2;

    Tout[0] = (Tin / (hit_rate_t)(Tin + Ts)) * Cr * (1.0 - s);
    Tout[1] = (Ts / (hit_rate_t)(Tin + Ts)) * Cr;
  } break;
  case 3: {
    // s1 is relative to s0
    // s0 is relative to the first recirculation

    if (port_usage[0] == 0 || port_usage[1] == 0) {
      // Nothing to recirculate actually
      Tout[0] = 0;
      Tout[1] = 0;
      Tout[2] = 0;
      break;
    }

    hit_rate_t s0 = port_usage[1] / port_usage[0];
    hit_rate_t s1 = port_usage[2] / port_usage[1];

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

  return Tout[recirculations] / (avg_pkt_bytes * 8);
}

bps_t PerfOracle::get_port_capacity_bps() const { return port_capacity_bps; }

pps_t PerfOracle::get_port_capacity_pps() const {
  return port_capacity_bps / (avg_pkt_bytes * 8);
}

bps_t PerfOracle::get_max_input_bps() const {
  return port_capacity_bps * total_ports;
}

pps_t PerfOracle::get_max_input_pps() const {
  return get_max_input_bps() / (avg_pkt_bytes * 8);
}

void PerfOracle::debug() const {
  std::stringstream ss;
  for (int port = 0; port < total_ports; port++) {
    ss << "  ";
    ss << std::setw(2) << std::right << port;
    ss << ":";
    ss << std::setw(7) << std::left << ports_usage.at(port);
    if ((port + 1) % 4 == 0) {
      ss << "\n";
    }
  }

  Log::dbg() << "======================= PerfOracle =======================\n";
  Log::dbg() << "Ports:\n";
  Log::dbg() << ss.str();
  Log::dbg() << "Controller: " << controller_port_usage << "\n";
  Log::dbg() << "Recirculations:\n";
  for (int recirc_port = 0; recirc_port < total_recirc_ports; recirc_port++) {
    const recirc_port_usage_t &usage = recirc_ports_usage.at(recirc_port);
    Log::dbg() << "  Port " << recirc_port << ":";
    for (size_t i = 0; i < usage.size(); i++) {
      Log::dbg() << " " << usage[i];
    }
    Log::dbg() << "\n";
  }
  Log::dbg() << "==========================================================\n";
}

} // namespace tofino
