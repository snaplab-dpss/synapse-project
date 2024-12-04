#pragma once

#include <toml++/toml.hpp>

#include "target.h"

#include "x86/x86.h"
#include "tofino/tofino.h"
#include "tofino_cpu/tofino_cpu.h"

#include "../profiler.h"

inline targets_t build_targets(const toml::table &config) {
  return {
      // new tofino::TofinoTarget(config),
      // new tofino_cpu::TofinoCPUTarget(),
      new x86::x86Target(),
  };
}

inline void delete_targets(targets_t &targets) {
  for (const Target *target : targets) {
    delete target;
  }
  targets.clear();
}