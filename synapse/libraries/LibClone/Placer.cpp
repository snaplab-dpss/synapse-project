#include <LibClone/Placer.h>

#include <LibCore/Solver.h>
#include <LibClone/Device.h>
#include <LibClone/InfrastructureNode.h>

#include <LibCore/Debug.h>
#include <LibCore/Types.h>

#include <LibBDD/CallPath.h>
#include <LibBDD/Nodes/Node.h>
#include <LibBDD/Visitors/BDDVisualizer.h>

#include <LibSynapse/Target.h>

#include <cassert>
#include <queue>
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

using LibSynapse::InstanceId;
using LibSynapse::TargetType;

using LibClone::ComponentId;
using LibClone::DeviceId;
using LibClone::Port;

using func_name_t = std::string;
using arg_name_t  = std::string;

struct produced_consumed_args_t {
  std::string produced;
  std::unordered_map<func_name_t, arg_name_t> target_functions;
};

const std::unordered_map<func_name_t, std::pair<arg_name_t, arg_name_t>> allocator_args{
    {"map_allocate", {"map_out", "map"}}, {"vector_allocate", {"vector_out", "vector"}}, {"dchain_allocate", {"chain_out", "chain"}},
    {"cms_allocate", {"cms_out", "cms"}}, {"lpm_allocate", {"lpm_out", "lpm"}},          {"tb_allocate", {"tb_out", "tb"}},
};

