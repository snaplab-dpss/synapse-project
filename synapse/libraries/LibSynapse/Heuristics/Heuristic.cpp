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

bool Heuristic::is_finished() {
  if (stop_on_first_solution && !finished_eps.empty()) {
    return true;
  }

  // TODO: some stopping condition for a non-greedy approach.
  return false;
}

std::unique_ptr<EP> Heuristic::pop_best_finished() { return std::move(finished_eps.extract(finished_eps.begin()).value()); }

void Heuristic::rebuild_execution_plans_sets() {
  auto rebuilder = [this](std::multiset<std::unique_ptr<EP>, ep_cmp_t> &target) {
    std::vector<std::unique_ptr<EP>> backup;
    backup.reserve(target.size());
    while (!target.empty()) {
      backup.push_back(std::move(target.extract(target.begin()).value()));
    }

    auto new_ep_cmp = [this](const std::unique_ptr<EP> &e1, const std::unique_ptr<EP> &e2) { return (*config)(e1.get(), e2.get()); };
    target          = std::multiset<std::unique_ptr<EP>, ep_cmp_t>(new_ep_cmp);

    for (std::unique_ptr<EP> &ep : backup) {
      target.insert(std::move(ep));
    }

    backup.clear();
  };

  rebuilder(unfinished_eps);
  rebuilder(finished_eps);
}

std::unique_ptr<EP> Heuristic::pop_next_unfinished() {
  ep_it_t next_it = get_next_unfinished_it();
  assert(next_it != unfinished_eps.end() && "No more execution plans to pick");

  if (config->mutates(next_it->get())) {
    // Trigger a re-sort with the new mutated heuristic.
    // Otherwise the multiset won't use the new instance (modified).
    rebuild_execution_plans_sets();
    next_it = get_next_unfinished_it();
    assert(next_it != unfinished_eps.end() && "No more execution plans to pick");
  }

  return std::move(unfinished_eps.extract(next_it).value());
}

void Heuristic::add(std::vector<impl_t> &&new_implementations) {
  for (impl_t &impl : new_implementations) {
    assert(impl.result && "Invalid execution plan");
    if (impl.result->get_next_node()) {
      unfinished_eps.insert(std::move(impl.result));
    } else {
      EPViz::visualize(impl.result.get(), false);
      impl.result->get_ctx().get_perf_oracle().assert_final_state();
      finished_eps.insert(std::move(impl.result));
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
  Score best_score = get_score(it->get());

  while (1) {
    const std::unique_ptr<EP> &ep = *it;

    if ((it == unfinished_eps.end()) || (get_score(ep.get()) != best_score)) {
      it = unfinished_eps.begin();
    }

    if (SingletonRandomEngine::generate() % 2 == 0) {
      break;
    }

    it++;
  }

  return it;
}

} // namespace LibSynapse