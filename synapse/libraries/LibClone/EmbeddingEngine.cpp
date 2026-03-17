#include "LibClone/EmbeddingSolver.h"
#include <LibClone/EmbeddingEngine.h>

#include <LibCore/Solver.h>
#include <LibCore/Debug.h>
#include <LibCore/Types.h>

#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace LibClone {

using LibCore::expr_addr_to_obj_addr;
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

using LibClone::BDDInfo;
using LibClone::DeviceId;
using LibClone::InfrastructureInfo;
using LibClone::InfrastructureNode;
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
    {"vector_borrow", {"vector"}},
    {"vector_return", {"vector"}},
    {"vector_clear", {"vector"}},
    {"vector_sample_lt", {"vector"}},
    {"dchain_allocate_new_index", {"chain"}},
    {"dchain_rejuvenate_index", {"chain"}},
    {"dchain_expire_one_index", {"chain"}},
    {"dchain_is_index_allocated", {"chain"}},
    {"dchain_free_index", {"chain"}},
    {"cms_increment", {"cms"}},
    {"cms_count_min", {"cms"}},
    {"cms_periodic_cleanup", {"cms"}},
    {"lpm_free", {"lpm"}},
    {"lpm_from_file", {"lpm"}},
    {"lpm_update", {"lpm"}},
    {"lpm_lookup", {"lpm"}},
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

std::vector<std::pair<arg_name_t, addr_t>> extract_accessed_structures(const Call *call_node, u64 &is_allocator) {
  std::vector<std::pair<arg_name_t, addr_t>> accessed_structures;

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
      accessed_structures.push_back({arg_name, addr});
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
    accessed_structures.push_back({arg_translation_name, addr});

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

    klee::ref<klee::Expr> left = condition->getKid(0);
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
      u64 global_port                = solver_toolbox.value_from_expr(dst_expr);
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

void process_branch(const BDDNode *node, const InfrastructureNode *global_ports, MetaNodes &meta_nodes, bool in_root) {
  assert_or_panic(node->get_type() == BDDNodeType::Branch, "pre_process_branch expects a branch node");

  if (in_root) {
    std::pair<Port, DeviceId> port_info = concretize_port(node, global_ports);
    Port global_port                    = port_info.first;
    DeviceId device                     = port_info.second;

    if (MetaNode *existing = meta_nodes.find_by_port(global_port)) {
      existing->add_global_port_node(node->get_id(), global_port);
    } else {
      meta_nodes.create_for_port(node->get_id(), global_port, device);
    }
  } else {
    meta_nodes.create_process(node->get_id());
  }
}

void process_call(const BDDNode *node, MetaNodes &meta_nodes) {
  assert_or_panic(node->get_type() == BDDNodeType::Call, "pre_process_call expects a call node");

  const Call *call_node = dynamic_cast<const Call *>(node);
  u64 is_allocator      = 0;

  std::vector<std::pair<arg_name_t, addr_t>> accessed_structures = extract_accessed_structures(call_node, is_allocator);

  if (accessed_structures.empty()) {
    meta_nodes.create_process(node->get_id());
    return;
  }

  if (accessed_structures.size() > 1 && !meta_nodes.are_structures_together(accessed_structures)) {
    std::vector<MetaNode *> to_merge = meta_nodes.find_all_by_ds(accessed_structures);
    MetaNode *merged                 = meta_nodes.merge(to_merge);
    for (const std::pair<arg_name_t, addr_t> &entry : accessed_structures) {
      const arg_name_t &arg = entry.first;
      const addr_t addr     = entry.second;
      merged->add_ds_node(node->get_id(), arg, addr);
      return;
    }
  } else {
    const std::pair<arg_name_t, addr_t> &entry = accessed_structures[0];
    const arg_name_t &arg                      = entry.first;
    const addr_t addr                          = entry.second;
    if (MetaNode *existing = meta_nodes.find_by_ds(arg, addr)) {
      existing->add_ds_node(node->get_id(), arg, addr);
    } else {
      meta_nodes.create_for_ds(node->get_id(), arg, addr);
    }
  }
}

void process_route(const BDDNode *node, const InfrastructureNode *global_ports, MetaNodes &meta_nodes) {
  assert_or_panic(node->get_type() == BDDNodeType::Route, "pre_process_route expects a route node");

  const Route *route      = dynamic_cast<const Route *>(node);
  const RouteOp operation = route->get_operation();

  switch (operation) {
  case RouteOp::Forward: {
    std::pair<Port, DeviceId> port_info = concretize_port(node, global_ports);
    Port global_port                    = port_info.first;
    DeviceId device                     = port_info.second;

    if (MetaNode *existing = meta_nodes.find_by_port(global_port)) {
      existing->add_global_port_node(node->get_id(), global_port);
    } else {
      meta_nodes.create_for_port(node->get_id(), global_port, device);
    }
  } break;
  case RouteOp::Drop: {
    const BDDNode *prev = route->get_prev();
    if (MetaNode *owner = meta_nodes.find_by_node(prev->get_id())) {
      owner->add_node(route->get_id());
    } else {
      panic("BDD prev node missing from metanodes structure");
    }
  } break;
  case RouteOp::Broadcast:
    break;
  }
}

} // namespace

