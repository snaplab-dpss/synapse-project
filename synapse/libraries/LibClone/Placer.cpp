#include <LibCore/Solver.h>
#include <LibClone/Device.h>
#include <LibClone/Placer.h>
#include <LibClone/InfrastructureNode.h>

#include <LibCore/Debug.h>
#include <LibCore/Types.h>

#include <LibBDD/CallPath.h>
#include <LibBDD/Nodes/Node.h>
#include <LibBDD/Visitors/BDDVisualizer.h>

#include <LibSynapse/Target.h>

#include <cassert>
#include <klee/Expr.h>
#include <klee/util/Ref.h>
#include <queue>
#include <vector>
#include <optional>

namespace LibClone {

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

BDDNode *clone_subgraph(std::unique_ptr<BDD> &target_bdd, const BDDNode *start_node) {
  if (!start_node)
    return nullptr;

  std::unordered_map<bdd_node_id_t, BDDNode *> cloned_nodes;
  std::queue<const BDDNode *> to_process;

  BDDNode *cloned_start              = start_node->clone(target_bdd->get_mutable_manager(), false);
  cloned_nodes[start_node->get_id()] = cloned_start;
  BDDNode *result_root               = cloned_start;

  if (!bdd_node_match_pattern(start_node)) {
    to_process.push(start_node);
  }

  while (!to_process.empty()) {
    const BDDNode *current = to_process.front();
    to_process.pop();

    BDDNode *cloned_current = cloned_nodes[current->get_id()];

    if (current->get_type() == BDDNodeType::Branch) {
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

        if (!bdd_node_match_pattern(child)) {
          to_process.push(child);
        }
      }
    } else {
      if (const BDDNode *next = current->get_next()) {
        if (!cloned_nodes.count(next->get_id())) {
          BDDNode *cloned_next         = next->clone(target_bdd->get_mutable_manager(), false);
          cloned_nodes[next->get_id()] = cloned_next;

          cloned_current->set_next(cloned_next);
          cloned_next->set_prev(cloned_current);

          if (!bdd_node_match_pattern(next)) {
            to_process.push(next);
          }
        }
      }
    }
  }

  return result_root;
}

Branch *build_global_port_subgraph(std::unique_ptr<BDD> &_bdd, bdd_node_id_t root) {
  const BDDNode *node = _bdd->get_node_by_id(root);
  assert(node && "Root node not found");
  assert(node->get_type() == BDDNodeType::Branch && "Root node not a branch");
  std::cerr << "ROOT NODE: " << node->dump(true) << "\n";

  const Branch *branch = dynamic_cast<const Branch *>(node);

  klee::ref<klee::Expr> condition = branch->get_condition();
  assert(condition->getKind() == klee::Expr::Kind::Eq && condition->getNumKids() == 2);

  Branch *new_branch = _bdd->create_new_branch(condition);

  // NOTE: switch the following lines to print out the complete subgraph or just the first node
  //
  // BDDNode *new_on_true = branch->get_on_true()->clone(_bdd->get_mutable_manager(), false);
  BDDNode *new_on_true = clone_subgraph(_bdd, branch->get_on_true());
  new_branch->set_on_true(new_on_true);
  new_on_true->set_prev(new_branch);

  return new_branch;
}

Branch *build_code_path_subgraph(std::unique_ptr<BDD> &_bdd, bdd_node_id_t root) {
  const BDDNode *node = _bdd->get_node_by_id(root);
  assert(node && "Root node not found");
  assert(node->get_type() == BDDNodeType::Call && "Root node not a call");
  std::cerr << "ROOT NODE: " << node->dump(true) << "\n";

  const Call *previous_s2d_call_node = dynamic_cast<const Call *>(node->get_prev());
  const call_t &call                 = previous_s2d_call_node->get_call();

  klee::ref<klee::Expr> code_path_expr = call.args.at("code_path").expr;

  u64 code_path = solver_toolbox.value_from_expr(code_path_expr);

  symbol_t code_path_symbol = _bdd->get_mutable_symbol_manager()->get_symbol("code_path");

  klee::ref<klee::Expr> condition =
      solver_toolbox.exprBuilder->Eq(code_path_symbol.expr, solver_toolbox.exprBuilder->Constant(code_path, sizeof(bdd_node_id_t) * 8));

  Branch *new_branch = _bdd->create_new_branch(condition);

  // NOTE: switch the following lines to print out the complete subgraph or just the first node
  //
  // BDDNode *new_on_true = node->clone(_bdd->get_mutable_manager(), false);
  BDDNode *new_on_true = clone_subgraph(_bdd, node);
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
  std::list<const Call *> prev_borrows = node->get_prev_functions({"packet_borrow_next_chunk"}, stop_nodes);
  std::list<const Call *> prev_returns = node->get_prev_functions({"packet_return_chunk"}, stop_nodes);

  std::vector<const BDDNode *> hdr_parsing_ops;
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_borrows.begin(), prev_borrows.end());
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_returns.begin(), prev_returns.end());
  return hdr_parsing_ops;
}

} // namespace

