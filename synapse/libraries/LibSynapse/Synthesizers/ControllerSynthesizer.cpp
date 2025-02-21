#include <LibSynapse/Synthesizers/ControllerSynthesizer.h>
#include <LibSynapse/Modules/x86/x86.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

namespace {

using LibSynapse::Tofino::DS;
using LibSynapse::Tofino::DS_ID;
using LibSynapse::Tofino::TNA;
using LibSynapse::Tofino::TofinoContext;
using LibSynapse::Tofino::TofinoPort;

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
  for (const DS *ds : tofino_ctx->get_ds(obj)) {
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
  for (const DS *ds : tofino_ctx->get_ds(obj)) {
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
  const DS *ds                    = tofino_ctx->get_ds_from_id(id);
  assert(ds && "DS not found");
  return dynamic_cast<const T *>(ds);
}

} // namespace

ControllerSynthesizer::Transpiler::Transpiler(const ControllerSynthesizer *_synthesizer) : synthesizer(_synthesizer) {}

ControllerSynthesizer::code_t ControllerSynthesizer::Transpiler::transpile(klee::ref<klee::Expr> expr) {
  std::cerr << "Transpiling " << LibCore::expr_to_string(expr, false) << "\n";

  coders.emplace();
  coder_t &coder = coders.top();

  expr = LibCore::simplify(expr);

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
    panic("Unknown type (size=%u)\n", size);
  }

  return type;
}

