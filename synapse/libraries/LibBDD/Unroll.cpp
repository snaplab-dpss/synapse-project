#include <LibBDD/Unroll.h>
#include <LibCore/Expr.h>
#include <LibCore/Solver.h>
#include <LibCore/Debug.h>

#include <klee/util/ExprHashMap.h>

namespace LibBDD {

using LibCore::canonicalize_byte_swaps;
using LibCore::solver_toolbox;
using LibCore::symbol_t;

namespace {

struct op_t {
  klee::Expr::Kind kind;
  std::string name;
};

const std::vector<op_t> ops{
    {klee::Expr::Add, "op_add"},   {klee::Expr::Sub, "op_sub"},   {klee::Expr::Mul, "op_mul"},   {klee::Expr::UDiv, "op_udiv"},
    {klee::Expr::SDiv, "op_sdiv"}, {klee::Expr::URem, "op_urem"}, {klee::Expr::SRem, "op_srem"}, {klee::Expr::And, "op_and"},
    {klee::Expr::Or, "op_or"},     {klee::Expr::Xor, "op_xor"},   {klee::Expr::Not, "op_not"},   {klee::Expr::Shl, "op_shl"},
    {klee::Expr::LShr, "op_lshr"}, {klee::Expr::AShr, "op_ashr"},
};

const op_t *op_from_kind(klee::Expr::Kind kind) {
  for (const op_t &op : ops) {
    if (op.kind == kind) {
      return &op;
    }
  }
  return nullptr;
}

const op_t *op_from_name(const std::string &name) {
  for (const op_t &op : ops) {
    if (op.name == name) {
      return &op;
    }
  }
  return nullptr;
}

klee::ref<klee::Expr> build_op(klee::Expr::Kind kind, klee::ref<klee::Expr> a, klee::ref<klee::Expr> b) {
  klee::ExprBuilder *builder = solver_toolbox.exprBuilder.get();
  switch (kind) {
  case klee::Expr::Add:
    return builder->Add(a, b);
  case klee::Expr::Sub:
    return builder->Sub(a, b);
  case klee::Expr::Mul:
    return builder->Mul(a, b);
  case klee::Expr::UDiv:
    return builder->UDiv(a, b);
  case klee::Expr::SDiv:
    return builder->SDiv(a, b);
  case klee::Expr::URem:
    return builder->URem(a, b);
  case klee::Expr::SRem:
    return builder->SRem(a, b);
  case klee::Expr::And:
    return builder->And(a, b);
  case klee::Expr::Or:
    return builder->Or(a, b);
  case klee::Expr::Xor:
    return builder->Xor(a, b);
  case klee::Expr::Not:
    return builder->Not(a);
  case klee::Expr::Shl:
    return builder->Shl(a, b);
  case klee::Expr::LShr:
    return builder->LShr(a, b);
  case klee::Expr::AShr:
    return builder->AShr(a, b);
  default:
    panic("Not an arithmetic operation kind: %d", kind);
  }
}

// Unrolling state for one code path: the values already given a node (keyed structurally, so a
// shared subexpression gets one node), and the rewrites of everything beneath them, so a shared
// subexpression is also walked once.
struct scope_t {
  klee::ExprHashMap<klee::ref<klee::Expr>> unrolled;
  klee::ExprHashMap<klee::ref<klee::Expr>> rewritten;
};

class Unroller {
private:
  BDD &bdd;
  size_t n_symbols;

public:
  Unroller(BDD &_bdd) : bdd(_bdd), n_symbols(0) {}

  void run() {
    BDDNode *root = bdd.get_mutable_node_by_id(bdd.get_root()->get_id());
    unroll_path(root, scope_t());
  }

private:
  // Scopes are copied at branches: what was unrolled before a branch serves both sides, what one
  // side unrolls is not visible to the other.
  void unroll_path(BDDNode *node, scope_t scope) {
    while (node) {
      unroll_node(node, scope);

      switch (node->get_type()) {
      case BDDNodeType::Branch: {
        Branch *branch = dynamic_cast<Branch *>(node);
        unroll_path(branch->get_mutable_on_true(), scope);
        node = branch->get_mutable_on_false();
      } break;
      case BDDNodeType::Call:
      case BDDNodeType::Route: {
        node = node->get_mutable_next();
      } break;
      }
    }
  }

