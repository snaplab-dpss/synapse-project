#pragma once

#include <LibSynapse/Heuristics/BFS.h>
#include <LibSynapse/Heuristics/DFS.h>
#include <LibSynapse/Heuristics/Gallium.h>
#include <LibSynapse/Heuristics/Greedy.h>
#include <LibSynapse/Heuristics/MaxTput.h>
#include <LibSynapse/Heuristics/Random.h>
#include <LibSynapse/Heuristics/DSPrefSimple.h>
#include <LibSynapse/Heuristics/DSPrefHHTable.h>
#include <LibSynapse/Heuristics/DSPrefCuckoo.h>
#include <LibSynapse/Heuristics/MaxController.h>

namespace LibSynapse {

enum class HeuristicOption {
  BFS,
  DFS,
  Gallium,
  Greedy,
  MaxTput,
  Random,
  DSPrefSimple,
  DSPrefHHTable,
  DSPrefCuckoo,
  MaxController,
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
  case HeuristicOption::Random:
    cfg = std::make_unique<RandomCfg>();
    break;
  case HeuristicOption::Gallium:
    cfg = std::make_unique<GalliumCfg>();
    break;
  case HeuristicOption::Greedy:
    cfg = std::make_unique<GreedyCfg>();
    break;
  case HeuristicOption::MaxTput:
    cfg = std::make_unique<MaxTputCfg>();
    break;
  case HeuristicOption::DSPrefSimple:
    cfg = std::make_unique<DSPrefSimpleCfg>();
    break;
  case HeuristicOption::DSPrefHHTable:
    cfg = std::make_unique<DSPrefHHTable>();
    break;
  case HeuristicOption::DSPrefCuckoo:
    cfg = std::make_unique<DSPrefCuckoo>();
    break;
  case HeuristicOption::MaxController:
    cfg = std::make_unique<MaxControllerCfg>();
    break;
  }

  return cfg;
}

constexpr const char *const BFS_NAME             = "bfs";
constexpr const char *const DFS_NAME             = "dfs";
constexpr const char *const RANDOM_NAME          = "random";
constexpr const char *const GALLIUM_NAME         = "gallium";
constexpr const char *const GREEDY_NAME          = "greedy";
constexpr const char *const MAX_TPUT_NAME        = "max-tput";
constexpr const char *const DS_PREF_SIMPLE_NAME  = "ds-pref-simple";
constexpr const char *const DS_PREF_HHTABLE_NAME = "ds-pref-hhtable";
constexpr const char *const DS_PREF_CUCKOO_NAME  = "ds-pref-cuckoo";
constexpr const char *const MAX_CONTROLLER_NAME  = "max-controller";

const std::unordered_map<std::string, HeuristicOption> str_to_heuristic_opt{
    {BFS_NAME, HeuristicOption::BFS},
    {DFS_NAME, HeuristicOption::DFS},
    {RANDOM_NAME, HeuristicOption::Random},
    {GALLIUM_NAME, HeuristicOption::Gallium},
    {GREEDY_NAME, HeuristicOption::Greedy},
    {MAX_TPUT_NAME, HeuristicOption::MaxTput},
    {DS_PREF_SIMPLE_NAME, HeuristicOption::DSPrefSimple},
    {DS_PREF_HHTABLE_NAME, HeuristicOption::DSPrefHHTable},
    {DS_PREF_CUCKOO_NAME, HeuristicOption::DSPrefCuckoo},
    {MAX_CONTROLLER_NAME, HeuristicOption::MaxController},
};

const std::unordered_map<HeuristicOption, std::string> heuristic_opt_to_str{
    {HeuristicOption::BFS, BFS_NAME},
    {HeuristicOption::DFS, DFS_NAME},
    {HeuristicOption::Random, RANDOM_NAME},
    {HeuristicOption::Gallium, GALLIUM_NAME},
    {HeuristicOption::Greedy, GREEDY_NAME},
    {HeuristicOption::MaxTput, MAX_TPUT_NAME},
    {HeuristicOption::DSPrefSimple, DS_PREF_SIMPLE_NAME},
    {HeuristicOption::DSPrefHHTable, DS_PREF_HHTABLE_NAME},
    {HeuristicOption::DSPrefCuckoo, DS_PREF_CUCKOO_NAME},
    {HeuristicOption::MaxController, MAX_CONTROLLER_NAME},
};

} // namespace LibSynapse