Placer::Placer(const BDD &_bdd, const PhysicalNetwork &_phys_net) : bdd(setup_bdd(_bdd)), phys_net(_phys_net) {}

Symbols Placer::get_relevant_dataplane_state(std::unique_ptr<BDD> &new_bdd, const BDDNode *node, const bdd_node_ids_t &target_roots) {
  Symbols generated_symbols = node->get_prev_symbols(target_roots);
  generated_symbols.add(new_bdd->get_device());
  generated_symbols.add(new_bdd->get_time());

  const BDDNode *prev_node = node->get_prev();
  while (prev_node) {
    if (target_roots.find(prev_node->get_id()) != target_roots.end()) {
      if (prev_node->get_type() == BDDNodeType::Call) {
        const Call *call_node = dynamic_cast<const Call *>(prev_node);
        const call_t &call    = call_node->get_call();

        if (call.function_name == "packet_parse_header_vars") {
          Symbols prev_shared_symbols = shared_context[prev_node->get_id()];
          generated_symbols.add(prev_shared_symbols);
          break;
        }
      }
      break;
    }
    prev_node = prev_node->get_prev();
  }

  Symbols future_used_symbols;
  node->visit_nodes([&future_used_symbols](const BDDNode *future_node) {
    const Symbols local_future_symbols = future_node->get_used_symbols();
    future_used_symbols.add(local_future_symbols);
    return BDDNodeVisitAction::Continue;
  });

  shared_context[node->get_id()] = generated_symbols.intersect(future_used_symbols);

  return generated_symbols.intersect(future_used_symbols);
}

const std::vector<const BDDNode *> Placer::create_send_to_device_node(std::unique_ptr<BDD> &new_bdd, bdd_node_id_t current_id,
                                                                      bdd_node_id_t next_node_id) {

  std::vector<const BDDNode *> inserted_nodes;

  const BDDNode *current_node = new_bdd->get_node_by_id(current_id);
  assert(current_node && "BDDNode not found");

  // const BDDNode *next_node = new_bdd->get_node_by_id(next_node_id);
  // assert(next_node && "BDDNode not found");

  if (phys_net.get_placement(current_id) == phys_net.get_placement(next_node_id)) {
    return {};
  }

  InfrastructureNodeId current_instance_id = phys_net.get_placement(current_id).instance_id;
  InfrastructureNodeId next_instance_id    = phys_net.get_placement(next_node_id).instance_id;

  std::vector<std::pair<Port, LibSynapse::TargetType>> path = phys_net.find_path(current_instance_id, next_instance_id);

  if (path.size() == 0) {
    panic("No path found in physical network from device %ld to device %ld", current_instance_id, next_instance_id);
  }

  for (const auto &[port, target] : path) {

    klee::ref<klee::Expr> code_path_expr = solver_toolbox.exprBuilder->Constant(next_node_id, sizeof(bdd_node_id_t) * 8);
    klee::ref<klee::Expr> port_expr      = solver_toolbox.exprBuilder->Constant(port, sizeof(port) * 8);
    klee::ref<klee::Expr> target_expr    = solver_toolbox.exprBuilder->Constant(target.instance_id, sizeof(InstanceId) * 8);

    const std::optional<BDDNode *> parse_header_vars_node = create_parse_header_vars_node(new_bdd, current_id, next_node_id);

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

    Symbols symbols = get_relevant_dataplane_state(new_bdd, current_node, {});
    symbols.remove("packet_chunks");
    symbols.remove("DEVICE");
    const Call *s2d = new_bdd->create_new_call(new_bdd->get_mutable_node_by_id(new_bdd->get_id()), call, symbols);

    std::cerr << s2d->dump(true) << "\n";
    inserted_nodes.push_back(s2d);

    std::vector<const BDDNode *> hdr_parsing_ops = retreive_prev_hdr_parsing_ops(current_node);
    inserted_nodes.insert(inserted_nodes.end(), hdr_parsing_ops.begin(), hdr_parsing_ops.end());

    if (parse_header_vars_node.has_value()) {
      std::cerr << parse_header_vars_node.value()->dump(true) << "\n";
      inserted_nodes.push_back(parse_header_vars_node.value());
    } else {
      panic("Failed to create parse_header_vars node");
    }
  }

  return inserted_nodes;
}

