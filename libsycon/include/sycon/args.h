#pragma once

#include <string>

#include "time.h"
#include "util.h"

namespace sycon {

struct args_t {
  std::string p4_prog_name;
  bool run_ucli;
  int tna_version;
  time_ms_t expiration_time;
  u16 in_port;
  u16 out_port;
  bool model;

  // Wait until the input and output ports are ready.
  // Is is only relevant when running with the ASIC, not with the model.
  bool wait_for_ports;
};

extern args_t args;

}  // namespace sycon