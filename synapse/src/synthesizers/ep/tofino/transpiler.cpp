#include "transpiler.h"
#include "synthesizer.h"
#include "../../../util/exprs.h"
#include "../../../util/solver.h"

namespace synapse {
namespace tofino {
namespace {
code_t transpile_constant(klee::ref<klee::Expr> expr) {
  SYNAPSE_ASSERT(is_constant(expr), "Expected a constant expression");

  bytes_t width = expr->getWidth() / 8;

  coder_t code;
  code << width * 8 << "w";

  for (size_t byte = 0; byte < width; byte++) {
    klee::ref<klee::Expr> extract =
        solver_toolbox.exprBuilder->Extract(expr, (width - byte - 1) * 8, 8);
    u64 byte_value = solver_toolbox.value_from_expr(extract);

    std::stringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0') << byte_value;

    code << ss.str();
  }

  return code.dump();
}
} // namespace

Transpiler::Transpiler(const EPSynthesizer *_synthesizer) : synthesizer(_synthesizer) {}

code_t Transpiler::transpile(klee::ref<klee::Expr> expr) {
  Log::dbg() << "Transpile: " << expr_to_string(expr, false) << "\n";

  coders.emplace();
  coder_t &coder = coders.top();

  if (is_constant(expr)) {
    coder << transpile_constant(expr);
  } else {
    visit(expr);

    // HACK: clear the visited map so we force the transpiler to revisit all
    // expressions.
    visited.clear();
  }

  code_t code = coder.dump();
  coders.pop();

  SYNAPSE_ASSERT(!code.empty(), "Empty code");
  return code;
}

code_t Transpiler::type_from_size(bits_t size) const {
  coder_t coder;
  coder << "bit<" << size << ">";
  return coder.dump();
}

code_t Transpiler::type_from_expr(klee::ref<klee::Expr> expr) const {
  klee::Expr::Width width = expr->getWidth();
  SYNAPSE_ASSERT(width != klee::Expr::InvalidWidth, "Invalid width");
  return type_from_size(width);
}

klee::ExprVisitor::Action Transpiler::visitRead(const klee::ReadExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ReadExpr *>(&e);

  coder_t &coder = coders.top();

  EPSynthesizer::var_t var;
  if (synthesizer->get_var(expr, var)) {
    coder << var.name;
    return klee::ExprVisitor::Action::skipChildren();
  }

  Log::dbg() << expr_to_string(expr) << "\n";
  synthesizer->dbg_vars();

  SYNAPSE_PANIC("TODO");
  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitNotOptimized(const klee::NotOptimizedExpr &e) {
  SYNAPSE_PANIC("TODO");
  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitSelect(const klee::SelectExpr &e) {
  SYNAPSE_PANIC("TODO");
  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitConcat(const klee::ConcatExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ConcatExpr *>(&e);

  coder_t &coder = coders.top();

  EPSynthesizer::var_t var;
  if (synthesizer->get_var(expr, var)) {
    coder << var.name;
    return klee::ExprVisitor::Action::skipChildren();
  }

  Log::dbg() << expr_to_string(expr) << "\n";
  synthesizer->dbg_vars();

  SYNAPSE_PANIC("TODO");
  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitExtract(const klee::ExtractExpr &e) {
  SYNAPSE_PANIC("TODO");
  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitZExt(const klee::ZExtExpr &e) {
  SYNAPSE_PANIC("TODO");
  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitSExt(const klee::SExtExpr &e) {
  SYNAPSE_PANIC("TODO");
  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitAdd(const klee::AddExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " + ";
  coder << transpile(rhs);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitSub(const klee::SubExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " - ";
  coder << transpile(rhs);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitMul(const klee::MulExpr &e) {
  SYNAPSE_PANIC("TODO");
  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitUDiv(const klee::UDivExpr &e) {
  SYNAPSE_PANIC("TODO");
  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitSDiv(const klee::SDivExpr &e) {
  SYNAPSE_PANIC("TODO");
  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitURem(const klee::URemExpr &e) {
  SYNAPSE_PANIC("TODO");
  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitSRem(const klee::SRemExpr &e) {
  SYNAPSE_PANIC("TODO");
  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitNot(const klee::NotExpr &e) {
  SYNAPSE_PANIC("TODO");
  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitAnd(const klee::AndExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " & ";
  coder << transpile(rhs);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitOr(const klee::OrExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " | ";
  coder << transpile(rhs);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitXor(const klee::XorExpr &e) {
  SYNAPSE_PANIC("TODO");
  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitShl(const klee::ShlExpr &e) {
  SYNAPSE_PANIC("TODO");
  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitLShr(const klee::LShrExpr &e) {
  SYNAPSE_PANIC("TODO");
  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitAShr(const klee::AShrExpr &e) {
  SYNAPSE_PANIC("TODO");
  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitEq(const klee::EqExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  // Kind of a hack, but we need to handle the case where we have a comparison
  // with booleans.
  EPSynthesizer::var_t var;

  klee::ref<klee::Expr> var_expr;
  klee::ref<klee::Expr> const_expr;

  if (is_constant(lhs)) {
    const_expr = lhs;
    var_expr = rhs;
  } else {
    const_expr = rhs;
    var_expr = lhs;
  }

  if (is_constant(const_expr) && synthesizer->get_var(var_expr, var) && var.is_bool) {
    u64 value = solver_toolbox.value_from_expr(const_expr);
    if (value == 0) {
      coder << "!";
    }
    coder << var.name;
    return klee::ExprVisitor::Action::skipChildren();
  }

  coder << transpile(lhs);
  coder << " == ";
  coder << transpile(rhs);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitNe(const klee::NeExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  // Kind of a hack, but we need to handle the case where we have a comparison
  // with booleans.
  EPSynthesizer::var_t var;

  klee::ref<klee::Expr> var_expr;
  klee::ref<klee::Expr> const_expr;

  if (is_constant(lhs)) {
    const_expr = lhs;
    var_expr = rhs;
  } else {
    const_expr = rhs;
    var_expr = lhs;
  }

  if (is_constant(const_expr) && synthesizer->get_var(var_expr, var) && var.is_bool) {
    u64 value = solver_toolbox.value_from_expr(const_expr);
    if (value != 0) {
      coder << "!";
    }
    coder << var.name;
    return klee::ExprVisitor::Action::skipChildren();
  }

  coder << transpile(lhs);
  coder << " != ";
  coder << transpile(rhs);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitUlt(const klee::UltExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " < ";
  coder << transpile(rhs);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitUle(const klee::UleExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " <= ";
  coder << transpile(rhs);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitUgt(const klee::UgtExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " > ";
  coder << transpile(rhs);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitUge(const klee::UgeExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " >= ";
  coder << transpile(rhs);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitSlt(const klee::SltExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " < ";
  coder << transpile(rhs);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitSle(const klee::SleExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " <= ";
  coder << transpile(rhs);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitSgt(const klee::SgtExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " > ";
  coder << transpile(rhs);

  return klee::ExprVisitor::Action::skipChildren();
}

klee::ExprVisitor::Action Transpiler::visitSge(const klee::SgeExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " >= ";
  coder << transpile(rhs);

  return klee::ExprVisitor::Action::skipChildren();
}

} // namespace tofino
} // namespace synapse