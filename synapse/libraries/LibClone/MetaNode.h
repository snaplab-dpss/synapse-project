#pragma once

#include <LibCore/Types.h>
#include <LibClone/PhysicalNetwork.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace LibClone {

using LibBDD::bdd_node_id_t;
using LibBDD::bdd_node_ids_t;
using LibClone::DeviceId;
using LibClone::Port;

using func_name_t = std::string;
using arg_name_t  = std::string;

using MetaNodeId = u16;

enum class MetaNodeType {
  DATA_STRUCTURE,
  GLOBAL_PORT,
  PROCESS,
};

class MetaNode {
private:
  MetaNodeId id;
  MetaNodeType type;

  std::unordered_map<arg_name_t, std::unordered_set<addr_t>> data_structures;
  Port port;

  bdd_node_ids_t nodes;

  DeviceId assigned_device = -1;

public:
  MetaNode(const MetaNodeId &id, const bdd_node_ids_t &node);
  MetaNode(const MetaNodeId &id, const Port &port, const bdd_node_ids_t &nodes);
  MetaNode(const MetaNodeId &id, const std::unordered_map<arg_name_t, std::unordered_set<addr_t>> &&data_structures, const bdd_node_ids_t &nodes);

  // Node management
  void add_node(const bdd_node_id_t &node_id);
  bool has_node(const bdd_node_id_t &node_id) const;
  const bdd_node_ids_t &get_nodes() const;

  // Data structure management
  void add_ds_node(const bdd_node_id_t &node_id, const arg_name_t &ds, const addr_t &addr);
  bool has_data_structure(const arg_name_t &ds, const addr_t &addr) const;
  void add_data_structure(const arg_name_t &ds, const addr_t &addr);
  const std::unordered_map<arg_name_t, std::unordered_set<addr_t>> &get_data_structures() const;

  // Global port management
  void add_global_port_node(const bdd_node_id_t &node_id, const Port &port);
  bool has_global_port(const Port &port) const;
  const Port &get_global_port() const;

  // Placement
  void set_assigned_device(DeviceId device);
  DeviceId get_assigned_device() const;
  bool is_fixed() const; // true if has global port (device pre-determined)

  static std::unique_ptr<MetaNode> merge(std::unique_ptr<MetaNode> m1, std::unique_ptr<MetaNode> m2);
  static std::unique_ptr<MetaNode> merge(std::vector<std::unique_ptr<MetaNode>> nodes);

  // Identification
  const MetaNodeId &get_id() const;
  MetaNodeType get_type() const;

  void debug() const;
};

class MetaNodes {
private:
  std::vector<std::unique_ptr<MetaNode>> nodes;

  std::vector<std::unique_ptr<MetaNode>>::const_iterator find_if(std::function<bool(const std::unique_ptr<MetaNode> &)> predicate) const;
  std::vector<std::unique_ptr<MetaNode>>::iterator find_if(std::function<bool(const std::unique_ptr<MetaNode> &)> predicate);

public:
  MetaNodes();

  MetaNodes(const MetaNodes &other)            = delete;
  MetaNodes &operator=(const MetaNodes &other) = delete;
  MetaNodes(MetaNodes &&other)                 = default;
  MetaNodes &operator=(MetaNodes &&other)      = default;

  // Creation
  MetaNode *create_process(bdd_node_id_t node_id);
  MetaNode *create_for_port(bdd_node_id_t node_id, Port port, DeviceId device);
  MetaNode *create_for_ds(bdd_node_id_t node_id, const arg_name_t &ds, addr_t addr);

  // Lookup
  MetaNode *find_by_ds(addr_t addr) const;
  MetaNode *find_by_ds(const arg_name_t &ds, addr_t addr) const;
  MetaNode *find_by_port(Port port) const;
  MetaNode *find_by_node(bdd_node_id_t node_id) const;
  MetaNode *find(MetaNodeId id) const;

  // Existence checks
  bool has_ds(addr_t addr) const;
  bool has_port(Port port) const;
  bool has_node(bdd_node_id_t node_id) const;

  // Multi-structure queries
  bool are_structures_together(const std::vector<std::pair<arg_name_t, addr_t>> &structures) const;
  std::vector<MetaNode *> find_all_by_ds(const std::vector<std::pair<arg_name_t, addr_t>> &structures) const;

  // Merge
  MetaNode *merge(std::vector<MetaNode *> to_merge);

  // Removal
  void remove(MetaNode *node);

  // Access
  std::vector<const MetaNode *> get_all() const;
  size_t size() const;
  bool empty() const;

  void debug() const;
};

} // namespace LibClone
