#pragma once

#include <string>
#include <optional>
#include <vector>

#include "time.h"
#include "util.h"

namespace sycon {

struct args_t {
  std::string p4_prog_name;
  bool run_ucli;
  int tna_version;
  std::vector<u16> ports;
  bool model;
  bool bench_mode;

  // Wait until the input and output ports are ready.
  // Is is only relevant when running with the ASIC, not with the model.
  bool wait_for_ports;
  // For tests only: every token bucket -- a data-plane meter, or libnf's on the controller -- is
  // built with this rate and burst instead of the NF's own, so a test can drive a bucket out of
  // profile with a handful of packets. Both or neither are given.
  std::optional<u64> test_token_bucket_bytes_per_sec;
  std::optional<u64> test_token_bucket_burst_bytes;
};

extern args_t args;

} // namespace sycon