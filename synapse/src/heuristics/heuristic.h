#pragma once

#include "config.h"
#include "../execution_plan/execution_plan.h"
#include "../random_engine.h"

template <class HCfg> class Heuristic {
protected:
  HCfg configuration;
  std::multiset<impl_t, HCfg> execution_plans;
  typename std::set<impl_t, HCfg>::iterator best_it;
  bool stop_on_first_solution;
  std::unordered_map<const EP *, i64> ep_refs;

public:
  Heuristic(bool _stop_on_first_solution)
      : stop_on_first_solution(_stop_on_first_solution) {}

  ~Heuristic() {
    for (auto &[ep, count] : ep_refs) {
      delete ep;
    }

    ep_refs.clear();
    execution_plans.clear();
  }

  bool finished() { return get_next_it() == execution_plans.end(); }

  const EP *get() {
    update_best_it();
    return best_it->result;
  }

  const EP *pop() {
    auto it = get_next_it();
    ASSERT(it != execution_plans.end(), "No more execution plans to pick");

    impl_t best = *it;

    if (configuration.mutates(best)) {
      std::vector<impl_t> eps(execution_plans.begin(), execution_plans.end());

      // Trigger a re-sort with the new mutated heuristic.
      execution_plans = std::multiset<impl_t, HCfg>(configuration);
      execution_plans.insert(eps.begin(), eps.end());

      reset_best_it();
      it = get_next_it();
      ASSERT(it != execution_plans.end(), "No more execution plans to pick");

      best = *it;
    }

    execution_plans.erase(it);
    reset_best_it();

    if (best.decision.ep) {
      ep_refs[best.decision.ep]--;
    }

    ep_refs[best.result]--;

    return best.result;
  }

  void cleanup() {
    std::unordered_set<const EP *> deleted;

    for (const auto &[ep, count] : ep_refs) {
      if (count == 0) {
        deleted.insert(ep);
        delete ep;
      }
    }

    for (const EP *ep : deleted) {
      ep_refs.erase(ep);
    }
  }

  void add(const std::vector<impl_t> &new_implementations) {
    for (const impl_t &impl : new_implementations) {
      execution_plans.insert(impl);

      ASSERT(impl.decision.ep, "Invalid execution plan");
      ep_refs[impl.decision.ep]++;

      ASSERT(impl.result, "Invalid execution plan");
      ep_refs[impl.result]++;
    }

    reset_best_it();
  }

  void add(EP *ep) {
    ASSERT(execution_plans.empty(),
           "Cannot add execution plan to non-empty heuristic");
    execution_plans.emplace(ep);
    reset_best_it();

    ASSERT(ep, "Invalid execution plan");
    ep_refs[ep]++;
  }

  size_t size() const { return execution_plans.size(); }
  const HCfg *get_cfg() const { return &configuration; }
  Score get_score(const EP *e) const { return configuration.score(e); }

private:
  void update_best_it() {
    ASSERT(execution_plans.size(), "No execution plans to pick");

    if (best_it != execution_plans.end()) {
      return;
    }

    best_it = execution_plans.begin();
    Score best_score = get_score(best_it->result);

    while (1) {
      if (best_it == execution_plans.end() ||
          get_score(best_it->result) != best_score) {
        best_it = execution_plans.begin();
      }

      if (RandomEngine::generate() % 2 == 0) {
        break;
      }

      best_it++;
    }
  }

  void reset_best_it() { best_it = execution_plans.end(); }

  typename std::set<impl_t, HCfg>::iterator get_next_it() {
    if (execution_plans.size() == 0) {
      PANIC("No more execution plans to pick!\n");
    }

    update_best_it();

    auto it = best_it;
    ASSERT(it != execution_plans.end(), "Invalid iterator");

    if (stop_on_first_solution && !it->result->get_next_node()) {
      return execution_plans.end();
    }

    while (it != execution_plans.end() && !it->result->get_next_node()) {
      it++;
    }

    return it;
  }
};
