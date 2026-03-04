#include "LibClone/MetaNode.h"
#include <LibClone/EmbeddingEngine.h>

#include <LibCore/Solver.h>

#include <LibCore/Debug.h>
#include <LibCore/Types.h>

#include <LibBDD/CallPath.h>
#include <LibBDD/Nodes/Node.h>
#include <LibBDD/Visitors/BDDVisualizer.h>

#include <cassert>
#include <klee/util/Ref.h>
#include <queue>
#include <set>
#include <unordered_map>
#include <vector>

namespace LibClone {

using LibCore::expr_addr_to_obj_addr;
using LibCore::expr_to_string;

using LibCore::solver_toolbox;
using LibCore::symbol_t;
using LibCore::symbol_translation_t;
using LibCore::SymbolManager;
using LibCore::Symbols;

using LibBDD::arg_t;
using LibBDD::BDD;
using LibBDD::bdd_node_id_t;
using LibBDD::BDDNode;
using LibBDD::BDDNodeType;
using LibBDD::BDDNodeVisitAction;
using LibBDD::BDDViz;
using LibBDD::Branch;
using LibBDD::Call;
using LibBDD::call_t;
using LibBDD::Route;
using LibBDD::RouteOp;

using LibClone::ComponentId;
using LibClone::DeviceId;
using LibClone::InfrastructureNode;
using LibClone::InfrastructureNodeId;
using LibClone::MetaNode;
using LibClone::Port;

const std::unordered_map<func_name_t, std::pair<arg_name_t, arg_name_t>> alloc_func_data_structures{
    {"map_allocate", {"map_out", "map"}}, {"vector_allocate", {"vector_out", "vector"}}, {"dchain_allocate", {"chain_out", "chain"}},
    {"cms_allocate", {"cms_out", "cms"}}, {"lpm_allocate", {"lpm_out", "lpm"}},          {"tb_allocate", {"tb_out", "tb"}},
};

const std::unordered_map<func_name_t, std::vector<arg_name_t>> func_data_structures{

    {"map_get", {"map"}},
    {"map_put", {"map"}},
    {"map_erase", {"map"}},
    {"map_size", {"map"}},
    {"expire_items_single_map", {"map", "vector", "chain"}},
    {"expire_items_single_map_iteratively", {"map", "vector"}},

    // vector operations
    {"vector_borrow", {"vector"}},
    {"vector_return", {"vector"}},
    {"vector_clear", {"vector"}},
    {"vector_sample_lt", {"vector"}},

    // dchain operations
    {"dchain_allocate_new_index", {"chain"}},
    {"dchain_rejuvenate_index", {"chain"}},
    {"dchain_expire_one_index", {"chain"}},
    {"dchain_is_index_allocated", {"chain"}},
    {"dchain_free_index", {"chain"}},

    // cms operations
    {"cms_increment", {"cms"}},
    {"cms_count_min", {"cms"}},
    {"cms_periodic_cleanup", {"cms"}},

    // lpm operations
    {"lpm_free", {"lpm"}},
    {"lpm_from_file", {"lpm"}},
    {"lpm_update", {"lpm"}},
    {"lpm_lookup", {"lpm"}},

    // tb operations
    {"tb_is_tracing", {"tb"}},
    {"tb_trace", {"tb"}},
    {"tb_update_and_check", {"tb"}},
    {"tb_expire", {"tb"}},
};

const std::unordered_map<func_name_t, std::vector<arg_name_t>> alloc_size_params{
    {"map_allocate", {"capacity", "key_size"}}, {"vector_allocate", {"capacity", "elem_size"}}, {"dchain_allocate", {"index_range"}},
    {"cms_allocate", {"height", "width"}},      {"tb_allocate", {"capacity", "key_size"}},      {"lpm_allocate", {}},
};

namespace {

bool are_structures_in_same_MetaNode(std::unordered_set<std::pair<arg_name_t, addr_t>, PairHash> accessed_structures,
                                     const std::unordered_map<addr_t, MetaNodeId> &data_structures_meta_nodes) {
  std::set<MetaNodeId> meta_node_ids;
  for (const auto &[arg_name, addr] : accessed_structures) {
    if (data_structures_meta_nodes.find(addr) == data_structures_meta_nodes.end()) {
      return false;
    } else if (meta_node_ids.empty()) {
      meta_node_ids.insert(data_structures_meta_nodes.at(addr));
    } else if (meta_node_ids.find(data_structures_meta_nodes.at(addr)) == meta_node_ids.end()) {
      return false;
    }
  }
  return true;
}

u64 extract_mem_requirements(const Call *call_node) {

  const call_t &call           = call_node->get_call();
  const std::string &func_name = call.function_name;

  u64 size = 1;

  auto it = alloc_size_params.find(func_name);
  assert_or_panic(it != alloc_size_params.end(), "Allocator function should have its size parameters defined");

  const std::vector<arg_name_t> &param_names = it->second;

  for (const arg_name_t &param : param_names) {

    auto arg_it = call.args.find(param);
    assert_or_panic(arg_it != call.args.end(), "Target function should have the parameter argument");

    klee::ref<klee::Expr> size_expr = arg_it->second.expr;
    assert_or_panic(LibCore::is_constant(size_expr), "Target function parameter argument should be a constant");

    const u64 param_size = solver_toolbox.value_from_expr(size_expr);

    size *= param_size;
  }
  return size;
}

std::unordered_set<std::pair<arg_name_t, addr_t>, PairHash> extract_accessed_structures(const Call *call_node, u64 &is_allocator) {
  std::unordered_set<std::pair<arg_name_t, addr_t>, PairHash> accessed_structures;

  const call_t &call           = call_node->get_call();
  const std::string &func_name = call.function_name;

  auto consumer_it = func_data_structures.find(func_name);
  if (consumer_it != func_data_structures.end()) {
    const std::vector<arg_name_t> &arg_names = consumer_it->second;

    for (const std::string &arg_name : arg_names) {
      auto arg_it = call.args.find(arg_name);
      assert_or_panic(arg_it != call.args.end(), "Target function should have the argument");

      klee::ref<klee::Expr> addr_expr = arg_it->second.expr;
      assert_or_panic(LibCore::is_constant(addr_expr), "Target function argument should be a constant");

      const addr_t addr = expr_addr_to_obj_addr(addr_expr);

      if (accessed_structures.find({arg_name, addr}) == accessed_structures.end()) {
        accessed_structures.insert({arg_name, addr});
      }
    }
  }

  auto allocator_it = alloc_func_data_structures.find(func_name);
  if (allocator_it != alloc_func_data_structures.end()) {
    const std::pair<arg_name_t, arg_name_t> &arg_pair = allocator_it->second;
    const arg_name_t &produced_arg_name               = arg_pair.first;
    const arg_name_t &arg_translation_name            = arg_pair.second;
    auto arg_it                                       = call.args.find(produced_arg_name);
    assert_or_panic(arg_it != call.args.end(), "Target function should have the produced argument");

    klee::ref<klee::Expr> addr_expr = arg_it->second.out;
    assert_or_panic(LibCore::is_constant(addr_expr), "Target function produced argument should be a constant");

    const addr_t addr = expr_addr_to_obj_addr(addr_expr);
    if (accessed_structures.find({arg_translation_name, addr}) == accessed_structures.end()) {
      accessed_structures.insert({arg_translation_name, addr});
    }

    is_allocator = extract_mem_requirements(call_node);
  }

  return accessed_structures;
}

std::pair<Port, DeviceId> concretize_port(const BDDNode *node, const InfrastructureNode *global_ports) {
  assert(node && "Node should not be null");

  switch (node->get_type()) {
  case BDDNodeType::Branch: {

    const Branch *branch                    = dynamic_cast<const Branch *>(node);
    std::unordered_set<std::string> symbols = branch->get_used_symbols();

    assert_or_panic(symbols.find("DEVICE") != symbols.end(), "Branch condition must involve the DEVICE symbol to be concretized as a port");

    klee::ref<klee::Expr> condition = branch->get_condition();
    assert(condition->getKind() == klee::Expr::Kind::Eq && condition->getNumKids() == 2);

    klee::ref<klee::Expr> left  = condition->getKid(0);
    klee::ref<klee::Expr> right = condition->getKid(1);
    assert(left->getKind() == klee::Expr::Kind::Constant);

    u64 global_port = solver_toolbox.value_from_expr(left);
    assert(global_ports->has_link(global_port));
    const Device *device = global_ports->get_link(global_port).second->get_device();

    return std::make_pair(global_port, device->get_id());
  } break;

  case BDDNodeType::Route: {

    const Route *route      = dynamic_cast<const Route *>(node);
    const RouteOp operation = route->get_operation();
    switch (operation) {
    case RouteOp::Forward: {

      klee::ref<klee::Expr> dst_expr = route->get_dst_device();

      u64 global_port = solver_toolbox.value_from_expr(dst_expr);
      assert(global_ports->has_link(global_port));
      const Device *device = global_ports->get_link(global_port).second->get_device();

      return std::make_pair(global_port, device->get_id());

    } break;
    default:
      break;
    }
  } break;
  default:
    break;
  }
  panic("Node cannot be concretized as a port");
}

void pre_process_branch(const BDDNode *node, const InfrastructureNode *global_ports, std::unordered_map<Port, MetaNodeId> &port_meta_nodes,
                        std::unordered_map<MetaNodeId, std::shared_ptr<MetaNode>> &meta_nodes, bool in_root) {
  assert_or_panic(node->get_type() == BDDNodeType::Branch, "pre_process_branch expects a branch node");

  if (in_root) {
    std::pair<Port, DeviceId> port_info = concretize_port(node, global_ports);
    Port global_port                    = port_info.first;
    DeviceId device                     = port_info.second;

    if (port_meta_nodes.find(global_port) == port_meta_nodes.end()) {
      std::shared_ptr<MetaNode> new_node = std::make_shared<MetaNode>(node->get_id(), global_port);
      new_node->set_assigned_device(device);
      assert_or_panic(meta_nodes.find(new_node->get_id()) == meta_nodes.end(), "Meta-node for port should not exist");
      port_meta_nodes.emplace(global_port, new_node->get_id());
      meta_nodes.emplace(new_node->get_id(), new_node);
    } else {
      MetaNodeId existing_node_id = port_meta_nodes.at(global_port);
      assert_or_panic(meta_nodes.find(existing_node_id) != meta_nodes.end(), "Meta-node for port should exist");
      std::shared_ptr<MetaNode> &existing_node = meta_nodes.at(existing_node_id);
      existing_node->add_global_port_component(node->get_id(), global_port, 1, 0);
    }
  } else {
    // In case we are not in the root, we should consider branch nodes as components to be assigned during embeddinng.
    std::shared_ptr<MetaNode> new_node = std::make_shared<MetaNode>(node->get_id());
    assert_or_panic(meta_nodes.find(new_node->get_id()) == meta_nodes.end(), "Meta-node for port should not exist");
    meta_nodes.emplace(new_node->get_id(), new_node);
  }
}

void pre_process_call(const BDDNode *node, std::unordered_map<addr_t, MetaNodeId> &data_structures_meta_nodes,
                      std::unordered_map<MetaNodeId, std::shared_ptr<MetaNode>> &meta_nodes) {
  assert_or_panic(node->get_type() == BDDNodeType::Call, "pre_process_call expects a call node");

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t call     = call_node->get_call();

  u64 is_allocator = 0;

  std::unordered_set<std::pair<arg_name_t, addr_t>, PairHash> accessed_structures = extract_accessed_structures(call_node, is_allocator);

  if (accessed_structures.size() > 1) {
    // NOTE: There is only 2 BDD Operations that require more than 1 data structure (espire_items_single_map and
    // expire_items_single_map_iteratively), and they are only present in the BDD Process so we can be sure that they will be at process root, and
    // thus their accessed structures will be present in the data_structures_meta_nodes
    if (!are_structures_in_same_MetaNode(accessed_structures, data_structures_meta_nodes)) {

      std::vector<std::shared_ptr<MetaNode>> nodes_to_merge;
      std::unordered_set<addr_t> addrs_to_reset;
      for (const auto &[arg_name, addr] : accessed_structures) {
        // std::cerr << "Accessed data structure: " << arg_name << " at address " << addr << "\n";
        assert_or_panic(data_structures_meta_nodes.find(addr) != data_structures_meta_nodes.end(), "Meta-node for data structure should exist");
        MetaNodeId existing_node_id = data_structures_meta_nodes.at(addr);
        assert_or_panic(meta_nodes.find(existing_node_id) != meta_nodes.end(), "Meta-node for data structure should exist");
        nodes_to_merge.push_back(meta_nodes.at(existing_node_id));
        addrs_to_reset.insert(addr);
        meta_nodes.erase(existing_node_id);
      }
      std::shared_ptr<MetaNode> merged_node = MetaNode::merge(nodes_to_merge);
      assert_or_panic(meta_nodes.find(merged_node->get_id()) == meta_nodes.end(), "Merged meta-node should not exist");
      meta_nodes.emplace(merged_node->get_id(), merged_node);

      for (const addr_t &addr : addrs_to_reset) {
        data_structures_meta_nodes.at(addr) = merged_node->get_id();
      }
    } else {
      for (const auto &[arg_name, addr] : accessed_structures) {
        assert_or_panic(data_structures_meta_nodes.find(addr) != data_structures_meta_nodes.end(), "Meta-node for data structure should exist");
        MetaNodeId existing_node_id = data_structures_meta_nodes.at(addr);
        assert_or_panic(meta_nodes.find(existing_node_id) != meta_nodes.end(), "Meta-node for data structure should exist");
        std::shared_ptr<MetaNode> &existing_node = meta_nodes.at(existing_node_id);
        existing_node->add_ds_component(node->get_id(), {arg_name, addr}, 1, is_allocator);
        return;
      }
    }
  } else if (accessed_structures.size() == 1) {
    for (const auto &[arg_name, addr] : accessed_structures) {
      if (data_structures_meta_nodes.find(addr) == data_structures_meta_nodes.end()) {

        std::shared_ptr<MetaNode> new_node =
            std::make_shared<MetaNode>(node->get_id(), std::unordered_set<std::pair<arg_name_t, addr_t>, PairHash>{{{arg_name, addr}}}, is_allocator);
        assert_or_panic(meta_nodes.find(new_node->get_id()) == meta_nodes.end(), "Meta-node for data structure should not exist %u",
                        new_node->get_id());
        data_structures_meta_nodes.emplace(addr, new_node->get_id());
        meta_nodes.emplace(new_node->get_id(), new_node);
      } else {
        MetaNodeId existing_node_id = data_structures_meta_nodes.at(addr);
        assert_or_panic(meta_nodes.find(existing_node_id) != meta_nodes.end(), "Meta-node for data structure should exist");
        std::shared_ptr<MetaNode> &existing_node = meta_nodes.at(existing_node_id);
        existing_node->add_ds_component(node->get_id(), {arg_name, addr}, 1, is_allocator);
      }
    }

  } else {
    std::shared_ptr<MetaNode> new_node = std::make_shared<MetaNode>(call_node->get_id());
    assert_or_panic(meta_nodes.find(new_node->get_id()) == meta_nodes.end(), "Meta-node for port should not exist");
    meta_nodes.emplace(new_node->get_id(), new_node);
  }
}

void pre_process_route(const BDDNode *node, const InfrastructureNode *global_ports, std::unordered_map<Port, MetaNodeId> &port_meta_nodes,
                       std::unordered_map<MetaNodeId, std::shared_ptr<MetaNode>> &meta_nodes) {
  assert_or_panic(node->get_type() == BDDNodeType::Route, "pre_process_route expects a route node");

  const Route *route      = dynamic_cast<const Route *>(node);
  const RouteOp operation = route->get_operation();

  switch (operation) {
  case RouteOp::Forward: {

    std::pair<Port, DeviceId> port_info = concretize_port(node, global_ports);
    Port global_port                    = port_info.first;
    DeviceId device                     = port_info.second;

    if (port_meta_nodes.find(global_port) == port_meta_nodes.end()) {
      std::shared_ptr<MetaNode> new_node = std::make_shared<MetaNode>(node->get_id(), global_port);
      new_node->set_assigned_device(device);
      assert_or_panic(meta_nodes.find(new_node->get_id()) == meta_nodes.end(), "Meta-node for port should not exist");
      meta_nodes.emplace(new_node->get_id(), new_node);
      port_meta_nodes.emplace(global_port, new_node->get_id());
    } else {
      MetaNodeId existing_node_id = port_meta_nodes.at(global_port);
      assert_or_panic(meta_nodes.find(existing_node_id) != meta_nodes.end(), "Meta-node for port should exist");
      std::shared_ptr<MetaNode> &existing_node = meta_nodes.at(existing_node_id);
      existing_node->add_global_port_component(node->get_id(), global_port, 1, 0);
    }

  } break;
  default:
    break;
  }
}

} // namespace

EmbeddingEngine::EmbeddingEngine(const BDD &_bdd, const PhysicalNetwork &_phys_net) : bdd(_bdd), phys_net(_phys_net) {}

void EmbeddingEngine::pre_process() {
  std::cerr << "Pre-processing BDD for embedding...\n";
  const InfrastructureNode *global_ports = phys_net.get_node(-1);

  std::unordered_map<addr_t, MetaNodeId> data_structures_meta_nodes;
  std::unordered_map<Port, MetaNodeId> port_meta_nodes;

  const std::vector<Call *> init = bdd.get_init();

  for (const Call *call : init) {
    pre_process_call(call, data_structures_meta_nodes, meta_nodes);
  }

  const BDDNode *root = bdd.get_root();
  std::queue<const BDDNode *> to_process;
  std::unordered_set<bdd_node_id_t> visited;

  to_process.push(root);
  bool in_root = true;

  while (!to_process.empty()) {
    const BDDNode *current = to_process.front();
    to_process.pop();

    assert_or_panic(!visited.count(current->get_id()), "BDD contains cycles or duplicate nodes, which is not expected in a well-formed BDD");

    switch (current->get_type()) {
    case BDDNodeType::Branch: {
      const Branch *branch = dynamic_cast<const Branch *>(current);

      const BDDNode *on_true  = branch->get_on_true();
      const BDDNode *on_false = branch->get_on_false();
      assert(on_true && on_false && "Branch node must have both on_true and on_false nodes");

      pre_process_branch(current, global_ports, port_meta_nodes, meta_nodes, in_root);

      if (on_false->get_type() != BDDNodeType::Branch) {
        in_root = false;
      }

      to_process.push(on_true);
      to_process.push(on_false);

    } break;
    case BDDNodeType::Call: {
      pre_process_call(current, data_structures_meta_nodes, meta_nodes);
      to_process.push(current->get_next());
    } break;
    case BDDNodeType::Route: {
      pre_process_route(current, global_ports, port_meta_nodes, meta_nodes);
    } break;
    }

    visited.insert(current->get_id());
  }
}

void EmbeddingEngine::debug() const {
  std::cerr << "========== Embedding Engine ==========\n";
  std::cerr << "BDD Root Node ID: " << bdd.get_root()->get_id() << "\n";
  // phys_net.debug();
  std::cerr << "Meta Nodes:\n";
  for (const auto &[meta_node_id, meta_node] : meta_nodes) {
    std::cerr << "----------------------------------\n";
    meta_node->debug();
  }
}
} // namespace LibClone
