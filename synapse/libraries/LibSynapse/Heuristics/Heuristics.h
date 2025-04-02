#pragma once

#include <LibSynapse/Heuristics/BFS.h>
#include <LibSynapse/Heuristics/DFS.h>
#include <LibSynapse/Heuristics/Gallium.h>
#include <LibSynapse/Heuristics/Greedy.h>
#include <LibSynapse/Heuristics/MaxTput.h>
#include <LibSynapse/Heuristics/Random.h>
#include <LibSynapse/Heuristics/DSPref.h>
#include <LibSynapse/Heuristics/MaxController.h>

namespace LibSynapse {

enum class HeuristicOption {
  BFS,
  DFS,
  GALLIUM,
  GREEDY,
  MAX_TPUT,
  RANDOM,
  DS_PREF,
  MAX_CONTROLLER,
};

inline std::unique_ptr<HeuristicCfg> build_heuristic_cfg(HeuristicOption hopt) {
  std::unique_ptr<HeuristicCfg> cfg;

  switch (hopt) {
  case HeuristicOption::BFS:
    cfg = std::make_unique<BFSCfg>();
    break;
  case HeuristicOption::DFS:
    cfg = std::make_unique<DFSCfg>();
    break;
  case HeuristicOption::RANDOM:
    cfg = std::make_unique<RandomCfg>();
    break;
  case HeuristicOption::GALLIUM:
    cfg = std::make_unique<GalliumCfg>();
    break;
  case HeuristicOption::GREEDY:
    cfg = std::make_unique<GreedyCfg>();
    break;
  case HeuristicOption::MAX_TPUT:
    cfg = std::make_unique<MaxTputCfg>();
    break;
  case HeuristicOption::DS_PREF:
    cfg = std::make_unique<DSPrefCfg>();
    break;
  case HeuristicOption::MAX_CONTROLLER:
    cfg = std::make_unique<MaxControllerCfg>();
    break;
  }

  return cfg;
}

} // namespace LibSynapse