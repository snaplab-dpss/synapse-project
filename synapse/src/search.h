#pragma once

#include <memory>
#include <vector>
#include <unordered_set>
#include <toml++/toml.hpp>

#include "execution_plan/execution_plan.h"
#include "heuristics/heuristic.h"
#include "search_space.h"
#include "profiler.h"

struct search_meta_t {
  size_t ss_size;
  time_t elapsed_time;
  u64 steps;
  u64 backtracks;
  std::unordered_map<node_id_t, int> visits_per_node;
  std::unordered_map<node_id_t, float> avg_children_per_node;
  u64 avg_bdd_size;
  float branching_factor;
  float total_ss_size_estimation;
  int solutions;

  search_meta_t()
      : ss_size(0), elapsed_time(0), steps(0), backtracks(0), avg_bdd_size(0),
        branching_factor(0), total_ss_size_estimation(0), solutions(0) {}

  search_meta_t(const search_meta_t &other) = default;
  search_meta_t(search_meta_t &&other) = default;
};

struct search_solution_t {
  const EP *ep;
  const SearchSpace *search_space;
  Score score;
  std::string tput_estimation;
  std::string tput_speculation;
};

struct search_report_t {
  const std::string heuristic;
  const search_solution_t solution;
  const search_meta_t meta;
};

struct search_config_t {
  bool allow_bdd_reordering;
  std::vector<ep_id_t> peek;
  bool pause_and_show_on_backtrack;

  search_config_t() : allow_bdd_reordering(false), pause_and_show_on_backtrack(false) {}
};

class SearchEngine {
private:
  std::shared_ptr<BDD> bdd;
  Heuristic *h;
  const toml::table targets_config;
  Profiler profiler;
  Targets targets;
  const search_config_t search_config;

public:
  SearchEngine(const BDD *bdd, Heuristic *h, const toml::table &targets_config,
               const Profiler &profiler,
               search_config_t search_config = search_config_t());

  SearchEngine(const SearchEngine &) = delete;
  SearchEngine(SearchEngine &&) = delete;

  SearchEngine &operator=(const SearchEngine &) = delete;

  search_report_t search();
};
