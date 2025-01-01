#include <chrono>
#include <iomanip>

#include "search.h"
#include "log.h"
#include "targets/targets.h"
#include "heuristics/heuristics.h"
#include "visualizers/ss_visualizer.h"
#include "visualizers/ep_visualizer.h"

namespace {
struct search_step_report_t {
  int available_execution_plans;
  const EP *chosen;
  const Node *current;

  std::vector<TargetType> targets;
  std::vector<std::string> name;
  std::vector<std::vector<ep_id_t>> gen_ep_ids;

  search_step_report_t(int _available_execution_plans, const EP *_chosen,
                       const Node *_current)
      : available_execution_plans(_available_execution_plans), chosen(_chosen),
        current(_current) {}

  void save(const ModuleFactory *modgen, const std::vector<impl_t> &implementations) {
    if (implementations.empty()) {
      return;
    }

    targets.push_back(modgen->get_target());
    name.push_back(modgen->get_name());
    gen_ep_ids.emplace_back();
    available_execution_plans += implementations.size();

    for (const impl_t &impl : implementations) {
      ep_id_t next_ep_id = impl.result->get_id();
      gen_ep_ids.back().push_back(next_ep_id);
    }
  }
};

void log_search_iteration(const search_step_report_t &report,
                          const search_meta_t &search_meta) {
  TargetType platform = report.chosen->get_active_target();
  const EPMeta &meta = report.chosen->get_meta();

  Log::dbg() << "\n";
  Log::dbg() << "==========================================================\n";

  Log::dbg() << "EP ID:      " << report.chosen->get_id() << "\n";
  Log::dbg() << "Target:     " << platform << "\n";
  Log::dbg() << "Available:  " << report.available_execution_plans << "\n";

  if (report.chosen->has_active_leaf()) {
    EPLeaf leaf = report.chosen->get_active_leaf();
    if (leaf.node) {
      const Module *module = leaf.node->get_module();
      Log::dbg() << "Leaf:       " << module->get_name() << "\n";
    }
  }

  Log::dbg() << "Node:       " << report.current->dump(true) << "\n";

  ASSERT((report.targets.size() == report.name.size() &&
          report.targets.size() == report.gen_ep_ids.size()),
         "Mismatch in the number of targets");

  for (size_t i = 0; i < report.targets.size(); i++) {
    std::stringstream ep_ids;

    ep_ids << "[";
    for (size_t j = 0; j < report.gen_ep_ids[i].size(); j++) {
      if (j != 0) {
        ep_ids << ",";
      }
      ep_ids << report.gen_ep_ids[i][j];
    }
    ep_ids << "]";

    Log::dbg() << "MATCH:      " << report.targets[i] << "::" << report.name[i] << " -> "
               << report.gen_ep_ids[i].size() << " (" << ep_ids.str() << ") EPs\n";
  }

  Log::dbg() << "------------------------------------------\n";
  Log::dbg() << "Progress:         " << std::fixed << std::setprecision(2)
             << 100 * meta.get_bdd_progress() << " %\n";
  Log::dbg() << "Elapsed:          " << search_meta.elapsed_time << " s\n";
  Log::dbg() << "Backtracks:       " << int2hr(search_meta.backtracks) << "\n";
  Log::dbg() << "Branching factor: " << search_meta.branching_factor << "\n";
  Log::dbg() << "Avg BDD size:     " << int2hr(search_meta.avg_bdd_size) << "\n";
  Log::dbg() << "SS size (est):    " << int2hr(search_meta.total_ss_size_estimation)
             << "\n";
  Log::dbg() << "Current SS size:  " << int2hr(search_meta.ss_size) << "\n";
  Log::dbg() << "Search Steps:     " << int2hr(search_meta.steps) << "\n";
  Log::dbg() << "Candidates:       " << int2hr(search_meta.solutions) << "\n";

  if (report.targets.size() == 0) {
    Log::dbg() << "\n";
    Log::dbg() << "**DEAD END**: No module can handle this BDD node"
                  " in the current context.\n";
    Log::dbg() << "Deleting solution from search space.\n";
  }

  Log::dbg() << "==========================================================\n";
}

void peek_search_space(const std::vector<impl_t> &new_implementations,
                       const std::vector<ep_id_t> &peek, SearchSpace *search_space) {
  for (const impl_t &impl : new_implementations) {
    if (std::find(peek.begin(), peek.end(), impl.result->get_id()) != peek.end()) {
      Log::dbg() << "\n";
      impl.result->debug();
      BDDViz::visualize(impl.result->get_bdd(), false);
      EPViz::visualize(impl.result, false);
      SSVisualizer::visualize(search_space, impl.result, true);
    }
  }
}

void peek_backtrack(const EP *ep, SearchSpace *search_space,
                    bool pause_and_show_on_backtrack) {
  if (pause_and_show_on_backtrack) {
    Log::dbg() << "Backtracked to " << ep->get_id() << "\n";
    BDDViz::visualize(ep->get_bdd(), false);
    // EPViz::visualize(ep, false);
    SSVisualizer::visualize(search_space, ep, true);
  }
}

std::unique_ptr<Heuristic> build_heuristic(HeuristicOption hopt, bool not_greedy,
                                           std::shared_ptr<BDD> bdd,
                                           const Targets &targets,
                                           const toml::table &targets_config,
                                           const Profiler &profiler) {
  std::unique_ptr<HeuristicCfg> cfg;

  switch (hopt) {
  case HeuristicOption::BFS:
    cfg = std::make_unique<BFSCfg>();
    break;
  case HeuristicOption::DFS:
    cfg = std::make_unique<DFSCfg>();
    break;
  case HeuristicOption::RANDOM:
    cfg = std::make_unique<RandomCfg>();
    break;
  case HeuristicOption::GALLIUM:
    cfg = std::make_unique<GalliumCfg>();
    break;
  case HeuristicOption::GREEDY:
    cfg = std::make_unique<GreedyCfg>();
    break;
  case HeuristicOption::MAX_TPUT:
    cfg = std::make_unique<MaxTputCfg>();
    break;
  case HeuristicOption::DS_PREF:
    cfg = std::make_unique<DSPrefCfg>();
    break;
  }

  std::unique_ptr<EP> starting_ep =
      std::make_unique<EP>(bdd, targets, targets_config, profiler);

  std::unique_ptr<Heuristic> heuristic =
      std::make_unique<Heuristic>(std::move(cfg), std::move(starting_ep), !not_greedy);

  return heuristic;
}
} // namespace

