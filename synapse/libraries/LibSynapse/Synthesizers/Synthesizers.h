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
    switch (target.type.type) {
    case TargetArchitecture::Tofino: {
      std::cerr << "\n************** Synthesizing Tofino **************\n";
      std::filesystem::path out_file(out_dir / (name + "_" + target.type.instance_id + ".p4"));
      Tofino::TofinoSynthesizer synthesizer(ep, out_file, target.type);
      synthesizer.synthesize();
    } break;
    case TargetArchitecture::Controller: {
      std::cerr << "\n************ Synthesizing Controller ************\n";
      std::filesystem::path out_file(out_dir / (name + "_" + target.type.instance_id + ".cpp"));
      Controller::ControllerSynthesizer synthesizer(ep, out_file, target.type);
      synthesizer.synthesize();
    } break;
    case TargetArchitecture::x86: {
      // panic("TODO");
    } break;
    }
  }
}

} // namespace LibSynapse