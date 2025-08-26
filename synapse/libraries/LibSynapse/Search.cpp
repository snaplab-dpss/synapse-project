#include <LibSynapse/Search.h>
#include <LibBDD/Visitors/BDDVisualizer.h>
#include <LibSynapse/Visualizers/EPVisualizer.h>
#include <LibSynapse/Visualizers/SSVisualizer.h>
#include <LibCore/Debug.h>

#include <chrono>
#include <iomanip>
#include <cmath>

namespace LibSynapse {

using LibBDD::BDDNodeVisitAction;
using LibBDD::BDDViz;

using LibCore::int2hr;
using LibCore::pps2bps;
using LibCore::scientific;

namespace {
struct search_step_report_t {
  const EP *chosen;
  const BDDNode *current;

  std::vector<TargetType> targets;
  std::vector<std::string> name;
  std::vector<std::vector<ep_id_t>> gen_ep_ids;

  search_step_report_t(const EP *_chosen, const BDDNode *_current) : chosen(_chosen), current(_current) {}

  void save(const ModuleFactory *modgen, const std::vector<impl_t> &implementations) {
    if (implementations.empty()) {
      return;
    }

    targets.push_back(modgen->get_target());
    name.push_back(modgen->get_name());
    gen_ep_ids.emplace_back();

    for (const impl_t &impl : implementations) {
      ep_id_t next_ep_id = impl.result->get_id();
      gen_ep_ids.back().push_back(next_ep_id);
    }
  }
};

void log_search_iteration(const search_step_report_t &report, const search_meta_t &search_meta) {
  const TargetType platform = report.chosen->get_active_target();
  const EPMeta &meta        = report.chosen->get_meta();

  std::cerr << "\n";
  std::cerr << "==========================================================\n";
  std::cerr << "EP ID:      " << report.chosen->get_id() << "\n";
  std::cerr << "Target:     " << platform << "\n";

  if (report.chosen->has_active_leaf()) {
    EPLeaf leaf = report.chosen->get_active_leaf();
    if (leaf.node) {
      const Module *module = leaf.node->get_module();
      std::cerr << "Leaf:       " << module->get_name() << "\n";
    }
  }

  std::cerr << "Node:       " << report.current->dump(true) << "\n";

  assert((report.targets.size() == report.name.size() && report.targets.size() == report.gen_ep_ids.size()) && "Mismatch in the number of targets");

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

    std::cerr << "MATCH:      " << report.targets[i] << "::" << report.name[i] << " -> " << report.gen_ep_ids[i].size() << " (" << ep_ids.str()
              << ") EPs\n";
  }

  std::cerr << "------------------------------------------\n";
  std::cerr << "Progress:         " << std::fixed << std::setprecision(2) << 100 * meta.get_bdd_progress() << " %\n";
  std::cerr << "Elapsed:          " << search_meta.elapsed_time << " s\n";
  std::cerr << "Backtracks:       " << int2hr(search_meta.backtracks) << "\n";
  std::cerr << "Branching factor: " << search_meta.branching_factor << "\n";
  std::cerr << "Avg BDD size:     " << int2hr(search_meta.avg_bdd_size) << "\n";
  std::cerr << "SS size (est):    " << scientific(search_meta.total_ss_size_estimation) << "\n";
  std::cerr << "Current SS size:  " << int2hr(search_meta.ss_size) << "\n";
  std::cerr << "Search Steps:     " << int2hr(search_meta.steps) << "\n";
  std::cerr << "Unfinished EPs:   " << int2hr(search_meta.unfinished_eps) << "\n";
  std::cerr << "Finished EPs:     " << int2hr(search_meta.finished_eps) << "\n";

  if (report.gen_ep_ids.empty()) {
    std::cerr << "\n";
    std::cerr << "*** DEAD END ***: No module can handle this BDD node in the current context.\n";
    std::cerr << "Deleting solution from search space.\n";
  }

  std::cerr << "==========================================================\n";
}

void peek_search_space(const std::vector<impl_t> &new_implementations, const std::vector<ep_id_t> &peek, SearchSpace *search_space) {
  for (const impl_t &impl : new_implementations) {
    if (std::find(peek.begin(), peek.end(), impl.result->get_id()) != peek.end()) {
      impl.result->debug();
      ProfilerViz::visualize(impl.result->get_bdd(), impl.result->get_ctx().get_profiler(), false);
      EPViz::visualize(impl.result.get(), false);
      SSViz::visualize(search_space, impl.result.get(), false);
      dbg_pause();
    }
  }
}

void peek_backtrack(const EP *ep, SearchSpace *search_space, bool pause_and_show_on_backtrack) {
  if (pause_and_show_on_backtrack) {
    std::cerr << "Backtracked to " << ep->get_id() << "\n";
    ep->debug();
    BDDViz::visualize(ep->get_bdd(), false);
    // EPViz::visualize(ep, false);
    SSViz::visualize(search_space, ep, true);
  }
}

std::unique_ptr<Heuristic> build_heuristic(HeuristicOption hopt, bool not_greedy, const BDD &bdd, const Targets &targets,
                                           const targets_config_t &targets_config, const Profiler &profiler) {
  std::unique_ptr<HeuristicCfg> heuristic_cfg = build_heuristic_cfg(hopt);
  std::unique_ptr<EP> starting_ep             = std::make_unique<EP>(bdd, targets.get_view(), targets_config, profiler);
  std::unique_ptr<Heuristic> heuristic        = std::make_unique<Heuristic>(std::move(heuristic_cfg), std::move(starting_ep), !not_greedy);
  return heuristic;
}
} // namespace

