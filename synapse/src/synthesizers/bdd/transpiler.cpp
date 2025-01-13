#include "transpiler.h"
#include "synthesizer.h"
#include "../../util/exprs.h"
#include "../../util/solver.h"
#include "../../util/simplifier.h"
#include "../../system.h"

#define TODO(expr)                                                                                                     \
  synthesizer->stack_dbg();                                                                                            \
  panic("TODO: %s\n", expr_to_string(expr).c_str());

namespace synapse {
BDDTranspiler::BDDTranspiler(BDDSynthesizer *_synthesizer) : synthesizer(_synthesizer) {}

code_t BDDTranspiler::transpile(klee::ref<klee::Expr> expr) {
  coders.emplace();
  coder_t &coder = coders.top();

  expr = simplify(expr);

  if (is_constant(expr)) {
    assert(expr->getWidth() <= 64 && "Unsupported constant width");
    u64 value = solver_toolbox.value_from_expr(expr);
    coder << value;
    if (value > (1ull << 31)) {
      if (!is_constant_signed(expr)) {
        coder << "U";
      }
      coder << "LL";
    }
  } else {
    visit(expr);

    // HACK: clear the visited map so we force the transpiler to revisit all
    // expressions.
    visited.clear();
  }

  code_t code = coder.dump();
  coders.pop();

  assert(!code.empty() && "Empty code");
  return code;
}

code_t BDDTranspiler::type_from_size(bits_t size) {
  code_t type;

  switch (size) {
  case 1:
    type = "bool";
    break;
  case 8:
    type = "uint8_t";
    break;
  case 16:
    type = "uint16_t";
    break;
  case 32:
    type = "uint32_t";
    break;
  case 64:
    type = "uint64_t";
    break;
  default:
    panic("Unknown type (size=%u)\n", size);
  }

  return type;
}

code_t BDDTranspiler::type_from_expr(klee::ref<klee::Expr> expr) { return type_from_size(expr->getWidth()); }

klee::ExprVisitor::Action BDDTranspiler::visitRead(const klee::ReadExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ReadExpr *>(&e);

  coder_t &coder = coders.top();

  BDDSynthesizer::var_t var = synthesizer->stack_get(expr);

  if (!var.addr.isNull()) {
    coder << "*";
  }

  coder << var.name;

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitNotOptimized(const klee::NotOptimizedExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::NotOptimizedExpr *>(&e);
  TODO(expr);
  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitSelect(const klee::SelectExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::SelectExpr *>(&e);
  TODO(expr);
  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitConcat(const klee::ConcatExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ConcatExpr *>(&e);

  coder_t &coder = coders.top();

  BDDSynthesizer::var_t var = synthesizer->stack_get(expr);

  if (!var.addr.isNull() && expr->getWidth() > 8) {
    coder << "*";

    if (expr->getWidth() <= 64) {
      coder << "(";
      coder << type_from_size(expr->getWidth());
      coder << "*)";
    }
  }

  coder << var.name;
  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitExtract(const klee::ExtractExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ExtractExpr *>(&e);

  bits_t width              = e.width;
  bits_t offset             = e.offset;
  klee::ref<klee::Expr> arg = e.expr;

  coder_t &coder = coders.top();

  if (width != arg->getWidth()) {
    coder << "(";
    coder << type_from_expr(arg);
    coder << ")";
    coder << "(";
  }

  coder << transpile(arg);

  if (offset > 0) {
    coder << ">>";
    coder << offset;
  }

  if (width != arg->getWidth()) {
    coder << ")";
  }

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitZExt(const klee::ZExtExpr &e) {
  klee::ref<klee::Expr> arg = e.getKid(0);

  coder_t &coder = coders.top();

  coder << "(";
  coder << type_from_expr(arg);
  coder << ")";
  coder << "(";
  coder << transpile(arg);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitSExt(const klee::SExtExpr &e) {
  // FIXME: We should handle sign extension properly.

  klee::ref<klee::Expr> arg = e.getKid(0);

  coder_t &coder = coders.top();

  coder << "(";
  coder << type_from_expr(arg);
  coder << ")";
  coder << "(";
  coder << transpile(arg);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitAdd(const klee::AddExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " + ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitSub(const klee::SubExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " - ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitMul(const klee::MulExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " * ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitUDiv(const klee::UDivExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " / ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitSDiv(const klee::SDivExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " / ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitURem(const klee::URemExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " % ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitSRem(const klee::SRemExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " % ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitNot(const klee::NotExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> arg = e.getKid(0);

  coder << "!(";
  coder << transpile(arg);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitAnd(const klee::AndExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " & ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitOr(const klee::OrExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " | ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitXor(const klee::XorExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::XorExpr *>(&e);
  TODO(expr);
  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitShl(const klee::ShlExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ShlExpr *>(&e);
  TODO(expr);
  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitLShr(const klee::LShrExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::LShrExpr *>(&e);
  TODO(expr);
  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitAShr(const klee::AShrExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::AShrExpr *>(&e);
  TODO(expr);
  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitEq(const klee::EqExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  klee::ref<klee::Expr> var_expr;
  klee::ref<klee::Expr> const_expr;

  if (is_constant(lhs)) {
    const_expr = lhs;
    var_expr   = rhs;
  } else {
    const_expr = rhs;
    var_expr   = lhs;
  }

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " == ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitNe(const klee::NeExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  klee::ref<klee::Expr> var_expr;
  klee::ref<klee::Expr> const_expr;

  if (is_constant(lhs)) {
    const_expr = lhs;
    var_expr   = rhs;
  } else {
    const_expr = rhs;
    var_expr   = lhs;
  }

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " != ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitUlt(const klee::UltExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " < ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitUle(const klee::UleExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " <= ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitUgt(const klee::UgtExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " > ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitUge(const klee::UgeExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " >= ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitSlt(const klee::SltExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " < ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitSle(const klee::SleExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " <= ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitSgt(const klee::SgtExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " > ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDTranspiler::visitSge(const klee::SgeExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " >= ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

} // namespace synapse