const std::unordered_map<func_name_t, std::vector<arg_name_t>> consumer_args{
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

struct AccessedStructure {
  std::string struct_type;
  addr_t addr;
  bool operator<(const AccessedStructure &other) const {
    if (struct_type != other.struct_type)
      return struct_type < other.struct_type;
    return addr < other.addr;
  }
};

using AccessedStructuresSet = std::set<AccessedStructure>;

namespace {
BDD *setup_bdd(const BDD &bdd) {
  BDD *new_bdd = new BDD(bdd);
  return new_bdd;
}

bool bdd_node_match_pattern(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }

  const Call *call_op = dynamic_cast<const Call *>(node);
  const call_t call   = call_op->get_call();

  if (call.function_name != "send_to_device") {
    return false;
  }

  return true;
}

void check_accessed_structures(const Call *call_node, AccessedStructuresSet &accessed_structures) {
  const call_t &call           = call_node->get_call();
  const std::string &func_name = call.function_name;

  auto consumer_it = consumer_args.find(func_name);
  if (consumer_it != consumer_args.end()) {
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
}

BDDNode *clone_subgraph(std::unique_ptr<BDD> &target_bdd, const BDDNode *start_node, AccessedStructuresSet &accessed_structures) {
  if (!start_node)
    return nullptr;

  std::unordered_map<bdd_node_id_t, BDDNode *> cloned_nodes;
  std::queue<const BDDNode *> to_process;

  BDDNode *cloned_start              = start_node->clone(target_bdd->get_mutable_manager(), false);
  cloned_nodes[start_node->get_id()] = cloned_start;
  BDDNode *result_root               = cloned_start;

  to_process.push(start_node);

  while (!to_process.empty()) {
    const BDDNode *current = to_process.front();
    to_process.pop();

    BDDNode *cloned_current = cloned_nodes[current->get_id()];

    switch (current->get_type()) {
    case BDDNodeType::Branch: {
      const Branch *branch = dynamic_cast<const Branch *>(current);

      for (auto *child : {branch->get_on_true(), branch->get_on_false()}) {
        if (!child || cloned_nodes.count(child->get_id()))
          continue;

        BDDNode *cloned_child         = child->clone(target_bdd->get_mutable_manager(), false);
        cloned_nodes[child->get_id()] = cloned_child;

        if (child == branch->get_on_true()) {
          dynamic_cast<Branch *>(cloned_current)->set_on_true(cloned_child);
        } else {
          dynamic_cast<Branch *>(cloned_current)->set_on_false(cloned_child);
        }
        cloned_child->set_prev(cloned_current);

        to_process.push(child);
      }
      break;
    }
    case BDDNodeType::Call: {
      const Call *call_node = dynamic_cast<const Call *>(current);

      check_accessed_structures(call_node, accessed_structures);

      const BDDNode *next = current->get_next();
      assert(next && "Call node has no next node");
      if (!cloned_nodes.count(next->get_id())) {
        BDDNode *cloned_next         = next->clone(target_bdd->get_mutable_manager(), false);
        cloned_nodes[next->get_id()] = cloned_next;

        cloned_current->set_next(cloned_next);
        cloned_next->set_prev(cloned_current);

        to_process.push(next);
      }
      break;
    }
    case BDDNodeType::Route:
      break;
    }
  }

  return result_root;
}

Branch *build_global_port_subgraph(std::unique_ptr<BDD> &bdd, bdd_node_id_t root, AccessedStructuresSet &accessed_structures) {
  const BDDNode *node = bdd->get_node_by_id(root);
  assert(node && "Root node not found");
  assert(node->get_type() == BDDNodeType::Branch && "Root node not a branch");

  const Branch *branch = dynamic_cast<const Branch *>(node);

  klee::ref<klee::Expr> condition = branch->get_condition();
  assert(condition->getKind() == klee::Expr::Kind::Eq && condition->getNumKids() == 2);

  Branch *new_branch = bdd->create_new_branch(condition);

  // NOTE: switch the following lines to print out the complete subgraph or just the first node
  // BDDNode *new_on_true = branch->get_on_true()->clone(bdd->get_mutable_manager(), false);
  BDDNode *new_on_true = clone_subgraph(bdd, branch->get_on_true(), accessed_structures);
  new_branch->set_on_true(new_on_true);
  new_on_true->set_prev(new_branch);

  return new_branch;
}

Branch *build_code_path_subgraph(std::unique_ptr<BDD> &bdd, bdd_node_id_t root, AccessedStructuresSet &accessed_structures) {
  const BDDNode *node = bdd->get_node_by_id(root);
  assert(node && "Root node not found");
  assert(node->get_type() == BDDNodeType::Call && "Root node not a call");

  assert(node->get_prev()->get_type() == BDDNodeType::Route);
  assert(bdd_node_match_pattern(node->get_prev()->get_prev()) && "Root not preceded by s2d_node");

  const Call *previous_s2d_call_node = dynamic_cast<const Call *>(node->get_prev()->get_prev());
  const call_t &call                 = previous_s2d_call_node->get_call();

  klee::ref<klee::Expr> code_path_expr = call.args.at("code_path").expr;

  u64 code_path = solver_toolbox.value_from_expr(code_path_expr);

  symbol_t code_path_symbol = bdd->get_mutable_symbol_manager()->get_symbol("code_path");

  klee::ref<klee::Expr> condition =
      solver_toolbox.exprBuilder->Eq(code_path_symbol.expr, solver_toolbox.exprBuilder->Constant(code_path, sizeof(bdd_node_id_t) * 8));

  Branch *new_branch = bdd->create_new_branch(condition);

  // NOTE: switch the following lines to print out the complete subgraph or just the first node
  // BDDNode *new_on_true = node->clone(bdd->get_mutable_manager(), false);
  BDDNode *new_on_true = clone_subgraph(bdd, node, accessed_structures);
  new_branch->set_on_true(new_on_true);
  new_on_true->set_prev(new_branch);

  return new_branch;
}

void insert_parse_header_cpu_node(BDDNode *parse_header_cpu_node, BDDNode *&root, Branch *global_port_branch) {
  if (global_port_branch != nullptr) {
    BDDNode *first_code_path = global_port_branch->get_mutable_on_false();
    assert(first_code_path->get_type() == BDDNodeType::Branch);

    Branch *first_code_path_branch = dynamic_cast<Branch *>(first_code_path);
    parse_header_cpu_node->set_next(first_code_path_branch);
    first_code_path_branch->set_prev(parse_header_cpu_node);

    global_port_branch->set_on_false(parse_header_cpu_node);
    parse_header_cpu_node->set_prev(global_port_branch);
  } else {
    parse_header_cpu_node->set_next(root);
    root->set_prev(parse_header_cpu_node);
    root = parse_header_cpu_node;
  }
}

std::vector<const BDDNode *> retreive_prev_hdr_parsing_ops(const BDDNode *node) {
  bdd_node_ids_t stop_nodes            = node->get_prev_s2d_node_id();
  std::list<const Call *> prev_borrows = node->get_prev_functions({"packet_borrow_next_chunk", "packet_return_chunk"}, stop_nodes);

  std::vector<const BDDNode *> hdr_parsing_ops;
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_borrows.begin(), prev_borrows.end());

  if (node->get_type() == BDDNodeType::Call) {
    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call    = call_node->get_call();

    if (call.function_name == "packet_borrow_next_chunk" || call.function_name == "packet_return_chunk") {
      hdr_parsing_ops.push_back(call_node);
    }
  }

  return hdr_parsing_ops;
}

void concretize_port(BDDNode *node, const InfrastructureNode *global_ports) {

  if (!node) {
    return;
  }

  switch (node->get_type()) {
  case BDDNodeType::Branch: {

    Branch *branch                          = dynamic_cast<Branch *>(node);
    std::unordered_set<std::string> symbols = branch->get_used_symbols();

    if (symbols.find("DEVICE") == symbols.end()) {
      return;
    }

    klee::ref<klee::Expr> condition = branch->get_condition();

    assert(condition->getKind() == klee::Expr::Kind::Eq && condition->getNumKids() == 2);

    klee::ref<klee::Expr> left  = condition->getKid(0);
    klee::ref<klee::Expr> right = condition->getKid(1);

    assert(left->getKind() == klee::Expr::Kind::Constant);

    u64 global_port = solver_toolbox.value_from_expr(left);
    assert(global_ports->has_link(global_port));
    const Port local_port = global_ports->get_link(global_port).first;

    klee::ref<klee::Expr> new_condition = solver_toolbox.exprBuilder->Eq(right, solver_toolbox.exprBuilder->Constant(local_port, right->getWidth()));

    branch->set_condition(new_condition);
  } break;

  case BDDNodeType::Route: {

    Route *route            = dynamic_cast<Route *>(node);
    const RouteOp operation = route->get_operation();
    switch (operation) {
    case RouteOp::Forward: {

      if (!bdd_node_match_pattern(route->get_prev())) {
        klee::ref<klee::Expr> dst_expr = route->get_dst_device();

        u64 global_port = solver_toolbox.value_from_expr(dst_expr);
        assert(global_ports->has_link(global_port));
        const Port local_port = global_ports->get_link(global_port).first;

        klee::ref<klee::Expr> new_dst_device = solver_toolbox.exprBuilder->Constant(local_port, dst_expr->getWidth());

        route->set_dst_device(new_dst_device);
      }
    } break;
    default:
      break;
    }
  } break;
  default:
    break;
  }
}

Symbols get_relevant_dataplane_state(std::unique_ptr<BDD> &bdd, const BDDNode *node, const bdd_node_ids_t &target_roots) {
  Symbols generated_symbols = node->get_prev_symbols(target_roots);
  generated_symbols.add(bdd->get_device());
  generated_symbols.add(bdd->get_time());

  // NOTE: Besides the previous local_symbols, we should also add the current local symbols
  const std::unordered_set<std::string> ignoring_symbols{
      "packet_chunks",
  };

  if (node->get_type() == BDDNodeType::Call) {
    const Call *call_node        = dynamic_cast<const Call *>(node);
    const Symbols &local_symbols = call_node->get_local_symbols();

    for (const symbol_t &symbol : local_symbols.get()) {
      if (ignoring_symbols.find(symbol.base) != ignoring_symbols.end()) {
        continue;
      }

      generated_symbols.add(symbol);
    }
  }

  Symbols future_used_symbols;
  node->visit_nodes([&future_used_symbols](const BDDNode *future_node) {
    const Symbols local_future_symbols = future_node->get_used_symbols();
    future_used_symbols.add(local_future_symbols);
    return BDDNodeVisitAction::Continue;
  });

  return generated_symbols.intersect(future_used_symbols);
}

BDDNode *create_parse_header_cpu_node(std::unique_ptr<BDD> &bdd) {

  const call_t call{
      .function_name = "packet_parse_header_cpu",
      .extra_vars    = {},
      .args          = {},
      .ret           = {},
  };

  symbol_t code_path_symbol;
  if (bdd->get_mutable_symbol_manager()->has_symbol("code_path")) {
    code_path_symbol = bdd->get_mutable_symbol_manager()->get_symbol("code_path");
  } else {
    code_path_symbol = bdd->get_mutable_symbol_manager()->create_symbol("code_path", sizeof(bdd_node_id_t) * 8);
  }

  Symbols symbols;
  symbols.add(code_path_symbol);
  Call *parse_cpu = bdd->create_new_call(bdd->get_mutable_node_by_id(bdd->get_id()), call, symbols);

  return parse_cpu;
}

BDDNode *create_parse_header_vars_node(std::unique_ptr<BDD> &bdd, bdd_node_id_t current_id, bdd_node_id_t next_id,
                                       const Symbols &extra_symbols = {}) {

  const BDDNode *current_node = bdd->get_node_by_id(current_id);
  assert(current_node && "BDDNode not found");

  klee::ref<klee::Expr> code_path_expr = solver_toolbox.exprBuilder->Constant(next_id, sizeof(bdd_node_id_t) * 8);

  const arg_t code_path_arg{
      .expr        = code_path_expr,
      .fn_ptr_name = {},
      .in          = {},
      .out         = {},
      .meta        = {},
  };

  const call_t call{
      .function_name = "packet_parse_header_vars",
      .extra_vars    = {},
      .args          = {{"code_path", code_path_arg}},
      .ret           = {},
  };

  Symbols symbols = get_relevant_dataplane_state(bdd, current_node, {});
  symbols.add(extra_symbols);
  symbols.remove("packet_chunks");
  symbols.remove("next_time");

  Call *parse_vars = bdd->create_new_call(bdd->get_mutable_node_by_id(bdd->get_id()), call, symbols);

  return parse_vars;
}

const std::vector<const BDDNode *> create_send_to_device_node(std::unique_ptr<BDD> &bdd, bdd_node_id_t current_id, bdd_node_id_t next_node_id,
                                                              const PhysicalNetwork &phys_net) {

  std::vector<const BDDNode *> inserted_nodes;

  const BDDNode *current_node = bdd->get_node_by_id(current_id);
  assert(current_node && "BDDNode not found");

  if (phys_net.get_placement(current_id) == phys_net.get_placement(next_node_id)) {
    return {};
  }

  InfrastructureNodeId current_instance_id = phys_net.get_placement(current_id).instance_id;
  InfrastructureNodeId next_instance_id    = phys_net.get_placement(next_node_id).instance_id;

  std::vector<std::pair<Port, LibSynapse::TargetType>> path = phys_net.find_path(current_instance_id, next_instance_id);

  if (path.size() == 0) {
    panic("No path found in physical network from device %ld to device %ld", current_instance_id, next_instance_id);
  }

  std::vector<const BDDNode *> hdr_parsing_ops = retreive_prev_hdr_parsing_ops(current_node);

  Symbols hdr_parsing_symbols;

  // Check if any of the hdr_parsing_ops uses generated symbols
  for (const BDDNode *hdr_op : hdr_parsing_ops) {
    if (hdr_op->get_type() == BDDNodeType::Call) {
      const Call *call_node = dynamic_cast<const Call *>(hdr_op);
      const Symbols &used   = call_node->get_used_symbols();
      hdr_parsing_symbols.add(used);
    }
  }

  hdr_parsing_symbols.remove("packet_chunks");
  hdr_parsing_symbols.remove("next_time");

  for (const auto &[port, target] : path) {

    klee::ref<klee::Expr> code_path_expr = solver_toolbox.exprBuilder->Constant(next_node_id, sizeof(bdd_node_id_t) * 8);
    klee::ref<klee::Expr> port_expr      = solver_toolbox.exprBuilder->Constant(port, sizeof(port) * 8);
    klee::ref<klee::Expr> target_expr    = solver_toolbox.exprBuilder->Constant(target.instance_id, sizeof(InstanceId) * 8);

    const BDDNode *parse_header_vars_node = create_parse_header_vars_node(bdd, current_id, next_node_id, hdr_parsing_symbols);

    const arg_t code_path_arg{
        .expr        = code_path_expr,
        .fn_ptr_name = {},
        .in          = {},
        .out         = {},
        .meta        = {},
    };

    const arg_t port_arg{
        .expr        = port_expr,
        .fn_ptr_name = {},
        .in          = {},
        .out         = {},
        .meta        = {},
    };

    const arg_t target_arg{
        .expr        = target_expr,
        .fn_ptr_name = {},
        .in          = {},
        .out         = {},
        .meta        = {},

    };

    const call_t call{
        .function_name = "send_to_device",
        .extra_vars    = {},
        .args          = {{"code_path", code_path_arg}, {"outgoing_port", port_arg}, {"next_target", target_arg}},
        .ret           = {},
    };

    Symbols symbols = get_relevant_dataplane_state(bdd, current_node, {});
    symbols.add(hdr_parsing_symbols);
    symbols.remove("packet_chunks");
    symbols.remove("next_time");

    const Call *s2d = bdd->create_new_call(bdd->get_mutable_node_by_id(bdd->get_id()), call, symbols);

    inserted_nodes.push_back(s2d);

    const Route *route_to_device = new Route(bdd->get_id(), bdd->get_mutable_symbol_manager(), RouteOp::Forward, port_expr);
    inserted_nodes.push_back(route_to_device);

    inserted_nodes.push_back(parse_header_vars_node);

    inserted_nodes.insert(inserted_nodes.end(), hdr_parsing_ops.begin(), hdr_parsing_ops.end());
  }

  return inserted_nodes;
}

void handle_branch_node(std::unique_ptr<BDD> &bdd, bdd_node_id_t branch_id, bdd_node_id_t on_true_id, bdd_node_id_t on_false_id, bool in_root,
                        const PhysicalNetwork &phys_net) {

  const std::vector<const BDDNode *> s2d_true = create_send_to_device_node(bdd, branch_id, on_true_id, phys_net);

  if (!s2d_true.empty()) {
    bdd->add_cloned_non_branches(on_true_id, s2d_true);
  }

  if (!in_root) {
    const std::vector<const BDDNode *> s2d_false = create_send_to_device_node(bdd, branch_id, on_false_id, phys_net);

    if (!s2d_false.empty()) {
      bdd->add_cloned_non_branches(on_false_id, s2d_false);
    }
  }
}

void handle_node(std::unique_ptr<BDD> &bdd, bdd_node_id_t current_id, bdd_node_id_t next_id, const PhysicalNetwork &phys_net) {
  const std::vector<const BDDNode *> s2d = create_send_to_device_node(bdd, current_id, next_id, phys_net);
  if (!s2d.empty()) {
    bdd->add_cloned_non_branches(next_id, s2d);
  }
}

void add_send_to_device_nodes(std::unique_ptr<BDD> &bdd, const PhysicalNetwork &phys_net) {
  std::cerr << "==========================================\n";
  std::cerr << "=============Adding s2d nodes=============\n";
  std::cerr << "==========================================\n";

  const BDDNode *root = bdd->get_root();

  std::queue<const BDDNode *> queue;

  queue.push(root);

  bool in_root = true;

  while (!queue.empty()) {
    const BDDNode *current = queue.front();
    queue.pop();

    switch (current->get_type()) {
    case BDDNodeType::Branch: {
      const Branch *branch = dynamic_cast<const Branch *>(current);

      const BDDNode *on_true  = branch->get_on_true();
      const BDDNode *on_false = branch->get_on_false();
      assert(on_true && on_false && "Branch node must have both on_true and on_false nodes");

      if (on_false->get_type() != BDDNodeType::Branch) {
        in_root = false;
      }

      handle_branch_node(bdd, branch->get_id(), on_true->get_id(), on_false->get_id(), in_root, phys_net);

      queue.push(on_true);
      queue.push(on_false);

    } break;
    case BDDNodeType::Call: {
      const BDDNode *next = current->get_next();
      assert(next && "Call must have a next");
      handle_node(bdd, current->get_id(), next->get_id(), phys_net);
      queue.push(next);
    } break;
    case BDDNodeType::Route:
      break;
    }
  }
}

// void get_global_port_roots(std::unique_ptr<const BDD> &bdd, const PhysicalNetwork &phys_net,
//                            const std::unordered_map<LibSynapse::TargetType, target_roots_t> &target_roots) {
//   bdd_node_ids_t port_roots;
//
//   const BDDNode *root = bdd->get_root();
//   assert(root && "Root Node not found");
//
//   std::queue<const BDDNode *> queue;
//
//   queue.push(root);
//   assert(root->get_type() == BDDNodeType::Branch && "Root node not a branch");
//
//   while (!queue.empty()) {
//     BDDNode *current = queue.front();
//     queue.pop();
//
//     assert(current->get_type() == BDDNodeType::Branch && "Current node not a branch");
//     Branch *branch = dynamic_cast<Branch *>(current);
//
//     std::unordered_set<std::string> symbols = branch->get_used_symbols();
//     if (symbols.find("DEVICE") == symbols.end()) {
//       continue;
//     }
//
//     klee::ref<klee::Expr> condition = branch->get_condition();
//
//     assert(condition->getKind() == klee::Expr::Kind::Eq && condition->getNumKids() == 2);
//
//     klee::ref<klee::Expr> left  = condition->getKid(0);
//     klee::ref<klee::Expr> right = condition->getKid(1);
//
//     assert(left->getKind() == klee::Expr::Kind::Constant);
//
//     u64 global_port = solver_toolbox.value_from_expr(left);
//     assert(global_ports->has_link(global_port));
//
//     const InfrastructureNode *next_hop = global_ports->get_link(global_port).second;
//     const Device *next_device          = phys_net.get_device(next_hop->get_id());
//     target_roots[next_device->get_target()].port_roots.insert(current->get_id());
//
//     if (branch->get_on_false()->get_type() == BDDNodeType::Branch) {
//       queue.push(branch->get_on_false());
//     }
//   }
//
//   return port_roots;
// }
//
// void handle_branch_node_2(std::unique_ptr<BDD> &bdd, bdd_node_id_t branch_id, bdd_node_id_t on_true_id, bdd_node_id_t on_false_id,
//                           const PhysicalNetwork &phys_net) {
//
//   const std::vector<const BDDNode *> s2d_true = create_send_to_device_node(bdd, branch_id, on_true_id, phys_net);
//
//   if (!s2d_true.empty()) {
//     bdd->add_cloned_non_branches(on_true_id, s2d_true);
//   }
//
//   const std::vector<const BDDNode *> s2d_false = create_send_to_device_node(bdd, branch_id, on_false_id, phys_net);
//
//   if (!s2d_false.empty()) {
//     bdd->add_cloned_non_branches(on_false_id, s2d_false);
//   }
// }
//
// void handle_node_2(std::unique_ptr<BDD> &bdd, bdd_node_id_t current_id, bdd_node_id_t next_id, const PhysicalNetwork &phys_net) {
//   const std::vector<const BDDNode *> s2d = create_send_to_device_node(bdd, current_id, next_id, phys_net);
//   if (!s2d.empty()) {
//     bdd->add_cloned_non_branches(next_id, s2d);
//   }
// }
//
// std::unordered_map<LibSynapse::TargetType, target_roots_t> get_target_roots_2(std::unique_ptr<BDD> &bdd, const PhysicalNetwork &phys_net) {
//
//   std::cerr << "==========================================\n";
//   std::cerr << "========Retreiving Target Roots===========\n";
//   std::cerr << "==========================================\n";
//
//   const InfrastructureNode *global_ports = phys_net.get_node(-1);
//
//   std::unordered_map<LibSynapse::TargetType, target_roots_t> target_roots;
//   for (const auto &[target, _] : phys_net.get_target_list()) {
//     target_roots[target];
//   }
//
//   get_global_port_roots(bdd, phys_net, target_roots);
//
//   BDDNode *root = bdd->get_root();
//   assert(root && "Root Node not found");
//
//   std::queue<const BDDNode *> queue;
//
//   BDDNode *prev = root;
//
//   while (!queue.empty()) {
//     BDDNode *current = queue.front();
//     queue.pop();
//
//     switch (current->get_type()) {
//     case BDDNodeType::Branch: {
//       const Branch *branch    = dynamic_cast<const Branch *>(current);
//       const BDDNode *on_true  = branch->get_on_true();
//       const BDDNode *on_false = branch->get_on_false();
//
//       assert(on_true && on_false && "Branch node must have both on_true and on_false nodes");
//       if (condition) {
//       }
//
//     } break;
//     case BDDNodeType::Call: {
//     } break;
//     case BDDNodeType::Route: {
//     } break;
//     }
//   }
// }

std::unordered_map<LibSynapse::TargetType, target_roots_t> get_target_roots(std::unique_ptr<BDD> &bdd, const PhysicalNetwork &phys_net) {

  std::cerr << "==========================================\n";
  std::cerr << "========Retreiving Target Roots===========\n";
  std::cerr << "==========================================\n";

  const InfrastructureNode *global_ports = phys_net.get_node(-1);

  std::unordered_map<LibSynapse::TargetType, target_roots_t> target_roots;
  for (const auto &[target, _] : phys_net.get_target_list()) {
    target_roots[target];
  }

  bool in_root = true;

  BDDNode *root = bdd->get_mutable_root();
  assert(root && "Root Node not found");

  std::queue<BDDNode *> queue;

  queue.push(root);

  if (root->get_type() != BDDNodeType::Branch) {
    in_root = false;
  }

  while (!queue.empty()) {
    BDDNode *current = queue.front();
    queue.pop();

    switch (current->get_type()) {
    case BDDNodeType::Branch: {
      Branch *branch = dynamic_cast<Branch *>(current);

      if (in_root) {

        std::unordered_set<std::string> symbols = branch->get_used_symbols();
        if (symbols.find("DEVICE") == symbols.end()) {
          continue;
        }

        klee::ref<klee::Expr> condition = branch->get_condition();

        assert(condition->getKind() == klee::Expr::Kind::Eq && condition->getNumKids() == 2);

        klee::ref<klee::Expr> left  = condition->getKid(0);
        klee::ref<klee::Expr> right = condition->getKid(1);

        assert(left->getKind() == klee::Expr::Kind::Constant);

        u64 global_port = solver_toolbox.value_from_expr(left);
        assert(global_ports->has_link(global_port));

        const InfrastructureNode *next_hop = global_ports->get_link(global_port).second;
        const Device *next_device          = phys_net.get_device(next_hop->get_id());
        target_roots[next_device->get_target()].port_roots.insert(current->get_id());
        concretize_port(branch, global_ports);
        if (branch->get_on_false()->get_type() != BDDNodeType::Branch) {
          in_root = false;
        }
      }
      queue.push(branch->get_mutable_on_true());
      queue.push(branch->get_mutable_on_false());

      break;
    }
    case BDDNodeType::Call: {
      BDDNode *next = current->get_mutable_next();
      assert(next && "Call must have a next");
      queue.push(next);

      break;
    }
    case BDDNodeType::Route: {
      const BDDNode *prev = current->get_prev();
      BDDNode *next       = current->get_mutable_next();

      if (next) {
        if (bdd_node_match_pattern(prev)) {

          const Call *call_node = dynamic_cast<const Call *>(prev);
          const call_t &call    = call_node->get_call();

          klee::ref<klee::Expr> next_target_expr = call.args.at("next_target").expr;

          bits_t width                       = next_target_expr->getWidth();
          const klee::ConstantExpr *constant = dynamic_cast<const klee::ConstantExpr *>(next_target_expr.get());

          assert(width <= 64 && "Width too big");
          LibSynapse::InstanceId next_target_id = constant->getZExtValue(width);

          const Device *next_device = phys_net.get_device(next_target_id);
          target_roots[next_device->get_target()].target_roots.insert(next->get_id());
        }

        queue.push(next);
      }
      concretize_port(current, global_ports);

      break;
    }
    }
  }

  return target_roots;
}

void trim_init_nodes(std::unique_ptr<BDD> &bdd, AccessedStructuresSet &accessed_structures) {
  std::cerr << "==========================================\n";
  std::cerr << "===========Trimming Init Nodes===========\n";
  std::cerr << "==========================================\n";

  std::vector<Call *> init_nodes = bdd->get_init();
  std::vector<Call *> new_init;

  for (Call *call_node : init_nodes) {
    const call_t &call           = call_node->get_call();
    const std::string &func_name = call.function_name;

    auto allocator_it = allocator_args.find(func_name);
    if (allocator_it != allocator_args.end()) {
      const arg_name_t &produced_arg_name = allocator_it->second.first;
      const arg_name_t &consumed_arg_name = allocator_it->second.second;
      auto arg_it                         = call.args.find(produced_arg_name);

      assert_or_panic(arg_it != call.args.end(), "Target function should have the produced argument");

      klee::ref<klee::Expr> addr_expr = arg_it->second.out;
      assert_or_panic(LibCore::is_constant(addr_expr), "Target function produced argument should be a constant");

      const addr_t addr = expr_addr_to_obj_addr(addr_expr);

      if (accessed_structures.find({consumed_arg_name, addr}) != accessed_structures.end()) {
        new_init.push_back(call_node);
      }
      // std::cerr << "  Allocator Node " << call_node->dump(true) << " allocates " << produced_arg_name << " structure\n";
      // std::cerr << "    Checking for consumption of " << consumed_arg_name << " structure at address " << addr << "\n";
    }

    auto consumer_it = consumer_args.find(func_name);
    if (consumer_it != consumer_args.end()) {
      const std::vector<arg_name_t> &arg_names = consumer_it->second;
      for (const std::string &arg_name : arg_names) {
        auto arg_it = call.args.find(arg_name);

        assert_or_panic(arg_it != call.args.end(), "Target function should have the argument");

        klee::ref<klee::Expr> addr_expr = arg_it->second.expr;
        assert_or_panic(LibCore::is_constant(addr_expr), "Target function argument should be a constant");

        const addr_t addr = expr_addr_to_obj_addr(addr_expr);

        if (accessed_structures.find({arg_name, addr}) != accessed_structures.end()) {
          new_init.push_back(call_node);
        }
        // std::cerr << "  Consumer Node " << call_node->dump(true) << " accesses " << arg_name << " structure\n";
      }
    }
  }

  bdd->set_init(new_init);
}

std::unique_ptr<BDD> extract_target_bdd(std::unique_ptr<BDD> &global_bdd, bdd_node_ids_t port_roots, bdd_node_ids_t roots) {

  std::cerr << "==========================================\n";
  std::cerr << "===========Extracting Target BDD==========\n";
  std::cerr << "==========================================\n";

  AccessedStructuresSet accessed_structures;

  std::unique_ptr<BDD> extracted_bdd = std::make_unique<BDD>(*global_bdd);

  extracted_bdd->set_device(global_bdd->get_device());
  extracted_bdd->set_packet_len(global_bdd->get_packet_len());
  extracted_bdd->set_time(global_bdd->get_time());

  if (port_roots.empty() && roots.empty()) {
    std::cerr << "NO ROOTS DETECTED\n";
    return extracted_bdd;
  }

  BDDNode *new_root    = nullptr;
  Branch *current_node = nullptr;

  for (const bdd_node_id_t root : port_roots) {
    Branch *new_branch = build_global_port_subgraph(extracted_bdd, root, accessed_structures);

    if (new_root == nullptr) {
      new_root = new_branch;
    } else {
      current_node->set_on_false(new_branch);
      new_branch->set_prev(current_node);
    }
    current_node = new_branch;
  }

  Branch *last_port_branch       = current_node;
  BDDNode *parse_header_cpu_node = create_parse_header_cpu_node(extracted_bdd);

  for (const bdd_node_id_t root : roots) {
    Branch *new_branch = build_code_path_subgraph(extracted_bdd, root, accessed_structures);
    if (new_root == nullptr) {
      new_root = new_branch;
    } else {
      current_node->set_on_false(new_branch);
      new_branch->set_prev(current_node);
    }
    current_node = new_branch;
  }

  insert_parse_header_cpu_node(parse_header_cpu_node, new_root, last_port_branch);

  Route *drop_node = new Route(extracted_bdd->get_mutable_id(), extracted_bdd->get_mutable_symbol_manager(), LibBDD::RouteOp::Drop);
  extracted_bdd->get_mutable_manager().add_node(drop_node);
  extracted_bdd->get_mutable_id()++;

  current_node->set_on_false(drop_node);
  drop_node->set_prev(current_node);

  extracted_bdd->set_root(new_root);

  trim_init_nodes(extracted_bdd, accessed_structures);

  return extracted_bdd;
}

} // namespace

