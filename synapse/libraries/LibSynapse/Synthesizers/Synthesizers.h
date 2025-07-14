#pragma once

#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Synthesizers/TofinoSynthesizer.h>
#include <LibSynapse/Synthesizers/ControllerSynthesizer.h>

#include <filesystem>

namespace LibSynapse {

void synthesize(const EP *ep, std::string name, const std::filesystem::path &out_dir) {
  const TargetsView &targets = ep->get_targets();

  if (ep->get_bdd()->inspect().status != BDD::InspectionStatus::Ok) {
    panic("BDD is not OK: %s", ep->get_bdd()->inspect().message.c_str());
  }

  for (const TargetView &target : targets.elements) {
    switch (target.type) {
    case TargetType::Tofino: {
      std::cerr << "\n************** Synthesizing Tofino **************\n";
      std::filesystem::path out_file(out_dir / (name + ".p4"));
      Tofino::TofinoSynthesizer synthesizer(ep, out_file);
      synthesizer.synthesize();
    } break;
    case TargetType::Controller: {
      std::cerr << "\n************ Synthesizing Controller ************\n";
      std::filesystem::path out_file(out_dir / (name + ".cpp"));
      Controller::ControllerSynthesizer synthesizer(ep, out_file);
      synthesizer.synthesize();
    } break;
    case TargetType::x86: {
      // panic("TODO");
    } break;
    }
  }
}

} // namespace LibSynapse