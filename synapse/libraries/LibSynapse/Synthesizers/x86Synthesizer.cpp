#include <LibSynapse/Synthesizers/x86Synthesizer.h>
#include <LibSynapse/ExecutionPlan.h>
#include <klee/util/Ref.h>

namespace LibSynapse {
namespace x86 {

using LibCore::build_expr_mods;
using LibCore::bytes_in_expr;
using LibCore::expr_mod_t;
using LibCore::expr_to_ascii;
using LibCore::expr_to_string;
using LibCore::is_constant;
using LibCore::is_constant_signed;
using LibCore::simplify;
using LibCore::solver_toolbox;

using LibCore::expr_addr_to_obj_addr;

namespace {
constexpr const char *const NF_TEMPLATE_FILENAME       = "x86.nf.template.cpp";
constexpr const char *const PROFILER_TEMPLATE_FILENAME = "x86.profiler.template.cpp";

constexpr const char *const MARKER_NF_STATE   = "NF_STATE";
constexpr const char *const MARKER_NF_INIT    = "NF_INIT";
constexpr const char *const MARKER_NF_PROCESS = "NF_PROCESS";

constexpr const char *const CHUNKS_BORROWED       = "chunks_borrowed";
constexpr const char *const NUM_CHUNKS_BORROWED   = "num_chunks_borrowed";
constexpr const char *const CODE_PATH_HEADER_TYPE = "uint32_t";
constexpr const int CODE_PATH_HEADER_SIZE         = 4;

std::filesystem::path template_from_type(x86SynthesizerTarget target) {
  std::filesystem::path template_file = std::filesystem::path(__FILE__).parent_path() / "Templates";

  switch (target) {
  case x86SynthesizerTarget::NF:
    template_file /= NF_TEMPLATE_FILENAME;
    break;
  case x86SynthesizerTarget::Profiler:
    template_file /= PROFILER_TEMPLATE_FILENAME;
    break;
  }

  return template_file;
}
} // namespace

#define TODO(expr)                                                                                                                                   \
  synthesizer->dbg_vars();                                                                                                                           \
  panic("TODO: %s\n", expr_to_string(expr).c_str());

x86Synthesizer::Transpiler::Transpiler(const x86Synthesizer *_synthesizer) : synthesizer(_synthesizer) {}

code_t x86Synthesizer::Transpiler::transpile(klee::ref<klee::Expr> expr) {
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
bool x86Synthesizer::Transpiler::is_primitive_type(bits_t size) { return size == 1 || size == 8 || size == 16 || size == 32 || size == 64; }

code_t x86Synthesizer::Transpiler::type_from_size(bits_t size) {
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
    panic("Unknown type (size=%u)", size);
  }

  return type;
}

code_t x86Synthesizer::Transpiler::type_from_expr(klee::ref<klee::Expr> expr) { return type_from_size(expr->getWidth()); }

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitNotOptimized(const klee::NotOptimizedExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::NotOptimizedExpr *>(&e);
  TODO(expr);
  return Action::skipChildren();
}

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitRead(const klee::ReadExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ReadExpr *>(&e);

  coder_t &coder = coders.top();

  std::optional<x86Synthesizer::var_t> var = synthesizer->vars.get(expr);

  if (var.has_value()) {
    if (!var.value().addr.isNull()) {
      coder << "*";
    }

    coder << var.value().name;

    return Action::skipChildren();
  }

  std::cerr << expr_to_string(expr) << "\n";
  synthesizer->dbg_vars();

  panic("TODO: visitRead (%s)", expr_to_string(expr).c_str());
  return Action::skipChildren();
}

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitSelect(const klee::SelectExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::SelectExpr *>(&e);
  TODO(expr);
  return Action::skipChildren();
}

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitConcat(const klee::ConcatExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ConcatExpr *>(&e);

  coder_t &coder                           = coders.top();
  std::optional<x86Synthesizer::var_t> var = synthesizer->vars.get(expr);

  if (var.has_value()) {
    if (!var.value().addr.isNull() && expr->getWidth() > 8) {
      coder << "*";

      if (expr->getWidth() <= 64) {
        coder << "(";
        coder << type_from_size(expr->getWidth());
        coder << "*)";
      }
    }

    coder << var.value().name;
    return Action::skipChildren();
  }

  synthesizer->dbg_vars();
  panic("Variable %s not found in stack", expr_to_string(expr, true).c_str());
}

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitExtract(const klee::ExtractExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitZExt(const klee::ZExtExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitSExt(const klee::SExtExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitAdd(const klee::AddExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitSub(const klee::SubExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitMul(const klee::MulExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitUDiv(const klee::UDivExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitSDiv(const klee::SDivExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitURem(const klee::URemExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitSRem(const klee::SRemExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitNot(const klee::NotExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> arg = e.getKid(0);

  coder << "!(";
  coder << transpile(arg);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitAnd(const klee::AndExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitOr(const klee::OrExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitXor(const klee::XorExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::XorExpr *>(&e);
  TODO(expr);
  return Action::skipChildren();
}

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitShl(const klee::ShlExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ShlExpr *>(&e);
  TODO(expr);
  return Action::skipChildren();
}

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitLShr(const klee::LShrExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::LShrExpr *>(&e);
  TODO(expr);
  return Action::skipChildren();
}

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitAShr(const klee::AShrExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::AShrExpr *>(&e);
  TODO(expr);
  return Action::skipChildren();
}

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitEq(const klee::EqExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitNe(const klee::NeExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitUlt(const klee::UltExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitUle(const klee::UleExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitUgt(const klee::UgtExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitUge(const klee::UgeExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitSlt(const klee::SltExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitSle(const klee::SleExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitSgt(const klee::SgtExpr &e) {
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

klee::ExprVisitor::Action x86Synthesizer::Transpiler::visitSge(const klee::SgeExpr &e) {
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

code_t x86Synthesizer::var_t::get_slice(bits_t offset, bits_t slice_size) const {
  assert(offset + slice_size <= expr->getWidth() && "Out of bounds");

  coder_t coder;

  // If the expression has an address, then it's an array of bytes.
  // We need to check if the expression is within the range of the array.
  if (!addr.isNull()) {
    if (slice_size > 8 && slice_size <= 64) {
      coder << "(";
      coder << Transpiler::type_from_size(slice_size);
      coder << "*)";
    }

    coder << "(" + name << "+" << offset / 8 << ")";
    return coder.dump();
  }

  if (offset > 0) {
    coder << "(";
    coder << name;
    coder << ">>";
    coder << offset;
    coder << ") & ";
    coder << ((1 << slice_size) - 1);
  } else {
    coder << name;
    coder << " & ";
    coder << ((1ull << slice_size) - 1);
  }

  return coder.dump();
}

std::string x86Synthesizer::var_t::to_string() const {
  std::stringstream ss;
  ss << "var_t{";
  ss << "name: " << name << ", ";
  ss << "expr: " << expr_to_string(expr, true) << ", ";
  if (!addr.isNull())
    ss << "@" << expr_to_string(addr, true);
  ss << "}";
  return ss.str();
}

void x86Synthesizer::Stack::push(const var_t &var) {
  if (names.find(var.name) == names.end()) {
    frames.push_back(var);
    names.insert(var.name);
  }
}

void x86Synthesizer::Stack::push(const Stack &stack) {
  for (const var_t &var : stack.frames) {
    push(var);
  }
}

void x86Synthesizer::Stack::clear() {
  frames.clear();
  names.clear();
}

std::optional<x86Synthesizer::var_t> x86Synthesizer::Stack::get_exact(klee::ref<klee::Expr> expr) const {
  for (auto var_it = frames.rbegin(); var_it != frames.rend(); var_it++) {
    const var_t &var = *var_it;
    if (solver_toolbox.are_exprs_always_equal(var.expr, expr)) {
      return var;
    }
  }

  return {};
}

std::optional<x86Synthesizer::var_t> x86Synthesizer::Stack::get(klee::ref<klee::Expr> expr) const {
  if (std::optional<var_t> var = get_exact(expr)) {
    return var;
  }

  if (std::optional<var_t> var = get_by_addr(expr)) {
    return var;
  }

  for (auto var_it = frames.rbegin(); var_it != frames.rend(); ++var_it) {
    const var_t &var = *var_it;

    bits_t expr_size = expr->getWidth();
    bits_t var_size  = var.expr->getWidth();

    if (expr_size > var_size) {
      continue;
    }

    for (bits_t offset = 0; offset + expr_size <= var_size; offset += 8) {
      klee::ref<klee::Expr> var_slice = solver_toolbox.exprBuilder->Extract(var.expr, offset, expr_size);

      if (solver_toolbox.are_exprs_always_equal(var_slice, expr)) {
        const var_t out_var = {
            .name = var.get_slice(offset, expr_size),
            .expr = var_slice,
            .addr = var.addr,
        };

        return out_var;
      }
    }
  }
  return {};
}

std::optional<x86Synthesizer::var_t> x86Synthesizer::Stack::get_by_addr(klee::ref<klee::Expr> addr_expr) const {
  for (auto var_it = frames.rbegin(); var_it != frames.rend(); var_it++) {
    const var_t &var = *var_it;
    if (!var.addr.isNull() && solver_toolbox.are_exprs_always_equal(var.addr, addr_expr)) {
      return var;
    }
  }

  return {};
}

const std::vector<x86Synthesizer::var_t> &x86Synthesizer::Stack::get_all() const { return frames; }

std::vector<x86Synthesizer::var_t> &x86Synthesizer::Stack::get_all_mutable() { return frames; }

void x86Synthesizer::Stacks::push() { stacks.emplace_back(); }

void x86Synthesizer::Stacks::pop() { stacks.pop_back(); }

void x86Synthesizer::Stacks::clear() { stacks.clear(); }

void x86Synthesizer::Stacks::insert_front(const var_t &var) { stacks.front().push(var); }
void x86Synthesizer::Stacks::insert_front(const Stack &stack) { stacks.front().push(stack); }

void x86Synthesizer::Stacks::insert_back(const var_t &var) { stacks.back().push(var); }
void x86Synthesizer::Stacks::insert_back(const Stack &stack) { stacks.back().push(stack); }

x86Synthesizer::Stack x86Synthesizer::Stacks::squash() const {
  Stack squashed;
  for (const Stack &stack : stacks) {
    squashed.push(stack);
  }
  return squashed;
}

std::optional<x86Synthesizer::var_t> x86Synthesizer::Stacks::get(klee::ref<klee::Expr> expr) const {
  for (auto stack_it = stacks.rbegin(); stack_it != stacks.rend(); stack_it++) {
    if (std::optional<var_t> var = stack_it->get_exact(expr)) {
      return var;
    }
  }

  for (auto stack_it = stacks.rbegin(); stack_it != stacks.rend(); stack_it++) {
    if (std::optional<var_t> var = stack_it->get(expr)) {
      return var;
    }
  }

  for (auto stack_it = stacks.rbegin(); stack_it != stacks.rend(); stack_it++) {
    if (std::optional<var_t> var = stack_it->get_by_addr(expr)) {
      return var;
    }
  }

  return {};
}

std::optional<x86Synthesizer::var_t> x86Synthesizer::Stacks::get_by_addr(klee::ref<klee::Expr> addr) const {
  for (auto stack_it = stacks.rbegin(); stack_it != stacks.rend(); stack_it++) {
    if (std::optional<var_t> var = stack_it->get_by_addr(addr)) {
      return var;
    }
  }

  return {};
}

const std::vector<x86Synthesizer::Stack> &x86Synthesizer::Stacks::get_all() const { return stacks; }

void x86Synthesizer::Stacks::replace(const var_t &var, klee::ref<klee::Expr> new_expr) {
  for (auto it = stacks.rbegin(); it != stacks.rend(); ++it) {
    Stack &stack = *it;
    for (x86Synthesizer::var_t &v : stack.get_all_mutable()) {
      if (v.name == var.name) {
        klee::ref<klee::Expr> old_expr = v.expr;
        assert(old_expr->getWidth() == new_expr->getWidth() && "Width mismatch");
        v.expr = new_expr;
        return;
      }
    }
  }

  panic("Variable not found in stack: %s\nExpr: %s\n", var.name.c_str(), expr_to_string(new_expr).c_str());
}
/*x86Synthesizer::var_t x86Synthesizer::Stacks::get(klee::ref<klee::Expr> expr) const {
  x86Synthesizer::var_t var;
  if (find(expr, var)) {
    return var;
  }
  dbg();
  panic("Variable not found in stack: %s\n", expr_to_string(expr).c_str());
}

bool x86Synthesizer::Stacks::find(klee::ref<klee::Expr> expr, x86Synthesizer::var_t &out_var) const {
  for (auto it = stacks.rbegin(); it != stacks.rend(); ++it) {
    const Stack &stack = *it;

    for (const var_t &v : stack.get_vars()) {
      if (solver_toolbox.are_exprs_always_equal(v.expr, expr) || solver_toolbox.are_exprs_always_equal(v.addr, expr)) {
        out_var = v;
        return true;
      }

      klee::Expr::Width expr_bits = expr->getWidth();
      klee::Expr::Width var_bits  = v.expr->getWidth();

      if (expr_bits > var_bits) {
        continue;
      }

      for (bits_t offset = 0; offset <= var_bits - expr_bits; offset += 8) {
        klee::ref<klee::Expr> var_slice = solver_toolbox.exprBuilder->Extract(v.expr, offset, expr_bits);

        if (solver_toolbox.are_exprs_always_equal(var_slice, expr)) {
          out_var      = v;
          out_var.name = v.slice_var(offset, expr_bits);
          out_var.expr = var_slice;
          return true;
        }
      }
    }
  }

  return false;
}*/

bool x86Synthesizer::find_or_create_tmp_slice_var(klee::ref<klee::Expr> expr, coder_t &coder, var_t &out_var) {
  for (auto it = vars.get_all().rbegin(); it != vars.get_all().rend(); ++it) {
    const Stack &stack = *it;

    for (const var_t &v : stack.get_all()) {
      if (solver_toolbox.are_exprs_always_equal(v.expr, expr) || solver_toolbox.are_exprs_always_equal(v.addr, expr)) {
        out_var = v;
        return true;
      }

      klee::Expr::Width expr_bits = expr->getWidth();
      klee::Expr::Width var_bits  = v.expr->getWidth();

      if (expr_bits > var_bits) {
        continue;
      }

      for (bits_t offset = 0; offset <= var_bits - expr_bits; offset += 8) {
        klee::ref<klee::Expr> var_slice = solver_toolbox.exprBuilder->Extract(v.expr, offset, expr_bits);

        if (solver_toolbox.are_exprs_always_equal(var_slice, expr)) {
          out_var = build_var(v.name + "_slice", var_slice);

          if (expr_bits <= 64) {
            coder.indent();
            coder << transpiler.type_from_expr(out_var.expr) << " " << out_var.name;
            coder << " = ";
            if (!v.addr.isNull() && out_var.addr.isNull()) {
              coder << "*";
            }
            coder << v.get_slice(offset, expr_bits);
            coder << ";\n";
          } else {
            coder.indent();
            coder << "uint8_t " << out_var.name << "[" << expr_bits / 8 << "];\n";

            if (!v.addr.isNull()) {
              coder.indent();
              coder << "memcpy(";
              coder << "(void*)" << out_var.name << ", ";
              coder << "(void*)" << v.get_slice(offset, expr_bits);
              coder << ", ";
              coder << expr_bits / 8;
              coder << ");\n";
            } else {
              for (bytes_t b = 0; b < expr_bits / 8; b++) {
                klee::ref<klee::Expr> byte = solver_toolbox.exprBuilder->Extract(v.expr, offset + b * 8, 8);
                coder.indent();
                coder << out_var.name << "[" << b << "] = ";
                coder << transpiler.transpile(byte);
                coder << ";\n";
              }
            }
          }

          vars.insert_back(out_var);
          return true;
        }
      }
    }
  }

  return false;
}

x86Synthesizer::var_t x86Synthesizer::build_var(const std::string &name, klee::ref<klee::Expr> expr) { return build_var(name, expr, nullptr); }

x86Synthesizer::var_t x86Synthesizer::build_var(const std::string &name, klee::ref<klee::Expr> expr, klee::ref<klee::Expr> addr) {

  return var_t(create_unique_name(name), expr, addr);
}

x86Synthesizer::var_t x86Synthesizer::build_var_ptr(const std::string &base_name, klee::ref<klee::Expr> addr_expr, klee::ref<klee::Expr> value,
                                                    coder_t &coder, bool &found_in_stack) {

  bytes_t size = value->getWidth() / 8;

  std::optional<var_t> var = vars.get(addr_expr);

  if (!(found_in_stack = var.has_value())) {
    var = build_var(base_name, value, addr_expr);
    coder.indent();
    coder << "uint8_t " << var.value().name << "[" << size << "];\n";
  } else if (solver_toolbox.are_exprs_always_equal(var.value().expr, value)) {
    return var.value();
  }

  var_t stack_value;
  if (find_or_create_tmp_slice_var(value, coder, stack_value)) {
    const bits_t width = stack_value.expr->getWidth();
    if (Transpiler::is_primitive_type(width)) {
      coder.indent();
      coder << "*(" << Transpiler::type_from_size(width) << "*)";
      coder << var.value().name;
      coder << " = ";
      coder << stack_value.name;
      coder << ";\n";
    } else {
      coder.indent();
      coder << "memcpy(";
      coder << "(void*)" << var.value().name << ", ";
      coder << "(void*)" << stack_value.name << ", ";
      coder << size;
      coder << ");\n";
    }

    var.value().expr = stack_value.expr;
  } else {
    for (bytes_t b = 0; b < size; b++) {
      klee::ref<klee::Expr> byte = solver_toolbox.exprBuilder->Extract(var.value().expr, b * 8, 8);
      coder.indent();
      coder << var.value().name << "[" << b << "] = ";
      coder << transpiler.transpile(byte);
      coder << ";\n";
    }
  }

  return var.value();
}

code_t x86Synthesizer::create_unique_name(const std::string &base_name) {
  if (reserved_var_names.find(base_name) == reserved_var_names.end()) {
    reserved_var_names[base_name] = 0;
  }

  int &counter = reserved_var_names.at(base_name);

  coder_t coder;
  coder << base_name << "_" << counter;

  counter++;

  return coder.dump();
}

x86Synthesizer::x86Synthesizer(const EP *ep, x86SynthesizerTarget _target, std::filesystem::path _out_file, const InstanceId _instance_id)
    : out_file(_out_file), code_template(template_from_type(_target),
                                         {

                                             {MARKER_NF_STATE, 0},
                                             {MARKER_NF_INIT, 0},
                                             {MARKER_NF_PROCESS, 0},
                                         }),
      target_ep(ep), target(_target), transpiler(this), instance_id(_instance_id) {}

coder_t &x86Synthesizer::get_current_coder() { return in_nf_init ? get(MARKER_NF_INIT) : get(MARKER_NF_PROCESS); }

coder_t &x86Synthesizer::get(const std::string &marker) { return code_template.get(marker); }

void x86Synthesizer::synthesize() {
  std::cerr << "================= Synthesize NF INIT ================= \n";
  synthesize_nf_init();

  std::cerr << "================ Synthesize NF PROCESS ================ \n";
  change_to_process_coder();
  synthesize_nf_process();

  std::cerr << "========== Synthesize NF INIT POST PROCESS ============ \n";
  change_to_nf_init_coder();
  synthesize_nf_init_post_process();

  std::ofstream ofs(out_file);
  ofs << code_template.dump();
  ofs.close();
}

void x86Synthesizer::visit(const EP *ep) {
  const TargetType synthesizer_target = TargetType(TargetArchitecture::x86, instance_id);

  for (const bdd_node_id_t &id : ep->get_target_roots(synthesizer_target)) {
    const BDDNode *bdd_node = ep->get_bdd()->get_node_by_id(id);

    if (bdd_node == ep->get_bdd()->get_root()) {
      continue;
    }

    const EPNode *ep_node = ep->get_ep_node_from_bdd_node(bdd_node);

    if (ep_node) {
      visit(ep, ep_node);
    }
  }
}

void x86Synthesizer::visit(const EP *ep, const EPNode *ep_node) {
  coder_t &coder = get_current_coder();

  const Module *module = ep_node->get_module();

  const TargetType synthesizer_target = TargetType(TargetArchitecture::x86, instance_id);
  const TargetType current_target     = module->get_target();
  // const TargetType next_target        = module->get_next_target();

  if (synthesizer_target == current_target) {
    EPVisitor::log(ep_node);
    coder.indent();
    coder << "// EP node  " << ep_node->get_id() << ":" << module->get_name() << "\n";

    coder.indent();
    coder << "// BDD node " << module->get_node()->dump(true) << "\n";

    const EPVisitor::Action action = module->visit(*this, ep, ep_node);

    if (action == EPVisitor::Action::skipChildren) {
      return;
    }
  }

  for (const EPNode *child : ep_node->get_children()) {
    visit(ep, child);
  }
}

void x86Synthesizer::synthesize_nf_init() {
  coder_t &coder     = get_current_coder();
  const BDD *bdd     = target_ep->get_bdd();
  const Context &ctx = target_ep->get_ctx();

  coder << "bool nf_init() {\n";
  coder.inc();

  x86Target x86_target(instance_id);
  for (const Call *call_node : bdd->get_init()) {
    std::vector<std::unique_ptr<Module>> candidate_modules;
    for (const std::unique_ptr<ModuleFactory> &factory : x86_target.module_factories) {
      std::unique_ptr<Module> module = factory->create(bdd, target_ep->get_ctx(), call_node);
      if (module) {
        candidate_modules.push_back(std::move(module));
      }
    }

    if (candidate_modules.empty()) {
      ctx.debug();
      panic("No module found for call node %s", call_node->dump(true).c_str());
    } else if (candidate_modules.size() > 1) {
      std::stringstream err_msg;
      err_msg << "Multiple init module candidates for call node " << call_node->dump(true);
      err_msg << "\n";
      err_msg << "Candidates = [";
      for (size_t i = 0; i < candidate_modules.size(); i++) {
        err_msg << candidate_modules[i]->get_name();
        if (i + 1 < candidate_modules.size()) {
          err_msg << ", ";
        }
      }
      err_msg << "]";
      panic("%s", err_msg.str().c_str());
    }

    const Module *module = candidate_modules.front().get();

    coder.indent();
    coder << "// BDD node " << call_node->dump(true) << "\n";
    coder.indent();
    coder << "// Module " << module->get_name() << "\n";

    module->visit(*this, target_ep, nullptr);
  }
  if (target == x86SynthesizerTarget::Profiler) {
    for (u16 device : bdd->get_devices()) {
      coder.indent();
      coder << "ports.push_back(" << device << ");\n";
    }
  }
}

void x86Synthesizer::synthesize_nf_init_post_process() {
  coder_t &coder = get_current_coder();

  if (target == x86SynthesizerTarget::Profiler) {
    for (const auto &[node_id, map_addr] : nodes_to_map) {
      coder.indent();
      coder << "stats_per_map[" << transpiler.transpile(map_addr) << "]";
      coder << ".init(";
      coder << node_id;
      coder << ")";
      coder << ";\n";
    }

    for (bdd_node_id_t node_id : route_nodes) {
      coder.indent();
      coder << "forwarding_stats_per_route_op.insert({" << node_id << ", PortStats{}});\n";
    }

    for (bdd_node_id_t node_id : process_nodes) {
      coder.indent();
      coder << "node_pkt_counter.insert({" << node_id << ", 0});\n";
    }
  }

  coder.indent();
  coder << "return true;\n";

  coder.dec();
  coder << "}\n";
}

void x86Synthesizer::synthesize_nf_process() {
  coder_t &coder = get_current_coder();
  const BDD *bdd = target_ep->get_bdd();

  symbol_t device = bdd->get_device();
  symbol_t len    = bdd->get_packet_len();
  symbol_t now    = bdd->get_time();

  var_t device_var = build_var("device", device.expr);
  var_t len_var    = build_var("len", len.expr);
  var_t now_var    = build_var("now", now.expr);

  coder << "int nf_process(";
  coder << "uint16_t " << device_var.name << ", ";
  coder << "uint8_t *buffer, ";
  coder << "uint16_t " << len_var.name << ", ";
  coder << "time_ns_t " << now_var.name << ", ";
  coder << "struct rte_mbuf *mbuf";
  coder << ") {\n";

  coder.inc();

  coder.indent();
  coder << "debug_packet_info(";
  coder << device_var.name;
  coder << ", ";
  coder << "buffer";
  coder << ", ";
  coder << len_var.name;
  coder << ", ";
  coder << now_var.name;
  coder << ", ";
  coder << "mbuf";
  coder << ");\n";

  code_t trash = create_unique_name("trash");

  coder.indent();
  coder << "void* ";
  coder << trash;
  coder << ";\n";

  coder.indent();
  coder << "packet_borrow_next_chunk(";
  coder << "buffer, ";
  coder << "50, ";
  coder << "&" << trash;
  coder << ")";
  coder << ";\n";

  code_t code_path = create_unique_name("code_path");

  coder.indent();
  coder << CODE_PATH_HEADER_TYPE;
  coder << " ";
  coder << code_path;
  coder << " = ";
  coder << "0";
  coder << ";\n";

  coder.indent();
  coder << "if (packet_get_unread_length(buffer) >= sizeof(";
  coder << CODE_PATH_HEADER_TYPE;
  coder << "))";
  coder << "{\n";
  coder.inc();

  code_t code_path_hdr = create_unique_name("code_path_hdr");

  coder.indent();
  coder << "void *" << code_path_hdr << ";\n";

  coder.indent();
  coder << "packet_borrow_next_chunk(";
  coder << "buffer, ";
  coder << "sizeof(";
  coder << CODE_PATH_HEADER_TYPE;
  coder << "), ";
  coder << "&" << code_path_hdr;
  coder << ")";
  coder << ";\n";

  for (int i = 0; i < CODE_PATH_HEADER_SIZE; i++) {
    coder.indent();
    coder << code_path;
    coder << " += ";
    coder << "((uint8_t *) ";
    coder << code_path_hdr;
    coder << ")[";
    coder << std::to_string(i);
    coder << "]";
    coder << " << ";
    coder << std::to_string(8 * i);
    coder << ";\n";
  }

  coder.dec();
  coder.indent();
  coder << "}\n";

  coder.indent();
  coder << "packet_return_all_chunks(buffer);\n";

  coder.indent();
  coder << "std::cerr << \"CODE PATH: \" << ";
  coder << code_path;
  coder << " << \";\\n\";\n";

  coder.indent();
  coder << "if (" << code_path << " == 0) {\n";
  code_paths.push_back(0);
  coder.inc();

  vars.push();

  vars.insert_back(device_var);
  vars.insert_back(len_var);
  vars.insert_back(now_var);

  const BDDNode *root_node = target_ep->get_bdd()->get_root();

  const TargetType synthesizer_target = TargetType(TargetArchitecture::x86, instance_id);
  const bdd_node_ids_t root_nodes     = target_ep->get_target_roots(synthesizer_target);

  if (root_nodes.find(root_node->get_id()) != root_nodes.end()) {
    const EPNode *ep_node = target_ep->get_ep_node_from_bdd_node(root_node);

    coder.indent();
    coder << "std::cerr << \"Received Packet From Outside Network\\n\";\n";

    if (ep_node) {
      visit(target_ep, ep_node);
    }
  } else {
    coder.indent();
    coder << "return DROP;\n";
  }

  coder.dec();
  coder.indent();
  coder << "}\n";

  visit(target_ep);

  coder.indent();
  coder << "else {\n";
  coder.inc();
  coder.indent();
  coder << "return DROP;\n";
  coder.dec();
  coder.indent();
  coder << "}\n";

  coder.dec();
  coder << "}\n";
}

/*void x86Synthesizer::synthesize_nf_process() {
  coder_t &coder = get_current_coder();
  const BDD *bdd = target_ep->get_bdd();

  symbol_t device = bdd->get_device();
  symbol_t len    = bdd->get_packet_len();
  symbol_t now    = bdd->get_time();

  var_t device_var = build_var("device", device.expr);
  var_t len_var    = build_var("len", len.expr);
  var_t now_var    = build_var("now", now.expr);

  coder << "int nf_process(";
  coder << "uint16_t " << device_var.name << ", ";
  coder << "uint8_t *buffer, ";
  coder << "uint16_t " << len_var.name << ", ";
  coder << "time_ns_t " << now_var.name << ", ";
  coder << "struct rte_mbuf *mbuf";
  coder << ") {\n";

  coder.inc();
  vars.push();

  vars.insert_back(device_var);
  vars.insert_back(len_var);
  vars.insert_back(now_var);

  EPVisitor::visit(target_ep);
  coder.dec();
  coder << "}\n";
}*/

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::Ignore *node) { return EPVisitor::Action::doChildren; }

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::If *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> condition = node->get_condition();
  const std::vector<EPNode *> &children = ep_node->get_children();
  assert(children.size() == 2 && "If node must have exactly two children");

  const EPNode *then_node = children[0];
  const EPNode *else_node = children[1];

  coder.indent();
  coder << "if (";
  coder << transpiler.transpile(condition);
  coder << ") {\n";

  coder.inc();
  vars.push();
  visit(ep, then_node);
  vars.pop();
  coder.dec();

  coder.indent();
  coder << "} else {\n";

  coder.inc();
  vars.push();
  visit(ep, else_node);
  vars.pop();
  coder.dec();

  coder.indent();
  coder << "}";
  coder << " ";
  coder << "// ";
  coder << transpiler.transpile(condition);

  coder << "\n";

  return EPVisitor::Action::skipChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::Then *node) { return EPVisitor::Action::doChildren; }

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::Else *node) { return EPVisitor::Action::doChildren; }

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::Forward *node) {
  coder_t &coder                         = get(MARKER_NF_PROCESS);
  const klee::ref<klee::Expr> dst_device = node->get_dst_device();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  route_nodes.insert(node->get_node()->get_id());
  if (target == x86SynthesizerTarget::Profiler) {

    const BDDNode *future_node = node->get_node();
    coder.indent();
    coder << "forwarding_stats_per_route_op[" << future_node->get_id() << "].inc_fwd(";
    coder << transpiler.transpile(dst_device);
    coder << ");\n";
  }

  coder.indent();
  coder << "return " << transpiler.transpile(dst_device) << ";\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::Broadcast *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  route_nodes.insert(node->get_node()->get_id());
  if (target == x86SynthesizerTarget::Profiler) {
    const BDDNode *future_node = node->get_node();
    coder.indent();
    coder << "forwarding_stats_per_route_op[" << future_node->get_id() << "].inc_flood();\n";
  }

  coder.indent();
  coder << "return FLOOD;\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::Drop *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  route_nodes.insert(node->get_node()->get_id());
  if (target == x86SynthesizerTarget::Profiler) {
    const BDDNode *future_node = node->get_node();
    coder.indent();
    coder << "forwarding_stats_per_route_op[" << future_node->get_id() << "].inc_drop();\n";
  }

  coder.indent();
  coder << "return DROP;\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::ParseHeader *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> chunk_addr = node->get_chunk_addr();
  const klee::ref<klee::Expr> chunk      = node->get_chunk();
  const klee::ref<klee::Expr> length     = node->get_length();

  var_t hdr = build_var("hdr", chunk, chunk_addr);

  coder.indent();
  coder << "uint8_t* " << hdr.name << ";\n";

  coder.indent();
  coder << "packet_borrow_next_chunk(";
  coder << "buffer, ";
  coder << transpiler.transpile(length) << ", ";
  coder << "(void**)&" << hdr.name;
  coder << ")";
  coder << ";\n";

  vars.insert_back(hdr);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::ModifyHeader *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> chunk_addr_expr = node->get_chunk_addr_expr();
  const std::vector<expr_mod_t> changes       = node->get_changes();

  std::optional<x86Synthesizer::var_t> hdr = vars.get(chunk_addr_expr);
  assert(hdr.has_value() && "Header not found");

  for (const expr_mod_t &mod : changes) {
    std::vector<klee::ref<klee::Expr>> bytes = bytes_in_expr(mod.expr);
    for (size_t i = 0; i < bytes.size(); i++) {
      coder.indent();
      coder << hdr.value().name;
      coder << "[";
      coder << (mod.offset / 8) + i;
      coder << "] = ";
      coder << transpiler.transpile(bytes[i]);
      coder << ";\n";
    }
  }
  coder.indent();
  coder << "packet_return_chunk(";
  coder << "buffer, ";
  coder << hdr.value().name;
  coder << ")";
  coder << ";\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::MapAllocate *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> capacity = node->get_capacity();
  const klee::ref<klee::Expr> key_size = node->get_key_size();
  const klee::ref<klee::Expr> map_out  = node->get_map_out();
  const symbol_t success               = node->get_success();

  var_t map_out_var = build_var("map", map_out);
  var_t success_var = build_var("map_allocation_succeeded", success.expr);

  coder_t &coder_nf_state = get(MARKER_NF_STATE);
  coder_nf_state.indent();
  coder_nf_state << "struct KeyVecMap *";
  coder_nf_state << map_out_var.name;
  coder_nf_state << ";\n";

  coder.indent();
  coder << "int " << success_var.name << " = ";
  coder << "key_vec_map_allocate(";
  coder << transpiler.transpile(capacity) << ", ";
  coder << transpiler.transpile(key_size) << ", ";
  coder << "&" << map_out_var.name;
  coder << ");\n";

  coder.indent();
  coder << "if (!";
  coder << success_var.name;
  coder << ") {\n";

  coder.inc();
  coder.indent();
  coder << "return false;\n";

  coder.dec();
  coder.indent();
  coder << "}\n";

  vars.insert_back(map_out_var);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::MapGet *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init) {
    process_nodes.insert(node->get_node()->get_id());
  }

  const klee::ref<klee::Expr> map_addr  = node->get_map_addr_expr();
  const klee::ref<klee::Expr> key_addr  = node->get_key_addr_expr();
  const klee::ref<klee::Expr> key       = node->get_key();
  const klee::ref<klee::Expr> value_out = node->get_value_out();
  const klee::ref<klee::Expr> success   = node->get_success();
  const symbol_t &map_has_this_key      = node->get_map_has_this_key();

  std::optional<var_t> map_var = vars.get(map_addr);
  assert(map_var.has_value() && "Map not found");

  var_t r = build_var("map_hit", map_has_this_key.expr);
  var_t v = build_var("value", value_out);

  bool key_in_stack;
  var_t k = build_var_ptr("key", key_addr, key, coder, key_in_stack);

  coder.indent();
  coder << "int " << v.name << ";\n";

  coder.indent();
  coder << "int " << r.name << " = ";
  coder << "key_vec_map_get(";
  coder << map_var.value().name << ", ";
  coder << k.name << ", ";
  coder << "&" << v.name;
  coder << ")";
  coder << ";\n";

  if (target == x86SynthesizerTarget::Profiler) {
    const BDDNode *bdd_node = ep_node->get_module()->get_node();
    nodes_to_map.insert({bdd_node->get_id(), map_addr});
    coder.indent();
    coder << "stats_per_map[" << transpiler.transpile(map_addr) << "]";
    coder << ".update(";
    coder << bdd_node->get_id() << ", ";
    coder << k.name << ", ";
    coder << key->getWidth() / 8 << ", ";
    coder << "now"; // FIXME: we should get this from the stack
    coder << ")";
    coder << ";\n";
  }

  vars.insert_back(r);
  vars.insert_back(v);

  if (!key_in_stack) {
    vars.insert_back(k);
  } else {
    vars.replace(k, key);
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::ExpireItemsSingleMap *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> dchain_addr = node->get_dchain_addr();
  // const klee::ref<klee::Expr> vector_addr   = node->get_vector_addr();
  const klee::ref<klee::Expr> map_addr      = node->get_map_addr();
  const klee::ref<klee::Expr> time          = node->get_time();
  const klee::ref<klee::Expr> n_freed_flows = node->get_n_freed_flows();

  std::optional<var_t> dchain_var = vars.get(dchain_addr);
  if (!dchain_var.has_value()) {
    std::cerr << expr_addr_to_obj_addr(dchain_addr);
    dbg_vars();
    assert(dchain_var.has_value() && "Dchain not found");
  }

  /*std::optional<var_t> vector_var = vars.get(vector_addr);
  if (!vector_var.has_value()) {
    std::cerr << expr_addr_to_obj_addr(vector_addr) << "\n";
    dbg_vars();
    assert(vector_var.has_value() && "Vector not found");
  }*/

  std::optional<var_t> map_var = vars.get(map_addr);
  if (!map_var.has_value()) {
    std::cerr << expr_to_string(map_addr, true);
    dbg_vars();
    assert(map_var.has_value() && "Map not found");
  }
  var_t nfreed = build_var("freed_flows", n_freed_flows);

  coder.indent();
  coder << "int " << nfreed.name << " = ";
  if (target == x86SynthesizerTarget::Profiler) {
    panic("TODO-ExpireItemsSingleMap_Profiler: implement key_vec_map_profiler_expire_items_single_map");
    /*
    coder << "profiler_expire_items_single_map(";
    coder << dchain_var.value().name << ", ";
    coder << vector_var.value().name << ", ";
    coder << map_var.value().name << ", ";
    coder << "now"; // FIXME: we should get this from the stack
    coder << ")";
    coder << ";\n";
    coder.indent();
    coder << "expiration_tracker.update(" << nfreed.name << ", now);\n";
    */
  } else {
    coder << "key_vec_map_expire_items_single_map(";
    coder << dchain_var.value().name << ", ";
    coder << map_var.value().name << ", ";
    coder << transpiler.transpile(time);
    coder << ")";
    coder << ";\n";
  }

  vars.insert_back(nfreed);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::ExpireItemsSingleMapIteratively *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> vector_addr   = node->get_vector_addr();
  const klee::ref<klee::Expr> map_addr      = node->get_map_addr();
  const klee::ref<klee::Expr> start         = node->get_start();
  const klee::ref<klee::Expr> n_elems       = node->get_n_elems();
  const klee::ref<klee::Expr> n_freed_flows = node->get_n_freed_flows();

  /*std::optional<var_t> vector_var = vars.get(vector_addr);
  assert(vector_var.has_value() && "Vector not found");*/

  std::optional<var_t> map_var = vars.get(map_addr);
  assert(map_var.has_value() && "Map not found");

  var_t nfreed = build_var("freed_flows", n_freed_flows);

  coder.indent();
  coder << "int " << nfreed.name << " = ";
  coder << "key_vec_map_expire_items_single_map_iteratively(";
  coder << map_var.value().name << ", ";
  coder << transpiler.transpile(start) << ", ";
  coder << transpiler.transpile(n_elems);
  coder << ")";
  coder << ";\n";

  vars.insert_back(nfreed);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::DchainRejuvenateIndex *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> dchain_addr = node->get_dchain_addr();
  const klee::ref<klee::Expr> index       = node->get_index();
  const klee::ref<klee::Expr> time        = node->get_time();

  std::optional<var_t> dchain_var = vars.get(dchain_addr);
  assert(dchain_var.has_value() && "Dchain not found");

  coder.indent();
  coder << "dchain_rejuvenate_index(";
  coder << dchain_var.value().name << ", ";
  coder << transpiler.transpile(index) << ", ";
  coder << transpiler.transpile(time);
  coder << ")";
  coder << ";\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::VectorAllocate *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  klee::ref<klee::Expr> elem_size  = node->get_elem_size();
  klee::ref<klee::Expr> capacity   = node->get_capacity();
  klee::ref<klee::Expr> vector_out = node->get_vector_out();
  symbol_t success                 = node->get_success();

  var_t vector_out_var = build_var("vector", vector_out);
  var_t success_var    = build_var("vector_alloc_success", success.expr);

  coder_t &coder_nf_state = get(MARKER_NF_STATE);
  coder_nf_state.indent();
  coder_nf_state << "struct Vector *";
  coder_nf_state << vector_out_var.name;
  coder_nf_state << ";\n";

  coder.indent();
  coder << "int " << success_var.name << " = ";
  coder << "vector_allocate(";
  coder << transpiler.transpile(elem_size) << ", ";
  coder << transpiler.transpile(capacity) << ", ";
  coder << "&" << vector_out_var.name;
  coder << ");\n";

  coder.indent();
  coder << "if (!";
  coder << success_var.name;
  coder << ") {\n";

  coder.inc();
  coder.indent();
  coder << "return false;\n";

  coder.dec();
  coder.indent();
  coder << "}\n";

  vars.insert_back(vector_out_var);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::VectorRead *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> vector_addr_expr = node->get_vector_addr_expr();
  const klee::ref<klee::Expr> index            = node->get_index();
  const klee::ref<klee::Expr> value_addr       = node->get_value_addr();
  const klee::ref<klee::Expr> value            = node->get_value();

  var_t v = build_var("vector_value_out", value, value_addr);

  std::optional<var_t> vector_var = vars.get(vector_addr_expr);
  if (!vector_var.has_value()) {
    std::cerr << expr_addr_to_obj_addr(vector_addr_expr) << "\n";
    dbg_vars();
    assert(vector_var.has_value() && "Vector not found in stack");
  }

  coder.indent();
  coder << "uint8_t* " << v.name << " = 0;\n";

  coder.indent();
  coder << "vector_borrow(";
  coder << vector_var.value().name << ", ";
  coder << transpiler.transpile(index) << ", ";
  coder << "(void**)&" << v.name;
  coder << ")";
  coder << ";\n";

  vars.insert_back(v);
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::VectorWrite *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> value      = node->get_value();
  const klee::ref<klee::Expr> value_addr = node->get_value_addr();
  const std::vector<expr_mod_t> &changes = node->get_modifications();

  // Find the variable in the stack by address
  std::optional<var_t> v_opt = vars.get(value_addr);
  assert(v_opt.has_value() && "Vector cell not found in stack");
  var_t v = v_opt.value();

  var_t new_v;
  if (value->getKind() != klee::Expr::Constant && find_or_create_tmp_slice_var(value, coder, new_v)) {
    coder.indent();
    coder << "memcpy(";
    coder << "(void*)" << v.name << ", ";
    coder << "(void*)";
    if (new_v.addr.isNull()) {
      coder << "&";
    }
    coder << new_v.name << ", ";
    coder << value->getWidth() / 8;
    coder << ");\n";
    return {};
  }

  // For each modification, emit code to update the vector cell
  for (const expr_mod_t &mod : changes) {
    coder.indent();
    assert(mod.width <= 64 && "Vector element size is too large");
    if (mod.width == 8) {
      coder << v.name;
      coder << "[";
      coder << mod.offset / 8;
      coder << "]";
    } else {
      coder << "*(" << Transpiler::type_from_size(mod.width) << "*)";
      coder << v.name;
    }

    coder << " = ";
    coder << transpiler.transpile(mod.expr);
    coder << ";\n";
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::DchainAllocate *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  klee::ref<klee::Expr> index_range = node->get_index_range();
  klee::ref<klee::Expr> chain_out   = node->get_chain_out();
  symbol_t success                  = node->get_success();

  var_t chain_out_var = build_var("dchain", chain_out);
  var_t success_var   = build_var("is_dchain_allocated", success.expr);

  coder_t &coder_nf_state = get(MARKER_NF_STATE);
  coder_nf_state.indent();
  coder_nf_state << "struct DoubleChain *";
  coder_nf_state << chain_out_var.name;
  coder_nf_state << ";\n";

  coder.indent();
  coder << "int " << success_var.name << " = ";
  coder << "dchain_allocate(";
  coder << transpiler.transpile(index_range) << ", ";
  coder << "&" << chain_out_var.name;
  coder << ");\n";

  coder.indent();
  coder << "if (!";
  coder << success_var.name;
  coder << ") {\n";

  coder.inc();
  coder.indent();
  coder << "return false;\n";

  coder.dec();
  coder.indent();
  coder << "}\n";

  vars.insert_back(chain_out_var);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::DchainAllocateNewIndex *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> dchain_addr        = node->get_dchain_addr();
  const klee::ref<klee::Expr> time               = node->get_time();
  const klee::ref<klee::Expr> index_out          = node->get_index_out();
  const std::optional<symbol_t> not_out_of_space = node->get_not_out_of_space();

  var_t noos = build_var("not_out_of_space", not_out_of_space.value().expr);
  var_t i    = build_var("index", index_out);

  std::optional<var_t> dchain_var = vars.get(dchain_addr);
  assert(dchain_var.has_value() && "Dchain not found");

  coder.indent();
  coder << "int " << i.name << ";\n";

  coder.indent();
  coder << "int " << noos.name << " = ";
  coder << "dchain_allocate_new_index(";
  coder << dchain_var.value().name << ", ";
  coder << "&" << i.name << ", ";
  coder << transpiler.transpile(time);
  coder << ")";
  coder << ";\n";

  vars.insert_back(noos);
  vars.insert_back(i);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::MapPut *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> map_addr = node->get_map_addr();
  const klee::ref<klee::Expr> key_addr = node->get_key_addr();
  const klee::ref<klee::Expr> key      = node->get_key();
  const klee::ref<klee::Expr> value    = node->get_value(); // dchain_index

  std::optional<var_t> map_var = vars.get(map_addr);
  assert(map_var.has_value() && "Map not found");

  bool key_in_stack;
  var_t k = build_var_ptr("key", key_addr, key, coder, key_in_stack);

  coder.indent();
  coder << "key_vec_map_put(";
  coder << map_var.value().name << ", ";
  coder << k.name << ", ";
  coder << transpiler.transpile(value);
  coder << ")";
  coder << ";\n";

  if (target == x86SynthesizerTarget::Profiler) {
    const BDDNode *bdd_node = ep_node->get_module()->get_node();
    nodes_to_map.insert({bdd_node->get_id(), map_addr});
    coder.indent();
    coder << "stats_per_map[" << transpiler.transpile(map_addr) << "]";
    coder << ".update(";
    coder << bdd_node->get_id() << ", ";
    coder << k.name << ", ";
    coder << key->getWidth() / 8 << ", ";
    coder << "now";
    coder << ")";
    coder << ";\n";
  }

  if (!key_in_stack) {
    vars.insert_back(k);
  } else {
    vars.replace(k, key);
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::ChecksumUpdate *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> ip_hdr_addr = node->get_ip_hdr_addr();
  const klee::ref<klee::Expr> l4_hdr_addr = node->get_l4_hdr_addr();
  const symbol_t checksum                 = node->get_checksum();

  std::optional<var_t> ip_hdr_var = vars.get(ip_hdr_addr);
  assert(ip_hdr_var.has_value() && "IP_HDR not found");

  std::optional<var_t> l4_hdr_var = vars.get(l4_hdr_addr);
  assert(l4_hdr_var.has_value() && "Dchain not found");

  var_t c = build_var("checksum", checksum.expr);

  coder.indent();
  coder << "int " << c.name << " = ";
  coder << "rte_ipv4_udptcp_cksum(";
  coder << "(struct rte_ipv4_hdr*)";
  coder << ip_hdr_var.value().name << ", ";
  coder << "(void*)";
  coder << l4_hdr_var.value().name;
  coder << ")";
  coder << ";\n";

  vars.insert_back(c);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::DchainIsIndexAllocated *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> dchain_addr = node->get_dchain_addr();
  const klee::ref<klee::Expr> index       = node->get_index();
  const symbol_t is_allocated             = node->get_is_allocated();

  var_t ia = build_var("is_allocated", is_allocated.expr);

  std::optional<var_t> dchain_var = vars.get(dchain_addr);
  assert(dchain_var.has_value() && "Dchain not found");

  coder.indent();
  coder << "int " << ia.name << " = ";
  coder << "dchain_is_index_allocated(";
  coder << dchain_var.value().name << ", ";
  coder << transpiler.transpile(index);
  coder << ")";
  coder << ";\n";

  vars.insert_back(ia);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::CMSAllocate *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  klee::ref<klee::Expr> height           = node->get_height();
  klee::ref<klee::Expr> width            = node->get_width();
  klee::ref<klee::Expr> key_size         = node->get_key_size();
  klee::ref<klee::Expr> cleanup_interval = node->get_cleanup_interval();
  klee::ref<klee::Expr> cms_out          = node->get_cms_out();
  symbol_t success                       = node->get_success();

  var_t cms_out_var = build_var("cms", cms_out);
  var_t success_var = build_var("cms_allocation_succeeded", success.expr);

  coder_t &coder_nf_state = get(MARKER_NF_STATE);
  coder_nf_state.indent();
  coder_nf_state << "struct CMS *";
  coder_nf_state << cms_out_var.name;
  coder_nf_state << ";\n";

  coder.indent();
  coder << "int " << success_var.name << " = ";
  coder << "cms_allocate(";
  coder << transpiler.transpile(height) << ", ";
  coder << transpiler.transpile(width) << ", ";
  coder << transpiler.transpile(key_size) << ", ";
  coder << transpiler.transpile(cleanup_interval) << ", ";
  coder << "&" << cms_out_var.name;
  coder << ");\n";

  coder.indent();
  coder << "if (!";
  coder << success_var.name;
  coder << ") {\n";

  coder.inc();
  coder.indent();
  coder << "return false;\n";

  coder.dec();
  coder.indent();
  coder << "}\n";

  vars.insert_back(cms_out_var);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::CMSIncrement *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> cms_addr = node->get_cms_addr();
  const klee::ref<klee::Expr> key_addr = node->get_key_addr();
  const klee::ref<klee::Expr> key      = node->get_key();

  std::optional<var_t> cms_var = vars.get(cms_addr);
  assert(cms_var.has_value() && "CMS not found");

  bool key_in_stack;
  var_t k = build_var_ptr("hash", key_addr, key, coder, key_in_stack);

  coder.indent();
  coder << "cms_increment(";
  coder << cms_var.value().name << ", ";
  coder << k.name;
  coder << ")";
  coder << ";\n";

  if (!key_in_stack) {
    vars.insert_back(k);
  } else {
    vars.replace(k, key);
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::CMSCountMin *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> cms_addr     = node->get_cms_addr();
  const klee::ref<klee::Expr> key_addr     = node->get_key_addr();
  const klee::ref<klee::Expr> key          = node->get_key();
  const klee::ref<klee::Expr> min_estimate = node->get_min_estimate();

  std::optional<var_t> cms_var = vars.get(cms_addr);
  assert(cms_var.has_value() && "CMS not found");

  bool key_in_stack;
  var_t k = build_var_ptr("hash", key_addr, key, coder, key_in_stack);

  var_t me = build_var("min_estimate", min_estimate);

  coder.indent();
  coder << "int " << me.name << " = ";
  coder << "cms_count_min(";
  coder << cms_var.value().name << ", ";
  coder << k.name;
  coder << ")";
  coder << ";\n";

  if (!key_in_stack) {
    vars.insert_back(k);
  } else {
    vars.replace(k, key);
  }

  vars.insert_back(me);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::CMSPeriodicCleanup *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  klee::ref<klee::Expr> cms_addr        = node->get_cms_addr();
  klee::ref<klee::Expr> time            = node->get_time();
  klee::ref<klee::Expr> cleanup_success = node->get_cleanup_success();

  std::optional<var_t> cms_var = vars.get(cms_addr);
  assert(cms_var.has_value() && "CMS not found");

  var_t cs = build_var("cleanup_success", cleanup_success);

  coder.indent();
  coder << "int " << cs.name << " = ";
  coder << "cms_periodic_cleanup(";
  coder << cms_var.value().name << ", ";
  coder << transpiler.transpile(time);
  coder << ")";
  coder << ";\n";

  vars.insert_back(cs);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::MapErase *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> map_addr = node->get_map_addr();
  const klee::ref<klee::Expr> key_addr = node->get_key_addr();
  const klee::ref<klee::Expr> key      = node->get_key();
  const klee::ref<klee::Expr> trash    = node->get_trash();

  std::optional<var_t> map_var = vars.get(map_addr);
  assert(map_var.has_value() && "MAP not found");

  bool key_in_stack;
  var_t k = build_var_ptr("key", key_addr, key, coder, key_in_stack);

  coder.indent();
  coder << "map_erase(";
  coder << map_var.value().name << ", ";
  coder << k.name << ", ";
  coder << "&trash";
  coder << ")";
  coder << ";\n";

  if (target == x86SynthesizerTarget::Profiler) {
    const BDDNode *call_node = ep_node->get_module()->get_node();
    nodes_to_map.insert({call_node->get_id(), map_addr});
    coder.indent();
    coder << "stats_per_map[" << transpiler.transpile(map_addr) << "]";
    coder << ".update(";
    coder << call_node->get_id() << ", ";
    coder << k.name << ", ";
    coder << key->getWidth() / 8 << ", ";
    coder << "now"; // FIXME: we should get this from the stack
    coder << ")";
    coder << ";\n";
  }

  if (!key_in_stack) {
    vars.insert_back(k);
  } else {
    vars.replace(k, key);
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::DchainFreeIndex *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  klee::ref<klee::Expr> dchain_addr = node->get_dchain_addr();
  klee::ref<klee::Expr> index       = node->get_index();

  std::optional<var_t> dchain_var = vars.get(dchain_addr);
  assert(dchain_var.has_value() && "DCHAIN not found");

  coder.indent();
  coder << "dchain_free_index(";
  coder << dchain_var.value().name << ", ";
  coder << transpiler.transpile(index);
  coder << ")";
  coder << ";\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::ChtFindBackend *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: ChtFindBackend not implemented");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::HashObj *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: HashObj not implemented");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::SendToDevice *node) {
  coder_t &coder = get_current_coder();

  if (!in_nf_init) {
    process_nodes.insert(node->get_node()->get_id());
  }

  const std::vector<EPNode *> &children = ep_node->get_children();
  assert(children.size() == 1 && "Expected single child");

  const klee::ref<klee::Expr> outgoing_port = node->get_outgoing_port();
  const Symbols &symbols                    = node->get_symbols();

  const TargetType synthesizer_target = TargetType(TargetArchitecture::x86, instance_id);
  // const TargetType current_target     = node->get_target();
  const TargetType next_target = node->get_next_target();

  if (synthesizer_target != next_target) {

    coder.indent();
    coder << "packet_return_all_chunks(buffer);\n";

    code_t trash = create_unique_name("trash");

    coder.indent();
    coder << "void *";
    coder << trash;
    coder << ";\n";

    coder.indent();
    coder << "packet_borrow_next_chunk(";
    coder << "buffer, ";
    coder << "50, ";
    coder << "&" << trash;
    coder << ")";
    coder << ";\n";

    coder.indent();
    coder << "packet_insert_new_chunk(";
    coder << "(void **) &buffer, ";
    coder << "sizeof(";
    coder << CODE_PATH_HEADER_TYPE;
    coder << "), ";
    coder << CHUNKS_BORROWED << ", ";
    coder << "&" << NUM_CHUNKS_BORROWED << ", ";
    coder << "mbuf);\n";

    code_t path_hdr = create_unique_name("code_path_hdr");

    coder.indent();
    coder << "uint8_t *";
    coder << path_hdr;
    coder << " = ";
    coder << "(uint8_t *) ";
    coder << CHUNKS_BORROWED;
    coder << "[";
    coder << NUM_CHUNKS_BORROWED;
    coder << " - 1]";
    coder << ";\n";

    for (int i = 0; i < CODE_PATH_HEADER_SIZE; i++) {
      coder.indent();
      coder << path_hdr;
      coder << "[";
      coder << std::to_string(i);
      coder << "] = ";
      u64 code_path = ep_node->get_id() >> (8 * i);
      coder << std::to_string(code_path & 0xFFu);
      coder << ";\n";
    }

    coder.indent();
    coder << "struct Context_" << ep_node->get_id() << "{\n";

    coder.inc();

    for (const symbol_t &symbol : symbols.get()) {
      std::optional<var_t> var = vars.get(symbol.expr);

      if (!var.has_value()) {
        dbg_vars();
        panic("Variable %s not found in stack", expr_to_string(symbol.expr, true).c_str());
      }

      coder.indent();
      coder << transpiler.type_from_expr(var.value().expr) << " ";
      coder << var.value().name;
      coder << ";\n";
    }

    coder.dec();
    coder.indent();
    coder << "};\n";

    coder.indent();
    coder << "packet_insert_new_chunk(";
    coder << "(void **) &buffer, ";
    coder << "sizeof(struct Context_" << ep_node->get_id() << "), ";
    coder << CHUNKS_BORROWED << ", ";
    coder << "&" << NUM_CHUNKS_BORROWED << ", ";
    coder << "mbuf);\n";

    code_t ctx_hdr = create_unique_name("ctx_hdr");

    coder.indent();
    coder << "struct Context_" << ep_node->get_id() << " *";
    coder << ctx_hdr;
    coder << " = ";
    coder << "(struct Context_" << ep_node->get_id() << " *) ";
    coder << CHUNKS_BORROWED;
    coder << "[";
    coder << NUM_CHUNKS_BORROWED;
    coder << " - 1]";
    coder << ";\n";

    for (const symbol_t &symbol : symbols.get()) {
      std::optional<var_t> var = vars.get(symbol.expr);

      if (!var.has_value()) {
        dbg_vars();
        panic("Variable %s not found in stack", expr_to_string(symbol.expr, true).c_str());
      }

      coder.indent();
      coder << ctx_hdr << "->" << var.value().name;
      coder << " = ";
      coder << transpiler.transpile(var.value().expr);
      coder << ";\n";
    }

    coder.indent();
    coder << "packet_return_all_chunks(buffer);\n";

    coder.indent();
    coder << "return " << transpiler.transpile(outgoing_port) << ";\n";

    // return EPVisitor::Action::skipChildren;

  } else {

    coder.indent();
    const ep_node_id_t code_path = ep_node->get_id();
    code_paths.push_back(code_path);

    coder << ((code_paths.size() == 1) ? "if " : "else if ");
    coder << "(code_path_0 == " << ep_node->get_prev()->get_id() << ") {\n";

    coder.inc();
    coder.indent();
    coder << "std::cerr << \"Received Packet From Inside Network\\n\";\n";

    code_t trash = create_unique_name("trash");

    coder.indent();
    coder << "void* ";
    coder << trash;
    coder << ";\n";

    coder.indent();
    coder << "packet_borrow_next_chunk(";
    coder << "buffer, ";
    coder << "50";
    coder << ", ";
    coder << "&" << trash;
    coder << ")";
    coder << ";\n";

    coder.indent();
    coder << "packet_borrow_next_chunk(";
    coder << "buffer, ";
    coder << "sizeof(";
    coder << CODE_PATH_HEADER_TYPE;
    coder << "), ";
    coder << "&" << trash;
    coder << ")";
    coder << ";\n";

    coder.indent();
    coder << "struct Context_" << ep_node->get_prev()->get_id() << " {\n";

    coder.inc();

    for (const symbol_t &symbol : symbols.get()) {
      std::optional<var_t> var = vars.get(symbol.expr);

      coder.indent();
      coder << transpiler.type_from_expr(symbol.expr) << " ";

      if (!var.has_value()) {
        var_t symbol_var = build_var(symbol.name, symbol.expr);
        vars.insert_back(symbol_var);

        coder << symbol_var.name;
      } else {
        coder << var.value().name;
      }
      coder << ";\n";
    }

    coder.dec();
    coder.indent();
    coder << "};\n";

    code_t ctx_hdr = create_unique_name("ctx_hdr");

    coder.indent();
    coder << "void *" << ctx_hdr << ";\n";

    coder.indent();
    coder << "packet_borrow_next_chunk(";
    coder << "buffer, ";
    coder << "sizeof(struct Context_" << ep_node->get_prev()->get_id() << "), ";
    coder << "&" << ctx_hdr;
    coder << ")";
    coder << ";\n";

    code_t ctx = create_unique_name("ctx");

    coder.indent();
    coder << "struct Context_" << ep_node->get_prev()->get_id() << " *";
    coder << ctx;
    coder << " = ";
    coder << "(struct Context_" << ep_node->get_prev()->get_id() << " *) " << ctx_hdr;
    coder << ";\n";

    for (const symbol_t &symbol : symbols.get()) {
      std::optional<var_t> var = vars.get(symbol.expr);

      if (!var.has_value()) {
        dbg_vars();
        panic("Variable %s not found in stack", expr_to_string(symbol.expr, true).c_str());
      }

      coder.indent();
      coder << transpiler.type_from_expr(var.value().expr) << " ";
      coder << var.value().name;
      coder << " = ";
      coder << ctx << "->" << var.value().name;
      coder << ";\n";
    }

    coder.indent();
    coder << "packet_shrink_chunk(";
    coder << "(void**) &buffer, ";
    coder << "0, ";
    coder << CHUNKS_BORROWED << ", ";
    coder << NUM_CHUNKS_BORROWED << ", ";
    coder << "mbuf";
    coder << ")";
    coder << ";\n";

    coder.indent();
    coder << NUM_CHUNKS_BORROWED << "--";
    coder << ";\n";

    coder.indent();
    coder << "packet_shrink_chunk(";
    coder << "(void**) &buffer, ";
    coder << "0, ";
    coder << CHUNKS_BORROWED << ", ";
    coder << NUM_CHUNKS_BORROWED << ", ";
    coder << "mbuf";
    coder << ")";
    coder << ";\n";

    coder.indent();
    coder << NUM_CHUNKS_BORROWED << "--";
    coder << ";\n";

    coder.indent();
    coder << "packet_return_all_chunks(buffer);\n";

    const EPNode *child = children[0];

    visit(ep, child);

    coder.dec();
    coder.indent();
    coder << "}\n";
  }

  return EPVisitor::Action::skipChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::TokenBucketAllocate *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  klee::ref<klee::Expr> capacity = node->get_capacity();
  klee::ref<klee::Expr> rate     = node->get_rate();
  klee::ref<klee::Expr> burst    = node->get_burst();
  klee::ref<klee::Expr> key_size = node->get_key_size();
  klee::ref<klee::Expr> tb_out   = node->get_tb_out();
  symbol_t success               = node->get_success();

  var_t tb_out_var  = build_var("tb", tb_out);
  var_t success_var = build_var("tb_allocation_succeeded", success.expr);

  coder_t &coder_nf_state = get(MARKER_NF_STATE);
  coder_nf_state.indent();
  coder_nf_state << "struct TokenBucket *";
  coder_nf_state << tb_out_var.name;
  coder_nf_state << ";\n";

  coder.indent();
  coder << "tb_allocate(";
  coder << transpiler.transpile(capacity) << ", ";
  coder << transpiler.transpile(rate) << "ull, ";
  coder << transpiler.transpile(burst) << "ull, ";
  coder << transpiler.transpile(key_size) << ", ";
  coder << "&" << tb_out_var.name;
  coder << ");\n";

  coder.indent();
  coder << "if (!";
  coder << success_var.name;
  coder << ") {\n";

  coder.inc();
  coder.indent();
  coder << "return false;\n";

  coder.dec();
  coder.indent();
  coder << "}\n";

  vars.insert_back(tb_out_var);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::TokenBucketIsTracing *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> tb_addr    = node->get_tb_addr();
  const klee::ref<klee::Expr> key_addr   = node->get_key_addr();
  const klee::ref<klee::Expr> key        = node->get_key();
  const klee::ref<klee::Expr> index_out  = node->get_index_out();
  const klee::ref<klee::Expr> is_tracing = node->get_is_tracing();

  bool key_in_stack;
  var_t k = build_var_ptr("key", key_addr, key, coder, key_in_stack);

  var_t it    = build_var("is_tracing", is_tracing);
  var_t index = build_var("index", index_out);

  std::optional<var_t> tb_var = vars.get(tb_addr);
  assert(tb_var.has_value() && "TB not found");

  coder.indent();
  coder << "int " << index.name << ";\n";

  coder.indent();
  coder << "int " << it.name << " = ";
  coder << "tb_is_tracing(";
  coder << tb_var.value().name << ", ";
  coder << k.name << ", ";
  coder << "&" << index.name;
  coder << ")" << index.name;
  coder << ";\n";

  vars.insert_back(it);
  vars.insert_back(index);

  if (!key_in_stack) {
    vars.insert_back(k);
  } else {
    vars.replace(k, key);
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::TokenBucketTrace *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> tb_addr             = node->get_tb_addr();
  const klee::ref<klee::Expr> key_addr            = node->get_key_addr();
  const klee::ref<klee::Expr> key                 = node->get_key();
  const klee::ref<klee::Expr> pkt_len             = node->get_pkt_len();
  const klee::ref<klee::Expr> time                = node->get_time();
  const klee::ref<klee::Expr> index_out           = node->get_index_out();
  const klee::ref<klee::Expr> successfuly_tracing = node->get_successfuly_tracing();

  bool key_in_stack;
  var_t k = build_var_ptr("key", key_addr, key, coder, key_in_stack);

  var_t st = build_var("successfuly_tracing", successfuly_tracing);
  var_t i  = build_var("index", index_out);

  std::optional<var_t> tb_var = vars.get(tb_addr);
  assert(tb_var.has_value() && "TB not found");

  coder.indent();
  coder << "int " << i.name << ";\n";

  coder.indent();
  coder << "int " << st.name << " = ";
  coder << "tb_trace(";
  coder << tb_var.value().name << ", ";
  coder << k.name << ", ";
  coder << transpiler.transpile(pkt_len) << ", ";
  coder << transpiler.transpile(time) << ", ";
  coder << "&" << i.name;
  coder << ")";
  coder << ";\n";

  vars.insert_back(st);
  vars.insert_back(i);

  if (!key_in_stack) {
    vars.insert_back(k);
  } else {
    vars.replace(k, key);
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::TokenBucketUpdateAndCheck *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> tb_addr = node->get_tb_addr();
  const klee::ref<klee::Expr> index   = node->get_index();
  const klee::ref<klee::Expr> pkt_len = node->get_pkt_len();
  const klee::ref<klee::Expr> time    = node->get_time();
  const klee::ref<klee::Expr> pass    = node->get_pass();

  var_t p = build_var("pass", pass);

  std::optional<var_t> tb_var = vars.get(tb_addr);
  assert(tb_var.has_value() && "TB not found");

  coder.indent();
  coder << "int " << p.name << " = ";
  coder << "tb_update_and_check(";
  coder << tb_var.value().name << ", ";
  coder << transpiler.transpile(index) << ", ";
  coder << transpiler.transpile(pkt_len) << ", ";
  coder << transpiler.transpile(time);
  coder << ")";
  coder << ";\n";

  vars.insert_back(p);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::TokenBucketExpire *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  const klee::ref<klee::Expr> tb_addr = node->get_tb_addr();
  const klee::ref<klee::Expr> time    = node->get_time();

  std::optional<var_t> tb_var = vars.get(tb_addr);
  assert(tb_var.has_value() && "TB not found");

  coder.indent();
  coder << "tb_expire(";
  coder << tb_var.value().name << ", ";
  coder << transpiler.transpile(time);
  coder << ")";
  coder << ";\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action x86Synthesizer::visit(const EP *ep, const EPNode *ep_node, const x86::LPMAllocate *node) {
  coder_t &coder = get_current_coder();
  if (!in_nf_init)
    process_nodes.insert(node->get_node()->get_id());

  klee::ref<klee::Expr> lpm_out = node->get_lpm_out();
  symbol_t success              = node->get_success();

  var_t lpm_out_var = build_var("lpm", lpm_out);
  var_t success_var = build_var("lpm_alloc_success", success.expr);

  coder_t &coder_nf_state = get(MARKER_NF_STATE);
  coder_nf_state.indent();
  coder_nf_state << "struct LPM *";
  coder_nf_state << lpm_out_var.name;
  coder_nf_state << ";\n";

  coder.indent();
  coder << "int " << success_var.name << " = ";
  coder << "lpm_allocate(";
  coder << "&" << lpm_out_var.name;
  coder << ");\n";

  coder.indent();
  coder << "if (!";
  coder << success_var.name;
  coder << ") {\n";

  coder.inc();
  coder.indent();
  coder << "return false;\n";

  coder.dec();
  coder.indent();
  coder << "}\n";

  vars.insert_back(lpm_out_var);

  return EPVisitor::Action::doChildren;
}

void x86Synthesizer::dbg_vars() const {
  std::cerr << "================= Stack ================= \n";
  for (const Stack &stack : vars.get_all()) {
    std::cerr << "------------------------------------------\n";
    for (const var_t &var : stack.get_all()) {
      std::cerr << var.to_string() << "\n";
    }
  }
  std::cerr << "======================================== \n";
}

void x86Synthesizer::log(const EPNode *node) const {
  std::cerr << "[x86Synthesizer] ";
  EPVisitor::log(node);
}
} // namespace x86
} // namespace LibSynapse
