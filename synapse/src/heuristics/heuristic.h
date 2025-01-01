#pragma once

#include "config.h"
#include "../execution_plan/execution_plan.h"

template <class HCfg> class Heuristic {
protected:
  HCfg configuration;
  std::multiset<impl_t, HCfg> execution_plans;
  typename std::set<impl_t, HCfg>::iterator best_it;
  bool stop_on_first_solution;
  std::unordered_map<const EP *, i64> ep_refs;

public:
  Heuristic(bool _stop_on_first_solution);
  ~Heuristic();

  bool finished();
  const EP *get();
  const EP *pop();
  void cleanup();
  void add(const std::vector<impl_t> &new_implementations);
  void add(EP *ep);

  size_t size() const;
  const HCfg *get_cfg() const;
  Score get_score(const EP *e) const;

private:
  void update_best_it();
  void reset_best_it();
  typename std::set<impl_t, HCfg>::iterator get_next_it();
};
