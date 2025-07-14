#include <LibBDD/Visitors/PrinterDebug.h>
#include <LibBDD/Nodes/Nodes.h>
#include <LibBDD/BDD.h>
#include <LibCore/Expr.h>

namespace LibBDD {

using LibCore::expr_to_string;

void PrinterDebug::debug(const BDDNode *node) {
  PrinterDebug debug(false);
  node->visit(debug);
}

void PrinterDebug::visit(const BDD *bdd) {
  const BDDNode *root = bdd->get_root();
  assert(root && "No root node");

  klee::ref<klee::Expr> device     = bdd->get_device().expr;
  klee::ref<klee::Expr> packet_len = bdd->get_packet_len().expr;
  klee::ref<klee::Expr> time       = bdd->get_time().expr;

  std::cerr << "===========================================\n";
  std::cerr << "Init calls:\n";
  for (const Call *call : bdd->get_init()) {
    std::cerr << "\t" << call->get_call() << "\n";
  }
  std::cerr << "===========================================\n";

  visitRoot(root);

  std::cerr << "===========================================\n";
  std::cerr << "Base symbols:\n";
  std::cerr << "  " << expr_to_string(device, true) << "\n";
  std::cerr << "  " << expr_to_string(packet_len, true) << "\n";
  std::cerr << "  " << expr_to_string(time, true) << "\n";
  std::cerr << "===========================================\n";
}

void PrinterDebug::visitRoot(const BDDNode *root) { root->visit(*this); }

BDDVisitor::Action PrinterDebug::visit(const Branch *node) {
  bdd_node_id_t id                = node->get_id();
  klee::ref<klee::Expr> condition = node->get_condition();

  const BDDNode *on_true  = node->get_on_true();
  const BDDNode *on_false = node->get_on_false();

  std::string on_true_id  = on_true ? std::to_string(on_true->get_id()) : "X";
  std::string on_false_id = on_false ? std::to_string(on_false->get_id()) : "X";

  std::cerr << "===========================================\n";
  std::cerr << "node:      " << id << "\n";
  std::cerr << "type:      branch\n";
  std::cerr << "condition: ";
  condition->dump();
  std::cerr << "on true:   " << on_true_id << "\n";
  std::cerr << "on false:  " << on_false_id << "\n";
  visitConstraints(node);
  std::cerr << "===========================================\n";

  return traverse ? BDDVisitor::Action::Continue : BDDVisitor::Action::Stop;
}

BDDVisitor::Action PrinterDebug::visit(const Call *node) {
  bdd_node_id_t id    = node->get_id();
  const call_t &call  = node->get_call();
  const BDDNode *next = node->get_next();

  std::cerr << "===========================================\n";
  std::cerr << "node:      " << id << "\n";
  std::cerr << "type:      call\n";
  std::cerr << "function:  " << call.function_name << "\n";
  std::cerr << "args:      ";
  bool indent = false;
  for (const auto &arg : call.args) {
    if (indent)
      std::cerr << "           ";
    std::cerr << arg.first << " : ";
    arg.second.expr->dump();
    indent = true;
  }
  if (call.args.size() == 0) {
    std::cerr << "\n";
  }
  if (!call.ret.isNull()) {
    std::cerr << "ret:       ";
    call.ret->dump();
  }
  if (next) {
    std::cerr << "next:      " << next->get_id() << "\n";
  }
  visitConstraints(node);
  std::cerr << "===========================================\n";

  return traverse ? BDDVisitor::Action::Continue : BDDVisitor::Action::Stop;
}

BDDVisitor::Action PrinterDebug::visit(const Route *node) {
  bdd_node_id_t id                 = node->get_id();
  klee::ref<klee::Expr> dst_device = node->get_dst_device();
  RouteOp operation                = node->get_operation();
  const BDDNode *next              = node->get_next();

  std::cerr << "===========================================\n";
  std::cerr << "node:      " << id << "\n";
  std::cerr << "type:      route\n";
  std::cerr << "operation: ";
  switch (operation) {
  case RouteOp::Forward: {
    std::cerr << "fwd(" << expr_to_string(dst_device) << ")";
    break;
  }
  case RouteOp::Drop: {
    std::cerr << "drop()";
    break;
  }
  case RouteOp::Broadcast: {
    std::cerr << "bcast()";
    break;
  }
  }
  std::cerr << "\n";
  if (next) {
    std::cerr << "next:      " << next->get_id() << "\n";
  }
  visitConstraints(node);
  std::cerr << "===========================================\n";

  return traverse ? BDDVisitor::Action::Continue : BDDVisitor::Action::Stop;
}

void PrinterDebug::visitConstraints(const BDDNode *node) {
  const auto &constraints = node->get_constraints();
  if (constraints.size() > 0) {
    std::cerr << "constraints:\n";
    for (const auto &constraint : constraints) {
      std::cerr << "  " << expr_to_string(constraint, true) << "\n";
    }
  }
}

} // namespace LibBDD