SearchEngine::SearchEngine(const BDD &_bdd, HeuristicOption _hopt, const Profiler &_profiler, const targets_config_t &_targets_config,
                           const search_config_t &_search_config, const LibClone::PhysicalNetwork &_physical_network)
    : targets_config(_targets_config), search_config(_search_config), bdd(_bdd), targets(Targets(_targets_config, _physical_network)),
      profiler(_profiler), heuristic(build_heuristic(_hopt, search_config.not_greedy, bdd, targets, targets_config, profiler)) {}

search_report_t SearchEngine::search() {
  const auto start_search                   = std::chrono::steady_clock::now();
  std::unique_ptr<SearchSpace> search_space = std::make_unique<SearchSpace>(heuristic->get_cfg());

  search_meta_t meta;
  std::unordered_map<bdd_node_id_t, int> node_depth;
  bdd.get_root()->visit_nodes([this, &meta, &node_depth](const BDDNode *node) {
    const bdd_node_id_t id         = node->get_id();
    meta.avg_children_per_node[id] = 0;
    meta.visits_per_node[id]       = 0;
    node_depth[id]                 = bdd.get_node_depth(id);
    return BDDNodeVisitAction::Continue;
  });

  while (!heuristic->is_finished()) {
    meta.elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_search).count();

    std::unique_ptr<EP> ep = heuristic->pop_next_unfinished();
    search_space->activate_leaf(ep.get());

    meta.avg_bdd_size *= meta.steps;
    meta.avg_bdd_size += ep->get_bdd()->size();
    meta.steps++;
    meta.avg_bdd_size /= meta.steps;

    if (search_space->is_backtrack()) {
      meta.backtracks++;
      peek_backtrack(ep.get(), search_space.get(), search_config.pause_and_show_on_backtrack);
    }

    const BDDNode *node = ep->get_next_node();
    search_step_report_t report(ep.get(), node);

    double &avg_node_children = meta.avg_children_per_node[node->get_id()];
    int &node_visits          = meta.visits_per_node[node->get_id()];

    std::vector<impl_t> new_implementations;

    u64 children = 0;
    for (const std::unique_ptr<Target> &target : targets.elements) {
      for (const std::unique_ptr<ModuleFactory> &factory : target->module_factories) {
        std::vector<impl_t> implementations = factory->implement(ep.get(), node, bdd.get_mutable_symbol_manager(), !search_config.no_reorder);

        if (target->type == TargetType::Tofino) {
          children += implementations.size();
        }

        search_space->add_to_active_leaf(ep.get(), node, factory.get(), implementations);
        report.save(factory.get(), implementations);
        new_implementations.insert(new_implementations.end(), std::make_move_iterator(implementations.begin()),
                                   std::make_move_iterator(implementations.end()));
      }
    }

    if (children > 1) {
      avg_node_children *= node_visits;
      avg_node_children += children;
      node_visits++;
      avg_node_children /= node_visits;

      meta.branching_factor = 0;
      for (const auto &kv : meta.avg_children_per_node)
        meta.branching_factor += std::max(1.0, kv.second);
      meta.branching_factor /= meta.avg_children_per_node.size();

      meta.total_ss_size_estimation = 0;
      for (const auto &[id, depth] : node_depth) {
        meta.total_ss_size_estimation += std::pow(meta.branching_factor, depth + 1);
      }
    }

    meta.ss_size        = search_space->get_size();
    meta.unfinished_eps = heuristic->unfinished_size();
    meta.finished_eps   = heuristic->finished_size();

    log_search_iteration(report, meta);
    peek_search_space(new_implementations, search_config.peek, search_space.get());

    if (new_implementations.empty() && search_config.no_deadends) {
      ep->debug();

      const std::filesystem::path bdd_path{"deadend-bdd.dot"};
      const std::filesystem::path ep_path{"deadend-ep.dot"};
      const std::filesystem::path ss_path{"deadend-ss.dot"};

      ProfilerViz::dump_to_file(ep->get_bdd(), ep->get_ctx().get_profiler(), bdd_path);
      EPViz::dump_to_file(ep.get(), ep_path);
      SSViz::dump_to_file(search_space.get(), ep.get(), ss_path);

      panic("Dead end reached! No module can handle this BDD node in the current context.\n"
            "Dumping:\n"
            "  BDD: %s\n"
            "  EP:  %s\n"
            "  SS:  %s",
            std::filesystem::absolute(bdd_path).string().c_str(), std::filesystem::absolute(ep_path).string().c_str(),
            std::filesystem::absolute(ss_path).string().c_str());
    }

    heuristic->add(std::move(new_implementations));
  }

  meta.ss_size        = search_space->get_size();
  meta.unfinished_eps = heuristic->unfinished_size();
  meta.finished_eps   = heuristic->finished_size();

  std::unique_ptr<const EP> winner                       = heuristic->pop_best_finished();
  const Score score                                      = heuristic->get_score(winner.get());
  const std::vector<heuristic_metadata_t> heuristic_meta = heuristic->get_cfg()->get_metadata(winner.get());
  const pps_t tput_estimation_pps                        = winner->estimate_tput_pps();
  const bytes_t avg_pkt_size                             = profiler.get_avg_pkt_bytes();
  const bps_t tput_estimation_bps                        = pps2bps(tput_estimation_pps, avg_pkt_size);

  search_report_t report{
      heuristic->get_cfg()->name, std::move(winner), std::move(search_space), score, heuristic_meta, meta, tput_estimation_pps, tput_estimation_bps,
  };

  return report;
}
} // namespace LibSynapse