BDDNode *Placer::create_parse_header_cpu_node(std::unique_ptr<BDD> &new_bdd) {

  const call_t call{
      .function_name = "packet_parse_header_cpu",
      .extra_vars    = {},
      .args          = {},
      .ret           = {},
  };

  symbol_t code_path_symbol;
  if (new_bdd->get_mutable_symbol_manager()->has_symbol("code_path")) {
    code_path_symbol = new_bdd->get_mutable_symbol_manager()->get_symbol("code_path");
  } else {
    code_path_symbol = new_bdd->get_mutable_symbol_manager()->create_symbol("code_path", sizeof(bdd_node_id_t) * 8);
  }

  Symbols symbols;
  symbols.add(code_path_symbol);
  Call *parse_cpu = new_bdd->create_new_call(new_bdd->get_mutable_node_by_id(new_bdd->get_id()), call, symbols);

  return parse_cpu;
}

std::optional<BDDNode *> Placer::create_parse_header_vars_node(std::unique_ptr<BDD> &new_bdd, bdd_node_id_t current_id, bdd_node_id_t next_id) {

  const BDDNode *current_node = new_bdd->get_node_by_id(current_id);
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

  Symbols symbols = get_relevant_dataplane_state(new_bdd, current_node, {});
  symbols.remove("packet_chunks");
  symbols.remove("DEVICE");
  Call *parse_vars = new_bdd->create_new_call(new_bdd->get_mutable_node_by_id(new_bdd->get_id()), call, symbols);

  return parse_vars;
}

void Placer::handle_branch_node(std::unique_ptr<BDD> &new_bdd, bdd_node_id_t branch_id, bdd_node_id_t on_true_id, bdd_node_id_t on_false_id,
                                bool in_root) {

  const std::vector<const BDDNode *> s2d_true = create_send_to_device_node(new_bdd, branch_id, on_true_id);

  if (!s2d_true.empty()) {
    new_bdd->add_cloned_non_branches(on_true_id, s2d_true);
  }

  if (!in_root) {
    const std::vector<const BDDNode *> s2d_false = create_send_to_device_node(new_bdd, branch_id, on_false_id);

    if (!s2d_false.empty()) {
      new_bdd->add_cloned_non_branches(on_false_id, s2d_false);
    }
  }
}

void Placer::handle_node(std::unique_ptr<BDD> &new_bdd, bdd_node_id_t current_id, bdd_node_id_t next_id) {
  const std::vector<const BDDNode *> s2d = create_send_to_device_node(new_bdd, current_id, next_id);
  if (!s2d.empty()) {
    new_bdd->add_cloned_non_branches(next_id, s2d);
  }
}

