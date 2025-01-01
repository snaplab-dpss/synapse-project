#pragma once

#include <memory>

#include "heuristic_config.h"
#include "../execution_plan/execution_plan.h"

class Heuristic {
private:
  using ep_cmp_t = std::function<bool(const EP *, const EP *)>;
  using ep_it_t = typename std::set<const EP *, HeuristicCfg>::iterator;

  std::unique_ptr<HeuristicCfg> config;
  std::multiset<const EP *, ep_cmp_t> execution_plans;
  std::unordered_map<const EP *, const EP *> ancestors;
  std::unordered_map<const EP *, i64> ep_refs;
  ep_it_t best_it;
  bool stop_on_first_solution;

public:
  Heuristic(std::unique_ptr<HeuristicCfg> config, std::unique_ptr<EP> starting_ep,
            bool stop_on_first_solution);
  ~Heuristic();

  bool finished();
  const EP *get();
  const EP *pop();
  void cleanup();
  void add(std::vector<impl_t> &&new_implementations);

  size_t size() const;
  const HeuristicCfg *get_cfg() const;
  Score get_score(const EP *e) const;

private:
  void update_best_it();
  void reset_best_it();
  ep_it_t get_next_it();
};
