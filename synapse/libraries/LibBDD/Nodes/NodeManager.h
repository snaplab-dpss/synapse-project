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

  void free_node(BDDNode *node) {
    auto it = std::find_if(nodes.begin(), nodes.end(), [node](const std::unique_ptr<BDDNode> &n) { return n.get() == node; });

    if (it != nodes.end()) {
      assert((*it)->get_id() == node->get_id() && "Invalid node id");
      nodes.erase(it);
    }
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