SearchEngine::SearchEngine(const BDD *_bdd, HeuristicOption _hopt,
                           const Profiler &_profiler, const toml::table &_targets_config,
                           search_config_t _search_config)
    : targets_config(_targets_config), search_config(_search_config),
      bdd(std::make_shared<BDD>(*_bdd)), targets(Targets(_targets_config)),
      profiler(_profiler), heuristic(build_heuristic(_hopt, search_config.not_greedy, bdd,
                                                     targets, targets_config, profiler)) {
}

search_report_t SearchEngine::search() {
  auto start_search = std::chrono::steady_clock::now();

  search_meta_t meta;
  std::unordered_map<node_id_t, int> node_depth;

  std::unique_ptr<SearchSpace> search_space =
      std::make_unique<SearchSpace>(heuristic->get_cfg());

  bdd->get_root()->visit_nodes([this, &meta, &node_depth](const Node *node) {
    node_id_t id = node->get_id();
    meta.avg_children_per_node[id] = 0;
    meta.visits_per_node[id] = 0;
    node_depth[id] = this->bdd->get_node_depth(id);
    return NodeVisitAction::Continue;
  });

  while (!heuristic->is_finished()) {
    meta.elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::steady_clock::now() - start_search)
                            .count();

    std::unique_ptr<const EP> ep = heuristic->pop_next_unfinished();
    search_space->activate_leaf(ep.get());

    meta.avg_bdd_size *= meta.steps;
    meta.avg_bdd_size += ep->get_bdd()->size();
    meta.steps++;
    meta.avg_bdd_size /= meta.steps;

    if (search_space->is_backtrack()) {
      meta.backtracks++;
      peek_backtrack(ep.get(), search_space.get(),
                     search_config.pause_and_show_on_backtrack);
    }

    const Node *node = ep->get_next_node();
    search_step_report_t report(heuristic->size(), ep.get(), node);

    float &avg_node_children = meta.avg_children_per_node[node->get_id()];
    int &node_visits = meta.visits_per_node[node->get_id()];

    std::vector<impl_t> new_implementations;

    u64 children = 0;
    for (const std::shared_ptr<const Target> &target : targets.elements) {
      for (const std::unique_ptr<ModuleFactory> &modgen : target->module_factories) {
        const std::vector<impl_t> implementations =
            modgen->generate(ep.get(), node, !search_config.no_reorder);

        search_space->add_to_active_leaf(ep.get(), node, modgen.get(), implementations);
        report.save(modgen.get(), implementations);
        new_implementations.insert(new_implementations.end(), implementations.begin(),
                                   implementations.end());

        if (target->type == TargetType::Tofino) {
          children += implementations.size();
        }
      }
    }

    if (children > 1) {
      avg_node_children *= node_visits;
      avg_node_children += children;
      node_visits++;
      avg_node_children /= node_visits;

      meta.branching_factor = 0;
      for (const auto &kv : meta.avg_children_per_node)
        meta.branching_factor += std::max(1.0f, kv.second);
      meta.branching_factor /= meta.avg_children_per_node.size();

      meta.total_ss_size_estimation = 0;
      for (const auto &[id, depth] : node_depth) {
        meta.total_ss_size_estimation += pow(meta.branching_factor, depth + 1);
      }
    }

    meta.ss_size = search_space->get_size();
    meta.solutions = heuristic->size();

    log_search_iteration(report, meta);
    peek_search_space(new_implementations, search_config.peek, search_space.get());

    heuristic->add(std::move(new_implementations));
  }

  meta.ss_size = search_space->get_size();
  meta.solutions = heuristic->size();

  std::unique_ptr<const EP> winner = heuristic->pop_best_finished();
  Score score = heuristic->get_score(winner.get());
  std::string tput_estimation = SearchSpace::build_meta_tput_estimate(winner.get());
  std::string tput_speculation = SearchSpace::build_meta_tput_speculation(winner.get());

  search_report_t report{
      heuristic->get_cfg()->name,
      std::move(winner),
      std::move(search_space),
      score,
      tput_estimation,
      tput_speculation,
      meta,
  };

  return report;
}
