#pragma once

#include <memory>
#include <vector>
#include <unordered_set>
#include <toml++/toml.hpp>

#include "execution_plan/execution_plan.h"
#include "heuristics/heuristics.h"
#include "search_space.h"
#include "profiler.h"

namespace synapse {
struct search_meta_t {
  size_t ss_size;
  time_t elapsed_time;
  u64 steps;
  u64 backtracks;
  std::unordered_map<node_id_t, int> visits_per_node;
  std::unordered_map<node_id_t, double> avg_children_per_node;
  u64 avg_bdd_size;
  double branching_factor;
  double total_ss_size_estimation;
  int unfinished_eps;
  int finished_eps;

  search_meta_t()
      : ss_size(0), elapsed_time(0), steps(0), backtracks(0), avg_bdd_size(0),
        branching_factor(0), total_ss_size_estimation(0), unfinished_eps(0),
        finished_eps(0) {}

  search_meta_t(const search_meta_t &other) = default;
  search_meta_t(search_meta_t &&other) = default;
};

struct search_report_t {
  std::string heuristic;
  std::unique_ptr<const EP> ep;
  std::unique_ptr<SearchSpace> search_space;
  Score score;
  std::string tput_estimation;
  std::string tput_speculation;
  search_meta_t meta;
};

struct search_config_t {
  bool no_reorder;
  std::vector<ep_id_t> peek;
  bool pause_and_show_on_backtrack;
  bool not_greedy;

  search_config_t()
      : no_reorder(false), pause_and_show_on_backtrack(false), not_greedy(false) {}
};

class SearchEngine {
private:
  const toml::table targets_config;
  const search_config_t search_config;

  std::shared_ptr<BDD> bdd;
  Targets targets;
  Profiler profiler;
  std::unique_ptr<Heuristic> heuristic;

public:
  SearchEngine(const BDD *bdd, HeuristicOption hopt, const Profiler &profiler,
               const toml::table &targets_config, const search_config_t &search_config);

  SearchEngine(const SearchEngine &) = delete;
  SearchEngine(SearchEngine &&) = delete;
  SearchEngine &operator=(const SearchEngine &) = delete;
  ~SearchEngine() = default;

  search_report_t search();
};
} // namespace synapse