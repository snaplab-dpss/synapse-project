#include <LibSynapse/SearchSpace.h>
#include <LibCore/Types.h>
#include <LibCore/Expr.h>
#include <LibCore/Debug.h>

namespace LibSynapse {

using LibBDD::BDDNodeType;
using LibBDD::Branch;
using LibBDD::Call;
using LibBDD::Route;

using LibCore::expr_to_string;
using LibCore::pretty_print_expr;

namespace {
ss_node_id_t node_id_counter = 0;

std::string get_bdd_node_description(const BDDNode *node) {
  std::stringstream description;

  description << node->get_id();
  description << ":";

  switch (node->get_type()) {
  case BDDNodeType::Call: {
    const Call *call_node = dynamic_cast<const Call *>(node);
    description << call_node->get_call().function_name;
  } break;
  case BDDNodeType::Branch: {
    const Branch *branch_node       = dynamic_cast<const Branch *>(node);
    klee::ref<klee::Expr> condition = branch_node->get_condition();
    description << "if(";
    description << pretty_print_expr(condition);
    description << ")";
  } break;
  case BDDNodeType::Route: {
    const Route *route = dynamic_cast<const Route *>(node);
    RouteOp op         = route->get_operation();

    switch (op) {
    case RouteOp::Broadcast: {
      description << "broadcast()";
    } break;
    case RouteOp::Drop: {
      description << "drop()";
    } break;
    case RouteOp::Forward: {
      description << "forward(";
      description << expr_to_string(route->get_dst_device());
      description << ")";
    } break;
    }
  } break;
  }

  constexpr const int MAX_STR_SIZE = 250;
  std::string node_str             = description.str();
  if (node_str.size() > MAX_STR_SIZE) {
    node_str = node_str.substr(0, MAX_STR_SIZE);
    node_str += " [...]";
  }

  return TreeViz::sanitize_html_label(node_str);
}
} // namespace

void SearchSpace::activate_leaf(const EP *ep) {
  const ep_id_t ep_id = ep->get_id();

  if (!root) {
    const ss_node_id_t id   = node_id_counter++;
    const Score score       = hcfg->score(ep);
    const EPLeaf leaf       = ep->get_active_leaf();
    const TargetType target = ep->get_active_target();

    const bdd_node_data_t next_bdd_node_data{
        .id          = leaf.next->get_id(),
        .description = get_bdd_node_description(leaf.next),
    };

    const std::vector<heuristic_metadata_t> metadata = hcfg->get_metadata(ep);

    root        = new SSNode(id, ep_id, score, target, next_bdd_node_data, metadata);
    active_leaf = root;
    return;
  }

  auto ss_node_matcher = [ep_id](const SSNode *node) { return node->ep_id == ep_id; };

  auto found_it = std::find_if(leaves.begin(), leaves.end(), ss_node_matcher);
  assert(found_it != leaves.end() && "Leaf not found");

  active_leaf = *found_it;
  leaves.erase(found_it);

  backtrack = (last_eps.find(ep_id) == last_eps.end());
  last_eps.clear();
}

void SearchSpace::add_to_active_leaf(const EP *ep, const BDDNode *node, const ModuleFactory *modgen, const std::vector<impl_t> &implementations) {
  assert(active_leaf && "Active leaf not set");

  for (const impl_t &impl : implementations) {
    const ss_node_id_t id = node_id_counter++;
    const ep_id_t ep_id   = impl.result->get_id();
    const Score score     = hcfg->score(impl.result.get());

    const TargetType target = modgen->get_target();
    const BDDNode *next     = impl.result->get_next_node();

    std::stringstream description_ss;
    for (const auto &[key, value] : impl.decision.params) {
      description_ss << key << "=" << value << " ";
    }

    const module_data_t module_data{
        .type          = modgen->get_type(),
        .name          = modgen->get_name(),
        .description   = description_ss.str(),
        .bdd_reordered = impl.bdd_reordered,
        .hit_rate      = ep->get_active_leaf_hit_rate(),
    };

    const bdd_node_data_t bdd_node_data{
        .id          = node->get_id(),
        .description = get_bdd_node_description(node),
    };

    std::optional<bdd_node_data_t> next_bdd_node_data;
    if (next) {
      next_bdd_node_data = {
          .id          = next->get_id(),
          .description = get_bdd_node_description(next),
      };
    }

    const std::vector<heuristic_metadata_t> metadata = hcfg->get_metadata(impl.result.get());

    SSNode *new_node = new SSNode(id, ep_id, score, target, module_data, bdd_node_data, next_bdd_node_data, metadata);

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
} // namespace LibSynapse