NetworkPartitioner::NetworkPartitioner(const BDD &_bdd, const PhysicalNetwork &_phys_net) : bdd(setup_bdd(_bdd)), phys_net(_phys_net) {}

std::unordered_map<LibSynapse::TargetType, std::unique_ptr<const BDD>> NetworkPartitioner::process() {
  std::cerr << "==========================================\n";
  std::cerr << "=============Processing BDD===============\n";
  std::cerr << "==========================================\n";

  std::unordered_map<LibSynapse::TargetType, std::unique_ptr<const BDD>> target_bdds;

  const BDD *old_bdd           = get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  const PhysicalNetwork &_phys_net = get_physical_network();

  add_send_to_device_nodes(new_bdd, _phys_net);
  // BDDViz::visualize(new_bdd.get(), true);

  std::unordered_map<LibSynapse::TargetType, target_roots_t> target_roots = get_target_roots(new_bdd, _phys_net);

  for (const auto &[target, roots] : target_roots) {

    std::unique_ptr<BDD> target_bdd = extract_target_bdd(new_bdd, roots.port_roots, roots.target_roots);

    const LibBDD::BDD::inspection_report_t report = target_bdd->inspect();
    if (report.status != LibBDD::BDD::InspectionStatus::Ok) {
      BDDViz::visualize(target_bdd.get(), false);
    }
    assert_or_panic(report.status == LibBDD::BDD::InspectionStatus::Ok, "BDD inspection failed: %s", report.message.c_str());
    std::cout << "BDD inspection passed.\n";

    target_bdds.emplace(target, std::move(target_bdd));
  }

  return target_bdds;
}

} // namespace LibClone
