#pragma once

#include <LibCore/Debug.h>
#include <LibCore/Types.h>

#include <LibClone/Node.h>
#include <LibClone/Device.h>

#include <unordered_map>

namespace LibClone {

using ComponentId = u32;

class PhysicalNetwork {

private:
  std::unordered_map<InstanceId, std::unique_ptr<Device>> devices;
  std::unordered_map<NetworkNodeId, std::unique_ptr<NetworkNode>> nodes;
  std::unordered_map<ComponentId, LibSynapse::TargetType> placement_strategy;
  std::unordered_map<NetworkNodeId, std::unordered_map<NetworkNodeId, Port>> forwarding_table;

public:
  PhysicalNetwork(std::unordered_map<InstanceId, std::unique_ptr<Device>> &&_devices,
                  std::unordered_map<NetworkNodeId, std::unique_ptr<NetworkNode>> &&_nodes,
                  std::unordered_map<ComponentId, LibSynapse::TargetType> &&_placement_strategy,
                  std::unordered_map<NetworkNodeId, std::unordered_map<NetworkNodeId, Port>> &&_forwarding_table)
      : devices(std::move(_devices)), nodes(std::move(_nodes)), placement_strategy(std::move(_placement_strategy)),
        forwarding_table(std::move(_forwarding_table)){};

  PhysicalNetwork(const PhysicalNetwork &)            = delete;
  PhysicalNetwork &operator=(const PhysicalNetwork &) = delete;

  const std::unordered_map<InstanceId, std::unique_ptr<Device>> &get_devices() const { return devices; }
  const std::unordered_map<NetworkNodeId, std::unique_ptr<NetworkNode>> &get_nodes() const { return nodes; }
  const std::unordered_map<ComponentId, LibSynapse::TargetType> &get_placement_strategy() const { return placement_strategy; }
  const std::unordered_map<NetworkNodeId, std::unordered_map<NetworkNodeId, Port>> &get_forwarding_table() const { return forwarding_table; }

  const LibSynapse::TargetType get_placement(const ComponentId component_id) const;
  const static PhysicalNetwork parse(const std::filesystem::path &file_path);

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
