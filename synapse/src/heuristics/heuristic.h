#pragma once

#include <memory>

#include "heuristic_config.h"
#include "../execution_plan/execution_plan.h"

namespace synapse {
class Heuristic {
private:
  using ep_cmp_t = std::function<bool(EP *, EP *)>;
  using ep_it_t = typename std::set<EP *, HeuristicCfg>::iterator;

  std::unique_ptr<HeuristicCfg> config;
  std::multiset<EP *, ep_cmp_t> unfinished_eps;
  std::multiset<EP *, ep_cmp_t> finished_eps;
  bool stop_on_first_solution;

public:
  Heuristic(std::unique_ptr<HeuristicCfg> config, std::unique_ptr<EP> starting_ep, bool stop_on_first_solution);
  ~Heuristic();

  bool is_finished();
  void add(std::vector<impl_t> &&new_implementations);
  std::unique_ptr<EP> pop_best_finished();
  std::unique_ptr<EP> pop_next_unfinished();

  size_t unfinished_size() const;
  size_t finished_size() const;
  const HeuristicCfg *get_cfg() const;
  Score get_score(const EP *e) const;

private:
  void rebuild_execution_plans_sets();
  ep_it_t get_next_unfinished_it();
};
} // namespace synapse