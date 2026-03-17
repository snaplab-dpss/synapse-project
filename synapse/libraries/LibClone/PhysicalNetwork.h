#pragma once

#include <LibCore/Debug.h>
#include <LibCore/Types.h>

#include <LibClone/InfrastructureNode.h>
#include <LibClone/Device.h>

#include <unordered_map>

namespace LibClone {

using LibBDD::bdd_node_id_t;

struct LinkInfo {
  Port port;
  u16 cost;
};

using RoutingTable = std::unordered_map<InfrastructureNodeId, std::unordered_map<InfrastructureNodeId, LinkInfo>>;
using CostMatrix   = std::unordered_map<DeviceId, std::unordered_map<DeviceId, u16>>;

class PhysicalNetwork {

private:
  std::unordered_map<DeviceId, std::unique_ptr<Device>> devices;
  std::unordered_map<InfrastructureNodeId, std::unique_ptr<InfrastructureNode>> nodes;
  RoutingTable routing_table;

public:
  PhysicalNetwork(std::unordered_map<DeviceId, std::unique_ptr<Device>> &&_devices,
                  std::unordered_map<InfrastructureNodeId, std::unique_ptr<InfrastructureNode>> &&_nodes, RoutingTable &&_routing_table)
      : devices(std::move(_devices)), nodes(std::move(_nodes)), routing_table(std::move(_routing_table)){};

  PhysicalNetwork(const PhysicalNetwork &)            = delete;
  PhysicalNetwork &operator=(const PhysicalNetwork &) = delete;

  const std::unordered_map<DeviceId, std::unique_ptr<Device>> &get_devices() const { return devices; }
  const std::unordered_map<InfrastructureNodeId, std::unique_ptr<InfrastructureNode>> &get_nodes() const { return nodes; }
  const RoutingTable &get_routing_table() const { return routing_table; }

  bool has_device(const DeviceId device_id) const;
  const Device *get_device(const DeviceId device_id) const;
  const InfrastructureNode *get_node(const InfrastructureNodeId node_id) const;

  Port get_forwarding_port(const InfrastructureNodeId src, const InfrastructureNodeId dst) const;
  const std::vector<std::pair<Port, DeviceId>> find_path(const InfrastructureNodeId src_id, const InfrastructureNodeId dst_id) const;
  const std::set<Port> get_target_ports(const InfrastructureNodeId node_id) const;

  CostMatrix get_cost_matrix() const;

  const static PhysicalNetwork parse(const std::filesystem::path &file_path);
  void debug() const;
};

} // namespace LibClone
