#pragma once

#include <filesystem>

#include "tofino/synthesizer.h"

#define TOFINO_OUT_FILENAME "tofino.p4"
#define TOFINO_CPU_OUT_FILENAME "tofino_cpu.cpp"
#define X86_OUT_FILENAME "x86.cpp"

void synthesize(const EP *ep, const std::filesystem::path &out_dir) {
  const targets_t &targets = ep->get_targets();

  for (const Target *target : targets) {
    switch (target->type) {
    case TargetType::Tofino: {
      std::ofstream out(out_dir / TOFINO_OUT_FILENAME);
      tofino::EPSynthesizer synthesizer(out, ep->get_bdd());
      synthesizer.visit(ep);
    } break;
    case TargetType::TofinoCPU: {
      assert(false && "TODO");
    } break;
    case TargetType::x86: {
      assert(false && "TODO");
    } break;
    }
  }
}
