#pragma once

#include <string>
#include <vector>

#include "time.hpp"
#include "util.hpp"

namespace sycon {

struct args_t {
  std::string p4_prog_name;
  bool run_ucli;
  int tna_version;
  time_ms_t expiration_time;
  std::vector<u16> ports;
  bool model;
  bool bench_mode;

  // Wait until the input and output ports are ready.
  // Is is only relevant when running with the ASIC, not with the model.
  bool wait_for_ports;
};

extern args_t args;

} // namespace sycon