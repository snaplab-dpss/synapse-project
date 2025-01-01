#include "heuristic.h"
#include "heuristics.h"
#include "../random_engine.h"

Heuristic::Heuristic(std::unique_ptr<HeuristicCfg> _config, bool _stop_on_first_solution)
    : config(std::move(_config)),
      execution_plans(std::multiset<impl_t, impl_comparator_t>(
          [&](const impl_t &i1, const impl_t &i2) { return (*config)(i1, i2); })),
      stop_on_first_solution(_stop_on_first_solution) {}

Heuristic::~Heuristic() {
  for (auto &[ep, count] : ep_refs) {
    delete ep;
  }

  ep_refs.clear();
  execution_plans.clear();
}

bool Heuristic::finished() { return get_next_it() == execution_plans.end(); }

const EP *Heuristic::get() {
  update_best_it();
  return best_it->result;
}

const EP *Heuristic::pop() {
  auto next_it = get_next_it();
  ASSERT(next_it != execution_plans.end(), "No more execution plans to pick");

  if (config->mutates(*next_it)) {
    std::vector<impl_t> eps(execution_plans.begin(), execution_plans.end());

    // Trigger a re-sort with the new mutated heuristic.
    execution_plans = std::multiset<impl_t, impl_comparator_t>(
        [&](const impl_t &i1, const impl_t &i2) { return (*config)(i1, i2); });
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

void Heuristic::cleanup() {
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

void Heuristic::add(const std::vector<impl_t> &new_implementations) {
  for (const impl_t &impl : new_implementations) {
    execution_plans.insert(impl);

    ASSERT(impl.decision.ep, "Invalid execution plan");
    ep_refs[impl.decision.ep]++;

    ASSERT(impl.result, "Invalid execution plan");
    ep_refs[impl.result]++;
  }

  reset_best_it();
}

void Heuristic::add(EP *ep) {
  ASSERT(execution_plans.empty(), "Cannot add execution plan to non-empty heuristic");
  execution_plans.emplace(ep);
  reset_best_it();

  ASSERT(ep, "Invalid execution plan");
  ep_refs[ep]++;
}

size_t Heuristic::size() const { return execution_plans.size(); }

const HeuristicCfg *Heuristic::get_cfg() const { return config.get(); }

Score Heuristic::get_score(const EP *e) const { return config->score(e); }

void Heuristic::update_best_it() {
  ASSERT(execution_plans.size(), "No execution plans to pick");

  if (best_it != execution_plans.end()) {
    return;
  }

  best_it = execution_plans.begin();
  Score best_score = get_score(best_it->result);

  while (1) {
    if (best_it == execution_plans.end() || get_score(best_it->result) != best_score) {
      best_it = execution_plans.begin();
    }

    if (RandomEngine::generate() % 2 == 0) {
      break;
    }

    best_it++;
  }
}

void Heuristic::reset_best_it() { best_it = execution_plans.end(); }

typename std::set<impl_t, HeuristicCfg>::iterator Heuristic::get_next_it() {
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
