#pragma once

#include <LibCore/Types.h>
#include <LibClone/NF.h>
#include <LibClone/Port.h>

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
  NetworkNodeId id;
  NodeType type;
  const NF *nf;
  std::unordered_map<Port, std::pair<Port, const NetworkNode *>> links;

public:
  NetworkNode(NetworkNodeId _id, const NF *_nf, Port _port) : id(_id), type(NodeType::NF), nf(_nf) { assert(nf != nullptr); }
  NetworkNode(NetworkNodeId _id, Port _port) : id(_id), type(NodeType::GLOBAL_PORT), nf(nullptr) {}

  NetworkNodeId get_id() const { return id; }
  NodeType get_node_type() const { return type; }
  const NF *get_nf() const { return nf; }

  const std::unordered_map<Port, std::pair<Port, const NetworkNode *>> &get_links() const { return links; }

  bool has_link(const Port port) const { return links.find(port) != links.end(); }

  const std::pair<Port, const NetworkNode *> &get_connected_node(const Port port) const {
    assert(links.find(port) != links.end());
    return links.at(port);
  }

  void add_link(Port port_from, Port port_to, const NetworkNode *node) { links[port_from] = {port_to, node}; }

  void debug() const {
    std::cerr << id << "{";
    for (const auto &[sport, destination] : links) {
      std::cerr << "(" << sport << "->" << destination.first << ":" << destination.second->get_id() << "),";
    }
  }
};

} // namespace LibClone
