#pragma once

#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Synthesizers/TofinoSynthesizer.h>
#include <LibSynapse/Synthesizers/ControllerSynthesizer.h>
#include <LibSynapse/Synthesizers/x86Synthesizer.h>
#include <LibSynapse/Synthesizers/x86Synthesizer.h>

#include <LibBDD/Visitors/BDDVisualizer.h>

#include <filesystem>

namespace LibSynapse {

void synthesize(const EP *ep, std::string name, const std::filesystem::path &out_dir) {
  const TargetsView &targets = ep->get_targets();

  if (ep->get_bdd()->inspect().status != BDD::InspectionStatus::Ok) {
    panic("BDD is not OK: %s", ep->get_bdd()->inspect().message.c_str());
  }

  BDDViz::visualize(ep->get_bdd(), false);

  for (const std::pair<const TargetView, bool> &tv : targets.elements) {
    TargetView target = tv.first;
    switch (target.type.type) {
    case TargetArchitecture::Tofino: {
      std::cerr << "\n************** Synthesizing Tofino " + std::to_string(target.type.instance_id) + " **************\n";
      std::filesystem::path out_file(out_dir / (name + "_device_" + std::to_string(target.type.instance_id) + ".p4"));
      Tofino::TofinoSynthesizer synthesizer(ep, out_file, target.type.instance_id);
      synthesizer.synthesize();
    } break;
    case TargetArchitecture::Controller: {
      std::cerr << "\n************ Synthesizing Controller " + std::to_string(target.type.instance_id) + " ************\n";
      std::filesystem::path out_file(out_dir / (name + "_device_" + std::to_string(target.type.instance_id) + ".cpp"));
      Controller::ControllerSynthesizer synthesizer(ep, out_file, target.type.instance_id);
      synthesizer.synthesize();
    } break;
    case TargetArchitecture::x86: {
      std::cerr << "\n*************** Synthesizing x86 " + std::to_string(target.type.instance_id) + " ***************\n";
      std::filesystem::path out_file(out_dir / (name + "_device_" + std::to_string(target.type.instance_id) + ".cpp"));
      x86::x86Synthesizer synthesizer(ep, x86::x86SynthesizerTarget::NF, out_file, target.type.instance_id);
      synthesizer.synthesize();
    } break;
    }
  }
}

} // namespace LibSynapse
