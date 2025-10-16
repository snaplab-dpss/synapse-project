#pragma once

#include <LibCore/Types.h>
#include <LibClone/Device.h>

#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <cassert>

namespace LibClone {

using Port                 = u32;
using InfrastructureNodeId = i32;

enum class InfrastructureNodeType { GLOBAL_PORT, DEVICE };

class InfrastructureNode {
private:
  InfrastructureNodeId id;
  InfrastructureNodeType type;
  const Device *device;
  std::unordered_map<Port, std::pair<Port, const InfrastructureNode *>> links;

public:
  InfrastructureNode(const InfrastructureNodeId &_id, const Device *_device) : id(_id), type(InfrastructureNodeType::DEVICE), device(_device) {
    assert(device != nullptr);
  }

  InfrastructureNode(const InfrastructureNodeId &_id) : id(_id), type(InfrastructureNodeType::GLOBAL_PORT), device(nullptr) {}

  InfrastructureNodeId get_id() const { return id; }
  InfrastructureNodeType get_node_type() const { return type; }

  const Device *get_device() const {
    assert(type == InfrastructureNodeType::DEVICE);
    assert(device != nullptr);
    return device;
  }

  const std::unordered_map<Port, std::pair<Port, const InfrastructureNode *>> get_links() const { return links; }
  bool has_link(const Port source_port) const { return links.find(source_port) != links.end(); }
  const std::pair<Port, const InfrastructureNode *> &get_link(const Port source_port) { return links.at(source_port); }

  bool connects_to_global_port(const Port destination_port) const {
    return std::any_of(links.begin(), links.end(), [destination_port](const std::pair<Port, std::pair<Port, const InfrastructureNode *>> &link) {
      return link.second.first == destination_port && link.second.second->get_node_type() == InfrastructureNodeType::GLOBAL_PORT;
    });
  }

  const std::pair<Port, const InfrastructureNode *> &get_connected_node(const Port port) const {
    assert(links.find(port) != links.end());
    return links.at(port);
  }
  void add_link(Port port_from, Port port_to, const InfrastructureNode *node) { links[port_from] = {port_to, node}; }

  friend std::ostream &operator<<(std::ostream &os, const InfrastructureNode &node) {
    os << node.id << "{";
    for (const auto &[sport, destination] : node.links) {
      os << "(Src: " << sport << "-> Dst:" << destination.first << " Port:" << destination.second->get_id() << "),";
    }
    os << "}";
    return os;
  }
};
} // namespace LibClone
