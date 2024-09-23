#include "search_space.h"

#include "bdd/bdd.h"

static ss_node_id_t node_id_counter = 0;

void SearchSpace::activate_leaf(const EP *ep) {
  ep_id_t ep_id = ep->get_id();

  if (!root) {
    ss_node_id_t id = node_id_counter++;
    Score score = hcfg->score(ep);
    TargetType target = ep->get_current_platform();
    root = new SSNode(id, ep_id, score, target);
    active_leaf = root;
    return;
  }

  auto ss_node_matcher = [ep_id](const SSNode *node) {
    return node->ep_id == ep_id;
  };

  auto found_it = std::find_if(leaves.begin(), leaves.end(), ss_node_matcher);
  assert(found_it != leaves.end() && "Leaf not found");

  active_leaf = *found_it;
  leaves.erase(found_it);

  backtrack = (last_eps.find(ep_id) == last_eps.end());
  last_eps.clear();
}

static std::string get_bdd_node_description(const Node *node) {
  std::stringstream description;

  description << node->get_id();
  description << ": ";

  switch (node->get_type()) {
  case NodeType::CALL: {
    const Call *call_node = static_cast<const Call *>(node);
    description << call_node->get_call().function_name;
  } break;
  case NodeType::BRANCH: {
    const Branch *branch_node = static_cast<const Branch *>(node);
    klee::ref<klee::Expr> condition = branch_node->get_condition();
    description << "if (";
    description << pretty_print_expr(condition);
    description << ")";
  } break;
  case NodeType::ROUTE: {
    const Route *route = static_cast<const Route *>(node);
    RouteOperation op = route->get_operation();

    switch (op) {
    case RouteOperation::BCAST: {
      description << "broadcast()";
    } break;
    case RouteOperation::DROP: {
      description << "drop()";
    } break;
    case RouteOperation::FWD: {
      description << "forward(";
      description << route->get_dst_device();
      description << ")";
    } break;
    }
  } break;
  }

  std::string node_str = description.str();

  constexpr int MAX_STR_SIZE = 250;
  if (node_str.size() > MAX_STR_SIZE) {
    node_str = node_str.substr(0, MAX_STR_SIZE);
    node_str += " [...]";
  }

  Graphviz::sanitize_html_label(node_str);

  return node_str;
}

std::string SearchSpace::build_meta_tput_estimate(const EP *ep) {
  const Context &ctx = ep->get_ctx();
  const Profiler *profiler = ctx.get_profiler();
  int avg_pkt_size = profiler->get_avg_pkt_bytes();

  pps_t estimate_pps = ep->estimate_tput_pps();
  pps_t estimate_bps = estimate_pps * avg_pkt_size * 8;

  std::stringstream ss;
  ss << tput2str(estimate_bps, "bps", true);

  ss << " (";
  ss << tput2str(estimate_pps, "pps", true);
  ss << ")";

  return ss.str();
}

std::string SearchSpace::build_meta_tput_speculation(const EP *ep) {
  const Context &ctx = ep->get_ctx();
  const Profiler *profiler = ctx.get_profiler();
  int avg_pkt_size = profiler->get_avg_pkt_bytes();

  pps_t speculation_pps = ep->speculate_tput_pps();
  bps_t speculation_bps = speculation_pps * avg_pkt_size * 8;

  std::stringstream ss;
  ss << tput2str(speculation_bps, "bps", true);

  ss << " (";
  ss << tput2str(speculation_pps, "pps", true);
  ss << ")";

  return ss.str();
}

void SearchSpace::add_to_active_leaf(
    const EP *ep, const Node *node, const ModuleGenerator *modgen,
    const std::vector<impl_t> &implementations) {
  assert(active_leaf && "Active leaf not set");

  for (const impl_t &impl : implementations) {
    ss_node_id_t id = node_id_counter++;
    ep_id_t ep_id = impl.result->get_id();
    Score score = hcfg->score(impl.result);
    TargetType target = modgen->get_target();
    const Node *next = impl.result->get_next_node();

    std::stringstream description_ss;
    for (const auto &[key, value] : impl.decision.params) {
      description_ss << key << "=" << value << " ";
    }

    module_data_t module_data = {
        .type = modgen->get_type(),
        .name = modgen->get_name(),
        .description = description_ss.str(),
        .bdd_reordered = impl.bdd_reordered,
        .hit_rate = ep->get_active_leaf_hit_rate(),
    };

    bdd_node_data_t bdd_node_data = {
        .id = node->get_id(),
        .description = get_bdd_node_description(node),
    };

    std::optional<bdd_node_data_t> next_bdd_node_data;
    if (next) {
      next_bdd_node_data = {
          .id = next->get_id(),
          .description = get_bdd_node_description(next),
      };
    }

    std::map<std::string, std::string> metadata = {
        {"Tput", build_meta_tput_estimate(impl.result)},
        {"Spec", build_meta_tput_speculation(impl.result)},
    };

    SSNode *new_node = new SSNode(id, ep_id, score, target, module_data,
                                  bdd_node_data, next_bdd_node_data, metadata);

    active_leaf->children.push_back(new_node);
    leaves.push_back(new_node);

    size++;
    last_eps.insert(ep_id);
  }
}

SSNode *SearchSpace::get_root() const { return root; }
size_t SearchSpace::get_size() const { return size; }
const HeuristicCfg *SearchSpace::get_hcfg() const { return hcfg; }
bool SearchSpace::is_backtrack() const { return backtrack; }