std::unordered_map<LibSynapse::TargetType, target_roots_t> Placer::get_target_roots(std::unique_ptr<BDD> &_bdd) {

  std::cerr << "==========================================\n";
  std::cerr << "========Retreiving Target Roots===========\n";
  std::cerr << "==========================================\n";

  const InfrastructureNode *global_ports = phys_net.get_node(-1);

  std::unordered_map<LibSynapse::TargetType, target_roots_t> target_roots;
  for (const auto &[target, _] : phys_net.get_target_list()) {
    target_roots[target];
  }

  bool in_root = true;

  BDDNode *root = _bdd->get_mutable_root();
  assert(root && "Root Node not found");

  std::queue<BDDNode *> queue;

  queue.push(root);

  if (root->get_type() != BDDNodeType::Branch) {
    in_root = false;
  }

  while (!queue.empty()) {
    BDDNode *current = queue.front();
    queue.pop();

    if (current->get_type() == BDDNodeType::Branch) {
      assert(current->get_type() == BDDNodeType::Branch && "Root not a branch");
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
        if (branch->get_on_false()->get_type() != BDDNodeType::Branch) {
          in_root = false;
        }
      }
      queue.push(branch->get_mutable_on_true());
      queue.push(branch->get_mutable_on_false());

    } else if (current->get_next()) {
      BDDNode *next = current->get_mutable_next();
      assert(next && "SendToDevice must have child");

      if (bdd_node_match_pattern(current)) {

        const Call *call_node = dynamic_cast<const Call *>(current);
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
  }

  return target_roots;
}

std::unique_ptr<BDD> Placer::add_send_to_device_nodes() {
  std::cerr << "==========================================\n";
  std::cerr << "=============Adding s2d nodes=============\n";
  std::cerr << "==========================================\n";

  const BDD *old_bdd           = get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  const BDDNode *root = new_bdd->get_root();

  std::queue<const BDDNode *> queue;

  queue.push(root);

  bool in_root = true;

  while (!queue.empty()) {
    const BDDNode *current = queue.front();
    queue.pop();

    if (current->get_type() == BDDNodeType::Branch) {
      const Branch *branch = dynamic_cast<const Branch *>(current);

      const BDDNode *on_true  = branch->get_on_true();
      const BDDNode *on_false = branch->get_on_false();
      assert(on_true && on_false && "Branch node must have both on_true and on_false nodes");

      if (on_false->get_type() != BDDNodeType::Branch) {
        in_root = false;
      }

      handle_branch_node(new_bdd, branch->get_id(), on_true->get_id(), on_false->get_id(), in_root);

      queue.push(on_true);
      queue.push(on_false);

    } else {
      if (current->get_next()) {
        const BDDNode *next = current->get_next();

        handle_node(new_bdd, current->get_id(), next->get_id());
        queue.push(next);
      }
    }
  }
  return new_bdd;
}

std::unique_ptr<BDD> Placer::extract_target_bdd(std::unique_ptr<BDD> &global_bdd, bdd_node_ids_t port_roots, bdd_node_ids_t roots) {

  std::cerr << "==========================================\n";
  std::cerr << "===========Extracting Target BDD==========\n";
  std::cerr << "==========================================\n";

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
    Branch *new_branch = build_global_port_subgraph(extracted_bdd, root);

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
    Branch *new_branch = build_code_path_subgraph(extracted_bdd, root);
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

  std::cerr << new_root->dump(true) << "\n";
  extracted_bdd->set_root(new_root);

  // BDDViz::visualize(extracted_bdd.get(), false);

  return extracted_bdd;
}

std::unique_ptr<const BDD> Placer::concretize_ports(std::unique_ptr<BDD> &old_bdd) {
  std::cerr << "==========================================\n";
  std::cerr << "===========Concretizing Ports=============\n";
  std::cerr << "==========================================\n";

  std::unique_ptr<BDD> new_bdd           = std::make_unique<BDD>(*old_bdd);
  const InfrastructureNode *global_ports = phys_net.get_node(-1);

  bdd_node_id_t root_id = new_bdd->get_root()->get_id();
  assert(new_bdd->get_node_by_id(root_id) && "Root Node not found");

  if (new_bdd->get_node_by_id(root_id)->get_type() != BDDNodeType::Branch) {
    return new_bdd;
  }

  std::queue<bdd_node_id_t> queue;

  queue.push(root_id);

  while (!queue.empty()) {

    BDDNode *current = new_bdd->get_mutable_node_by_id(queue.front());
    assert(current && "Root Node not found");
    assert(current->get_type() == BDDNodeType::Branch && "Root not a branch");
    queue.pop();

    std::cerr << current->dump(true) << "\n";

    Branch *branch                          = dynamic_cast<Branch *>(current);
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
    const Port local_port = global_ports->get_link(global_port).first;

    klee::ref<klee::Expr> new_condition = solver_toolbox.exprBuilder->Eq(right, solver_toolbox.exprBuilder->Constant(local_port, right->getWidth()));

    branch->set_condition(new_condition);

    if (branch->get_on_false()->get_type() == BDDNodeType::Branch) {
      queue.push(branch->get_on_false()->get_id());
    }
  }

  return new_bdd;
}

std::unordered_map<LibSynapse::TargetType, std::unique_ptr<const BDD>> Placer::process() {
  std::cerr << "==========================================\n";
  std::cerr << "=============Processing BDD===============\n";
  std::cerr << "==========================================\n";

  std::unordered_map<LibSynapse::TargetType, std::unique_ptr<const BDD>> target_bdds;

  std::unique_ptr<BDD> processed_bdd = add_send_to_device_nodes();

  BDDViz::visualize(processed_bdd.get(), false);

  std::unordered_map<LibSynapse::TargetType, target_roots_t> target_roots = get_target_roots(processed_bdd);

  for (const auto &[target, _] : phys_net.get_target_list()) {

    std::unique_ptr<BDD> extracted_bdd = extract_target_bdd(processed_bdd, target_roots[target].port_roots, target_roots[target].target_roots);
    const LibBDD::BDD::inspection_report_t extracted_report = extracted_bdd->inspect();
    assert_or_panic(extracted_report.status == LibBDD::BDD::InspectionStatus::Ok, "BDD inspection failed: %s", extracted_report.message.c_str());

    std::unique_ptr<const BDD> target_bdd         = concretize_ports(extracted_bdd);
    const LibBDD::BDD::inspection_report_t report = target_bdd->inspect();
    assert_or_panic(report.status == LibBDD::BDD::InspectionStatus::Ok, "BDD inspection failed: %s", report.message.c_str());
    std::cout << "BDD inspection passed.\n";

    // BDDViz::visualize(target_bdd.get(), false);
    target_bdds.emplace(target, std::move(target_bdd));
  }

  return target_bdds;
}

} // namespace LibClone
