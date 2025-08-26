#pragma once

#include <LibCore/Types.h>
#include <LibClone/NF.h>

#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <cassert>

namespace LibClone {

using Port          = u32;
using NetworkNodeId = std::string;

enum class NetworkNodeType { GLOBAL_PORT, NF, DEVICE };

class NetworkNode {
private:
  NetworkNodeId id;
  NetworkNodeType type;
  const NF *nf;
  std::unordered_map<Port, std::pair<Port, const NetworkNode *>> links;

public:
  NetworkNode(const NetworkNodeId &_id, const NF *_nf) : id(_id), type(NetworkNodeType::NF), nf(_nf) { assert(nf != nullptr); }
  NetworkNode(const NetworkNodeId &_id) : id(_id), type(NetworkNodeType::GLOBAL_PORT), nf(nullptr) {}

  NetworkNodeId get_id() const { return id; }
  NetworkNodeType get_node_type() const { return type; }

  const NF *get_nf() const {
    assert(type == NetworkNodeType::NF);
    assert(nf != nullptr);
    return nf;
  }

  const std::unordered_map<Port, std::pair<Port, const NetworkNode *>> &get_links() const { return links; }
  bool has_link(const Port source_port) const { return links.find(source_port) != links.end(); }
  const std::pair<Port, const NetworkNode *> &get_link(const Port source_port) const { return links.at(source_port); }

  bool connects_to_global_port(const Port destination_port) const {
    return std::any_of(links.begin(), links.end(), [destination_port](const std::pair<Port, std::pair<Port, const NetworkNode *>> &link) {
      return link.second.first == destination_port && link.second.second->get_node_type() == NetworkNodeType::GLOBAL_PORT;
    });
  }

  const std::pair<Port, const NetworkNode *> &get_connected_node(const Port port) const {
    assert(links.find(port) != links.end());
    return links.at(port);
  }

  void add_link(Port port_from, Port port_to, const NetworkNode *node) { links[port_from] = {port_to, node}; }

  friend std::ostream &operator<<(std::ostream &os, const NetworkNode &node) {
    os << node.id << "{";
    for (const auto &[sport, destination] : node.links) {
      os << "(" << sport << "->" << destination.first << ":" << destination.second->get_id() << "),";
    }
    os << "}";
    return os;
  }
};

} // namespace LibClone
