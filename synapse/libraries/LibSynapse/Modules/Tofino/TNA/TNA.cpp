#include <LibSynapse/Modules/Tofino/TNA/TNA.h>
#include <LibCore/Math.h>

#include <klee/util/ExprVisitor.h>

namespace LibSynapse {
namespace Tofino {

using LibCore::get_unique_symbolic_reads;
using LibCore::is_power_of_two;
using LibCore::is_readLSB;
using LibCore::stitch_conditions;
using LibCore::symbolic_read_t;
using LibCore::symbolic_reads_t;

namespace {

class PHVBytesRetriever : public klee::ExprVisitor::ExprVisitor {
private:
  bytes_t used_phv_bytes;
  std::unordered_set<std::string> exception_symbols;
  symbolic_reads_t symbolic_reads;

public:
  PHVBytesRetriever() : used_phv_bytes(0) {}
  PHVBytesRetriever(const std::unordered_set<std::string> &_exception_symbols) : used_phv_bytes(0), exception_symbols(_exception_symbols) {}

  Action visitRead(const klee::ReadExpr &e) override final {
    assert(e.index->getKind() == klee::Expr::Kind::Constant && "Non-constant index");

    const klee::ConstantExpr *index_const = dynamic_cast<klee::ConstantExpr *>(e.index.get());
    const bytes_t byte                    = index_const->getZExtValue();
    const std::string name                = e.updates.root->name;
    const symbolic_read_t symbolic_read{byte, name};

    if (exception_symbols.contains(name)) {
      return Action::doChildren();
    }

    if (!symbolic_reads.contains(symbolic_read)) {
      symbolic_reads.insert({byte, name});
      used_phv_bytes += 1;
    }

    return Action::doChildren();
  }

  Action visitExpr(const klee::Expr &e) override final {
    if (e.getKind() == klee::Expr::Kind::Read) {
      return Action::doChildren();
    }

    for (size_t i = 0; i < e.getNumKids(); i++) {
      klee::ref<klee::Expr> kid = e.getKid(i);
      if (kid->getKind() == klee::Expr::Constant) {
        const u64 value = solver_toolbox.value_from_expr(kid);

        // If the value is 1 less than a power of two, the compiler can optimize it to not consume PHV resources.
        if (is_power_of_two(value + 1)) {
          continue;
        }

        const bytes_t width = kid->getWidth() / 8;

        used_phv_bytes += width;
      }
    }

    return Action::doChildren();
  }

  bytes_t get_used_phv_bytes() const { return used_phv_bytes; }
};

} // namespace

bool TNA::condition_meets_phv_limit(klee::ref<klee::Expr> expr) const {
  PHVBytesRetriever retriever;
  retriever.visit(expr);
  return retriever.get_used_phv_bytes() <= tna_config.properties.max_phv_bytes_in_condition;
}

void TNA::debug() const { pipeline.debug(); }

std::vector<tofino_port_t> TNA::plausible_ingress_ports_in_bdd_node(const BDD *bdd, const BDDNode *node) const {
  std::vector<tofino_port_t> plausible_ports;

  const klee::ConstraintManager constraints = bdd->get_constraints(node);
  for (const tofino_port_t port : tna_config.ports) {
    klee::ref<klee::Expr> handles_port = solver_toolbox.exprBuilder->Eq(
        bdd->get_device().expr, solver_toolbox.exprBuilder->Constant(port.nf_device, bdd->get_device().expr->getWidth()));
    if (solver_toolbox.is_expr_maybe_true(constraints, handles_port)) {
      plausible_ports.push_back(port);
    }
  }

  return plausible_ports;
}

bool TNA::is_simple_conditional_expr(klee::ref<klee::Expr> condition) const {
  bool is_simple = false;

  switch (condition->getKind()) {
  case klee::Expr::Kind::Eq:
  case klee::Expr::Kind::Ne:
  case klee::Expr::Kind::Ult:
  case klee::Expr::Kind::Ule:
  case klee::Expr::Kind::Ugt:
  case klee::Expr::Kind::Uge:
  case klee::Expr::Kind::Slt:
  case klee::Expr::Kind::Sle:
  case klee::Expr::Kind::Sgt:
  case klee::Expr::Kind::Sge: {
    klee::ref<klee::Expr> lhs = condition->getKid(0);
    klee::ref<klee::Expr> rhs = condition->getKid(1);
    is_simple                 = is_simple_conditional_expr(lhs) && is_simple_conditional_expr(rhs);
  } break;
  case klee::Expr::Kind::Not:
  case klee::Expr::Kind::ZExt:
  case klee::Expr::Kind::Extract: {
    is_simple = is_simple_conditional_expr(condition->getKid(0));
  } break;
  // A computed operand (e.g. rank = tz + 1) is materialized into a metadata
  // field before the gateway, so it's a valid condition input.
  case klee::Expr::Kind::Add:
  case klee::Expr::Kind::Sub: {
    is_simple = is_simple_conditional_expr(condition->getKid(0)) && is_simple_conditional_expr(condition->getKid(1));
  } break;
  case klee::Expr::Kind::Concat: {
    is_simple = is_readLSB(condition);
  } break;
  case klee::Expr::Kind::Read:
  case klee::Expr::Kind::Constant: {
    is_simple = true;
  } break;
  default:
    is_simple = false;
  }

  return is_simple;
}

bool TNA::is_simple_register_conditional_expr(const std::vector<klee::ref<klee::Expr>> &exprs, const symbol_t &register_value) const {
  if (exprs.size() > 2) {
    return false;
  }

  for (const klee::ref<klee::Expr> &expr : exprs) {
    if (!is_simple_conditional_expr(expr)) {
      return false;
    }
  }

  klee::ref<klee::Expr> combined_expr = stitch_conditions(exprs);

  PHVBytesRetriever retriever({register_value.name});
  retriever.visit(combined_expr);
  const bytes_t used_phv_bytes = retriever.get_used_phv_bytes();

  if (used_phv_bytes > tna_config.properties.max_phv_bytes_in_condition) {
    return false;
  }

  return true;
}

} // namespace Tofino
} // namespace LibSynapse