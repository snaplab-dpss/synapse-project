#pragma once

#include <memory>

#include "heuristic_config.h"
#include "../execution_plan/execution_plan.h"

class Heuristic {
private:
  using impl_comparator_t = std::function<bool(const impl_t &, const impl_t &)>;

protected:
  std::unique_ptr<HeuristicCfg> config;
  std::multiset<impl_t, impl_comparator_t> execution_plans;
  typename std::set<impl_t, HeuristicCfg>::iterator best_it;
  bool stop_on_first_solution;
  std::unordered_map<const EP *, i64> ep_refs;

public:
  Heuristic(std::unique_ptr<HeuristicCfg> config, bool stop_on_first_solution);
  ~Heuristic();

  bool finished();
  const EP *get();
  const EP *pop();
  void cleanup();
  void add(const std::vector<impl_t> &new_implementations);
  void add(EP *ep);

  size_t size() const;
  const HeuristicCfg *get_cfg() const;
  Score get_score(const EP *e) const;

private:
  void update_best_it();
  void reset_best_it();
  typename std::set<impl_t, HeuristicCfg>::iterator get_next_it();
};
