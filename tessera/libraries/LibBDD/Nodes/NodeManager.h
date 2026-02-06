#pragma once

#include <LibBDD/Nodes/Node.h>

#include <memory>
#include <vector>

namespace LibBDD {

class BDDNodeManager {
private:
  std::vector<std::unique_ptr<BDDNode>> nodes;

public:
  BDDNodeManager() {}
  BDDNodeManager(BDDNodeManager &&other) = default;

  void add_node(BDDNode *node) { nodes.emplace_back(node); }

  void free_node(BDDNode *node, bool recursive = false) {
    std::vector<BDDNode *> to_free;

    if (recursive) {
      to_free = node->get_mutable_reachable_nodes();
    } else {
      to_free.push_back(node);
    }

    for (BDDNode *to_free_node : to_free) {
      auto it = std::find_if(nodes.begin(), nodes.end(), [to_free_node](const std::unique_ptr<BDDNode> &n) { return n.get() == to_free_node; });
      if (it != nodes.end()) {
        assert((*it)->get_id() == to_free_node->get_id() && "Invalid node id");
        nodes.erase(it);
      }
    }
  }

  bool has_node(const BDDNode *node) const {
    return std::any_of(nodes.begin(), nodes.end(), [node](const std::unique_ptr<BDDNode> &n) { return n.get() == node; });
  }

  BDDNodeManager operator+(const BDDNodeManager &other) const {
    BDDNodeManager manager;
    for (const std::unique_ptr<BDDNode> &node : nodes)
      node->clone(manager);
    for (const std::unique_ptr<BDDNode> &node : other.nodes)
      node->clone(manager);
    return manager;
  }

  bool operator==(const BDDNodeManager &other) const { return nodes == other.nodes; }
};

} // namespace LibBDD