#include "utils.h"

namespace BDD {

std::vector<Node_ptr> get_call_nodes(Node_ptr root,
                                     const std::vector<std::string> &fnames) {
  auto targets = std::vector<Node_ptr>();
  auto nodes = std::vector<Node_ptr>{root};

  while (nodes.size()) {
    auto node = *nodes.begin();
    nodes.erase(nodes.begin());

    if (node->get_type() == BDD::Node::NodeType::BRANCH) {
      auto branch_node = cast_node<Branch>(node);
      nodes.push_back(branch_node->get_on_true());
      nodes.push_back(branch_node->get_on_false());
      continue;
    }

    if (node->get_type() != BDD::Node::NodeType::CALL) {
      continue;
    }

    auto call_node = cast_node<Call>(node);
    auto call = call_node->get_call();

    nodes.push_back(call_node->get_next());

    auto found_it = std::find(fnames.begin(), fnames.end(), call.function_name);

    if (found_it == fnames.end()) {
      continue;
    }

    targets.push_back(node);
  }

  return targets;
}

} // namespace BDD