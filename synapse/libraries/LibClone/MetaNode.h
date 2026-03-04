#pragma once

#include <LibCore/Types.h>
#include <LibClone/PhysicalNetwork.h>

#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace LibClone {

using LibClone::ComponentId;
using LibClone::DeviceId;
using LibClone::Port;

using func_name_t = std::string;
using arg_name_t  = std::string;

using MetaNodeId = u16;

struct PairHash {
  template <class T1, class T2> std::size_t operator()(const std::pair<T1, T2> &p) const {
    auto h1 = std::hash<T1>{}(p.first);
    auto h2 = std::hash<T2>{}(p.second);
    return h1 ^ (h2 << 1);
  }
};

enum class MetaNodeType {
  DATA_STRUCTURE,
  GLOBAL_PORT,
  PROCESS,
};

class MetaNode {
private:
  MetaNodeId id;
  MetaNodeType type;

  std::vector<ComponentId> components;
  std::unordered_set<std::pair<arg_name_t, addr_t>, PairHash> data_structures;
  Port port;

  u64 total_cpu = 0;
  u64 total_mem = 0;

  DeviceId assigned_device = -1;

public:
  MetaNode(const MetaNodeId &id);
  MetaNode(const MetaNodeId &id, const Port &port);
  MetaNode(const MetaNodeId &id, const std::unordered_set<std::pair<arg_name_t, addr_t>, PairHash> &&data_structures, const u64 &mem_req);

  // Component management
  void add_component(ComponentId comp_id, u64 cpu_req, u64 mem_req);
  bool has_component(ComponentId comp_id) const;
  const std::vector<ComponentId> &get_components() const;

  // Data structure management
  void add_ds_component(const ComponentId &comp_id, const std::pair<arg_name_t, addr_t> &ds, u64 cpu_req, u64 mem_req);
  bool has_data_structure(const std::pair<arg_name_t, addr_t> &ds) const;
  const std::unordered_set<std::pair<arg_name_t, addr_t>, PairHash> &get_data_structures() const;

  // Global port management
  void add_global_port_component(const ComponentId &comp_id, const Port &port, u64 cpu_req, u64 mem_req);
  bool has_global_port(Port port) const;
  const Port &get_global_port() const;

  // Placement
  void set_assigned_device(DeviceId device);
  DeviceId get_assigned_device() const;
  bool is_fixed() const; // true if has global port (device pre-determined)

  // Aggregated requirements
  u32 get_total_cpu() const;
  u32 get_total_mem() const;

  static std::shared_ptr<MetaNode> merge(const std::shared_ptr<MetaNode> &m1, const std::shared_ptr<MetaNode> &m2);
  static std::shared_ptr<MetaNode> merge(const std::vector<std::shared_ptr<MetaNode>> &nodes);

  // Identification
  const MetaNodeId &get_id() const;
  MetaNodeType get_type() const;

  void debug() const;
};

} // namespace LibClone
