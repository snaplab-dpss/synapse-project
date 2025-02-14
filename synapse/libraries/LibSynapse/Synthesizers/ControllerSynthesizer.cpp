#include <LibSynapse/Synthesizers/ControllerSynthesizer.h>
#include <LibSynapse/Modules/x86/x86.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

namespace {

using LibSynapse::Tofino::DS;
using LibSynapse::Tofino::DS_ID;
using LibSynapse::Tofino::TofinoContext;

constexpr const char *const MARKER_STATE                  = "STATE";
constexpr const char *const MARKER_NF_INIT                = "NF_INIT";
constexpr const char *const MARKER_NF_EXIT                = "NF_EXIT";
constexpr const char *const MARKER_NF_ARGS                = "NF_ARGS";
constexpr const char *const MARKER_NF_USER_SIGNAL_HANDLER = "NF_USER_SIGNAL_HANDLER";
constexpr const char *const MARKER_NF_PROCESS             = "NF_PROCESS";
constexpr const char *const TEMPLATE_FILENAME             = "controller.template.cpp";

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

  // If the expression has an address, then it's an array of bytes.
  // We need to check if the expression is within the range of the array.
  if (!addr.isNull()) {
    if (size > 8 && size <= 64) {
      coder << "(";
      coder << Transpiler::type_from_expr(expr);
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
    coder << ((1 << size) - 1);
  } else {
    coder << name;
    coder << " & ";
    coder << ((1ull << size) - 1);
  }

  return coder.dump();
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
        var_t out_var{.name = var.get_slice(offset, expr_size), .expr = var_slice, .addr = nullptr};
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
                      {MARKER_STATE, 1},
                      {MARKER_NF_INIT, 1},
                      {MARKER_NF_EXIT, 1},
                      {MARKER_NF_ARGS, 1},
                      {MARKER_NF_USER_SIGNAL_HANDLER, 1},
                      {MARKER_NF_PROCESS, 1},
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

ControllerSynthesizer::coder_t &ControllerSynthesizer::get(const std::string &marker) { return Synthesizer::get(marker); }

void ControllerSynthesizer::synthesize() {
  synthesize_nf_init();
  synthesize_nf_process();
  Synthesizer::dump();
}

void ControllerSynthesizer::synthesize_nf_init() {
  // coder_t &nf_init       = get(MARKER_NF_INIT);
  const LibBDD::BDD *bdd = ep->get_bdd();

  ControllerTarget controller_target;
  for (const LibBDD::Call *call_node : bdd->get_init()) {
    for (const std::unique_ptr<ModuleFactory> &factory : controller_target.module_factories) {
      std::unique_ptr<Module> module = factory->create(bdd, ep->get_ctx(), call_node);
      if (module) {
        std::cerr << "Node: " << call_node->dump(true) << "\n";
        std::cerr << "Module: " << module->get_name() << "\n";
      }
    }
  }
}

void ControllerSynthesizer::synthesize_nf_process() { EPVisitor::visit(ep); }

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
  panic("TODO: Tofino::SendToController");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::Ignore *node) {
  panic("TODO: Controller::Ignore");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::ParseHeader *node) {
  panic("TODO: Controller::ParseHeader");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::ModifyHeader *node) {
  panic("TODO: Controller::ModifyHeader");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::ChecksumUpdate *node) {
  panic("TODO: Controller::ChecksumUpdate");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::If *node) {
  panic("TODO: Controller::If");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::Then *node) {
  panic("TODO: Controller::Then");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::Else *node) {
  panic("TODO: Controller::Else");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::Forward *node) {
  panic("TODO: Controller::Forward");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::Broadcast *node) {
  panic("TODO: Controller::Broadcast");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::Drop *node) {
  panic("TODO: Controller::Drop");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TableLookup *node) {
  panic("TODO: Controller::TableLookup");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TableUpdate *node) {
  panic("TODO: Controller::TableUpdate");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TableDelete *node) {
  panic("TODO: Controller::TableDelete");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DchainAllocateNewIndex *node) {
  panic("TODO: Controller::DchainAllocateNewIndex");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DchainRejuvenateIndex *node) {
  panic("TODO: Controller::DchainRejuvenateIndex");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DchainIsIndexAllocated *node) {
  panic("TODO: Controller::DchainIsIndexAllocated");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::DchainFreeIndex *node) {
  panic("TODO: Controller::DchainFreeIndex");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::VectorRead *node) {
  panic("TODO: Controller::VectorRead");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::VectorWrite *node) {
  panic("TODO: Controller::VectorWrite");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::MapGet *node) {
  panic("TODO: Controller::MapGet");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::MapPut *node) {
  panic("TODO: Controller::MapPut");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::MapErase *node) {
  panic("TODO: Controller::MapErase");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::ChtFindBackend *node) {
  panic("TODO: Controller::ChtFindBackend");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::HashObj *node) {
  panic("TODO: Controller::HashObj");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::VectorRegisterLookup *node) {
  panic("TODO: Controller::VectorRegisterLookup");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::VectorRegisterUpdate *node) {
  panic("TODO: Controller::VectorRegisterUpdate");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::FCFSCachedTableRead *node) {
  panic("TODO: Controller::FCFSCachedTableRead");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::FCFSCachedTableWrite *node) {
  panic("TODO: Controller::FCFSCachedTableWrite");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::FCFSCachedTableDelete *node) {
  panic("TODO: Controller::FCFSCachedTableDelete");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::HHTableRead *node) {
  panic("TODO: Controller::HHTableRead");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::HHTableConditionalUpdate *node) {
  panic("TODO: Controller::HHTableConditionalUpdate");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::HHTableUpdate *node) {
  panic("TODO: Controller::HHTableUpdate");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::HHTableDelete *node) {
  panic("TODO: Controller::HHTableDelete");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketIsTracing *node) {
  panic("TODO: Controller::TokenBucketIsTracing");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketTrace *node) {
  panic("TODO: Controller::TokenBucketTrace");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketUpdateAndCheck *node) {
  panic("TODO: Controller::TokenBucketUpdateAndCheck");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketExpire *node) {
  panic("TODO: Controller::TokenBucketExpire");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::MeterInsert *node) {
  panic("TODO: Controller::MeterInsert");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::IntegerAllocatorFreeIndex *node) {
  panic("TODO: Controller::IntegerAllocatorFreeIndex");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::CMSUpdate *node) {
  panic("TODO: Controller::CMSUpdate");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::CMSQuery *node) {
  panic("TODO: Controller::CMSQuery");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::CMSIncrement *node) {
  panic("TODO: Controller::CMSIncrement");
}

EPVisitor::Action ControllerSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Controller::CMSCountMin *node) {
  panic("TODO: Controller::CMSCountMin");
}

ControllerSynthesizer::var_t ControllerSynthesizer::alloc_var(const code_t &proposed_name, klee::ref<klee::Expr> expr,
                                                              klee::ref<klee::Expr> addr) {
  const code_t name = create_unique_name(proposed_name);
  const var_t var{.name = name, .expr = expr, .addr = addr};
  vars.insert_back(var);
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