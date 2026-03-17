#include <LibClone/MetaNode.h>

#include <LibCore/Debug.h>
#include <algorithm>

namespace LibClone {

MetaNode::MetaNode(const MetaNodeId &_id, const bdd_node_ids_t &_nodes) : id(_id), type(MetaNodeType::PROCESS), nodes(_nodes) {}
MetaNode::MetaNode(const MetaNodeId &_id, const Port &_port, const bdd_node_ids_t &_nodes)
    : id(_id), type(MetaNodeType::GLOBAL_PORT), port(_port), nodes(_nodes) {}
MetaNode::MetaNode(const MetaNodeId &_id, const std::unordered_map<arg_name_t, std::unordered_set<addr_t>> &&_data_structures,
                   const bdd_node_ids_t &_nodes)
    : id(_id), type(MetaNodeType::DATA_STRUCTURE), data_structures(std::move(_data_structures)), nodes(_nodes) {}

void MetaNode::add_node(const bdd_node_id_t &node_id) {
  assert_or_panic(!has_node(node_id), "Node %lu already exists in meta-node", node_id);
  nodes.insert(node_id);
}

bool MetaNode::has_node(const bdd_node_id_t &node_id) const { return nodes.find(node_id) != nodes.end(); }

const bdd_node_ids_t &MetaNode::get_nodes() const { return nodes; }

void MetaNode::add_ds_node(const bdd_node_id_t &node_id, const arg_name_t &ds, const addr_t &addr) {
  assert_or_panic(type == MetaNodeType::DATA_STRUCTURE, "Only data structure meta-nodes can have data structures");
  assert_or_panic(has_data_structure(ds, addr), "Data structure must be associated with the meta-node before adding a component");
  add_node(node_id);
}

bool MetaNode::has_data_structure(const arg_name_t &ds, const addr_t &addr) const {
  assert_or_panic(type == MetaNodeType::DATA_STRUCTURE, "Only data structure meta-nodes can have data structures");
  if (data_structures.find(ds) == data_structures.end()) {
    return false;
  }

  const std::unordered_set<addr_t> stored_addrs = data_structures.at(ds);

  if (stored_addrs.find(addr) == stored_addrs.end()) {
    return false;
  }

  return true;
}

void MetaNode::add_data_structure(const arg_name_t &ds, const addr_t &addr) {
  assert_or_panic(type == MetaNodeType::DATA_STRUCTURE, "Only data structure meta-nodes can have data structures");
  data_structures[ds].insert(addr);
}

const std::unordered_map<arg_name_t, std::unordered_set<addr_t>> &MetaNode::get_data_structures() const {
  assert_or_panic(type == MetaNodeType::DATA_STRUCTURE, "Only data structure meta-nodes can have data structures");
  return data_structures;
}

void MetaNode::add_global_port_node(const bdd_node_id_t &node_id, const Port &c_port) {
  assert_or_panic(type == MetaNodeType::GLOBAL_PORT, "Only global port meta-nodes can have global ports");
  assert_or_panic(has_global_port(c_port), "Global port must be associated with the meta-node before adding a component");
  add_node(node_id);
}

bool MetaNode::has_global_port(const Port &global_port) const {
  assert_or_panic(type == MetaNodeType::GLOBAL_PORT, "Only global port meta-nodes can have global ports");
  return global_port == this->port;
}

const Port &MetaNode::get_global_port() const {
  assert_or_panic(type == MetaNodeType::GLOBAL_PORT, "Only global port meta-nodes can have global ports");
  return port;
}

void MetaNode::set_assigned_device(DeviceId device) { assigned_device = device; }

DeviceId MetaNode::get_assigned_device() const { return assigned_device; }

bool MetaNode::is_fixed() const { return type == MetaNodeType::GLOBAL_PORT && assigned_device != -1; }

const MetaNodeId &MetaNode::get_id() const { return id; }
MetaNodeType MetaNode::get_type() const { return type; }

std::unique_ptr<MetaNode> MetaNode::merge(std::unique_ptr<MetaNode> m1, std::unique_ptr<MetaNode> m2) {

  assert_or_panic(m1->get_type() == m2->get_type(), "Meta-nodes must be of the same type to be merged");

  switch (m1->get_type()) {
  case MetaNodeType::PROCESS: {
    std::unique_ptr<MetaNode> merged_node = std::make_unique<MetaNode>(m1->get_id(), m1->get_nodes());
    for (const bdd_node_id_t &node_id : m2->get_nodes()) {
      merged_node->add_node(node_id);
    }
    return merged_node;
  } break;
  case MetaNodeType::GLOBAL_PORT: {
    assert_or_panic(m1->get_global_port() == m2->get_global_port(), "Global ports must be the same to be merged");
    assert_or_panic(m1->get_assigned_device() != -1, "Global Port Node should have a assignment");
    assert_or_panic(m2->get_assigned_device() != -1, "Global Port Node should have a assignment");
    assert_or_panic(m1->get_assigned_device() == m2->get_assigned_device(), "Assigned devices must be the same to be merged");
    std::unique_ptr<MetaNode> merged_node = std::make_unique<MetaNode>(m1->get_id(), m1->get_global_port(), m1->get_nodes());

    for (const bdd_node_id_t &node_id : m2->get_nodes()) {
      merged_node->add_global_port_node(node_id, m2->get_global_port());
    }
    merged_node->set_assigned_device(m1->get_assigned_device());
    return merged_node;
  } break;
  case MetaNodeType::DATA_STRUCTURE: {

    std::unique_ptr<MetaNode> merged_node = std::make_unique<MetaNode>(m1->get_id(), std::move(m1->get_data_structures()), m1->get_nodes());
    const std::unordered_map<arg_name_t, std::unordered_set<addr_t>> m2_ds = m2->get_data_structures();

    for (const auto &[ds, addrs] : m2_ds) {
      for (const addr_t &addr : addrs) {
        merged_node->add_data_structure(ds, addr);
      }
    }
    for (const bdd_node_id_t &node_id : m2->get_nodes()) {
      merged_node->add_node(node_id);
    }

    return merged_node;
  } break;
  }

  panic("Undefined MetaNodeType in merge function");
}

std::unique_ptr<MetaNode> MetaNode::merge(std::vector<std::unique_ptr<MetaNode>> nodes) {
  assert_or_panic(nodes.size() >= 2, "At least two meta-nodes are required for merging");

  std::unique_ptr<MetaNode> merged_node = std::move(nodes[0]);
  for (size_t i = 1; i < nodes.size(); ++i) {
    merged_node = merge(std::move(merged_node), std::move(nodes[i]));
  }

  return merged_node;
}

void MetaNode::debug() const {
  std::cerr << "MetaNode ID: " << id << "\n";
  switch (type) {
  case MetaNodeType::PROCESS: {
    std::cerr << "Type: PROCESS\n";
  } break;
  case MetaNodeType::GLOBAL_PORT: {
    std::cerr << "Type: GLOBAL_PORT\n";
    std::cerr << "Global Port: " << port << "\n";
  } break;
  case MetaNodeType::DATA_STRUCTURE: {
    std::cerr << "Type: DATA_STRUCTURE\n";
    std::cerr << "Data Structures: {\n";
    for (const auto &[ds, addrs] : data_structures) {
      for (const addr_t &addr : addrs) {
        std::cerr << "    { DS: " << ds << ", Addr: " << addr << " }\n";
      }
    }
    std::cerr << "}\n";
  } break;

  default:
    break;
  }

  std::cerr << "Nodes {\n";
  for (const bdd_node_id_t &node_id : nodes) {
    std::cerr << "      { Node ID: " << node_id << " }\n";
  }
  std::cerr << "}\n";
  std::cerr << "#Nodes: " << nodes.size() << "\n";
  std::cerr << "Assigned Device: " << assigned_device << "\n";
}

// MetaNodes class

MetaNodes::MetaNodes() = default;

std::vector<std::unique_ptr<MetaNode>>::const_iterator MetaNodes::find_if(std::function<bool(const std::unique_ptr<MetaNode> &)> predicate) const {
  return std::find_if(nodes.begin(), nodes.end(), predicate);
}

std::vector<std::unique_ptr<MetaNode>>::iterator MetaNodes::find_if(std::function<bool(const std::unique_ptr<MetaNode> &)> predicate) {
  return std::find_if(nodes.begin(), nodes.end(), predicate);
}

MetaNode *MetaNodes::create_process(bdd_node_id_t node_id) {
  std::unique_ptr<MetaNode> node = std::make_unique<MetaNode>(node_id, bdd_node_ids_t{node_id});
  MetaNode *ptr                  = node.get();
  nodes.push_back(std::move(node));
  return ptr;
}

MetaNode *MetaNodes::create_for_port(bdd_node_id_t node_id, Port port, DeviceId device) {
  std::unique_ptr<MetaNode> node = std::make_unique<MetaNode>(node_id, port, bdd_node_ids_t{node_id});
  node->set_assigned_device(device);
  MetaNode *ptr = node.get();
  nodes.push_back(std::move(node));
  return ptr;
}

MetaNode *MetaNodes::create_for_ds(bdd_node_id_t node_id, const arg_name_t &ds, addr_t addr) {
  std::unordered_map<arg_name_t, std::unordered_set<addr_t>> ds_map;
  ds_map[ds].insert(addr);
  std::unique_ptr<MetaNode> node = std::make_unique<MetaNode>(node_id, std::move(ds_map), bdd_node_ids_t{node_id});
  MetaNode *ptr                  = node.get();
  nodes.push_back(std::move(node));
  return ptr;
}

MetaNode *MetaNodes::find_by_ds(addr_t addr) const {
  std::vector<std::unique_ptr<MetaNode>>::const_iterator it = std::find_if(nodes.begin(), nodes.end(), [addr](const std::unique_ptr<MetaNode> &n) {
    if (n->get_type() != MetaNodeType::DATA_STRUCTURE)
      return false;
    const std::unordered_map<arg_name_t, std::unordered_set<addr_t>> &ds_map = n->get_data_structures();
    for (const std::pair<const arg_name_t, std::unordered_set<addr_t>> &entry : ds_map) {
      const std::unordered_set<addr_t> &addrs = entry.second;
      if (addrs.count(addr))
        return true;
    }
    return false;
  });
  return (it != nodes.end()) ? it->get() : nullptr;
}

MetaNode *MetaNodes::find_by_ds(const arg_name_t &ds, addr_t addr) const {
  std::vector<std::unique_ptr<MetaNode>>::const_iterator it =
      std::find_if(nodes.begin(), nodes.end(), [&ds, addr](const std::unique_ptr<MetaNode> &n) {
        if (n->get_type() != MetaNodeType::DATA_STRUCTURE)
          return false;
        const std::unordered_map<arg_name_t, std::unordered_set<addr_t>> &ds_map         = n->get_data_structures();
        std::unordered_map<arg_name_t, std::unordered_set<addr_t>>::const_iterator found = ds_map.find(ds);
        return found != ds_map.end() && found->second.count(addr);
      });
  return (it != nodes.end()) ? it->get() : nullptr;
}

MetaNode *MetaNodes::find_by_port(Port port) const {
  std::vector<std::unique_ptr<MetaNode>>::const_iterator it = std::find_if(nodes.begin(), nodes.end(), [port](const std::unique_ptr<MetaNode> &n) {
    return n->get_type() == MetaNodeType::GLOBAL_PORT && n->get_global_port() == port;
  });
  return (it != nodes.end()) ? it->get() : nullptr;
}

MetaNode *MetaNodes::find_by_node(bdd_node_id_t node_id) const {
  std::vector<std::unique_ptr<MetaNode>>::const_iterator it =
      std::find_if(nodes.begin(), nodes.end(), [node_id](const std::unique_ptr<MetaNode> &n) { return n->has_node(node_id); });
  return (it != nodes.end()) ? it->get() : nullptr;
}

MetaNode *MetaNodes::find(MetaNodeId id) const {
  std::vector<std::unique_ptr<MetaNode>>::const_iterator it =
      std::find_if(nodes.begin(), nodes.end(), [id](const std::unique_ptr<MetaNode> &n) { return n->get_id() == id; });
  return (it != nodes.end()) ? it->get() : nullptr;
}

bool MetaNodes::has_ds(addr_t addr) const { return find_by_ds(addr) != nullptr; }

bool MetaNodes::has_port(Port port) const { return find_by_port(port) != nullptr; }

bool MetaNodes::has_node(bdd_node_id_t node_id) const { return find_by_node(node_id) != nullptr; }

bool MetaNodes::are_structures_together(const std::vector<std::pair<arg_name_t, addr_t>> &structures) const {
  if (structures.empty())
    return true;

  std::set<MetaNodeId> found_ids;
  for (const std::pair<arg_name_t, addr_t> &entry : structures) {
    const arg_name_t &arg = entry.first;
    const addr_t addr     = entry.second;
    MetaNode *node        = find_by_ds(arg, addr);
    if (!node)
      return false;
    found_ids.insert(node->get_id());
  }
  return found_ids.size() == 1;
}

std::vector<MetaNode *> MetaNodes::find_all_by_ds(const std::vector<std::pair<arg_name_t, addr_t>> &structures) const {
  std::vector<MetaNode *> result;
  std::set<MetaNodeId> added;
  for (const std::pair<arg_name_t, addr_t> &entry : structures) {
    const arg_name_t &arg = entry.first;
    const addr_t addr     = entry.second;
    MetaNode *node        = find_by_ds(arg, addr);
    if (node) {
      std::pair<std::set<MetaNodeId>::iterator, bool> inserted = added.insert(node->get_id());
      if (inserted.second) {
        result.push_back(node);
      }
    }
  }
  return result;
}

MetaNode *MetaNodes::merge(std::vector<MetaNode *> to_merge) {
  assert_or_panic(to_merge.size() >= 2, "Need at least 2 nodes to merge");

  std::vector<std::unique_ptr<MetaNode>> extracted;
  for (MetaNode *ptr : to_merge) {
    std::vector<std::unique_ptr<MetaNode>>::iterator it =
        std::find_if(nodes.begin(), nodes.end(), [ptr](const std::unique_ptr<MetaNode> &n) { return n.get() == ptr; });
    assert_or_panic(it != nodes.end(), "Node to merge not found in container");
    extracted.push_back(std::move(*it));
    *it = nullptr;
  }

  std::vector<std::unique_ptr<MetaNode>>::iterator new_end =
      std::remove_if(nodes.begin(), nodes.end(), [](const std::unique_ptr<MetaNode> &n) { return n == nullptr; });
  nodes.erase(new_end, nodes.end());

  std::unique_ptr<MetaNode> merged = MetaNode::merge(std::move(extracted));
  MetaNode *result                 = merged.get();
  nodes.push_back(std::move(merged));
  return result;
}

void MetaNodes::remove(MetaNode *node) {
  std::vector<std::unique_ptr<MetaNode>>::iterator new_end =
      std::remove_if(nodes.begin(), nodes.end(), [node](const std::unique_ptr<MetaNode> &n) { return n.get() == node; });
  nodes.erase(new_end, nodes.end());
}

std::vector<const MetaNode *> MetaNodes::get_all() const {
  std::vector<const MetaNode *> result;
  result.reserve(nodes.size());
  for (const auto &ptr : nodes) {
    result.push_back(ptr.get());
  }
  return result;
}

void MetaNodes::debug() const {
  std::cerr << "========== MetaNodes ==========\n";
  std::cerr << "Count: " << nodes.size() << "\n";
  for (const std::unique_ptr<MetaNode> &node : nodes) {
    std::cerr << "----------------------------------\n";
    node->debug();
  }
}

} // namespace LibClone
