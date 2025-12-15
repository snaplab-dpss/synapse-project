#pragma once

#include <LibCore/Debug.h>
#include <LibCore/Types.h>

#include <LibClone/InfrastructureNode.h>
#include <LibClone/Device.h>

#include <unordered_map>

namespace LibClone {

using ComponentId = LibBDD::bdd_node_id_t;

class PhysicalNetwork {

private:
  std::unordered_map<DeviceId, std::unique_ptr<Device>> devices;
  std::unordered_map<InfrastructureNodeId, std::unique_ptr<InfrastructureNode>> nodes;
  std::unordered_map<ComponentId, LibSynapse::TargetType> placement_strategy;
  std::unordered_map<InfrastructureNodeId, std::unordered_map<InfrastructureNodeId, Port>> forwarding_table;

public:
  PhysicalNetwork(std::unordered_map<DeviceId, std::unique_ptr<Device>> &&_devices,
                  std::unordered_map<InfrastructureNodeId, std::unique_ptr<InfrastructureNode>> &&_nodes,
                  std::unordered_map<ComponentId, LibSynapse::TargetType> &&_placement_strategy,
                  std::unordered_map<InfrastructureNodeId, std::unordered_map<InfrastructureNodeId, Port>> &&_forwarding_table)
      : devices(std::move(_devices)), nodes(std::move(_nodes)), placement_strategy(std::move(_placement_strategy)),
        forwarding_table(std::move(_forwarding_table)){};

  PhysicalNetwork(const PhysicalNetwork &)            = delete;
  PhysicalNetwork &operator=(const PhysicalNetwork &) = delete;

  const std::unordered_map<DeviceId, std::unique_ptr<Device>> &get_devices() const { return devices; }
  const std::unordered_map<InfrastructureNodeId, std::unique_ptr<InfrastructureNode>> &get_nodes() const { return nodes; }
  const std::unordered_map<ComponentId, LibSynapse::TargetType> &get_placement_strategy() const { return placement_strategy; }
  const std::unordered_map<InfrastructureNodeId, std::unordered_map<InfrastructureNodeId, Port>> &get_forwarding_table() const {
    return forwarding_table;
  }

  const LibSynapse::TargetType get_placement(const ComponentId component_id) const;
  bool has_device(const DeviceId device_id) const;
  const Device *get_device(const DeviceId device_id) const;
  const InfrastructureNode *get_node(const InfrastructureNodeId node_id) const;
  Port get_forwarding_port(const InfrastructureNodeId src, const InfrastructureNodeId dst) const;
  const std::vector<std::pair<Port, LibSynapse::TargetType>> find_path(const InfrastructureNodeId src_id, const InfrastructureNodeId dst_id) const;
  const std::unordered_map<LibSynapse::TargetType, bool> get_target_list(const ComponentId root_node = 1) const;
  const static PhysicalNetwork parse(const std::filesystem::path &file_path);

  void add_placement(ComponentId component, LibSynapse::TargetType);

  void debug() const {
    std::cerr << "========== Physical Network ==========\n";
    std::cerr << "Devices:\n";
    for (const auto &[_, device] : devices) {
      std::cerr << "  " << *device << "\n";
    }
    std::cerr << "Nodes:\n";
    for (const auto &[_, node] : nodes) {
      std::cerr << "  " << *node << "\n";
    }

    std::cerr << "Placement Strategy:\n";
    for (const auto &[component_id, node_id] : placement_strategy) {
      std::cerr << "{ Component ID: " << component_id << ", Node ID: " << node_id << " }\n";
    }

    std::cerr << "Forwarding Table:\n";
    for (const auto &[src_node_id, table] : forwarding_table) {
      for (const auto &[dst_node_id, port] : table) {
        std::cerr << "{ Src Node ID: " << src_node_id << ", Dst Node ID: " << dst_node_id << ", Port: " << port << " }\n";
      }
    }
    std::cerr << "=======================================\n";
  }
};

} // namespace LibClone