  void unroll_node(BDDNode *node, scope_t &scope) {
    auto unroll = [&](klee::ref<klee::Expr> &expr) {
      if (expr.isNull()) {
        return;
      }
      expr = rewrite(canonicalize_byte_swaps(expr), false, node, scope);
    };

    switch (node->get_type()) {
    case BDDNodeType::Call: {
      Call *call_node = dynamic_cast<Call *>(node);
      call_t call     = call_node->get_call();
      for (auto &[name, arg] : call.args) {
        unroll(arg.expr);
        unroll(arg.in);
        unroll(arg.out);
      }
      for (auto &[name, extra_var] : call.extra_vars) {
        unroll(extra_var.first);
        unroll(extra_var.second);
      }
      call_node->set_call(call);
    } break;
    case BDDNodeType::Branch: {
      Branch *branch                  = dynamic_cast<Branch *>(node);
      klee::ref<klee::Expr> condition = branch->get_condition();
      unroll(condition);
      branch->set_condition(condition);
    } break;
    case BDDNodeType::Route: {
      Route *route                     = dynamic_cast<Route *>(node);
      klee::ref<klee::Expr> dst_device = route->get_dst_device();
      unroll(dst_device);
      route->set_dst_device(dst_device);
    } break;
    }
  }

  // Rewrites `expr` so that it holds no nested arithmetic; `nested` says whether `expr` itself
  // sits beneath an arithmetic operation, in which case an arithmetic `expr` becomes a node.
  klee::ref<klee::Expr> rewrite(klee::ref<klee::Expr> expr, bool nested, BDDNode *consumer, scope_t &scope) {
    if (expr->getKind() == klee::Expr::Constant) {
      return expr;
    }

    if (nested) {
      auto found_it = scope.rewritten.find(expr);
      if (found_it != scope.rewritten.end()) {
        return found_it->second;
      }
    }

    const bool arithmetic = is_arithmetic_op(expr);
    const unsigned n_kids = expr->getNumKids();

    std::vector<klee::ref<klee::Expr>> kids(n_kids);
    bool changed = false;
    for (unsigned i = 0; i < n_kids; i++) {
      kids[i] = rewrite(expr->getKid(i), nested || arithmetic, consumer, scope);
      changed |= kids[i].get() != expr->getKid(i).get();
    }

    klee::ref<klee::Expr> result = changed ? expr->rebuild(kids.data()) : expr;

    if (arithmetic && nested) {
      auto found_it = scope.unrolled.find(result);
      result        = found_it != scope.unrolled.end() ? found_it->second : emit(result, consumer, scope);
    }

    if (nested) {
      scope.rewritten[expr] = result;
    }

    return result;
  }

  // Gives `expr` (a single arithmetic operation over already rewritten operands) its own node
  // before `consumer`, and returns the symbol that stands for it from now on.
  klee::ref<klee::Expr> emit(klee::ref<klee::Expr> expr, BDDNode *consumer, scope_t &scope) {
    const op_t *op     = op_from_kind(expr->getKind());
    const bits_t width = expr->getWidth();
    assert_or_panic(op, "Not an arithmetic operation");
    assert_or_panic(width % 8 == 0, "Unrolling an arithmetic operation of width %u", width);

    SymbolManager *symbol_manager = bdd.get_mutable_symbol_manager();
    const symbol_t symbol         = symbol_manager->create_symbol("unrolled__" + std::to_string(n_symbols++), width);

    call_t call;
    call.function_name = op->name;
    call.args["a"].expr = expr->getKid(0);
    if (expr->getNumKids() == 2) {
      call.args["b"].expr = expr->getKid(1);
    }
    call.ret = symbol.expr;

    Call *node = bdd.create_new_call(call, Symbols({symbol}));
    insert_before(consumer, node);

    scope.unrolled[expr] = symbol.expr;
    return symbol.expr;
  }

