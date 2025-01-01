#include "heuristic.h"
#include "heuristics.h"
#include "../random_engine.h"

Heuristic::Heuristic(std::unique_ptr<HeuristicCfg> _config,
                     std::unique_ptr<EP> starting_ep, bool _stop_on_first_solution)
    : config(std::move(_config)),
      execution_plans(std::multiset<const EP *, ep_cmp_t>(
          [&](const EP *e1, const EP *e2) { return (*config)(e1, e2); })),
      stop_on_first_solution(_stop_on_first_solution) {
  const EP *ep = starting_ep.release();
  ASSERT(ep, "Invalid execution plan");

  execution_plans.emplace(ep);
  reset_best_it();

  ep_refs[ep]++;
}

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
  return *best_it;
}

const EP *Heuristic::pop() {
  auto next_it = get_next_it();
  ASSERT(next_it != execution_plans.end(), "No more execution plans to pick");

  if (config->mutates(*next_it)) {
    std::vector<const EP *> eps(execution_plans.begin(), execution_plans.end());

    // Trigger a re-sort with the new mutated heuristic.
    execution_plans = std::multiset<const EP *, ep_cmp_t>(
        [&](const EP *e1, const EP *e2) { return (*config)(e1, e2); });
    execution_plans.insert(eps.begin(), eps.end());

    reset_best_it();
    next_it = get_next_it();
    ASSERT(next_it != execution_plans.end(), "No more execution plans to pick");
  }

  const EP *chosen_ep = *next_it;

  auto ancestor_it = ancestors.find(chosen_ep);
  if (ancestor_it != ancestors.end()) {
    ep_refs[ancestor_it->second]--;
    ancestors.erase(chosen_ep);
  }

  ep_refs[chosen_ep]--;
  execution_plans.erase(next_it);

  reset_best_it();

  return chosen_ep;
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

void Heuristic::add(std::vector<impl_t> &&new_implementations) {
  for (impl_t &impl : new_implementations) {
    const EP *ep = impl.result;

    ASSERT(ep, "Invalid execution plan");
    execution_plans.insert(ep);
    ep_refs[ep]++;

    ASSERT(impl.decision.ep, "Invalid execution plan");
    ancestors[ep] = impl.decision.ep;
    ep_refs[impl.decision.ep]++;
  }

  reset_best_it();

  new_implementations.clear();
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
  Score best_score = get_score(*best_it);

  while (1) {
    if (best_it == execution_plans.end() || get_score(*best_it) != best_score) {
      best_it = execution_plans.begin();
    }

    if (RandomEngine::generate() % 2 == 0) {
      break;
    }

    best_it++;
  }
}

void Heuristic::reset_best_it() { best_it = execution_plans.end(); }

Heuristic::ep_it_t Heuristic::get_next_it() {
  if (execution_plans.size() == 0) {
    PANIC("No more execution plans to pick from!\n");
  }

  update_best_it();

  ep_it_t it = best_it;
  ASSERT(it != execution_plans.end(), "Invalid iterator");

  if (stop_on_first_solution && !(*it)->get_next_node()) {
    return execution_plans.end();
  }

  while (it != execution_plans.end() && !(*it)->get_next_node()) {
    it++;
  }

  return it;
}
