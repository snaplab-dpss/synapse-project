#pragma once

#include <filesystem>

#include "tofino/synthesizer.h"

#define TOFINO_OUT_FILENAME "tofino.p4"
#define TOFINO_CPU_OUT_FILENAME "controller.cpp"
#define X86_OUT_FILENAME "x86.cpp"

namespace synapse {
void synthesize(const EP *ep, const std::filesystem::path &out_dir) {
  const TargetsView &targets = ep->get_targets();

  for (const TargetView &target : targets.elements) {
    switch (target.type) {
    case TargetType::Tofino: {
      std::ofstream out(out_dir / TOFINO_OUT_FILENAME);
      tofino::EPSynthesizer synthesizer(out, ep->get_bdd());
      synthesizer.visit(ep);
    } break;
    case TargetType::Controller: {
      // ASSERT(false , "TODO");
    } break;
    case TargetType::x86: {
      // ASSERT(false , "TODO");
    } break;
    }
  }
}
} // namespace synapse