  void insert_before(BDDNode *consumer, BDDNode *node) {
    BDDNode *prev = consumer->get_mutable_prev();

    if (!prev) {
      bdd.set_root(node);
    } else if (prev->get_type() == BDDNodeType::Branch) {
      Branch *branch = dynamic_cast<Branch *>(prev);
      if (branch->get_on_true() == consumer) {
        branch->set_on_true(node);
      } else {
        assert_or_panic(branch->get_on_false() == consumer, "Consumer not attached to its branch");
        branch->set_on_false(node);
      }
    } else {
      prev->set_next(node);
    }

    node->set_prev(prev);
    node->set_next(consumer);
    consumer->set_prev(node);
  }
};

bool has_nested_arithmetic(klee::ref<klee::Expr> expr, bool nested) {
  if (expr.isNull() || expr->getKind() == klee::Expr::Constant) {
    return false;
  }
  const bool arithmetic = is_arithmetic_op(expr);
  if (arithmetic && nested) {
    return true;
  }
  for (unsigned i = 0; i < expr->getNumKids(); i++) {
    if (has_nested_arithmetic(expr->getKid(i), nested || arithmetic)) {
      return true;
    }
  }
  return false;
}

void check_invariant(const BDD &bdd) {
  bdd.get_root()->visit_nodes([](const BDDNode *node) {
    std::vector<klee::ref<klee::Expr>> exprs;
    switch (node->get_type()) {
    case BDDNodeType::Call: {
      const call_t &call = dynamic_cast<const Call *>(node)->get_call();
      for (const auto &[name, arg] : call.args) {
        exprs.insert(exprs.end(), {arg.expr, arg.in, arg.out});
      }
      for (const auto &[name, extra_var] : call.extra_vars) {
        exprs.insert(exprs.end(), {extra_var.first, extra_var.second});
      }
    } break;
    case BDDNodeType::Branch: {
      exprs.push_back(dynamic_cast<const Branch *>(node)->get_condition());
    } break;
    case BDDNodeType::Route: {
      exprs.push_back(dynamic_cast<const Route *>(node)->get_dst_device());
    } break;
    }
    for (const klee::ref<klee::Expr> &expr : exprs) {
      assert_or_panic(!has_nested_arithmetic(expr, false), "Node %lu still holds nested arithmetic", node->get_id());
    }
    return BDDNodeVisitAction::Continue;
  });
}

} // namespace

bool is_arithmetic_op(klee::ref<klee::Expr> expr) {
  switch (expr->getKind()) {
  case klee::Expr::Add:
  case klee::Expr::Sub:
  case klee::Expr::Mul:
  case klee::Expr::UDiv:
  case klee::Expr::SDiv:
  case klee::Expr::URem:
  case klee::Expr::SRem:
  case klee::Expr::Shl:
  case klee::Expr::LShr:
  case klee::Expr::AShr:
    return true;
  case klee::Expr::And:
  case klee::Expr::Or:
  case klee::Expr::Xor:
  case klee::Expr::Not:
    // On single bits these are the logical connectives.
    return expr->getWidth() > 1;
  default:
    return false;
  }
}

const std::vector<std::string> &unrolled_op_function_names() {
  static const std::vector<std::string> names = [] {
    std::vector<std::string> op_names;
    for (const op_t &op : ops) {
      op_names.push_back(op.name);
    }
    return op_names;
  }();
  return names;
}

bool is_unrolled_op(const call_t &call) { return op_from_name(call.function_name) != nullptr; }

klee::ref<klee::Expr> unrolled_op_value(const call_t &call) {
  const op_t *op = op_from_name(call.function_name);
  assert_or_panic(op, "Not an unrolled op: %s", call.function_name.c_str());
  klee::ref<klee::Expr> a = call.args.at("a").expr;
  klee::ref<klee::Expr> b = call.args.count("b") ? call.args.at("b").expr : klee::ref<klee::Expr>();
  return build_op(op->kind, a, b);
}

void unroll_arithmetic(BDD &bdd) {
  Unroller unroller(bdd);
  unroller.run();
  check_invariant(bdd);
}

} // namespace LibBDD