EmbeddingEngine::EmbeddingEngine(const BDD &_bdd, const PhysicalNetwork &_phys_net) : bdd(_bdd), phys_net(_phys_net), solver(EmbeddingSolver()) {}

BDDInfo EmbeddingEngine::get_bdd_info() const {
  std::cerr << "Processing BDD for embedding...\n";
  const InfrastructureNode *global_ports = phys_net.get_node(-1);

  BDDInfo info;

  const std::vector<Call *> init = bdd.get_init();
  for (const Call *call : init) {
    process_call(call, info.meta_nodes);
    info.nodes.insert(call->get_id());
  }

  const BDDNode *root = bdd.get_root();
  std::queue<const BDDNode *> to_process;
  bdd_node_ids_t visited;

  to_process.push(root);
  bool in_root = true;

  while (!to_process.empty()) {
    const BDDNode *current = to_process.front();
    to_process.pop();

    if (visited.count(current->get_id())) {
      continue;
    }

    visited.insert(current->get_id());
    info.nodes.insert(current->get_id());

    switch (current->get_type()) {
    case BDDNodeType::Branch: {
      const Branch *branch    = dynamic_cast<const Branch *>(current);
      const BDDNode *on_true  = branch->get_on_true();
      const BDDNode *on_false = branch->get_on_false();

      info.links.push_back({current->get_id(), on_true->get_id()});
      info.links.push_back({current->get_id(), on_false->get_id()});

      process_branch(current, global_ports, info.meta_nodes, in_root);

      if (on_false->get_type() != BDDNodeType::Branch) {
        in_root = false;
      }

      to_process.push(on_true);
      to_process.push(on_false);
    } break;
    case BDDNodeType::Call: {
      const BDDNode *next = current->get_next();
      assert_or_panic(next, "Call node should have a next");
      info.links.push_back({current->get_id(), next->get_id()});

      process_call(current, info.meta_nodes);
      to_process.push(next);
    } break;
    case BDDNodeType::Route: {
      process_route(current, global_ports, info.meta_nodes);
    } break;
    }

    visited.insert(current->get_id());
  }

  return info;
}

InfrastructureInfo EmbeddingEngine::get_infrastructure_info() const {
  InfrastructureInfo info;

  for (const auto &[device_id, device_ptr] : phys_net.get_devices()) {
    info.devices[device_id] = device_ptr.get();
  }

  info.cost_matrix = phys_net.get_cost_matrix();
  return info;
}

EmbeddingSolution EmbeddingEngine::solve(const BDDInfo &bdd_info, const InfrastructureInfo &infra_info) { return solver.solve(bdd_info, infra_info); }

void EmbeddingEngine::debug(const BDDInfo &bdd_info, const InfrastructureInfo &infra_info) const {
  std::cerr << "========== Embedding Engine ==========\n";
  std::cerr << "BDD Root Node ID: " << bdd.get_root()->get_id() << "\n";
  std::cerr << "BDD Node Count: " << bdd_info.nodes.size() << "\n";
  std::cerr << "BDD Edge Count: " << bdd_info.links.size() << "\n";
  bdd_info.meta_nodes.debug();

  std::cerr << "=======================================\n";

  for (const auto &[_, device] : infra_info.devices) {
    std::cerr << *device << "\n";
  }

  std::cerr << "Cost Matrix:\n";
  for (const auto &[src_id, link] : infra_info.cost_matrix) {
    std::cerr << "        ";
    std::cerr << "Device " << src_id << " -> {\n";
    for (const auto &[dst_id, cost] : link) {
      std::cerr << "        ";
      std::cerr << "        ";
      std::cerr << "Dst: " << dst_id << ", Cost: " << cost << "\n";
    }

    std::cerr << "        ";
    std::cerr << "}\n";
  }
}

} // namespace LibClone
