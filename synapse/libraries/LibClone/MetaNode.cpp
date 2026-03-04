#include <LibClone/MetaNode.h>

namespace LibClone {

MetaNode::MetaNode(const MetaNodeId &_id) : id(_id), type(MetaNodeType::PROCESS) { add_component(_id, 1, 0); }
MetaNode::MetaNode(const MetaNodeId &_id, const Port &_port) : id(_id), type(MetaNodeType::GLOBAL_PORT), port(_port) { add_component(_id, 1, 0); }
MetaNode::MetaNode(const MetaNodeId &_id, const std::unordered_set<std::pair<arg_name_t, addr_t>, PairHash> &&_data_structures, const u64 &_mem_req)
    : id(_id), type(MetaNodeType::DATA_STRUCTURE), data_structures(_data_structures), total_mem(_mem_req) {
  add_component(_id, 1, 0);
}

void MetaNode::add_component(ComponentId comp_id, u64 cpu_req, u64 mem_req) {
  assert(!has_component(comp_id) && "Component already exists in meta-node");
  components.push_back(comp_id);
  total_cpu += cpu_req;
  total_mem += mem_req;
}

bool MetaNode::has_component(ComponentId comp_id) const { return std::find(components.begin(), components.end(), comp_id) != components.end(); }

const std::vector<ComponentId> &MetaNode::get_components() const { return components; }

void MetaNode::add_ds_component(const ComponentId &comp_id, const std::pair<arg_name_t, addr_t> &ds, u64 cpu_req, u64 mem_req) {
  assert(type == MetaNodeType::DATA_STRUCTURE && "Only data structure meta-nodes can have data structures");
  assert(has_data_structure(ds) && "Data structure must be associated with the meta-node before adding a component");
  add_component(comp_id, cpu_req, mem_req);
}

bool MetaNode::has_data_structure(const std::pair<arg_name_t, addr_t> &ds) const {
  assert(type == MetaNodeType::DATA_STRUCTURE && "Only data structure meta-nodes can have data structures");
  for (const auto &[arg, ds_addr] : data_structures) {
    if (ds.first == arg && ds.second == ds_addr) {
      return true;
    }
  }
  return false;
}

const std::unordered_set<std::pair<arg_name_t, addr_t>, PairHash> &MetaNode::get_data_structures() const {
  assert(type == MetaNodeType::DATA_STRUCTURE && "Only data structure meta-nodes can have data structures");
  return data_structures;
}

void MetaNode::add_global_port_component(const ComponentId &comp_id, const Port &c_port, u64 cpu_req, u64 mem_req) {
  assert_or_panic(type == MetaNodeType::GLOBAL_PORT, "Only global port meta-nodes can have global ports");
  assert_or_panic(has_global_port(c_port), "Global port must be associated with the meta-node before adding a component");
  add_component(comp_id, cpu_req, mem_req);
}

bool MetaNode::has_global_port(Port c_port) const {
  assert(type == MetaNodeType::GLOBAL_PORT && "Only global port meta-nodes can have global ports");
  return c_port == this->port;
}

const Port &MetaNode::get_global_port() const {
  assert(type == MetaNodeType::GLOBAL_PORT && "Only global port meta-nodes can have global ports");
  return port;
}

void MetaNode::set_assigned_device(DeviceId device) { assigned_device = device; }

DeviceId MetaNode::get_assigned_device() const { return assigned_device; }

bool MetaNode::is_fixed() const { return type == MetaNodeType::GLOBAL_PORT && assigned_device != -1; }

u32 MetaNode::get_total_cpu() const { return total_cpu; }

u32 MetaNode::get_total_mem() const { return total_mem; }

const MetaNodeId &MetaNode::get_id() const { return id; }
MetaNodeType MetaNode::get_type() const { return type; }

std::shared_ptr<MetaNode> MetaNode::merge(const std::shared_ptr<MetaNode> &m1, const std::shared_ptr<MetaNode> &m2) {

  assert_or_panic(m1->get_type() == m2->get_type(), "Meta-nodes must be of the same type to be merged");

  const MetaNodeId new_id = std::min(m1->get_id(), m2->get_id());

  switch (m1->get_type()) {
  case MetaNodeType::PROCESS: {
    std::shared_ptr<MetaNode> merged_node = std::make_shared<MetaNode>(new_id);
    for (const auto &comp_id : m1->get_components()) {
      merged_node->add_component(comp_id, 1, 0);
    }
    for (const auto &comp_id : m2->get_components()) {
      if (!merged_node->has_component(comp_id)) {
        merged_node->add_component(comp_id, 1, 0);
      }
    }
    return merged_node;
  } break;
  case MetaNodeType::GLOBAL_PORT: {
    assert_or_panic(m1->get_global_port() == m2->get_global_port(), "Global ports must be the same to be merged");
    assert_or_panic(m1->get_assigned_device() == m2->get_assigned_device() && m1->get_assigned_device() != -1 && m2->get_assigned_device() != -1,
                    "Assigned devices must be the same to be merged");
    std::shared_ptr<MetaNode> merged_node = std::make_shared<MetaNode>(new_id, m1->get_global_port());
    for (const auto &comp_id : m1->get_components()) {
      merged_node->add_component(comp_id, 1, 0);
    }
    for (const auto &comp_id : m2->get_components()) {
      if (!merged_node->has_component(comp_id)) {
        merged_node->add_component(comp_id, 1, 0);
      }
    }
    return merged_node;
  } break;
  case MetaNodeType::DATA_STRUCTURE: {
    std::unordered_set<std::pair<arg_name_t, addr_t>, PairHash> merged_data_structures = m1->get_data_structures();
    merged_data_structures.insert(m2->get_data_structures().begin(), m2->get_data_structures().end());

    std::shared_ptr<MetaNode> merged_node =
        std::make_shared<MetaNode>(new_id, std::move(merged_data_structures), m1->get_total_mem() + m2->get_total_mem());
    for (const auto &comp_id : m1->get_components()) {
      merged_node->add_component(comp_id, 1, 0);
    }
    for (const auto &comp_id : m2->get_components()) {
      if (!merged_node->has_component(comp_id)) {
        merged_node->add_component(comp_id, 1, 0);
      }
    }

    return merged_node;
  } break;
  }

  panic("Undefined MetaNodeType in merge function");
}

std::shared_ptr<MetaNode> MetaNode::merge(const std::vector<std::shared_ptr<MetaNode>> &nodes) {
  assert_or_panic(!nodes.empty(), "Cannot merge an empty list of meta-nodes");
  assert_or_panic(nodes.size() >= 2, "At least two meta-nodes are required for merging");

  std::shared_ptr<MetaNode> merged_node = nodes[0];
  for (size_t i = 1; i < nodes.size(); ++i) {
    merged_node = merge(merged_node, nodes[i]);
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
    for (const auto &[arg, addr] : data_structures) {
      std::cerr << "    { Arg: " << arg << ", Addr: " << addr << " }\n";
    }
    std::cerr << "}\n";
  } break;

  default:
    break;
  }

  std::cerr << "Components {\n";
  for (const auto &comp_id : components) {
    std::cerr << "      { Component ID: " << comp_id << " }\n";
  }
  std::cerr << "}\n";
  std::cerr << "Total CPU: " << total_cpu << "\n";
  std::cerr << "Total Mem: " << total_mem << "\n";
  std::cerr << "Assigned Device: " << assigned_device << "\n";
}
} // namespace LibClone
