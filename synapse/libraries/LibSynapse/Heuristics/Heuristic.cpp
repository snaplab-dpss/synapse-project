#include <LibSynapse/Heuristics/Heuristic.h>
#include <LibCore/RandomEngine.h>

#include <LibSynapse/Visualizers/EPVisualizer.h>

namespace LibSynapse {

Heuristic::Heuristic(std::unique_ptr<HeuristicCfg> _config, std::unique_ptr<EP> starting_ep, bool _stop_on_first_solution)
    : config(std::move(_config)), stop_on_first_solution(_stop_on_first_solution) {
  EP *ep = starting_ep.release();
  assert(ep && "Invalid execution plan");

  unfinished_eps.emplace(ep);

  rebuild_execution_plans_sets();
}

Heuristic::~Heuristic() {
  for (const EP *ep : unfinished_eps) {
    delete ep;
  }

  for (const EP *ep : finished_eps) {
    delete ep;
  }
}

bool Heuristic::is_finished() {
  if (stop_on_first_solution && !finished_eps.empty()) {
    return true;
  }

  // TODO: some stopping condition for a non-greedy approach.
  return false;
}

std::unique_ptr<EP> Heuristic::pop_best_finished() {
  ep_it_t best_finished_it = finished_eps.begin();
  EP *best                 = *best_finished_it;
  finished_eps.erase(best_finished_it);
  return std::unique_ptr<EP>(best);
}

void Heuristic::rebuild_execution_plans_sets() {
  auto rebuilder = [this](std::multiset<EP *, ep_cmp_t> &target) {
    std::vector<EP *> backup(target.begin(), target.end());
    target = std::multiset<EP *, ep_cmp_t>([this](EP *e1, EP *e2) { return (*config)(e1, e2); });
    target.insert(backup.begin(), backup.end());
  };

  rebuilder(unfinished_eps);
  rebuilder(finished_eps);
}

std::unique_ptr<EP> Heuristic::pop_next_unfinished() {
  ep_it_t next_it = get_next_unfinished_it();
  assert(next_it != unfinished_eps.end() && "No more execution plans to pick");

  if (config->mutates(*next_it)) {
    // Trigger a re-sort with the new mutated heuristic.
    // Otherwise the multiset won't use the new instance (modified).
    rebuild_execution_plans_sets();
    next_it = get_next_unfinished_it();
    assert(next_it != unfinished_eps.end() && "No more execution plans to pick");
  }

  EP *chosen_ep = *next_it;
  unfinished_eps.erase(next_it);

  return std::unique_ptr<EP>(chosen_ep);
}

void Heuristic::add(std::vector<impl_t> &&new_implementations) {
  for (impl_t &impl : new_implementations) {
    EP *ep = impl.result;
    assert(ep && "Invalid execution plan");

    if (ep->get_next_node()) {
      unfinished_eps.insert(ep);
    } else {
      EPViz::visualize(ep, false);
      ep->get_ctx().get_perf_oracle().assert_final_state();
      finished_eps.insert(ep);
    }
  }

  new_implementations.clear();
}

size_t Heuristic::unfinished_size() const { return unfinished_eps.size(); }
size_t Heuristic::finished_size() const { return finished_eps.size(); }

const HeuristicCfg *Heuristic::get_cfg() const { return config.get(); }

Score Heuristic::get_score(const EP *e) const { return config->score(e); }

Heuristic::ep_it_t Heuristic::get_next_unfinished_it() {
  assert(!unfinished_eps.empty() && "No execution plans to pick");

  ep_it_t it       = unfinished_eps.begin();
  Score best_score = get_score(*it);

  while (1) {
    EP *ep = *it;

    if ((it == unfinished_eps.end()) || (get_score(ep) != best_score)) {
      it = unfinished_eps.begin();
    }

    if (LibCore::SingletonRandomEngine::generate() % 2 == 0) {
      break;
    }

    it++;
  }

  return it;
}

} // namespace LibSynapse