ControllerSynthesizer::code_t ControllerSynthesizer::Transpiler::type_from_expr(klee::ref<klee::Expr> expr) {
  return type_from_size(expr->getWidth());
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitRead(const klee::ReadExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ReadExpr *>(&e);

  coder_t &coder = coders.top();

  if (std::optional<ControllerSynthesizer::var_t> var = synthesizer->vars.get(expr)) {
    coder << var->name;
    return Action::skipChildren();
  }

  std::cerr << LibCore::expr_to_string(expr) << "\n";
  synthesizer->dbg_vars();

  panic("TODO: visitRead");
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

  if (std::optional<ControllerSynthesizer::var_t> var = synthesizer->vars.get(expr)) {
    coder << var->name;
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

  if (std::optional<ControllerSynthesizer::var_t> var = synthesizer->vars.get(expr)) {
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
  coder << transpile(arg);
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

  coder << transpile(lhs);
  coder << " + ";
  coder << transpile(rhs);

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitSub(const klee::SubExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " - ";
  coder << transpile(rhs);

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

  coder << transpile(lhs);
  coder << " & ";
  coder << transpile(rhs);

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitOr(const klee::OrExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " | ";
  coder << transpile(rhs);

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

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " == ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

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

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " != ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitUlt(const klee::UltExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " < ";
  coder << transpile(rhs);

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitUle(const klee::UleExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " <= ";
  coder << transpile(rhs);

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitUgt(const klee::UgtExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " > ";
  coder << transpile(rhs);

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitUge(const klee::UgeExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " >= ";
  coder << transpile(rhs);

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitSlt(const klee::SltExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " < ";
  coder << transpile(rhs);

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitSle(const klee::SleExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " <= ";
  coder << transpile(rhs);

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitSgt(const klee::SgtExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " > ";
  coder << transpile(rhs);

  return Action::skipChildren();
}

klee::ExprVisitor::Action ControllerSynthesizer::Transpiler::visitSge(const klee::SgeExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << transpile(lhs);
  coder << " >= ";
  coder << transpile(rhs);

  return Action::skipChildren();
}

ControllerSynthesizer::code_t ControllerSynthesizer::var_t::get_slice(bits_t offset, bits_t size) const {
  assert(offset + size <= expr->getWidth() && "Out of bounds");

  coder_t coder;

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

std::optional<ControllerSynthesizer::var_t> ControllerSynthesizer::Stack::get(klee::ref<klee::Expr> expr) const {
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
        var_t out_var{.name = var.get_slice(offset, expr_size), .expr = var_slice};
        return out_var;
      }
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

std::optional<ControllerSynthesizer::var_t> ControllerSynthesizer::Stacks::get(klee::ref<klee::Expr> expr) const {
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

  return std::nullopt;
}

void ControllerSynthesizer::Stacks::clear() { stacks.clear(); }

std::vector<ControllerSynthesizer::Stack> ControllerSynthesizer::Stacks::get_all() const { return stacks; }

ControllerSynthesizer::ControllerSynthesizer(const EP *_ep, std::ostream &_out, const LibBDD::BDD *bdd)
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
                  _out),
      ep(_ep), transpiler(this) {
  LibCore::symbol_t now     = bdd->get_time();
  LibCore::symbol_t device  = bdd->get_device();
  LibCore::symbol_t pkt_len = bdd->get_packet_len();

  alloc_var("now", now.expr);
  alloc_var("cpu_hdr->in_port", device.expr);
  alloc_var("size", pkt_len.expr);
}

ControllerSynthesizer::coder_t &ControllerSynthesizer::get_current_coder() {
  return in_nf_init ? get(MARKER_NF_INIT) : get(MARKER_NF_PROCESS);
}

ControllerSynthesizer::coder_t &ControllerSynthesizer::get(const std::string &marker) { return Synthesizer::get(marker); }

void ControllerSynthesizer::synthesize() {
  synthesize_nf_init();
  synthesize_nf_process();
  synthesize_state_member_init_list();
  Synthesizer::dump();
}

void ControllerSynthesizer::synthesize_nf_init() {
  coder_t &nf_init       = get(MARKER_NF_INIT);
  const LibBDD::BDD *bdd = ep->get_bdd();

  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  const TNA &tna                  = tofino_ctx->get_tna();

  for (const TofinoPort &port : tna.get_ports()) {
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
      std::unique_ptr<Module> module = factory->create(bdd, ep->get_ctx(), call_node);
      if (module) {
        candidate_modules.push_back(std::move(module));
      }
    }

    if (candidate_modules.empty()) {
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

    module->visit(*this, ep, nullptr);
  }
}

void ControllerSynthesizer::synthesize_nf_process() {
  change_to_process_coder();
  EPVisitor::visit(ep);
}

void ControllerSynthesizer::synthesize_state_member_init_list() {
  coder_t &coder = get(MARKER_STATE_MEMBER_INIT_LIST);

  if (state_member_init_list.empty()) {
    return;
  }

  for (size_t i = 0; i < state_member_init_list.size(); i++) {
    coder << ",\n";
    coder.indent();
    coder << state_member_init_list[i];
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

  const EPNode *next_node = children[0];
  ep_node_id_t code_path  = ep_node->get_id();
  code_paths.push_back(code_path);

  const LibCore::Symbols &symbols = node->get_symbols();
  for (const LibCore::symbol_t &symbol : symbols.get()) {
    bytes_t width = symbol.expr->getWidth();

    assert(width % 8 == 0 && "Unexpected width (not a multiple of 8)");
    assert(width >= 8 && "Unexpected width (less than 8)");

    var_t var = alloc_cpu_hdr_extra_var(symbol.name, symbol.expr);

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
  coder << "(SWAP_ENDIAN_16(cpu_hdr->code_path) == " << ep_node->get_id() << ") {\n";

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
  coder.indent();
  coder << "// TODO: Controller::ParseHeader\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::ModifyHeader *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::ModifyHeader\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::ChecksumUpdate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::ChecksumUpdate\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::If *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::If\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::Then *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::Then\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::Else *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::Else\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::Forward *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::Forward\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::Broadcast *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::Broadcast\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::Drop *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::Drop\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TableAllocate *node) {
  const addr_t obj                                 = node->get_obj();
  std::unordered_set<const Tofino::Table *> tables = get_tofino_ds_from_obj<Tofino::Table>(ep, obj);

  for (const Tofino::Table *table : tables) {
    transpile_table_decl(table);
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TableLookup *node) {
  coder_t &coder = get_current_coder();

  const addr_t obj                                 = node->get_obj();
  const std::vector<klee::ref<klee::Expr>> &keys   = node->get_keys();
  const std::vector<klee::ref<klee::Expr>> &values = node->get_values();
  std::optional<LibCore::symbol_t> found           = node->get_found();

  std::unordered_set<const Tofino::Table *> tables = get_tofino_ds_from_obj<Tofino::Table>(ep, obj);
  const Tofino::Table *table                       = *tables.begin();

  var_t key_var = alloc_fields("table_key", keys);

  coder.indent();
  coder << "fields_t<" << keys.size() << "> " << key_var.name << ";\n";
  for (size_t i = 0; i < keys.size(); i++) {
    coder.indent();
    coder << key_var.name << "[" << i << "] = ";
    coder << transpiler.transpile(keys[i]);
    coder << ";\n";
  }

  var_t value_var = alloc_fields("table_value", values);
  coder.indent();
  coder << "fields_t<" << values.size() << "> " << value_var.name << ";\n";

  coder.indent();
  if (found) {
    var_t found_var = alloc_var("table_entry_found", found->expr);
    coder << transpiler.type_from_expr(found->expr);
    coder << found_var.name;
    coder << " = ";
  }
  coder << "state->" << table->id;
  coder << ".get(";
  coder << key_var.name << ", ";
  coder << value_var.name;
  coder << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TableUpdate *node) {
  coder_t &coder = get_current_coder();

  const addr_t obj                                 = node->get_obj();
  const std::vector<klee::ref<klee::Expr>> &keys   = node->get_keys();
  const std::vector<klee::ref<klee::Expr>> &values = node->get_values();

  std::unordered_set<const Tofino::Table *> tables = get_tofino_ds_from_obj<Tofino::Table>(ep, obj);

  for (const Tofino::Table *table : tables) {
    var_t key_var   = alloc_fields("table_key", keys);
    var_t value_var = alloc_fields("table_value", values);

    coder.indent();
    coder << "fields_t<" << keys.size() << "> " << key_var.name << ";\n";
    for (size_t i = 0; i < keys.size(); i++) {
      coder.indent();
      coder << key_var.name << "[" << i << "] = ";
      coder << transpiler.transpile(keys[i]);
      coder << ";\n";
    }

    coder.indent();
    coder << "fields_t<" << values.size() << "> " << value_var.name << ";\n";
    for (size_t i = 0; i < values.size(); i++) {
      coder.indent();
      coder << value_var.name << "[" << i << "] = ";
      coder << transpiler.transpile(values[i]);
      coder << ";\n";
    }

    coder.indent();
    coder << "state->" << table->id;
    coder << ".put(";
    coder << key_var.name << ", ";
    coder << value_var.name;
    coder << ");\n";
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TableDelete *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::TableDelete\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DchainAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::DchainAllocate\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DchainAllocateNewIndex *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::DchainAllocateNewIndex\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DchainRejuvenateIndex *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::DchainRejuvenateIndex\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DchainIsIndexAllocated *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::DchainIsIndexAllocated\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DchainFreeIndex *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::DchainFreeIndex\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::VectorAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::VectorAllocate\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::VectorRead *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::VectorRead\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::VectorWrite *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::VectorWrite\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::MapAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::MapAllocate\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::MapGet *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::MapGet\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::MapPut *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::MapPut\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::MapErase *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::MapErase\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::ChtAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::ChtAllocate\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::ChtFindBackend *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::ChtFindBackend\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::HashObj *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::HashObj\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::VectorRegisterAllocate *node) {
  addr_t obj = node->get_obj();

  const Tofino::VectorRegister *vector_register = get_unique_tofino_ds_from_obj<Tofino::VectorRegister>(ep, obj);
  transpile_vector_register_decl(vector_register);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::VectorRegisterLookup *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::VectorRegisterLookup\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::VectorRegisterUpdate *node) {
  coder_t &coder = get_current_coder();

  const addr_t obj                                      = node->get_obj();
  const klee::ref<klee::Expr> index                     = node->get_index();
  const klee::ref<klee::Expr> old_value                 = node->get_old_value();
  const klee::ref<klee::Expr> new_value                 = node->get_new_value();
  const std::vector<LibCore::expr_mod_t> &modifications = node->get_modifications();

  const Tofino::VectorRegister *vector_register = get_unique_tofino_ds_from_obj<Tofino::VectorRegister>(ep, obj);

  var_t vector_register_value = alloc_var("value", new_value);

  coder.indent();
  coder << "buffer_t " << vector_register_value.name << ";\n";

  coder.indent();
  coder << "state->" << vector_register->id << ".get(";
  coder << transpiler.transpile(index);
  coder << ", " << vector_register_value.name;
  coder << ");\n";

  for (const LibCore::expr_mod_t &mod : modifications) {
    bytes_t offset = mod.offset / 8;
    for (klee::ref<klee::Expr> byte_expr : LibCore::bytes_in_expr(mod.expr)) {
      coder.indent();
      coder << vector_register_value.name;
      coder << "[" << offset << "] = ";
      coder << transpiler.transpile(byte_expr);
      coder << ";\n";
      offset++;
    }
  }

  coder.indent();
  coder << "state->" << vector_register->id << ".put(";
  coder << transpiler.transpile(index);
  coder << ", " << vector_register_value.name;
  coder << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::FCFSCachedTableAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::FCFSCachedTableAllocate\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::FCFSCachedTableRead *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::FCFSCachedTableRead\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::FCFSCachedTableWrite *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::FCFSCachedTableWrite\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::FCFSCachedTableDelete *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::FCFSCachedTableDelete\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::HHTableAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::HHTableAllocate\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::HHTableRead *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::HHTableRead\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::HHTableConditionalUpdate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::HHTableConditionalUpdate\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::HHTableUpdate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::HHTableUpdate\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::HHTableDelete *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::HHTableDelete\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::TokenBucketAllocate\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketIsTracing *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::TokenBucketIsTracing\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketTrace *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::TokenBucketTrace\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketUpdateAndCheck *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::TokenBucketUpdateAndCheck\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketExpire *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::TokenBucketExpire\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::MeterAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::MeterAllocate\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::MeterInsert *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::MeterInsert\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::IntegerAllocatorAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::IntegerAllocatorAllocate\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::IntegerAllocatorFreeIndex *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::IntegerAllocatorFreeIndex\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::CMSAllocate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::CMSAllocate\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::CMSUpdate *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::CMSUpdate\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::CMSQuery *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::CMSQuery\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::CMSIncrement *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::CMSIncrement\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::CMSCountMin *node) {
  coder_t &coder = get_current_coder();
  coder.indent();
  coder << "// TODO: Controller::CMSCountMin\n";
  return EPVisitor::Action::doChildren;
}

ControllerSynthesizer::var_t ControllerSynthesizer::alloc_var(const code_t &proposed_name, klee::ref<klee::Expr> expr) {
  const code_t name = create_unique_name(proposed_name);
  const var_t var{.name = name, .expr = expr};
  vars.insert_back(var);
  return var;
}

ControllerSynthesizer::var_t ControllerSynthesizer::alloc_cpu_hdr_extra_var(const code_t &proposed_name, klee::ref<klee::Expr> expr) {
  const code_t name = create_unique_name(proposed_name);
  const var_t var{.name = name, .expr = expr};
  var_t cpu_hdr_extra_var{.name = "cpu_hdr->" + name, .expr = expr};
  vars.insert_back(cpu_hdr_extra_var);
  return var;
}

ControllerSynthesizer::var_t ControllerSynthesizer::alloc_fields(const code_t &proposed_name,
                                                                 const std::vector<klee::ref<klee::Expr>> &fields) {
  const code_t name = create_unique_name(proposed_name);
  const var_t var{.name = name, .expr = nullptr};

  for (size_t i = 0; i < fields.size(); i++) {
    klee::ref<klee::Expr> field = fields[i];
    const var_t field_var{.name = name + "[" + std::to_string(i) + "]", .expr = field};
    vars.insert_back(field_var);
  }

  return var;
}

ControllerSynthesizer::code_t ControllerSynthesizer::create_unique_name(const code_t &prefix) {
  if (reserved_var_names.find(prefix) == reserved_var_names.end()) {
    reserved_var_names[prefix] = 0;
  }

  int &counter = reserved_var_names[prefix];

  coder_t coder;
  coder << prefix << "_" << counter;

  counter++;

  return coder.dump();
}

ControllerSynthesizer::code_t ControllerSynthesizer::assert_unique_name(const code_t &name) {
  if (reserved_var_names.find(name) == reserved_var_names.end()) {
    reserved_var_names[name] = 0;
  }

  assert(reserved_var_names.at(name) == 0 && "Name already used");
  return name;
}

void ControllerSynthesizer::transpile_table_decl(const Tofino::Table *table) {
  coder_t &state_fields = get(MARKER_STATE_FIELDS);

  const u64 nb_keys   = table->keys.size();
  const u64 nb_values = table->params.size();
  const code_t name   = assert_unique_name(table->id);

  state_fields.indent();
  state_fields << "Table<" << nb_keys << "," << nb_values << "> " << name << ";\n";

  coder_t member_init_list;
  member_init_list << name;
  member_init_list << "(";
  member_init_list << "\"Ingress\", ";
  member_init_list << "\"" << table->id << "\"";
  member_init_list << ")";
  state_member_init_list.push_back(member_init_list.dump());
}

void ControllerSynthesizer::transpile_register_decl(const Tofino::Register *reg) {
  coder_t &state_fields = get(MARKER_STATE_FIELDS);

  const code_t name = assert_unique_name(reg->id);

  state_fields.indent();
  state_fields << "PrimitiveRegister " << name << ";\n";

  coder_t member_init_list;
  member_init_list << name;
  member_init_list << "(";
  member_init_list << "\"Ingress\", ";
  member_init_list << "\"" << reg->id << "\"";
  member_init_list << ")";
  state_member_init_list.push_back(member_init_list.dump());
}

void ControllerSynthesizer::transpile_vector_register_decl(const Tofino::VectorRegister *vector_register) {
  coder_t &state_fields = get(MARKER_STATE_FIELDS);

  const code_t name = assert_unique_name(vector_register->id);

  state_fields.indent();
  state_fields << "VectorRegister " << name << ";\n";

  coder_t member_init_list;
  member_init_list << name;
  member_init_list << "(";
  member_init_list << "\"Ingress\", ";
  member_init_list << "{";
  for (const Tofino::Register &reg : vector_register->regs) {
    member_init_list << "\"" << reg.id << "\",";
  }
  member_init_list << "}";
  member_init_list << ")";
  state_member_init_list.push_back(member_init_list.dump());
}

void ControllerSynthesizer::dbg_vars() const {
  std::cerr << "================= Stack ================= \n";
  for (const Stack &stack : vars.get_all()) {
    std::cerr << "------------------------------------------\n";
    for (const var_t &var : stack.get_all()) {
      std::cerr << var.name;
      std::cerr << ": ";
      std::cerr << LibCore::expr_to_string(var.expr, true);
      std::cerr << "\n";
    }
  }
  std::cerr << "======================================== \n";
}

} // namespace Controller
} // namespace LibSynapse