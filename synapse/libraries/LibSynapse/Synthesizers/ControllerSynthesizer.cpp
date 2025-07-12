#include <LibSynapse/Synthesizers/ControllerSynthesizer.h>
#include <LibSynapse/Modules/x86/x86.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>

namespace LibSynapse {
namespace Controller {

namespace {

using LibSynapse::Tofino::DS;
using LibSynapse::Tofino::DS_ID;
using LibSynapse::Tofino::TNA;
using LibSynapse::Tofino::tofino_port_t;
using LibSynapse::Tofino::TofinoContext;

constexpr const char *const MARKER_STATE_FIELDS           = "STATE_FIELDS";
constexpr const char *const MARKER_STATE_MEMBER_INIT_LIST = "STATE_MEMBER_INIT_LIST";
constexpr const char *const MARKER_NF_INIT                = "NF_INIT";
constexpr const char *const MARKER_NF_EXIT                = "NF_EXIT";
constexpr const char *const MARKER_NF_ARGS                = "NF_ARGS";
constexpr const char *const MARKER_NF_USER_SIGNAL_HANDLER = "NF_USER_SIGNAL_HANDLER";
constexpr const char *const MARKER_NF_PROCESS             = "NF_PROCESS";
constexpr const char *const MARKER_CPU_HDR_EXTRA          = "CPU_HDR_EXTRA";
constexpr const char *const TEMPLATE_FILENAME             = "controller.template.cpp";

template <class T> std::unordered_set<const T *> get_tofino_ds_from_obj(const EP *ep, addr_t obj) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();

  std::unordered_set<const T *> ds_matches;
  for (const DS *ds : tofino_ctx->get_data_structures().get_ds(obj)) {
    const T *ds_match = dynamic_cast<const T *>(ds);
    if (ds_match) {
      ds_matches.insert(ds_match);
    }
  }

  assert(!ds_matches.empty() && "No DS matches");
  return ds_matches;
}

template <class T> const T *get_unique_tofino_ds_from_obj(const EP *ep, addr_t obj) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();

  std::unordered_set<const T *> ds_matches;
  for (const DS *ds : tofino_ctx->get_data_structures().get_ds(obj)) {
    const T *ds_match = dynamic_cast<const T *>(ds);
    if (ds_match) {
      ds_matches.insert(ds_match);
    }
  }

  assert(!ds_matches.empty() && "No DS matches");
  assert(ds_matches.size() == 1 && "Multiple DS matches");
  return *ds_matches.begin();
}

template <class T> const T *get_tofino_ds(const EP *ep, DS_ID id) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  const DS *ds                    = tofino_ctx->get_data_structures().get_ds_from_id(id);
  assert(ds && "DS not found");
  return dynamic_cast<const T *>(ds);
}

time_ns_t get_expiration_time(const Context &ctx) {
  const std::optional<expiration_data_t> expiration_data = ctx.get_expiration_data();
  assert_or_panic(expiration_data.has_value(), "Expiration data not found");
  return expiration_data->expiration_time;
}

} // namespace

ControllerSynthesizer::Transpiler::Transpiler(const ControllerSynthesizer *_synthesizer) : synthesizer(_synthesizer) {}

