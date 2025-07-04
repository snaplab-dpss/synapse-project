#pragma once

#include <LibCore/Types.h>

#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <cassert>

namespace LibClone {

using NetworkNodeId = std::string;
enum class NodeType { GLOBAL_PORT, NF };

class NetworkNode {
private:
  const NetworkNodeId name;
  const NodeType node_type;

  std::unordered_map<u32, std::pair<u32, const NetworkNode *>> children;

public:
  NetworkNode(const NetworkNodeId &_name, NodeType _node_type) : name(_name), node_type(_node_type), children() {}

  ~NetworkNode() = default;

  const NetworkNodeId &get_name() const { return name; }
  NodeType get_node_type() const { return node_type; }

  const std::unordered_map<u32, std::pair<u32, const NetworkNode *>> &get_children() const { return children; }

  bool has_child(const u32 port) const { return children.find(port) != children.end(); }

  const std::pair<u32, const NetworkNode *> &get_child(const u32 port) const {
    assert(children.find(port) != children.end());
    return children.at(port);
  }

  void add_child(u32 port_from, u32 port_to, const NetworkNode *node) { children[port_from] = {port_to, node}; }

  void debug() const { std::cerr << "Node{name=" << name << ",type=" << (node_type == NodeType::GLOBAL_PORT ? "GLOBAL_PORT" : "NF") << "}"; }
};

} // namespace LibClone
