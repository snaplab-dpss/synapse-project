#pragma once

#include "../../synthesizer.h"

#include <klee/util/ExprVisitor.h>

namespace synapse {
namespace tofino {

class EPSynthesizer;

class Transpiler : public klee::ExprVisitor::ExprVisitor {
private:
  std::stack<coder_t> coders;
  const EPSynthesizer *synthesizer;

public:
  Transpiler(const EPSynthesizer *synthesizer);

  code_t transpile(klee::ref<klee::Expr> expr);

  static code_t type_from_size(bits_t size);
  static code_t type_from_expr(klee::ref<klee::Expr> expr);
  static code_t transpile_literal(u64 value, bits_t size);

  Action visitNotOptimized(const klee::NotOptimizedExpr &e);
  Action visitRead(const klee::ReadExpr &e);
  Action visitSelect(const klee::SelectExpr &e);
  Action visitConcat(const klee::ConcatExpr &e);
  Action visitExtract(const klee::ExtractExpr &e);
  Action visitZExt(const klee::ZExtExpr &e);
  Action visitSExt(const klee::SExtExpr &e);
  Action visitAdd(const klee::AddExpr &e);
  Action visitSub(const klee::SubExpr &e);
  Action visitMul(const klee::MulExpr &e);
  Action visitUDiv(const klee::UDivExpr &e);
  Action visitSDiv(const klee::SDivExpr &e);
  Action visitURem(const klee::URemExpr &e);
  Action visitSRem(const klee::SRemExpr &e);
  Action visitNot(const klee::NotExpr &e);
  Action visitAnd(const klee::AndExpr &e);
  Action visitOr(const klee::OrExpr &e);
  Action visitXor(const klee::XorExpr &e);
  Action visitShl(const klee::ShlExpr &e);
  Action visitLShr(const klee::LShrExpr &e);
  Action visitAShr(const klee::AShrExpr &e);
  Action visitEq(const klee::EqExpr &e);
  Action visitNe(const klee::NeExpr &e);
  Action visitUlt(const klee::UltExpr &e);
  Action visitUle(const klee::UleExpr &e);
  Action visitUgt(const klee::UgtExpr &e);
  Action visitUge(const klee::UgeExpr &e);
  Action visitSlt(const klee::SltExpr &e);
  Action visitSle(const klee::SleExpr &e);
  Action visitSgt(const klee::SgtExpr &e);
  Action visitSge(const klee::SgeExpr &e);
};

} // namespace tofino
} // namespace synapse