ControllerSynthesizer::code_t ControllerSynthesizer::Transpiler::transpile(klee::ref<klee::Expr> expr, transpiler_opt_t opt) {
  loaded_opt = opt;

  std::cerr << "Transpiling: " << LibCore::expr_to_string(expr) << "\n";
  expr = LibCore::simplify(expr);
  std::cerr << "Simplified:  " << LibCore::expr_to_string(expr) << "\n";

  coders.emplace();
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> endian_swap_target;

  if (LibCore::is_constant(expr)) {
    assert(expr->getWidth() <= 64 && "Unsupported constant width");
    u64 value = LibCore::solver_toolbox.value_from_expr(expr);
    coder << value;
    if (value > (1ull << 31)) {
      if (!LibCore::is_constant_signed(expr)) {
        coder << "U";
      }
      coder << "LL";
    }
  } else if (LibCore::match_endian_swap_pattern(expr, endian_swap_target)) {
    bits_t size = endian_swap_target->getWidth();
    if (size == 16) {
      coder << "bswap16(" << transpile(endian_swap_target, loaded_opt) << ")";
    } else if (size == 32) {
      coder << "bswap32(" << transpile(endian_swap_target, loaded_opt) << ")";
    } else {
      panic("FIXME: incompatible endian swap size %d", size);
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

ControllerSynthesizer::code_t ControllerSynthesizer::Transpiler::type_from_size(bits_t size) {
  code_t type;

  switch (size) {
  case 1:
    type = "bool";
    break;
  case 8:
    type = "u8";
    break;
  case 16:
    type = "u16";
    break;
  case 32:
    type = "u32";
    break;
  case 64:
    type = "u64";
    break;
  default:
    panic("Unknown type (size=%u)", size);
  }

  return type;
}

ControllerSynthesizer::code_t ControllerSynthesizer::Transpiler::type_from_expr(klee::ref<klee::Expr> expr) {
  return type_from_size(expr->getWidth());
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitRead(const klee::ReadExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ReadExpr *>(&e);

  coder_t &coder = coders.top();

  if (std::optional<ControllerSynthesizer::var_t> var = synthesizer->vars.get(expr, loaded_opt)) {
    coder << var->name;
    return Action::skipChildren();
  }

  std::cerr << LibCore::expr_to_string(expr) << "\n";
  synthesizer->dbg_vars();

  panic("TODO: visitRead (%s)", LibCore::expr_to_string(expr).c_str());
  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitNotOptimized(const klee::NotOptimizedExpr &e) {
  panic("TODO: visitNotOptimized");
  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitSelect(const klee::SelectExpr &e) {
  panic("TODO: visitSelect");
  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitConcat(const klee::ConcatExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ConcatExpr *>(&e);
  coder_t &coder             = coders.top();

  if (std::optional<ControllerSynthesizer::var_t> var = synthesizer->vars.get(expr, loaded_opt)) {
    if (var->is_ptr) {
      coder << "*(" << type_from_size(expr->getWidth()) << "*)";
      coder << var->name;
    } else if (var->is_buffer) {
      coder << "(" << type_from_size(expr->getWidth()) << ")";
      coder << var->name << ".get(";
      coder << "0, ";
      coder << expr->getWidth() / 8;
      coder << ")";
    } else {
      coder << var->name;
    }

    return Action::skipChildren();
  }

  std::cerr << LibCore::expr_to_string(expr) << "\n";
  synthesizer->dbg_vars();

  panic("TODO: visitConcat");
  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitExtract(const klee::ExtractExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ExtractExpr *>(&e);
  coder_t &coder             = coders.top();

  if (std::optional<ControllerSynthesizer::var_t> var = synthesizer->vars.get(expr, loaded_opt)) {
    coder << var->name;
    return Action::skipChildren();
  }

  synthesizer->dbg_vars();
  panic("TODO: visitExtract");
  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitZExt(const klee::ZExtExpr &e) {
  klee::ref<klee::Expr> arg = e.getKid(0);
  coder_t &coder            = coders.top();

  coder << "(";
  coder << type_from_expr(arg);
  coder << ")";
  coder << "(";
  coder << transpile(arg, loaded_opt);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitSExt(const klee::SExtExpr &e) {
  panic("TODO: visitSExt");
  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitAdd(const klee::AddExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " + ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitSub(const klee::SubExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " - ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitMul(const klee::MulExpr &e) {
  panic("TODO: visitMul");
  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitUDiv(const klee::UDivExpr &e) {
  panic("TODO: visitUDiv");
  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitSDiv(const klee::SDivExpr &e) {
  panic("TODO: visitSDiv");
  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitURem(const klee::URemExpr &e) {
  panic("TODO: visitURem");
  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitSRem(const klee::SRemExpr &e) {
  panic("TODO: visitSRem");
  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitNot(const klee::NotExpr &e) {
  panic("TODO: visitNot");
  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitAnd(const klee::AndExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " & ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitOr(const klee::OrExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " | ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitXor(const klee::XorExpr &e) {
  panic("TODO: visitXor");
  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitShl(const klee::ShlExpr &e) {
  panic("TODO: visitShl");
  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitLShr(const klee::LShrExpr &e) {
  panic("TODO: visitLShr");
  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitAShr(const klee::AShrExpr &e) {
  panic("TODO: visitAShr");
  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitEq(const klee::EqExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  klee::ref<klee::Expr> var_expr;
  klee::ref<klee::Expr> const_expr;

  if (LibCore::is_constant(lhs)) {
    const_expr = lhs;
    var_expr   = rhs;
  } else {
    const_expr = rhs;
    var_expr   = lhs;
  }

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " == ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitNe(const klee::NeExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  klee::ref<klee::Expr> var_expr;
  klee::ref<klee::Expr> const_expr;

  if (LibCore::is_constant(lhs)) {
    const_expr = lhs;
    var_expr   = rhs;
  } else {
    const_expr = rhs;
    var_expr   = lhs;
  }

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " != ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitUlt(const klee::UltExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " < ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitUle(const klee::UleExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " <= ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitUgt(const klee::UgtExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " > ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitUge(const klee::UgeExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " >= ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitSlt(const klee::SltExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " < ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitSle(const klee::SleExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " <= ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitSgt(const klee::SgtExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " > ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitSge(const klee::SgeExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " >= ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

ControllerSynthesizer::code_t ControllerSynthesizer::var_t::get_slice(bits_t offset, bits_t size, transpiler_opt_t opt) const {
  assert(offset + size <= expr->getWidth() && "Out of bounds");

  coder_t coder;

  if (is_header && (opt & TRANSPILER_OPT_INVERT_HEADERS)) {
    offset = expr->getWidth() - (offset + size);
  }

  if (is_ptr) {
    coder << "*(";
    coder << Transpiler::type_from_size(size);
    coder << "*)(";
    coder << name;
    coder << " + ";
    coder << offset / 8;
    coder << ")";
  } else if (is_buffer) {
    coder << "(";
    coder << Transpiler::type_from_size(size);
    coder << ")";
    coder << name << ".get(";
    coder << offset / 8;
    coder << ", ";
    coder << size / 8;
    coder << ")";
  } else {
    if (offset > 0) {
      coder << "(";
      coder << name;
      coder << ">>";
      coder << offset;
      coder << ") & ";
      coder << ((1 << size) - 1);
    } else {
      coder << name;
      coder << " & ";
      coder << ((1ull << size) - 1);
    }
  }

  return coder.dump();
}

ControllerSynthesizer::code_t ControllerSynthesizer::var_t::get_stem() const {
  size_t pos = name.find_last_of('.');
  if (pos == std::string::npos) {
    return name;
  }

  return name.substr(pos);
}

void ControllerSynthesizer::Stack::push(const var_t &var) {
  if (names.find(var.name) == names.end()) {
    frames.push_back(var);
    names.insert(var.name);
  }
}

void ControllerSynthesizer::Stack::push(const Stack &stack) {
  for (const var_t &var : stack.frames) {
    push(var);
  }
}

void ControllerSynthesizer::Stack::clear() {
  frames.clear();
  names.clear();
}

std::optional<ControllerSynthesizer::var_t> ControllerSynthesizer::Stack::get_exact(klee::ref<klee::Expr> expr) const {
  for (auto var_it = frames.rbegin(); var_it != frames.rend(); var_it++) {
    const var_t &var = *var_it;
    if (LibCore::solver_toolbox.are_exprs_always_equal(var.expr, expr)) {
      return var;
    }
  }

  return std::nullopt;
}

std::optional<ControllerSynthesizer::var_t> ControllerSynthesizer::Stack::get(klee::ref<klee::Expr> expr, transpiler_opt_t opt) const {
  if (std::optional<var_t> var = get_exact(expr)) {
    return var;
  }

  for (auto var_it = frames.rbegin(); var_it != frames.rend(); var_it++) {
    const var_t &var = *var_it;

    bits_t expr_size = expr->getWidth();
    bits_t var_size  = var.expr->getWidth();

    if (expr_size > var_size) {
      continue;
    }

    for (bits_t offset = 0; offset + expr_size <= var_size; offset += 8) {
      klee::ref<klee::Expr> var_slice = LibCore::solver_toolbox.exprBuilder->Extract(var.expr, offset, expr_size);

      if (LibCore::solver_toolbox.are_exprs_always_equal(var_slice, expr)) {
        const var_t out_var = {
            .name      = var.get_slice(offset, expr_size, opt),
            .expr      = var_slice,
            .addr      = std::nullopt,
            .is_ptr    = false,
            .is_buffer = false,
            .is_header = false,
        };

        return out_var;
      }
    }
  }
  return std::nullopt;
}

std::optional<ControllerSynthesizer::var_t> ControllerSynthesizer::Stack::get_by_addr(addr_t addr) const {
  for (auto var_it = frames.rbegin(); var_it != frames.rend(); var_it++) {
    const var_t &var = *var_it;
    if (var.addr.has_value() && var.addr.value() == addr) {
      return var;
    }
  }

  return std::nullopt;
}

std::vector<ControllerSynthesizer::var_t> ControllerSynthesizer::Stack::get_all() const { return frames; }

void ControllerSynthesizer::Stacks::push() { stacks.emplace_back(); }

void ControllerSynthesizer::Stacks::pop() { stacks.pop_back(); }

void ControllerSynthesizer::Stacks::insert_front(const var_t &var) { stacks.front().push(var); }
void ControllerSynthesizer::Stacks::insert_front(const Stack &stack) { stacks.front().push(stack); }

void ControllerSynthesizer::Stacks::insert_back(const var_t &var) { stacks.back().push(var); }
void ControllerSynthesizer::Stacks::insert_back(const Stack &stack) { stacks.back().push(stack); }

ControllerSynthesizer::Stack ControllerSynthesizer::Stacks::squash() const {
  Stack squashed;
  for (const Stack &stack : stacks) {
    squashed.push(stack);
  }
  return squashed;
}

std::optional<ControllerSynthesizer::var_t> ControllerSynthesizer::Stacks::get(klee::ref<klee::Expr> expr, transpiler_opt_t opt) const {
  for (auto stack_it = stacks.rbegin(); stack_it != stacks.rend(); stack_it++) {
    if (std::optional<var_t> var = stack_it->get_exact(expr)) {
      return var;
    }
  }

  for (auto stack_it = stacks.rbegin(); stack_it != stacks.rend(); stack_it++) {
    if (std::optional<var_t> var = stack_it->get(expr, opt)) {
      return var;
    }
  }

  return std::nullopt;
}

std::optional<ControllerSynthesizer::var_t> ControllerSynthesizer::Stacks::get_by_addr(addr_t addr) const {
  for (auto stack_it = stacks.rbegin(); stack_it != stacks.rend(); stack_it++) {
    if (std::optional<var_t> var = stack_it->get_by_addr(addr)) {
      return var;
    }
  }

  return std::nullopt;
}

void ControllerSynthesizer::Stacks::clear() { stacks.clear(); }

std::vector<ControllerSynthesizer::Stack> ControllerSynthesizer::Stacks::get_all() const { return stacks; }

ControllerSynthesizer::ControllerSynthesizer(const EP *_ep, std::filesystem::path _out_file)
    : Synthesizer(std::filesystem::path(__FILE__).parent_path() / "Templates" / TEMPLATE_FILENAME,
                  {
                      {MARKER_STATE_FIELDS, 1},
                      {MARKER_STATE_MEMBER_INIT_LIST, 3},
                      {MARKER_NF_INIT, 1},
                      {MARKER_NF_EXIT, 1},
                      {MARKER_NF_ARGS, 1},
                      {MARKER_NF_USER_SIGNAL_HANDLER, 1},
                      {MARKER_NF_PROCESS, 1},
                      {MARKER_CPU_HDR_EXTRA, 1},
                  },
                  _out_file),
      target_ep(_ep), transpiler(this) {}

ControllerSynthesizer::coder_t &ControllerSynthesizer::get_current_coder() { return in_nf_init ? get(MARKER_NF_INIT) : get(MARKER_NF_PROCESS); }

ControllerSynthesizer::coder_t &ControllerSynthesizer::get(const std::string &marker) { return Synthesizer::get(marker); }

void ControllerSynthesizer::synthesize() {
  synthesize_nf_init();

  vars.clear();
  vars.push();

  const LibBDD::BDD *bdd = target_ep->get_bdd();

  LibCore::symbol_t now     = bdd->get_time();
  LibCore::symbol_t device  = bdd->get_device();
  LibCore::symbol_t pkt_len = bdd->get_packet_len();

  alloc_var("now", now.expr, {}, NO_OPTION);
  alloc_var("size", pkt_len.expr, {}, NO_OPTION);

  synthesize_nf_process();
  synthesize_state_member_init_list();
  Synthesizer::dump();
}

void ControllerSynthesizer::synthesize_nf_init() {
  coder_t &nf_init       = get(MARKER_NF_INIT);
  const LibBDD::BDD *bdd = target_ep->get_bdd();

  const Context &ctx              = target_ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  const TNA &tna                  = tofino_ctx->get_tna();

  for (const tofino_port_t &port : tna.tna_config.ports) {
    nf_init.indent();
    nf_init << "state->ingress_port_to_nf_dev.add_entry(";
    nf_init << "asic_get_dev_port(" << port.front_panel_port << "), ";
    nf_init << port.nf_device;
    nf_init << ");\n";

    nf_init.indent();
    nf_init << "state->forward_nf_dev.add_entry(";
    nf_init << port.nf_device << ", ";
    nf_init << "asic_get_dev_port(" << port.front_panel_port << ")";
    nf_init << ");\n";
  }

  ControllerTarget controller_target;
  for (const LibBDD::Call *call_node : bdd->get_init()) {
    std::vector<std::unique_ptr<Module>> candidate_modules;
    for (const std::unique_ptr<ModuleFactory> &factory : controller_target.module_factories) {
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

    nf_init.indent();
    nf_init << "// BDD node " << call_node->dump(true) << "\n";
    nf_init.indent();
    nf_init << "// Module " << module->get_name() << "\n";

    module->visit(*this, target_ep, nullptr);
  }
}

void ControllerSynthesizer::synthesize_nf_process() {
  change_to_process_coder();
  EPVisitor::visit(target_ep);
}

void ControllerSynthesizer::synthesize_state_member_init_list() {
  coder_t &coder = get(MARKER_STATE_MEMBER_INIT_LIST);

  for (const code_t &member : state_member_init_list) {
    coder << ",\n";
    coder.indent();
    coder << member;
  }
}

void ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node) {
  coder_t &coder = get(MARKER_NF_PROCESS);

  if (ep_node->get_module()->get_target() == TargetType::Controller) {
    coder.indent();
    coder << "// EP node  " << ep_node->get_id() << "\n";
    coder.indent();
    coder << "// BDD node " << ep_node->get_module()->get_node()->dump(true) << "\n";
  }

  EPVisitor::visit(ep, ep_node);
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::SendToController *node) {
  coder_t &coder     = get_current_coder();
  coder_t &cpu_extra = get(MARKER_CPU_HDR_EXTRA);

  const std::vector<EPNode *> &children = ep_node->get_children();
  assert(children.size() == 1 && "Expected single child");

  const EPNode *next_node      = children[0];
  const ep_node_id_t code_path = ep_node->get_id();
  code_paths.push_back(code_path);

  const LibCore::Symbols &symbols = node->get_symbols();
  for (const LibCore::symbol_t &symbol : symbols.get()) {
    const bits_t width = symbol.expr->getWidth();

    assert(width % 8 == 0 && "Unexpected width (not a multiple of 8)");
    assert(width >= 8 && "Unexpected width (less than 8)");

    const var_t var = alloc_var(symbol.name, symbol.expr, {}, EXACT_NAME | IS_CPU_HDR_EXTRA | (width > 64 ? IS_PTR : NO_OPTION));

    cpu_extra.indent();
    switch (width) {
    case 8: {
      cpu_extra << "u8 ";
      cpu_extra << var.name;
    } break;
    case 16: {
      cpu_extra << "u16 ";
      cpu_extra << var.name;
    } break;
    case 32: {
      cpu_extra << "u32 ";
      cpu_extra << var.name;
    } break;
    case 64: {
      cpu_extra << "u64 ";
      cpu_extra << var.name;
    } break;
    default: {
      cpu_extra << "u8 ";
      cpu_extra << var.name;
      cpu_extra << "[";
      cpu_extra << symbol.expr->getWidth() / 8;
      cpu_extra << "]";
    }
    }
    cpu_extra << ";\n";
  }

  coder.indent();
  coder << ((code_paths.size() == 1) ? "if " : "else if ");
  coder << "(bswap16(cpu_hdr->code_path) == " << ep_node->get_id() << ") {\n";

  coder.inc();
  visit(ep, next_node);
  coder.dec();

  coder.indent();
  coder << "}\n";

  return EPVisitor::Action::skipChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::Ignore *node) {
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::ParseHeader *node) {
  coder_t &coder = get_current_coder();

  const addr_t chunk_addr      = node->get_chunk_addr();
  klee::ref<klee::Expr> chunk  = node->get_chunk();
  klee::ref<klee::Expr> length = node->get_length();

  var_t hdr = alloc_var("hdr", chunk, chunk_addr, IS_PTR | IS_HEADER);

  coder.indent();
  coder << "u8* " << hdr.name << " = ";
  coder << "packet_consume(";
  coder << "pkt";
  coder << ", " << transpiler.transpile(length);
  coder << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::ModifyHeader *node) {
  coder_t &coder = get_current_coder();

  const addr_t chunk_addr                             = node->get_chunk_addr();
  const std::vector<LibCore::expr_mod_t> &changes     = node->get_changes();
  const std::vector<LibCore::expr_byte_swap_t> &swaps = node->get_swaps();

  const std::optional<var_t> hdr = vars.get_by_addr(chunk_addr);
  assert(hdr.has_value() && "Header not found");

  std::unordered_set<bytes_t> bytes_already_dealt_with;
  for (const LibCore::expr_byte_swap_t &byte_swap : swaps) {
    coder.indent();
    coder << "std::swap(";
    coder << hdr.value().name << "[" << byte_swap.byte0 << "]";
    coder << ", ";
    coder << hdr.value().name << "[" << byte_swap.byte1 << "]";
    coder << ");\n";

    bytes_already_dealt_with.insert(byte_swap.byte0);
    bytes_already_dealt_with.insert(byte_swap.byte1);
  }

  for (const LibCore::expr_mod_t &mod : changes) {
    LibCore::symbolic_reads_t symbolic_reads = LibCore::get_unique_symbolic_reads(mod.expr, "checksum");
    if (!symbolic_reads.empty()) {
      continue;
    }

    if (bytes_already_dealt_with.find(mod.offset / 8) != bytes_already_dealt_with.end()) {
      continue;
    }

    const bytes_t size = mod.width / 8;
    for (bytes_t i = 0; i < size; i++) {
      coder.indent();
      coder << hdr.value().name << "[" << ((mod.offset / 8) + i) << "] = ";

      if (size == 1) {
        coder << transpiler.transpile(mod.expr);
      } else {
        coder << transpiler.transpile(LibCore::solver_toolbox.exprBuilder->Extract(mod.expr, i * 8, 8));
      }

      coder << ";\n";
    }
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::ChecksumUpdate *node) {
  coder_t &coder = get_current_coder();

  const addr_t ip_hdr_addr         = node->get_ip_hdr_addr();
  const addr_t l4_hdr_addr         = node->get_l4_hdr_addr();
  const LibCore::symbol_t checksum = node->get_checksum();

  std::optional<var_t> ip_hdr = vars.get_by_addr(ip_hdr_addr);
  std::optional<var_t> l4_hdr = vars.get_by_addr(l4_hdr_addr);

  assert(ip_hdr.has_value() && "IP header not found");
  assert(l4_hdr.has_value() && "L4 header not found");

  coder.indent();
  coder << "trigger_update_ipv4_tcpudp_checksums = true;\n";
  coder.indent();
  coder << "l3_hdr = (void *)" << ip_hdr.value().name << ";\n";
  coder.indent();
  coder << "l4_hdr = (void *)" << l4_hdr.value().name << ";\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::If *node) {
  coder_t &ingress = get_current_coder();

  const klee::ref<klee::Expr> condition = node->get_condition();
  const std::vector<EPNode *> &children = ep_node->get_children();
  assert(children.size() == 2 && "If node must have 2 children");

  const EPNode *then_node = children[0];
  const EPNode *else_node = children[1];

  ingress.indent();
  ingress << "if (";
  ingress << transpiler.transpile(condition);
  ingress << ") {\n";

  ingress.inc();
  vars.push();
  visit(ep, then_node);
  vars.pop();
  ingress.dec();

  ingress.indent();
  ingress.stream << "} else {\n";

  ingress.inc();
  vars.push();
  visit(ep, else_node);
  vars.pop();
  ingress.dec();

  ingress.indent();
  ingress << "}\n";

  return EPVisitor::Action::skipChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::Then *node) {
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::Else *node) {
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::Forward *node) {
  coder_t &coder = get_current_coder();

  klee::ref<klee::Expr> dst_device = node->get_dst_device();

  coder.indent();
  coder << "cpu_hdr->egress_dev = bswap16(";
  coder << transpiler.transpile(dst_device);
  coder << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::Broadcast *node) {
  panic("TODO: Controller::Broadcast");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::Drop *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "result.forward = false;\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::AbortTransaction *node) {
  coder_t &coder = get_current_coder();
  abort_transaction(coder);
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneMapTableAllocate *node) {
  const addr_t obj                  = node->get_obj();
  const Tofino::MapTable *map_table = get_unique_tofino_ds_from_obj<Tofino::MapTable>(ep, obj);

  transpile_map_table_decl(map_table);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneMapTableLookup *node) {
  coder_t &coder = get_current_coder();

  const addr_t obj                             = node->get_obj();
  const klee::ref<klee::Expr> key              = node->get_key();
  const klee::ref<klee::Expr> value            = node->get_value();
  const std::optional<LibCore::symbol_t> found = node->get_found();

  const Tofino::MapTable *map_table = get_unique_tofino_ds_from_obj<Tofino::MapTable>(ep, obj);

  const var_t key_var   = transpile_buffer_decl_and_set(coder, map_table->id + "_key", key, true);
  const var_t value_var = alloc_var("value", value, {}, NO_OPTION);

  coder.indent();
  coder << "u32 " << value_var.name << ";\n";

  coder.indent();
  if (found.has_value()) {
    const var_t found_var = alloc_var("found", found->expr, {}, NO_OPTION);
    coder << "bool " << found_var.name << " = ";
  }
  coder << "state->" << map_table->id << ".get(";
  coder << key_var.name;
  coder << ", " << value_var.name;
  coder << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneMapTableUpdate *node) {
  coder_t &coder = get_current_coder();

  const addr_t obj                  = node->get_obj();
  const klee::ref<klee::Expr> key   = node->get_key();
  const klee::ref<klee::Expr> value = node->get_value();

  const Tofino::MapTable *map_table = get_unique_tofino_ds_from_obj<Tofino::MapTable>(ep, obj);

  const var_t key_var = transpile_buffer_decl_and_set(coder, map_table->id + "_key", key, true);

  coder.indent();
  coder << "state->" << map_table->id << ".put(";
  coder << key_var.name;
  coder << ", " << transpiler.transpile(value);
  coder << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneMapTableDelete *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::MapTableDelete");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneGuardedMapTableAllocate *node) {
  const addr_t obj                                 = node->get_obj();
  const Tofino::GuardedMapTable *guarded_map_table = get_unique_tofino_ds_from_obj<Tofino::GuardedMapTable>(ep, obj);

  transpile_guarded_map_table_decl(guarded_map_table);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneGuardedMapTableLookup *node) {
  coder_t &coder = get_current_coder();

  const addr_t obj                             = node->get_obj();
  const klee::ref<klee::Expr> key              = node->get_key();
  const klee::ref<klee::Expr> value            = node->get_value();
  const std::optional<LibCore::symbol_t> found = node->get_found();

  const Tofino::GuardedMapTable *guarded_map_table = get_unique_tofino_ds_from_obj<Tofino::GuardedMapTable>(ep, obj);

  const var_t key_var   = transpile_buffer_decl_and_set(coder, guarded_map_table->id + "_key", key, true);
  const var_t value_var = alloc_var("value", value, {}, NO_OPTION);

  coder.indent();
  coder << "u32 " << value_var.name << ";\n";

  coder.indent();
  if (found.has_value()) {
    const var_t found_var = alloc_var("found", found->expr, {}, NO_OPTION);
    coder << "bool " << found_var.name << " = ";
  }
  coder << "state->" << guarded_map_table->id << ".get(";
  coder << key_var.name;
  coder << ", " << value_var.name;
  coder << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneGuardedMapTableGuardCheck *node) {
  coder_t &coder = get_current_coder();

  const addr_t obj                            = node->get_obj();
  const LibCore::symbol_t &guard_allow_symbol = node->get_guard_allow();
  klee::ref<klee::Expr> guard_allow_condition = node->get_guard_allow_condition();

  const Tofino::GuardedMapTable *guarded_map_table = get_unique_tofino_ds_from_obj<Tofino::GuardedMapTable>(ep, obj);

  const var_t guard_allow_var = alloc_var("guard_allow", guard_allow_symbol.expr, {}, NO_OPTION);

  coder.indent();
  coder << "bool " << guard_allow_var.name << " = ";
  coder << "state->" << guarded_map_table->id << ".guard_check();\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneGuardedMapTableUpdate *node) {
  coder_t &coder = get_current_coder();

  const addr_t obj                  = node->get_obj();
  const klee::ref<klee::Expr> key   = node->get_key();
  const klee::ref<klee::Expr> value = node->get_value();

  const Tofino::GuardedMapTable *guarded_map_table = get_unique_tofino_ds_from_obj<Tofino::GuardedMapTable>(ep, obj);

  const var_t key_var = transpile_buffer_decl_and_set(coder, guarded_map_table->id + "_key", key, true);

  coder.indent();
  coder << "state->" << guarded_map_table->id << ".put(";
  coder << key_var.name;
  coder << ", " << transpiler.transpile(value);
  coder << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneGuardedMapTableDelete *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::GuardedMapTableDelete");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneVectorTableAllocate *node) {
  const addr_t obj                        = node->get_obj();
  const Tofino::VectorTable *vector_table = get_unique_tofino_ds_from_obj<Tofino::VectorTable>(ep, obj);

  transpile_vector_table_decl(vector_table);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneVectorTableLookup *node) {
  coder_t &coder = get_current_coder();

  const addr_t obj                  = node->get_obj();
  const klee::ref<klee::Expr> index = node->get_index();
  const klee::ref<klee::Expr> value = node->get_value();

  const Tofino::VectorTable *vector_table = get_unique_tofino_ds_from_obj<Tofino::VectorTable>(ep, obj);

  var_t value_var = alloc_var("value", value, {}, IS_BUFFER);

  coder.indent();
  coder << "buffer_t " << value_var.name << ";\n";

  coder.indent();
  coder << "state->" << vector_table->id << ".read(";
  coder << transpiler.transpile(index);
  coder << ", " << value_var.name;
  coder << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneVectorTableUpdate *node) {
  coder_t &coder = get_current_coder();

  const addr_t obj                  = node->get_obj();
  const klee::ref<klee::Expr> key   = node->get_key();
  const klee::ref<klee::Expr> value = node->get_value();

  const Tofino::VectorTable *vector_table = get_unique_tofino_ds_from_obj<Tofino::VectorTable>(ep, obj);

  const var_t value_var = transpile_buffer_decl_and_set(coder, vector_table->id + "_value", value, true);

  coder.indent();
  coder << "state->" << vector_table->id << ".write(";
  coder << transpiler.transpile(key);
  coder << ", " << value_var.name;
  coder << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneDchainTableAllocate *node) {
  const addr_t obj                = node->get_obj();
  const time_ns_t expiration_time = get_expiration_time(ep->get_ctx());

  const Tofino::DchainTable *dchain_table = get_unique_tofino_ds_from_obj<Tofino::DchainTable>(ep, obj);

  transpile_dchain_table_decl(dchain_table, expiration_time);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneDchainTableIsIndexAllocated *node) {
  coder_t &coder = get_current_coder();

  const addr_t obj                     = node->get_obj();
  klee::ref<klee::Expr> index          = node->get_index();
  const LibCore::symbol_t is_allocated = node->get_is_allocated();

  const Tofino::DchainTable *dchain_table = get_unique_tofino_ds_from_obj<Tofino::DchainTable>(ep, obj);

  var_t is_allocated_var = alloc_var("is_allocated", is_allocated.expr, {}, NO_OPTION);

  coder.indent();
  coder << "bool " << is_allocated_var.name << " = ";
  coder << "state->" << dchain_table->id;
  coder << ".is_index_allocated(";
  coder << transpiler.transpile(index);
  coder << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneDchainTableRefreshIndex *node) {
  coder_t &coder = get_current_coder();

  const addr_t obj            = node->get_obj();
  klee::ref<klee::Expr> index = node->get_index();

  const Tofino::DchainTable *dchain_table = get_unique_tofino_ds_from_obj<Tofino::DchainTable>(ep, obj);

  coder.indent();
  coder << "state->" << dchain_table->id;
  coder << ".refresh_index(";
  coder << transpiler.transpile(index);
  coder << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneDchainTableAllocateNewIndex *node) {
  coder_t &coder = get_current_coder();

  const addr_t obj                      = node->get_obj();
  klee::ref<klee::Expr> allocated_index = node->get_allocated_index();
  klee::ref<klee::Expr> success         = node->get_success();

  const Tofino::DchainTable *dchain_table = get_unique_tofino_ds_from_obj<Tofino::DchainTable>(ep, obj);

  var_t allocated_index_var = alloc_var("allocated_index", allocated_index, {}, NO_OPTION);
  var_t success_var         = alloc_var("success", success, {}, NO_OPTION);

  coder.indent();
  coder << "u32 " << allocated_index_var.name << ";\n";

  coder.indent();
  coder << "bool " << success_var.name << " = ";
  coder << "state->" << dchain_table->id;
  coder << ".allocate_new_index(";
  coder << allocated_index_var.name;
  coder << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneDchainTableFreeIndex *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::DchainTableFreeIndex");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DchainAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::DchainAllocate");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DchainAllocateNewIndex *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::DchainAllocateNewIndex");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DchainRejuvenateIndex *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::DchainRejuvenateIndex");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DchainIsIndexAllocated *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::DchainIsIndexAllocated");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DchainFreeIndex *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::DchainFreeIndex");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::VectorAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::VectorAllocate");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::VectorRead *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::VectorRead");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::VectorWrite *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::VectorWrite");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::MapAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::MapAllocate");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::MapGet *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::MapGet");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::MapPut *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::MapPut");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::MapErase *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::MapErase");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::ChtAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::ChtAllocate");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::ChtFindBackend *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::ChtFindBackend");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::HashObj *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::HashObj");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneVectorRegisterAllocate *node) {
  addr_t obj = node->get_obj();

  const Tofino::VectorRegister *vector_register = get_unique_tofino_ds_from_obj<Tofino::VectorRegister>(ep, obj);
  transpile_vector_register_decl(vector_register);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneVectorRegisterLookup *node) {
  coder_t &coder = get_current_coder();

  const addr_t obj                  = node->get_obj();
  const klee::ref<klee::Expr> index = node->get_index();
  const klee::ref<klee::Expr> value = node->get_value();

  const Tofino::VectorRegister *vector_register = get_unique_tofino_ds_from_obj<Tofino::VectorRegister>(ep, obj);

  var_t value_var = alloc_var("value", value, {}, IS_BUFFER);

  coder.indent();
  coder << "buffer_t " << value_var.name << ";\n";

  coder.indent();
  coder << "state->" << vector_register->id << ".get(";
  coder << transpiler.transpile(index);
  coder << ", " << value_var.name;
  coder << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneVectorRegisterUpdate *node) {
  coder_t &coder = get_current_coder();

  const addr_t obj                      = node->get_obj();
  const klee::ref<klee::Expr> index     = node->get_index();
  const klee::ref<klee::Expr> old_value = node->get_old_value();
  const klee::ref<klee::Expr> new_value = node->get_new_value();

  const Tofino::VectorRegister *vector_register = get_unique_tofino_ds_from_obj<Tofino::VectorRegister>(ep, obj);

  const var_t value_var = transpile_buffer_decl_and_set(coder, vector_register->id + "_value", new_value, true);

  coder.indent();
  coder << "state->" << vector_register->id << ".put(";
  coder << transpiler.transpile(index);
  coder << ", " << value_var.name;
  coder << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneFCFSCachedTableAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::FCFSCachedTableAllocate");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneFCFSCachedTableRead *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::FCFSCachedTableRead");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneFCFSCachedTableWrite *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::FCFSCachedTableWrite");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneFCFSCachedTableDelete *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::FCFSCachedTableDelete");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneHHTableAllocate *node) {
  const addr_t obj                = node->get_obj();
  const time_ns_t expiration_time = get_expiration_time(ep->get_ctx());

  const Tofino::HHTable *hh_table = get_unique_tofino_ds_from_obj<Tofino::HHTable>(ep, obj);

  transpile_hh_table_decl(hh_table, expiration_time);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneHHTableRead *node) {
  coder_t &coder = get_current_coder();
  coder.indent();

  const addr_t obj                          = node->get_obj();
  const klee::ref<klee::Expr> key           = node->get_key();
  const klee::ref<klee::Expr> value         = node->get_value();
  const LibCore::symbol_t &map_has_this_key = node->get_hit();

  const Tofino::HHTable *hh_table = get_unique_tofino_ds_from_obj<Tofino::HHTable>(ep, obj);

  const var_t key_var   = transpile_buffer_decl_and_set(coder, hh_table->id + "_key", key, true);
  const var_t value_var = alloc_var("value", value, {}, NO_OPTION);

  coder.indent();
  coder << "u32 " << value_var.name << ";\n";

  coder.indent();
  const var_t found_var = alloc_var("found", map_has_this_key.expr, {}, NO_OPTION);
  coder << "bool " << found_var.name << " = ";
  coder << "state->" << hh_table->id << ".get(";
  coder << key_var.name;
  coder << ", " << value_var.name;
  coder << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneHHTableUpdate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::HHTableUpdate");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneHHTableIsIndexAllocated *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::HHTableUpdate");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneHHTableDelete *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::HHTableDelete");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::TokenBucketAllocate");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketIsTracing *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::TokenBucketIsTracing");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketTrace *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::TokenBucketTrace");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketUpdateAndCheck *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::TokenBucketUpdateAndCheck");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketExpire *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::TokenBucketExpire");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneMeterAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::MeterAllocate");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneMeterInsert *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::MeterInsert");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneIntegerAllocatorAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::IntegerAllocatorAllocate");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneIntegerAllocatorFreeIndex *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::IntegerAllocatorFreeIndex");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::CMSAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::CMSAllocate");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::CMSUpdate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::CMSUpdate");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::CMSQuery *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::CMSQuery");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::CMSIncrement *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::CMSIncrement");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::CMSCountMin *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  panic("TODO: Controller::CMSCountMin");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneCMSAllocate *node) {
  const addr_t obj                          = node->get_obj();
  const time_ns_t periodic_cleanup_interval = node->get_cleanup_internal();

  const Tofino::CountMinSketch *cms = get_unique_tofino_ds_from_obj<Tofino::CountMinSketch>(ep, obj);

  transpile_cms_decl(cms, periodic_cleanup_interval);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneCMSQuery *node) {
  coder_t &coder = get_current_coder();

  const addr_t obj                         = node->get_obj();
  const klee::ref<klee::Expr> key          = node->get_key();
  const klee::ref<klee::Expr> min_estimate = node->get_min_estimate();

  const Tofino::CountMinSketch *cms = get_unique_tofino_ds_from_obj<Tofino::CountMinSketch>(ep, obj);

  const var_t key_var          = transpile_buffer_decl_and_set(coder, cms->id + "_key", key, true);
  const var_t min_estimate_var = alloc_var("min_estimate", min_estimate, {}, NO_OPTION);

  coder.indent();
  coder << "u32 " << min_estimate_var.name << " = ";
  coder << "state->" << cms->id << ".count_min(";
  coder << key_var.name;
  coder << ");\n";

  return EPVisitor::Action::doChildren;
}

ControllerSynthesizer::var_t ControllerSynthesizer::alloc_var(const code_t &proposed_name, klee::ref<klee::Expr> expr, std::optional<addr_t> addr,
                                                              var_alloc_opt_t opt) {
  if ((opt & EXACT_NAME) && !(opt & IS_CPU_HDR_EXTRA)) {
    assert_unique_name(proposed_name);
  }

  const var_t var{
      .name      = (opt & EXACT_NAME) ? proposed_name : create_unique_name(proposed_name),
      .expr      = expr,
      .addr      = addr,
      .is_ptr    = (opt & IS_PTR) != 0,
      .is_buffer = (opt & IS_BUFFER) != 0,
      .is_header = (opt & IS_HEADER) != 0,
  };

  bool skip_alloc = ((opt & SKIP_ALLOC) != 0);

  if (opt & IS_CPU_HDR) {
    var_t cpu_hdr_var = var;

    switch (var.expr->getWidth()) {
    case 8: {
      cpu_hdr_var.name = "cpu_hdr->" + var.name;
    } break;
    case 16: {
      cpu_hdr_var.name = "bswap16(cpu_hdr->" + var.name + ")";
    } break;
    case 32: {
      cpu_hdr_var.name = "bswap32(cpu_hdr->" + var.name + ")";
    } break;
    case 64: {
      cpu_hdr_var.name = "bswap64(cpu_hdr->" + var.name + ")";
    } break;
    default: {
      panic("Unexpected width in cpu hdr (%d bits)", var.expr->getWidth());
    } break;
    }

    if (!skip_alloc) {
      vars.insert_back(cpu_hdr_var);
    }
  } else if (opt & IS_CPU_HDR_EXTRA) {
    var_t cpu_hdr_extra_var = var;

    bits_t width = var.expr->getWidth();
    assert(width % 8 == 0 && "Unexpected width (not a multiple of 8)");
    assert(width >= 8 && "Unexpected width (less than 8)");

    switch (width) {
    case 16: {
      cpu_hdr_extra_var.name = "bswap16(cpu_hdr_extra->" + var.name + ")";
    } break;
    case 32: {
      cpu_hdr_extra_var.name = "bswap32(cpu_hdr_extra->" + var.name + ")";
    } break;
    case 64: {
      cpu_hdr_extra_var.name = "bswap64(cpu_hdr_extra->" + var.name + ")";
    } break;
    default: {
      cpu_hdr_extra_var.name = "cpu_hdr_extra->" + var.name;
    } break;
    }

    if (!skip_alloc) {
      vars.insert_back(cpu_hdr_extra_var);
    }
  } else {
    if (!skip_alloc) {
      vars.insert_back(var);
    }
  }

  return var;
}

ControllerSynthesizer::code_t ControllerSynthesizer::create_unique_name(const code_t &prefix) {
  if (reserved_var_names.find(prefix) == reserved_var_names.end()) {
    reserved_var_names[prefix] = 0;
  }

  int &counter = reserved_var_names.at(prefix);

  coder_t coder;
  coder << prefix << "_" << counter;

  counter++;

  return coder.dump();
}

ControllerSynthesizer::code_t ControllerSynthesizer::assert_unique_name(const code_t &name) {
  if (reserved_var_names.find(name) == reserved_var_names.end()) {
    reserved_var_names[name] = 0;
  }

  if (reserved_var_names.at(name) > 0) {
    dbg_vars();
    panic("Name conflict in ControllerSynthesizer: '%s' already used", name.c_str());
  }

  reserved_var_names[name]++;

  return name;
}

void ControllerSynthesizer::transpile_map_table_decl(const Tofino::MapTable *map_table) {
  coder_t &state_fields = get(MARKER_STATE_FIELDS);

  const code_t name                  = assert_unique_name(map_table->id);
  const time_ns_t expiration_time    = get_expiration_time(target_ep->get_ctx());
  const time_ms_t expiration_time_ms = expiration_time / MILLION;
  bool time_aware                    = false;

  state_fields.indent();
  state_fields << "MapTable " << name << ";\n";

  synapse_data_structures_instances.push_back(name);

  coder_t member_init_list;
  member_init_list << name;
  member_init_list << "(";
  member_init_list << "\"" << name << "\",";
  member_init_list << "{";
  for (const Tofino::Table &table : map_table->tables) {
    member_init_list << "\"Ingress." << table.id << "\",";
    if (table.time_aware == Tofino::TimeAware::Yes) {
      time_aware = true;
    }
  }
  member_init_list << "}";

  if (time_aware) {
    member_init_list << ", " << expiration_time_ms << "LL";
  }

  member_init_list << ")";
  state_member_init_list.push_back(member_init_list.dump());
}

void ControllerSynthesizer::transpile_guarded_map_table_decl(const Tofino::GuardedMapTable *guarded_map_table) {
  coder_t &state_fields = get(MARKER_STATE_FIELDS);

  const code_t name                  = assert_unique_name(guarded_map_table->id);
  const time_ns_t expiration_time    = get_expiration_time(target_ep->get_ctx());
  const time_ms_t expiration_time_ms = expiration_time / MILLION;
  bool time_aware                    = false;

  state_fields.indent();
  state_fields << "GuardedMapTable " << name << ";\n";

  synapse_data_structures_instances.push_back(name);

  coder_t member_init_list;
  member_init_list << name;
  member_init_list << "(";
  member_init_list << "\"" << name << "\",";
  member_init_list << "{";
  for (const Tofino::Table &table : guarded_map_table->tables) {
    member_init_list << "\"Ingress." << table.id << "\",";
    if (table.time_aware == Tofino::TimeAware::Yes) {
      time_aware = true;
    }
  }
  member_init_list << "},";
  member_init_list << "\"Ingress." << guarded_map_table->guard.id << "\"";

  if (time_aware) {
    member_init_list << ", " << expiration_time_ms << "LL";
  }

  member_init_list << ")";
  state_member_init_list.push_back(member_init_list.dump());
}

void ControllerSynthesizer::transpile_vector_table_decl(const Tofino::VectorTable *vector_table) {
  coder_t &state_fields = get(MARKER_STATE_FIELDS);

  const code_t name = assert_unique_name(vector_table->id);

  state_fields.indent();
  state_fields << "VectorTable " << name << ";\n";

  synapse_data_structures_instances.push_back(name);

  coder_t member_init_list;
  member_init_list << name;
  member_init_list << "(";
  member_init_list << "\"" << name << "\",";
  member_init_list << "{";
  for (const Tofino::Table &table : vector_table->tables) {
    member_init_list << "\"Ingress." << table.id << "\",";
  }
  member_init_list << "}";
  member_init_list << ")";
  state_member_init_list.push_back(member_init_list.dump());
}

void ControllerSynthesizer::transpile_dchain_table_decl(const Tofino::DchainTable *dchain_table, time_ns_t expiration_time) {
  coder_t &state_fields = get(MARKER_STATE_FIELDS);

  const code_t name                  = assert_unique_name(dchain_table->id);
  const time_ms_t expiration_time_ms = expiration_time / MILLION;

  state_fields.indent();
  state_fields << "DchainTable " << name << ";\n";

  synapse_data_structures_instances.push_back(name);

  coder_t member_init_list;
  member_init_list << name;
  member_init_list << "(";
  member_init_list << "\"" << name << "\",";
  member_init_list << "{";
  for (const Tofino::Table &table : dchain_table->tables) {
    member_init_list << "\"Ingress." << table.id << "\",";
  }
  member_init_list << "}";
  member_init_list << ", " << expiration_time_ms << "LL";
  member_init_list << ")";
  state_member_init_list.push_back(member_init_list.dump());
}

void ControllerSynthesizer::transpile_vector_register_decl(const Tofino::VectorRegister *vector_register) {
  coder_t &state_fields = get(MARKER_STATE_FIELDS);

  const code_t name = assert_unique_name(vector_register->id);

  state_fields.indent();
  state_fields << "VectorRegister " << name << ";\n";

  synapse_data_structures_instances.push_back(name);

  coder_t member_init_list;
  member_init_list << name;
  member_init_list << "(";
  member_init_list << "\"" << name << "\",";
  member_init_list << "{";
  for (const Tofino::Register &reg : vector_register->regs) {
    member_init_list << "\"Ingress." << reg.id << "\",";
  }
  member_init_list << "}";
  member_init_list << ")";
  state_member_init_list.push_back(member_init_list.dump());
}

void ControllerSynthesizer::transpile_hh_table_decl(const Tofino::HHTable *hh_table, time_ns_t expiration_time) {
  coder_t &state_fields = get(MARKER_STATE_FIELDS);

  const code_t name                  = assert_unique_name(hh_table->id);
  const time_ms_t expiration_time_ms = expiration_time / MILLION;

  state_fields.indent();
  state_fields << "HHTable " << name << ";\n";

  synapse_data_structures_instances.push_back(name);

  coder_t member_init_list;
  member_init_list << name;
  member_init_list << "(";
  member_init_list << "\"" << name << "\",";
  member_init_list << "{";
  for (const Tofino::Table &table : hh_table->tables) {
    member_init_list << "\"Ingress." << table.id << "\", ";
  }
  member_init_list << "}";
  member_init_list << ", \"Ingress." << hh_table->cached_counters.id << "\"";
  member_init_list << ", {";
  for (const Tofino::Register &cms_row : hh_table->count_min_sketch) {
    member_init_list << "\"Ingress." << cms_row.id << "\", ";
  }
  member_init_list << "}";
  member_init_list << ", \"Ingress." << hh_table->threshold.id << "\"";
  member_init_list << ", \"IngressDeparser." << hh_table->digest.id << "\"";
  member_init_list << ", " << expiration_time_ms << "LL";
  member_init_list << ")";
  state_member_init_list.push_back(member_init_list.dump());
}

void ControllerSynthesizer::transpile_cms_decl(const Tofino::CountMinSketch *cms, time_ns_t periodic_cleanup_interval) {
  coder_t &state_fields = get(MARKER_STATE_FIELDS);

  const code_t name                            = assert_unique_name(cms->id);
  const time_ms_t periodic_cleanup_interval_ms = periodic_cleanup_interval / MILLION;

  state_fields.indent();
  state_fields << "CountMinSketch " << name << ";\n";

  synapse_data_structures_instances.push_back(name);

  coder_t member_init_list;
  member_init_list << name;
  member_init_list << "(";
  member_init_list << "\"" << name << "\",";
  member_init_list << "{";
  for (const Tofino::Register &row : cms->rows) {
    member_init_list << "\"Ingress." << row.id << "\", ";
  }
  member_init_list << "}";
  member_init_list << ", " << periodic_cleanup_interval_ms << "LL";
  member_init_list << ")";
  state_member_init_list.push_back(member_init_list.dump());
}

ControllerSynthesizer::var_t ControllerSynthesizer::transpile_buffer_decl_and_set(coder_t &coder, const code_t &proposed_name,
                                                                                  klee::ref<klee::Expr> expr, bool skip_alloc) {
  std::vector<code_t> bytes;
  for (klee::ref<klee::Expr> byte : LibCore::bytes_in_expr(expr, true)) {
    bytes.push_back(transpiler.transpile(byte));
  }

  const var_t var    = alloc_var(proposed_name, expr, {}, IS_BUFFER | (skip_alloc ? SKIP_ALLOC : NO_OPTION));
  const bytes_t size = expr->getWidth() / 8;
  assert(size == bytes.size() && "Size mismatch");

  coder.indent();
  coder << "buffer_t " << var.name << "(" << size << ");\n";
  for (bytes_t i = 0; i < size; i++) {
    coder.indent();
    coder << var.name << "[" << i << "] = " << bytes[i] << ";\n";
  }

  return var;
}

void ControllerSynthesizer::abort_transaction(coder_t &coder) {
  coder.indent();
  coder << "result.abort_transaction = true;\n";
  coder.indent();
  coder << "cpu_hdr->trigger_dataplane_execution = 1;\n";
  coder.indent();
  coder << "return result;\n";
}

void ControllerSynthesizer::dbg_vars() const {
  std::cerr << "================= Stack ================= \n";
  for (const Stack &stack : vars.get_all()) {
    std::cerr << "------------------------------------------\n";
    for (const var_t &var : stack.get_all()) {
      if (var.is_buffer) {
        std::cerr << "[buffer]";
      }
      if (var.is_ptr) {
        std::cerr << "[ptr]";
      }
      if (var.is_header) {
        std::cerr << "[header]";
      }
      std::cerr << " ";
      std::cerr << var.name;
      std::cerr << ": ";
      std::cerr << LibCore::expr_to_string(var.expr, true);
      std::cerr << "\n";
    }
  }
  std::cerr << "======================================== \n";
}

void ControllerSynthesizer::log(const EPNode *node) const {
  std::cerr << "[ControllerSynthesizer] ";
  EPVisitor::log(node);
}

} // namespace Controller
} // namespace LibSynapse