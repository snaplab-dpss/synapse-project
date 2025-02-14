#pragma once

#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Synthesizers/TofinoSynthesizer.h>
#include <LibSynapse/Synthesizers/ControllerSynthesizer.h>

#include <filesystem>

namespace LibSynapse {

void synthesize(const EP *ep, std::string name, const std::filesystem::path &out_dir) {
  const TargetsView &targets = ep->get_targets();

  for (const TargetView &target : targets.elements) {
    switch (target.type) {
    case TargetType::Tofino: {
      std::ofstream out(out_dir / (name + ".p4"));
      Tofino::TofinoSynthesizer synthesizer(ep, out, ep->get_bdd());
      synthesizer.synthesize();
    } break;
    case TargetType::Controller: {
      std::ofstream out(out_dir / (name + ".cpp"));
      Controller::ControllerSynthesizer synthesizer(ep, out, ep->get_bdd());
      synthesizer.synthesize();
    } break;
    case TargetType::x86: {
      // panic("TODO");
    } break;
    }
  }
}

} // namespace LibSynapse