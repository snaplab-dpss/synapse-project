#include "heuristic.h"
#include "heuristics.h"
#include "../random_engine.h"

template <class HCfg>
Heuristic<HCfg>::Heuristic(bool _stop_on_first_solution)
    : stop_on_first_solution(_stop_on_first_solution) {}

template <class HCfg> Heuristic<HCfg>::~Heuristic() {
  for (auto &[ep, count] : ep_refs) {
    delete ep;
  }

  ep_refs.clear();
  execution_plans.clear();
}

template <class HCfg> bool Heuristic<HCfg>::finished() {
  return get_next_it() == execution_plans.end();
}

template <class HCfg> const EP *Heuristic<HCfg>::get() {
  update_best_it();
  return best_it->result;
}

template <class HCfg> const EP *Heuristic<HCfg>::pop() {
  auto next_it = get_next_it();
  ASSERT(next_it != execution_plans.end(), "No more execution plans to pick");

  if (configuration.mutates(*next_it)) {
    std::vector<impl_t> eps(execution_plans.begin(), execution_plans.end());

    // Trigger a re-sort with the new mutated heuristic.
    execution_plans = std::multiset<impl_t, HCfg>(configuration);
    execution_plans.insert(eps.begin(), eps.end());

    reset_best_it();
    next_it = get_next_it();
    ASSERT(next_it != execution_plans.end(), "No more execution plans to pick");
  }

  if (next_it->decision.ep) {
    ep_refs[next_it->decision.ep]--;
  }
  ep_refs[next_it->result]--;

  const EP *ep = next_it->result;

  execution_plans.erase(next_it);
  reset_best_it();

  return ep;
}

template <class HCfg> void Heuristic<HCfg>::cleanup() {
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

template <class HCfg>
void Heuristic<HCfg>::add(const std::vector<impl_t> &new_implementations) {
  for (const impl_t &impl : new_implementations) {
    execution_plans.insert(impl);

    ASSERT(impl.decision.ep, "Invalid execution plan");
    ep_refs[impl.decision.ep]++;

    ASSERT(impl.result, "Invalid execution plan");
    ep_refs[impl.result]++;
  }

  reset_best_it();
}

template <class HCfg> void Heuristic<HCfg>::add(EP *ep) {
  ASSERT(execution_plans.empty(),
         "Cannot add execution plan to non-empty heuristic");
  execution_plans.emplace(ep);
  reset_best_it();

  ASSERT(ep, "Invalid execution plan");
  ep_refs[ep]++;
}

template <class HCfg> size_t Heuristic<HCfg>::size() const {
  return execution_plans.size();
}

template <class HCfg> const HCfg *Heuristic<HCfg>::get_cfg() const {
  return &configuration;
}

template <class HCfg> Score Heuristic<HCfg>::get_score(const EP *e) const {
  return configuration.score(e);
}

template <class HCfg> void Heuristic<HCfg>::update_best_it() {
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

template <class HCfg> void Heuristic<HCfg>::reset_best_it() {
  best_it = execution_plans.end();
}

template <class HCfg>
typename std::set<impl_t, HCfg>::iterator Heuristic<HCfg>::get_next_it() {
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

EXPLICIT_HEURISTIC_TEMPLATE_CLASS_INSTANTIATION(Heuristic)