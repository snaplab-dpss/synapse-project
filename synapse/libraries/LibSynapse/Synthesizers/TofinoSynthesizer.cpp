#include <LibSynapse/Synthesizers/TofinoSynthesizer.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

namespace {

constexpr const char *const MARKER_CPU_HEADER                   = "CPU_HEADER";
constexpr const char *const MARKER_RECIRC_HEADER                = "RECIRCULATION_HEADER";
constexpr const char *const MARKER_CUSTOM_HEADERS               = "CUSTOM_HEADERS";
constexpr const char *const MARKER_INGRESS_HEADERS              = "INGRESS_HEADERS";
constexpr const char *const MARKER_INGRESS_METADATA             = "INGRESS_METADATA";
constexpr const char *const MARKER_INGRESS_PARSER               = "INGRESS_PARSER";
constexpr const char *const MARKER_INGRESS_CONTROL              = "INGRESS_CONTROL";
constexpr const char *const MARKER_INGRESS_CONTROL_APPLY        = "INGRESS_CONTROL_APPLY";
constexpr const char *const MARKER_INGRESS_CONTROL_APPLY_RECIRC = "INGRESS_CONTROL_APPLY_RECIRC";
constexpr const char *const MARKER_INGRESS_DEPARSER             = "INGRESS_DEPARSER";
constexpr const char *const MARKER_EGRESS_HEADERS               = "EGRESS_HEADERS";
constexpr const char *const MARKER_EGRESS_METADATA              = "EGRESS_METADATA";
constexpr const char *const TEMPLATE_FILENAME                   = "tofino.template.p4";

template <class T> const T *get_tofino_ds(const EP *ep, DS_ID id) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  const DS *ds                    = tofino_ctx->get_ds_from_id(id);
  assert(ds && "DS not found");
  return dynamic_cast<const T *>(ds);
}

const Parser &get_tofino_parser(const EP *ep) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  const TNA &tna                  = tofino_ctx->get_tna();
  return tna.parser;
}

bool natural_compare(const std::string &a, const std::string &b) {
  size_t i = 0, j = 0;

  while (i < a.size() && j < b.size()) {
    if (std::isdigit(a[i]) && std::isdigit(b[j])) {
      size_t start_i = i, start_j = j;
      while (i < a.size() && std::isdigit(a[i]))
        ++i;
      while (j < b.size() && std::isdigit(b[j]))
        ++j;

      int num_a = std::stoi(a.substr(start_i, i - start_i));
      int num_b = std::stoi(b.substr(start_j, j - start_j));

      if (num_a != num_b)
        return num_a < num_b;
    } else {
      if (a[i] != b[j])
        return a[i] < b[j];
      ++i;
      ++j;
    }
  }

  return a.size() < b.size();
}

} // namespace

TofinoSynthesizer::Transpiler::Transpiler(TofinoSynthesizer *_synthesizer) : synthesizer(_synthesizer) {}

TofinoSynthesizer::code_t TofinoSynthesizer::Transpiler::transpile_constant(klee::ref<klee::Expr> expr) {
  assert(LibCore::is_constant(expr) && "Expected a constant expression");

  bytes_t width = expr->getWidth() / 8;

  coder_t code;
  code << width * 8 << "w0x";

  for (size_t byte = 0; byte < width; byte++) {
    klee::ref<klee::Expr> extract = LibCore::solver_toolbox.exprBuilder->Extract(expr, (width - byte - 1) * 8, 8);
    u64 byte_value                = LibCore::solver_toolbox.value_from_expr(extract);

    std::stringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0') << byte_value;
    code << ss.str();
  }

  return code.dump();
}

TofinoSynthesizer::code_t TofinoSynthesizer::Transpiler::transpile(klee::ref<klee::Expr> expr, transpiler_opt_t opt) {
  loaded_opt = opt;

  std::cerr << "Transpiling " << LibCore::expr_to_string(expr, false) << "\n";
  expr = LibCore::simplify(expr);
  std::cerr << "Simplified to " << LibCore::expr_to_string(expr, false) << "\n";

  coders.emplace();
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> endian_swap_target;

  if (LibCore::is_constant(expr)) {
    coder << transpile_constant(expr);
  } else if (LibCore::match_endian_swap_pattern(expr, endian_swap_target)) {
    bits_t size = endian_swap_target->getWidth();
    if (size == 16) {
      coder << "bswap16(" << transpile(endian_swap_target, loaded_opt) << ")";
    } else if (size == 32) {
      coder << "bswap32(" << transpile(endian_swap_target, loaded_opt) << ")";
    } else {
      panic("FIXME: incompatible endian swap size %d\n", size);
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

TofinoSynthesizer::code_t TofinoSynthesizer::Transpiler::type_from_size(bits_t size) {
  coder_t coder;
  coder << "bit<" << size << ">";
  return coder.dump();
}

TofinoSynthesizer::code_t TofinoSynthesizer::Transpiler::type_from_expr(klee::ref<klee::Expr> expr) {
  klee::Expr::Width width = expr->getWidth();
  assert(width != klee::Expr::InvalidWidth && "Invalid width");

  if (LibCore::is_conditional(expr)) {
    return "bool";
  }

  return type_from_size(width);
}

TofinoSynthesizer::code_t TofinoSynthesizer::Transpiler::transpile_literal(u64 value, bits_t size) {
  coder_t coder;
  coder << size << "w" << value;
  return coder.dump();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitRead(const klee::ReadExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ReadExpr *>(&e);

  coder_t &coder = coders.top();

  if (std::optional<TofinoSynthesizer::var_t> var = synthesizer->ingress_vars.get(expr, loaded_opt)) {
    coder << var->name;
    return Action::skipChildren();
  }

  std::cerr << LibCore::expr_to_string(expr) << "\n";
  synthesizer->dbg_vars();

  panic("TODO: visitRead");
  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitNotOptimized(const klee::NotOptimizedExpr &e) {
  panic("TODO: visitNotOptimized");
  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitSelect(const klee::SelectExpr &e) {
  panic("TODO: visitSelect");
  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitConcat(const klee::ConcatExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ConcatExpr *>(&e);
  coder_t &coder             = coders.top();

  if (std::optional<TofinoSynthesizer::var_t> var = synthesizer->ingress_vars.get(expr, loaded_opt)) {
    coder << var->name;
    return Action::skipChildren();
  }

  std::cerr << LibCore::expr_to_string(expr) << "\n";
  synthesizer->dbg_vars();

  panic("TODO: visitConcat");
  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitExtract(const klee::ExtractExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ExtractExpr *>(&e);
  coder_t &coder             = coders.top();

  if (std::optional<TofinoSynthesizer::var_t> var = synthesizer->ingress_vars.get(expr, loaded_opt)) {
    coder << var->name;
    return Action::skipChildren();
  }

  synthesizer->dbg_vars();
  panic("TODO: visitExtract: %s", LibCore::expr_to_string(expr).c_str());
  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitZExt(const klee::ZExtExpr &e) {
  klee::ref<klee::Expr> arg = e.getKid(0);
  coder_t &coder            = coders.top();

  if (std::optional<TofinoSynthesizer::var_t> var = synthesizer->ingress_vars.get(arg, loaded_opt)) {
    // HACK: hot fix for a Tofino bug related to bit slices and implicit conversions.
    // This is a temporary fix until we find a better solution.
    if (var->original_name == "meta.dev" && var->original_size == e.width) {
      coder << var->original_name;
      return Action::skipChildren();
    }
  }

  coder << "(";
  coder << type_from_size(e.width);
  coder << ")";
  coder << "(";
  coder << transpile(arg, loaded_opt);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitSExt(const klee::SExtExpr &e) {
  panic("TODO: visitSExt");
  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitAdd(const klee::AddExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " + ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitSub(const klee::SubExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " - ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitMul(const klee::MulExpr &e) {
  panic("TODO: visitMul");
  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitUDiv(const klee::UDivExpr &e) {
  panic("TODO: visitUDiv");
  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitSDiv(const klee::SDivExpr &e) {
  panic("TODO: visitSDiv");
  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitURem(const klee::URemExpr &e) {
  panic("TODO: visitURem");
  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitSRem(const klee::SRemExpr &e) {
  panic("TODO: visitSRem");
  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitNot(const klee::NotExpr &e) {
  panic("TODO: visitNot");
  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitAnd(const klee::AndExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " & ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitOr(const klee::OrExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " | ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitXor(const klee::XorExpr &e) {
  panic("TODO: visitXor");
  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitShl(const klee::ShlExpr &e) {
  panic("TODO: visitShl");
  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitLShr(const klee::LShrExpr &e) {
  panic("TODO: visitLShr");
  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitAShr(const klee::AShrExpr &e) {
  panic("TODO: visitAShr");
  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitEq(const klee::EqExpr &e) {
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

  if (LibCore::is_constant(const_expr)) {
    std::optional<TofinoSynthesizer::var_t> var = synthesizer->ingress_vars.get(var_expr, loaded_opt);
    if (var && var->is_bool()) {
      u64 value = LibCore::solver_toolbox.value_from_expr(const_expr);
      if (value == 0) {
        coder << "!";
      }
      coder << var->name;
      return Action::skipChildren();
    }
  }

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " == ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitNe(const klee::NeExpr &e) {
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

  if (LibCore::is_constant(const_expr)) {
    std::optional<TofinoSynthesizer::var_t> var = synthesizer->ingress_vars.get(var_expr);
    if (var && var->is_bool()) {
      u64 value = LibCore::solver_toolbox.value_from_expr(const_expr);
      if (value != 0) {
        coder << "!";
      }
      coder << var->name;
      return Action::skipChildren();
    }
  }

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " != ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitUlt(const klee::UltExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " < ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitUle(const klee::UleExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " <= ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitUgt(const klee::UgtExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " > ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitUge(const klee::UgeExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " >= ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitSlt(const klee::SltExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " < ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitSle(const klee::SleExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " <= ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitSgt(const klee::SgtExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " > ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitSge(const klee::SgeExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt) << ")";
  coder << " >= ";
  coder << "(" << transpile(rhs, loaded_opt) << ")";

  return Action::skipChildren();
}

TofinoSynthesizer::code_t TofinoSynthesizer::get_parser_state_name(const ParserState *state, bool state_init) {
  coder_t coder;

  if (state_init) {
    coder << "parser_init";
    return coder.dump();
  }

  coder << "parser_";
  bool first = true;
  for (LibBDD::node_id_t id : state->ids) {
    if (!first) {
      coder << "_";
    } else {
      first = false;
    }
    coder << id;
  }

  return coder.dump();
}

TofinoSynthesizer::code_t TofinoSynthesizer::build_register_action_name(const EPNode *node, const Register *reg,
                                                                        RegisterActionType action) const {
  coder_t coder;
  coder << reg->id;
  coder << "_";
  switch (action) {
  case RegisterActionType::READ:
    coder << "read";
    break;
  case RegisterActionType::WRITE:
    coder << "write";
    break;
  case RegisterActionType::SWAP:
    coder << "update";
    break;
  }
  coder << "_";
  coder << node->get_id();
  return coder.dump();
}

void TofinoSynthesizer::transpile_action_decl(coder_t &coder, const std::string action_name,
                                              const std::vector<klee::ref<klee::Expr>> &params, bool params_are_buffers) {
  assert(!params.empty() && "Empty action");

  std::vector<var_t> params_vars;

  for (klee::ref<klee::Expr> param : params) {
    const std::string param_name = action_name + "_param";
    const var_t param_value_var  = alloc_var(param_name, param, GLOBAL | (params_are_buffers ? BUFFER : 0));

    params_vars.push_back(param_value_var);
    param_value_var.declare(coder, TofinoSynthesizer::Transpiler::transpile_literal(0, param->getWidth()));
  }

  coder.indent();
  coder << "action " << action_name << "(";

  for (size_t i = 0; i < params.size(); i++) {
    const klee::ref<klee::Expr> param = params[i];

    if (i != 0) {
      coder << ", ";
    }

    coder << TofinoSynthesizer::Transpiler::type_from_expr(param);
    coder << " ";
    coder << "_" << params_vars[i].name;
  }

  coder << ") {\n";

  coder.inc();

  for (const var_t &param : params_vars) {
    coder.indent();
    coder << param.name;
    coder << " = ";
    coder << "_" << param.name;
    coder << ";\n";
  }

  coder.dec();
  coder.indent();
  coder << "}\n";

  coder << "\n";
}

void TofinoSynthesizer::transpile_table_decl(coder_t &coder, const Table *table, const std::vector<klee::ref<klee::Expr>> &keys,
                                             const std::vector<klee::ref<klee::Expr>> &values, bool values_are_buffers,
                                             std::vector<var_t> &keys_vars) {
  const code_t action_name = table->id + "_get_value";
  if (!values.empty()) {
    transpile_action_decl(coder, action_name, values, values_are_buffers);
  }

  for (auto it = keys.rbegin(); it != keys.rend(); it++) {
    klee::ref<klee::Expr> key  = *it;
    const std::string key_name = table->id + "_key";
    const var_t key_var        = alloc_var(key_name, key, GLOBAL | SKIP_STACK_ALLOC);
    keys_vars.push_back(key_var);
  }

  for (const var_t &key : keys_vars) {
    key.declare(coder, TofinoSynthesizer::Transpiler::transpile_literal(0, key.expr->getWidth()));
  }

  coder.indent();
  coder << "table " << table->id << " {\n";
  coder.inc();

  coder.indent();
  coder << "key = {\n";
  coder.inc();

  for (const var_t &key : keys_vars) {
    coder.indent();
    coder << key.name << ": exact;\n";
  }

  coder.dec();
  coder.indent();
  coder << "}\n";

  coder.indent();
  coder << "actions = {";

  if (!values.empty()) {
    coder << "\n";
    coder.inc();

    coder.indent();
    coder << action_name << ";\n";

    coder.dec();
    coder.indent();
  } else {
    coder << "\n";
    coder.inc();

    coder.indent();
    coder << " NoAction;\n";

    coder.dec();
    coder.indent();
  }

  coder << "}\n";

  coder.indent();
  coder << "size = " << table->num_entries << ";\n";

  if (table->time_aware == TimeAware::Yes) {
    coder.indent();
    coder << "idle_timeout = true;\n";
  }

  coder.dec();
  coder.indent();
  coder << "}\n";
}

void TofinoSynthesizer::transpile_lpm_decl(coder_t &coder, const LPM *lpm, klee::ref<klee::Expr> addr, klee::ref<klee::Expr> device) {
  const code_t action_name = lpm->id + "_get_device";
  transpile_action_decl(coder, action_name, {device}, false);

  const std::string key_name = "ipv4_addr";
  const var_t key_var        = alloc_var(key_name, addr, GLOBAL);

  key_var.declare(coder, TofinoSynthesizer::Transpiler::transpile_literal(0, key_var.expr->getWidth()));

  coder.indent();
  coder << "table " << lpm->id << " {\n";
  coder.inc();

  coder.indent();
  coder << "key = {\n";
  coder.inc();

  coder.indent();
  coder << key_var.name << ": ternary;\n";

  coder.dec();
  coder.indent();
  coder << "}\n";

  coder.indent();
  coder << "actions = { " << action_name << "; }\n";

  coder.indent();
  coder << "size = " << lpm->capacity << ";\n";

  coder.dec();
  coder.indent();
  coder << "}\n";
}

void TofinoSynthesizer::transpile_register_decl(coder_t &coder, const Register *reg, klee::ref<klee::Expr> index,
                                                klee::ref<klee::Expr> value) {
  // * Template:
  // Register<{VALUE_WIDTH}, _>({CAPACITY}, {INIT_VALUE}) {NAME};
  // * Example:
  // Register<bit<32>, _>(1024, 0) my_register;

  const u64 init_value = 0;

  coder.indent();
  coder << "Register<";
  coder << TofinoSynthesizer::Transpiler::type_from_size(reg->value_size);
  coder << ",_>";

  coder << "(";
  coder << reg->num_entries;
  coder << ", ";
  coder << init_value;
  coder << ")";

  coder << " ";

  coder << reg->id;
  coder << ";\n";
}

void TofinoSynthesizer::transpile_register_read_action_decl(coder_t &coder, const Register *reg, const code_t &name) {
  // * Example:
  // RegisterAction<value_t, hash_t, bool>(reg) reg_read = {
  // 		void apply(inout value_t value, out value_t out_value) {
  // 			out_value = value;
  // 		}
  // 	};

  code_t value_type = TofinoSynthesizer::Transpiler::type_from_size(reg->value_size);
  code_t index_type = TofinoSynthesizer::Transpiler::type_from_size(reg->index_size);

  coder.indent();
  coder << "RegisterAction<";
  coder << value_type;
  coder << ", ";
  coder << index_type;
  coder << ", ";
  coder << value_type;
  coder << ">";

  coder << "(";
  coder << reg->id;
  coder << ")";

  coder << " ";
  coder << name;
  coder << " = {\n";

  coder.inc();

  coder.indent();
  coder << "void apply(";
  coder << "inout " << value_type << " value";
  coder << ", ";
  coder << "out " << value_type << " out_value) {\n";

  coder.inc();
  coder.indent();
  coder << "out_value = value;\n";

  coder.dec();
  coder.indent();
  coder << "}\n";

  coder.dec();
  coder.indent();
  coder << "};\n";
}

void TofinoSynthesizer::transpile_register_write_action_decl(coder_t &coder, const Register *reg, const code_t &name,
                                                             const var_t &write_value) {
  // * Example:
  // RegisterAction<value_t, hash_t, void>(reg) reg_write = {
  // 		void apply(inout value_t value) {
  // 			value = some_external_var;
  // 		}
  // 	};

  code_t value_type = TofinoSynthesizer::Transpiler::type_from_size(reg->value_size);
  code_t index_type = TofinoSynthesizer::Transpiler::type_from_size(reg->index_size);

  coder.indent();
  coder << "RegisterAction<";
  coder << value_type;
  coder << ", ";
  coder << index_type;
  coder << ", ";
  coder << "void";
  coder << ">";

  coder << "(";
  coder << reg->id;
  coder << ")";

  coder << " ";
  coder << name;
  coder << " = {\n";

  coder.inc();

  coder.indent();
  coder << "void apply(";
  coder << "inout " << value_type << " value";
  coder << ") {\n";

  coder.inc();
  coder.indent();
  coder << "value = " << write_value.name << ";\n";

  coder.dec();
  coder.indent();
  coder << "}\n";

  coder.dec();
  coder.indent();
  coder << "};\n";
}

void TofinoSynthesizer::transpile_fcfs_cached_table_decl(coder_t &coder, const FCFSCachedTable *fcfs_cached_table,
                                                         const klee::ref<klee::Expr> key, const klee::ref<klee::Expr> value) {
  std::vector<var_t> keys_vars;
  for (const Table &table : fcfs_cached_table->tables) {
    transpile_table_decl(coder, &table, {key}, {value}, true, keys_vars);
  }

  std::cerr << coder.dump();
  dbg_pause();

  panic("TODO: transpile_fcfs_cached_table_decl");
}

TofinoSynthesizer::code_t TofinoSynthesizer::var_t::get_type() const {
  return force_bool ? "bool" : TofinoSynthesizer::Transpiler::type_from_expr(expr);
}

bool TofinoSynthesizer::var_t::is_bool() const { return force_bool || LibCore::is_conditional(expr); }

void TofinoSynthesizer::var_t::declare(coder_t &coder, std::optional<code_t> assignment) const {
  coder.indent();
  coder << get_type();
  coder << " ";
  coder << get_stem();
  if (assignment.has_value()) {
    coder << " = ";
    coder << *assignment;
  }
  coder << ";\n";
}

TofinoSynthesizer::var_t TofinoSynthesizer::var_t::get_slice(bits_t offset, bits_t size, transpiler_opt_t opt) const {
  assert(offset + size <= expr->getWidth() && "Invalid slice");

  bits_t lo;
  bits_t hi;

  if (!is_header_field && !is_buffer) {
    lo = offset;
    hi = offset + size - 1;
  } else {
    bits_t expr_width = expr->getWidth();
    lo                = expr_width - (offset + size);
    hi                = expr_width - offset - 1;
  }

  code_t slice_name                = name + "[" + std::to_string(hi) + ":" + std::to_string(lo) + "]";
  klee::ref<klee::Expr> slice_expr = LibCore::solver_toolbox.exprBuilder->Extract(expr, offset, size);

  return var_t(original_name, original_expr, original_size, slice_name, slice_expr, size, force_bool, is_header_field, is_buffer);
}

TofinoSynthesizer::code_t TofinoSynthesizer::var_t::get_stem() const {
  size_t pos = name.find_last_of('.');
  if (pos == std::string::npos) {
    return name;
  }
  return name.substr(pos + 1);
}

std::string TofinoSynthesizer::var_t::to_string() const {
  std::stringstream ss;
  ss << "var_t{";
  ss << "oname: " << original_name << ", ";
  ss << "oexpr: " << LibCore::expr_to_string(original_expr, true) << ", ";
  ss << "osize: " << original_size << ", ";
  ss << "name: " << name << ", ";
  ss << "expr: " << LibCore::expr_to_string(expr, true) << ", ";
  ss << "size: " << size << ", ";
  ss << "force_bool: " << force_bool << ", ";
  ss << "is_header_field: " << is_header_field << ", ";
  ss << "is_buffer: " << is_buffer;
  ss << "}";
  return ss.str();
}

void TofinoSynthesizer::Stack::push(const var_t &var) {
  if (names.find(var.name) == names.end()) {
    frames.push_back(var);
    names.insert(var.name);
  }
}

void TofinoSynthesizer::Stack::push(const Stack &stack) {
  for (const var_t &var : stack.frames) {
    push(var);
  }
}

void TofinoSynthesizer::Stack::clear() {
  frames.clear();
  names.clear();
}

std::optional<TofinoSynthesizer::var_t> TofinoSynthesizer::Stack::get_exact(klee::ref<klee::Expr> expr) const {
  for (auto var_it = frames.rbegin(); var_it != frames.rend(); var_it++) {
    const var_t &var = *var_it;
    if (LibCore::solver_toolbox.are_exprs_always_equal(var.expr, expr)) {
      return var;
    }
  }

  return std::nullopt;
}

std::optional<TofinoSynthesizer::var_t> TofinoSynthesizer::Stack::get_exact_hdr(klee::ref<klee::Expr> expr) const {
  for (auto var_it = frames.rbegin(); var_it != frames.rend(); var_it++) {
    const var_t &var = *var_it;
    if (var.is_header_field && LibCore::solver_toolbox.are_exprs_always_equal(var.expr, expr)) {
      return var;
    }
  }

  return std::nullopt;
}

std::optional<TofinoSynthesizer::var_t> TofinoSynthesizer::Stack::get(klee::ref<klee::Expr> expr, transpiler_opt_t opt) const {
  if (std::optional<var_t> var = get_exact(expr)) {
    if (var->is_header_field && (opt & TRANSPILER_OPT_SWAP_HDR_ENDIANNESS)) {
      if (var->size == 16) {
        var->name = "bswap16(" + var->name + ")";
      } else if (var->size == 32) {
        var->name = "bswap32(" + var->name + ")";
      } else {
        panic("FIXME: incompatible endian swap size %d\n", var->size);
      }
    }

    return var;
  }

  for (auto var_it = frames.rbegin(); var_it != frames.rend(); var_it++) {
    const var_t &var = *var_it;

    const bits_t expr_size = expr->getWidth();
    const bits_t var_size  = var.size;

    if (expr_size > var_size) {
      continue;
    }

    for (bits_t offset = 0; offset + expr_size <= var_size; offset += 8) {
      klee::ref<klee::Expr> var_slice = LibCore::solver_toolbox.exprBuilder->Extract(var.expr, offset, expr_size);

      if (LibCore::solver_toolbox.are_exprs_always_equal(var_slice, expr)) {
        var_t slice = var.get_slice(offset, expr_size, opt);

        if (slice.is_header_field && (opt & TRANSPILER_OPT_SWAP_HDR_ENDIANNESS)) {
          if (slice.size == 16) {
            slice.name = "bswap16(" + slice.name + ")";
          } else if (slice.size == 32) {
            slice.name = "bswap32(" + slice.name + ")";
          } else {
            panic("FIXME: incompatible endian swap size %d\n", slice.size);
          }
        }

        return slice;
      }
    }
  }

  return std::nullopt;
}

std::optional<TofinoSynthesizer::var_t> TofinoSynthesizer::Stack::get_hdr(klee::ref<klee::Expr> expr, transpiler_opt_t opt) const {
  if (std::optional<var_t> var = get_exact(expr)) {
    if (var->is_header_field) {
      if (opt & TRANSPILER_OPT_SWAP_HDR_ENDIANNESS) {
        if (var->size == 16) {
          var->name = "bswap16(" + var->name + ")";
        } else if (var->size == 32) {
          var->name = "bswap32(" + var->name + ")";
        } else {
          panic("FIXME: incompatible endian swap size %d\n", var->size);
        }
      }

      return var;
    }
  }

  for (auto var_it = frames.rbegin(); var_it != frames.rend(); var_it++) {
    const var_t &var = *var_it;

    if (!var.is_header_field) {
      continue;
    }

    const bits_t expr_size = expr->getWidth();
    const bits_t var_size  = var.size;

    if (expr_size > var_size) {
      continue;
    }

    for (bits_t offset = 0; offset + expr_size <= var_size; offset += 8) {
      klee::ref<klee::Expr> var_slice = LibCore::solver_toolbox.exprBuilder->Extract(var.expr, offset, expr_size);

      if (LibCore::solver_toolbox.are_exprs_always_equal(var_slice, expr)) {
        var_t slice = var.get_slice(offset, expr_size, opt);

        if (slice.is_header_field && (opt & TRANSPILER_OPT_SWAP_HDR_ENDIANNESS)) {
          if (slice.size == 16) {
            slice.name = "bswap16(" + slice.name + ")";
          } else if (slice.size == 32) {
            slice.name = "bswap32(" + slice.name + ")";
          } else {
            panic("FIXME: incompatible endian swap size %d\n", slice.size);
          }
        }

        return slice;
      }
    }
  }

  return std::nullopt;
}

std::vector<TofinoSynthesizer::var_t> TofinoSynthesizer::Stack::get_all() const { return frames; }

void TofinoSynthesizer::Stacks::push() { stacks.emplace_back(); }

void TofinoSynthesizer::Stacks::pop() { stacks.pop_back(); }

void TofinoSynthesizer::Stacks::insert_front(const var_t &var) { stacks.front().push(var); }
void TofinoSynthesizer::Stacks::insert_front(const Stack &stack) { stacks.front().push(stack); }

void TofinoSynthesizer::Stacks::insert_back(const var_t &var) { stacks.back().push(var); }
void TofinoSynthesizer::Stacks::insert_back(const Stack &stack) { stacks.back().push(stack); }

TofinoSynthesizer::Stack TofinoSynthesizer::Stacks::squash() const {
  Stack squashed;
  for (const Stack &stack : stacks) {
    squashed.push(stack);
  }
  return squashed;
}

TofinoSynthesizer::Stack TofinoSynthesizer::Stacks::squash_hdrs_only() const {
  Stack squashed;
  for (const Stack &stack : stacks) {
    for (const var_t &var : stack.get_all()) {
      if (var.is_header_field) {
        squashed.push(var);
      }
    }
  }
  return squashed;
}

std::optional<TofinoSynthesizer::var_t> TofinoSynthesizer::Stacks::get(klee::ref<klee::Expr> expr, transpiler_opt_t opt) const {
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

std::optional<TofinoSynthesizer::var_t> TofinoSynthesizer::Stacks::get_hdr(klee::ref<klee::Expr> expr, transpiler_opt_t opt) const {
  for (auto stack_it = stacks.rbegin(); stack_it != stacks.rend(); stack_it++) {
    if (std::optional<var_t> var = stack_it->get_exact_hdr(expr)) {
      return var;
    }
  }

  for (auto stack_it = stacks.rbegin(); stack_it != stacks.rend(); stack_it++) {
    if (std::optional<var_t> var = stack_it->get_hdr(expr, opt)) {
      return var;
    }
  }

  return std::nullopt;
}

void TofinoSynthesizer::Stacks::clear() { stacks.clear(); }

std::vector<TofinoSynthesizer::Stack> TofinoSynthesizer::Stacks::get_all() const { return stacks; }

TofinoSynthesizer::TofinoSynthesizer(const EP *_ep, std::filesystem::path _out_path)
    : Synthesizer(std::filesystem::path(__FILE__).parent_path() / "Templates" / TEMPLATE_FILENAME,
                  {
                      {MARKER_CPU_HEADER, 1},
                      {MARKER_RECIRC_HEADER, 1},
                      {MARKER_CUSTOM_HEADERS, 0},
                      {MARKER_INGRESS_HEADERS, 1},
                      {MARKER_INGRESS_METADATA, 1},
                      {MARKER_INGRESS_PARSER, 1},
                      {MARKER_INGRESS_CONTROL, 1},
                      {MARKER_INGRESS_CONTROL_APPLY, 3},
                      {MARKER_INGRESS_CONTROL_APPLY_RECIRC, 3},
                      {MARKER_INGRESS_DEPARSER, 2},
                      {MARKER_EGRESS_HEADERS, 1},
                      {MARKER_EGRESS_METADATA, 1},
                  },
                  _out_path),
      ep(_ep), transpiler(this) {}

TofinoSynthesizer::coder_t &TofinoSynthesizer::get(const std::string &marker) {
  if (marker == MARKER_INGRESS_CONTROL_APPLY && active_recirc_code_path) {
    return Synthesizer::get(MARKER_INGRESS_CONTROL_APPLY_RECIRC);
  }
  return Synthesizer::get(marker);
}

void TofinoSynthesizer::synthesize() {
  const LibBDD::BDD *bdd = ep->get_bdd();

  LibCore::symbol_t device = bdd->get_device();
  LibCore::symbol_t time   = bdd->get_time();

  alloc_var("meta.dev", device.expr, GLOBAL | EXACT_NAME);
  alloc_var("meta.time", LibCore::solver_toolbox.exprBuilder->Extract(time.expr, 0, 32), GLOBAL | EXACT_NAME);

  ingress_vars.push();

  EPVisitor::visit(ep);

  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);
  ingress_apply.indent();
  ingress_apply << "ig_tm_md.bypass_egress = 1;";

  // Transpile the parser after the whole EP has been visited so we have all the
  // headers available.

  ingress_vars.clear();
  hdrs_stacks.clear();

  ingress_vars.push();
  hdrs_stacks.push();

  alloc_var("meta.dev", LibCore::solver_toolbox.exprBuilder->Extract(device.expr, 0, 16), GLOBAL | EXACT_NAME);
  alloc_var("meta.time", LibCore::solver_toolbox.exprBuilder->Extract(time.expr, 0, 32), GLOBAL | EXACT_NAME);

  ingress_vars.push();

  transpile_parser(get_tofino_parser(ep));

  coder_t &cpu_hdr = get(MARKER_CPU_HEADER);
  for (const var_t &var : cpu_hdr_vars.get_all()) {
    bits_t pad = var.is_bool() ? 7 : (8 - var.expr->getWidth()) % 8;

    if (pad > 0) {
      cpu_hdr.indent();
      cpu_hdr << "@padding bit<";
      cpu_hdr << pad;
      cpu_hdr << "> pad_";
      cpu_hdr << var.get_stem();
      cpu_hdr << ";\n";
    }

    var.declare(cpu_hdr);
  }

  coder_t &recirc_hdr = get(MARKER_RECIRC_HEADER);
  for (const var_t &var : recirc_hdr_vars.get_all()) {
    bits_t pad = var.is_bool() ? 7 : (8 - var.expr->getWidth()) % 8;

    if (pad > 0) {
      recirc_hdr.indent();
      recirc_hdr << "@padding bit<";
      recirc_hdr << pad;
      recirc_hdr << "> pad_";
      recirc_hdr << var.get_stem();
      recirc_hdr << ";\n";
    }

    var.declare(recirc_hdr);
  }

  coder_t &ingress_deparser = get(MARKER_INGRESS_DEPARSER);
  ingress_deparser.indent();
  ingress_deparser << "pkt.emit(hdr);";

  Synthesizer::dump();
}

void TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  if (ep_node->get_module()->get_target() == TargetType::Tofino) {
    ingress_apply.indent();
    ingress_apply << "// EP node  " << ep_node->get_id() << "\n";
    ingress_apply.indent();
    ingress_apply << "// BDD node " << ep_node->get_module()->get_node()->dump(true) << "\n";
  }

  EPVisitor::visit(ep, ep_node);
}

void TofinoSynthesizer::transpile_parser(const Parser &parser) {
  coder_t &ingress_parser = get(MARKER_INGRESS_PARSER);

  std::vector<const ParserState *> states{parser.get_initial_state()};
  bool state_init = true;

  while (!states.empty()) {
    const ParserState *state = states.front();
    states.erase(states.begin());

    ingress_vars.push();
    hdrs_stacks.push();

    for (LibBDD::node_id_t id : state->ids) {
      if (parser_vars.find(id) != parser_vars.end()) {
        const Stack &vars = parser_vars.at(id);
        ingress_vars.insert_back(vars);
      }

      if (parser_hdrs.find(id) != parser_hdrs.end()) {
        const Stack &vars = parser_hdrs.at(id);
        hdrs_stacks.insert_back(vars);
      }
    }

    switch (state->type) {
    case ParserStateType::EXTRACT: {
      const ParserStateExtract *extract = dynamic_cast<const ParserStateExtract *>(state);
      const code_t state_name           = get_parser_state_name(state, state_init);

      std::optional<var_t> hdr_var = parser_hdrs.at(*extract->ids.begin()).get(extract->hdr);
      assert(hdr_var && "Header not found");

      assert(extract->next && "Next state not found");
      const code_t next_state = get_parser_state_name(extract->next, false);

      ingress_parser.indent();
      ingress_parser << "state " << state_name << " {\n";

      ingress_parser.inc();
      ingress_parser.indent();
      ingress_parser << "pkt.extract(" << hdr_var->name << ");\n";

      ingress_parser.indent();
      ingress_parser << "transition " << next_state << ";\n";

      ingress_parser.dec();
      ingress_parser.indent();
      ingress_parser << "}\n";

      states.push_back(extract->next);
    } break;
    case ParserStateType::SELECT: {
      const ParserStateSelect *select = dynamic_cast<const ParserStateSelect *>(state);
      const code_t state_name         = get_parser_state_name(state, state_init);

      ingress_parser.indent();
      ingress_parser << "state " << state_name << " {\n";

      ingress_parser.inc();
      ingress_parser.indent();
      ingress_parser << "transition select (" << transpiler.transpile(select->field) << ") {\n";

      ingress_parser.inc();

      const code_t next_true  = get_parser_state_name(select->on_true, false);
      const code_t next_false = get_parser_state_name(select->on_false, false);

      for (int value : select->values) {
        ingress_parser.indent();
        ingress_parser << value << ": " << next_true << ";\n";
      }

      ingress_parser.indent();
      ingress_parser << "default: " << next_false << ";\n";

      ingress_parser.dec();
      ingress_parser.indent();
      ingress_parser << "}\n";

      ingress_parser.dec();
      ingress_parser.indent();
      ingress_parser << "}\n";

      states.push_back(select->on_true);
      states.push_back(select->on_false);
    } break;
    case ParserStateType::TERMINATE: {
      const ParserStateTerminate *terminate = dynamic_cast<const ParserStateTerminate *>(state);
      const code_t state_name               = get_parser_state_name(state, state_init);

      ingress_parser.indent();
      ingress_parser << "state " << state_name << " {\n";

      ingress_parser.inc();
      ingress_parser.indent();
      ingress_parser << "transition ";
      if (terminate->accept) {
        ingress_parser << "accept";
      } else {
        ingress_parser << "reject";
      }
      ingress_parser << ";\n";

      ingress_parser.dec();
      ingress_parser.indent();
      ingress_parser << "}\n";
    } break;
    }

    ingress_vars.pop();
    hdrs_stacks.pop();

    state_init = false;
  }
}

TofinoSynthesizer::var_t TofinoSynthesizer::alloc_var(const code_t &proposed_name, klee::ref<klee::Expr> expr, alloc_opt_t option) {
  assert(option & (LOCAL | GLOBAL) && "Neither LOCAL nor GLOBAL specified");

  const code_t name = (option & EXACT_NAME) ? proposed_name : create_unique_name(proposed_name);
  const var_t var(name, expr, expr->getWidth(), option & FORCE_BOOL, option & (HEADER | HEADER_FIELD), option & BUFFER);

  if (!(option & SKIP_STACK_ALLOC)) {
    if (option & HEADER) {
      hdrs_stacks.insert_back(var);
    } else {
      ingress_vars.insert_back(var);
    }
  }

  return var;
}

code_path_t TofinoSynthesizer::alloc_recirc_coder() {
  size_t size = recirc_coders.size();
  recirc_coders.emplace_back();
  return size;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::SendToController *node) {
  coder_t &ingress_apply          = get(MARKER_INGRESS_CONTROL_APPLY);
  const LibCore::Symbols &symbols = node->get_symbols();

  ingress_apply.indent();
  ingress_apply << "send_to_controller(";
  ingress_apply << ep_node->get_id();
  ingress_apply << ");\n";

  for (const LibCore::symbol_t &symbol : symbols.get()) {
    std::optional<var_t> var = ingress_vars.get(symbol.expr);

    if (!var) {
      // This can happen when the symbol is not used and synapse optimized it away.
      continue;
    }

    var_t cpu_var = *var;
    cpu_var.name  = "hdr.cpu." + var->get_stem();
    cpu_hdr_vars.push(cpu_var);

    ingress_apply.indent();
    ingress_apply << cpu_var.name;
    ingress_apply << " = ";
    ingress_apply << var->name;
    ingress_apply << ";\n";
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Recirculate *node) {
  assert(ep_node->get_children().size() == 1);
  const EPNode *next = ep_node->get_children()[0];

  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);
  coder_t &recirc        = get(MARKER_INGRESS_CONTROL_APPLY_RECIRC);

  // 1. Allocate a new recirculation code path
  code_path_t code_path = alloc_recirc_coder();

  // 2. Build the recirculation header and populate it with the current stack
  ingress_apply.indent();
  ingress_apply << "hdr.recirc.setValid();\n";
  ingress_apply.indent();
  ingress_apply << "hdr.recirc.code_path = " << code_path << ";\n";

  Stacks stack_backup = ingress_vars;

  std::vector<var_t> recirc_vars;
  for (const var_t &var : ingress_vars.squash().get_all()) {
    var_t recirc_var = var;

    if (var.is_header_field) {
      recirc_vars.push_back(recirc_var);
      continue;
    }

    recirc_var.name = "hdr.recirc." + var.get_stem();

    recirc_hdr_vars.push(var);
    recirc_vars.push_back(recirc_var);

    ingress_apply.indent();
    ingress_apply << recirc_var.name;
    ingress_apply << " = ";
    ingress_apply << var.name;
    ingress_apply << ";\n";
  }

  // 3. Forward to recirculation port
  int port = node->get_recirc_port();
  ingress_apply.indent();
  ingress_apply << "fwd(" << port << ");\n";

  // 4. Replace the ingress apply coder with the recirc coder
  active_recirc_code_path = code_path;

  // 5. Clear the stack, rebuild it with hdr.recirc fields, and setup the recirculation code block
  ingress_vars.clear();
  ingress_vars.push();
  for (const var_t &var : recirc_vars) {
    ingress_vars.insert_back(var);
  }

  recirc.indent();
  recirc << "if (hdr.recirc.code_path == " << code_path << ") {\n";
  recirc.inc();

  ingress_vars.push();
  visit(ep, next);
  ingress_vars.pop();
  recirc.dec();

  recirc.indent();
  recirc << "}\n";

  // 6. Revert the state back to before the recirculation was made
  active_recirc_code_path.reset();
  ingress_vars = stack_backup;

  return EPVisitor::Action::skipChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Ignore *node) {
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::If *node) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL_APPLY);

  const std::vector<klee::ref<klee::Expr>> &conditions = node->get_conditions();
  const std::vector<EPNode *> &children                = ep_node->get_children();
  assert(children.size() == 2 && "If node must have 2 children");

  const EPNode *then_node = children[0];
  const EPNode *else_node = children[1];

  if (conditions.size() == 1) {
    const klee::ref<klee::Expr> condition = conditions[0];

    ingress.indent();
    ingress << "if (";
    ingress << transpiler.transpile(condition);
    ingress << ") {\n";

    ingress.inc();
    ingress_vars.push();
    visit(ep, then_node);
    ingress_vars.pop();
    ingress.dec();

    ingress.indent();
    ingress.stream << "} else {\n";

    ingress.inc();
    ingress_vars.push();
    visit(ep, else_node);
    ingress_vars.pop();
    ingress.dec();

    ingress.indent();
    ingress << "}\n";

    return EPVisitor::Action::skipChildren;
  }

  const var_t cond_var = alloc_var("cond", LibCore::solver_toolbox.exprBuilder->True(), LOCAL | FORCE_BOOL);
  cond_var.declare(ingress, "false");

  for (klee::ref<klee::Expr> condition : conditions) {
    ingress.indent();
    ingress << "if (";
    ingress << transpiler.transpile(condition);
    ingress << ") {\n";

    ingress.inc();
  }

  ingress.indent();
  ingress << cond_var.name << " = true;\n";

  for (size_t i = 0; i < conditions.size(); i++) {
    ingress.dec();
    ingress.indent();
    ingress << "}\n";
  }

  ingress.indent();
  ingress << "if (";
  ingress << cond_var.name;
  ingress << ") {\n";

  ingress.inc();
  ingress_vars.push();
  visit(ep, then_node);
  ingress_vars.pop();
  ingress.dec();

  ingress.indent();
  ingress << "} else {\n";

  ingress.inc();
  ingress_vars.push();
  visit(ep, else_node);
  ingress_vars.pop();
  ingress.dec();

  ingress.indent();
  ingress << "}\n";

  return EPVisitor::Action::skipChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::ParserCondition *node) {
  parser_vars[node->get_node()->get_id()] = ingress_vars.squash_hdrs_only();
  parser_hdrs[node->get_node()->get_id()] = hdrs_stacks.squash();
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Then *node) {
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Else *node) {
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Forward *node) {
  klee::ref<klee::Expr> dst_device = node->get_dst_device();
  coder_t &ingress                 = get(MARKER_INGRESS_CONTROL_APPLY);

  ingress.indent();
  ingress << "nf_dev[15:0] = " << transpiler.transpile(dst_device) << ";\n";
  ingress.indent();
  ingress << "trigger_forward = true;\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Drop *node) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL_APPLY);

  ingress.indent();
  ingress << "drop();\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::ParserReject *node) {
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Broadcast *node) {
  panic("TODO: Broadcast");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::ParserExtraction *node) {
  const klee::ref<klee::Expr> expr = node->get_hdr();

  // On header valid only.
  ingress_vars.push();
  hdrs_stacks.push();

  const code_t hdr_name = create_unique_name("hdr");
  const var_t hdr       = alloc_var("hdr." + hdr_name, expr, LOCAL | EXACT_NAME | HEADER);
  const var_t hdr_data  = alloc_var("hdr." + hdr_name + ".data", expr, LOCAL | EXACT_NAME | HEADER_FIELD);

  coder_t &custom_hdrs = get(MARKER_CUSTOM_HEADERS);
  custom_hdrs.indent();
  custom_hdrs << "header " << hdr_name << "_h {\n";

  custom_hdrs.inc();
  custom_hdrs.indent();
  custom_hdrs << TofinoSynthesizer::Transpiler::type_from_expr(expr) << " " << hdr_data.get_stem() << ";\n";

  custom_hdrs.dec();
  custom_hdrs.indent();
  custom_hdrs << "}\n";

  coder_t &ingress_hdrs = get(MARKER_INGRESS_HEADERS);
  ingress_hdrs.indent();
  ingress_hdrs << hdr_name << "_h " << hdr_name << ";\n";

  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);
  ingress_apply.indent();
  ingress_apply << "if(" << hdr.name << ".isValid()) {\n";

  ingress_apply.inc();

  parser_vars[node->get_node()->get_id()] = ingress_vars.squash_hdrs_only();
  parser_hdrs[node->get_node()->get_id()] = hdrs_stacks.squash();

  assert(ep_node->get_children().size() == 1 && "ParserExtraction must have 1 child");
  visit(ep, ep_node->get_children()[0]);

  ingress_apply.dec();

  ingress_apply.indent();
  ingress_apply << "}\n";

  ingress_vars.pop();
  hdrs_stacks.pop();

  return EPVisitor::Action::skipChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::ModifyHeader *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  klee::ref<klee::Expr> hdr    = node->get_hdr();
  std::optional<var_t> hdr_var = ingress_vars.get_hdr(hdr);
  assert(hdr_var && "Header not found");

  const std::vector<LibCore::expr_mod_t> &changes     = node->get_changes();
  const std::vector<LibCore::expr_byte_swap_t> &swaps = node->get_swaps();

  std::cerr << "hdr_var: " << hdr_var->to_string() << "\n";

  for (const LibCore::expr_byte_swap_t &byte_swap : swaps) {
    ingress_apply.indent();
    ingress_apply << "swap(";
    ingress_apply << hdr_var->get_slice(byte_swap.byte0 * 8, 8).name;
    ingress_apply << ", ";
    ingress_apply << hdr_var->get_slice(byte_swap.byte1 * 8, 8).name;
    ingress_apply << ");\n";
  }

  for (const LibCore::expr_mod_t &mod : changes) {
    auto swapped = [&mod](const LibCore::expr_byte_swap_t &byte_swap) -> bool {
      assert(mod.width == 8 && "TODO: deal with non-byte modifications with swaps");
      return mod.offset == byte_swap.byte0 * 8 || mod.offset == byte_swap.byte1 * 8;
    };

    if (std::any_of(swaps.begin(), swaps.end(), swapped)) {
      continue;
    }

    klee::ref<klee::Expr> expr = mod.expr;
    ingress_apply.indent();
    ingress_apply << hdr_var->get_slice(mod.offset, mod.width).name;
    ingress_apply << " = ";
    ingress_apply << transpiler.transpile(expr);
    ingress_apply << ";\n";
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::MapTableLookup *node) {
  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  DS_ID map_table_id                      = node->get_id();
  std::vector<klee::ref<klee::Expr>> keys = node->get_keys();
  klee::ref<klee::Expr> value             = node->get_value();
  std::optional<LibCore::symbol_t> hit    = node->get_hit();

  const MapTable *map_table = get_tofino_ds<MapTable>(ep, map_table_id);

  LibBDD::node_id_t node_id = node->get_node()->get_id();
  const Table *table        = map_table->get_table(node_id);
  assert(table && "Table not found");

  std::vector<var_t> keys_vars;
  if (declared_ds.find(table->id) == declared_ds.end()) {
    transpile_table_decl(ingress, table, keys, {value}, false, keys_vars);
    ingress << "\n";
    declared_ds.insert(table->id);
  }
  assert(keys_vars.size() == keys.size());

  for (const var_t &key_var : keys_vars) {
    ingress_apply.indent();
    ingress_apply << key_var.name << " = " << transpiler.transpile(key_var.expr) << ";\n";
  }

  if (hit) {
    var_t hit_var = alloc_var("hit", hit->expr, LOCAL | FORCE_BOOL);
    hit_var.declare(ingress_apply, table->id + ".apply().hit");
  } else {
    ingress_apply.indent();
    ingress_apply << table->id << ".apply();\n";
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorTableLookup *node) {
  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  DS_ID vector_table_id       = node->get_id();
  klee::ref<klee::Expr> key   = node->get_key();
  klee::ref<klee::Expr> value = node->get_value();

  const VectorTable *vector_table = get_tofino_ds<VectorTable>(ep, vector_table_id);

  LibBDD::node_id_t node_id = node->get_node()->get_id();
  const Table *table        = vector_table->get_table(node_id);
  assert(table && "Table not found");

  code_t transpiled_key = transpiler.transpile(key, TRANSPILER_OPT_SWAP_HDR_ENDIANNESS);

  std::vector<var_t> keys_vars;
  if (declared_ds.find(table->id) == declared_ds.end()) {
    transpile_table_decl(ingress, table, {key}, {value}, true, keys_vars);
    ingress << "\n";
    declared_ds.insert(table->id);
  }

  assert(keys_vars.size() == 1);
  var_t key_var = keys_vars[0];

  ingress_apply.indent();
  ingress_apply << key_var.name << " = " << transpiled_key << ";\n";

  ingress_apply.indent();
  ingress_apply << table->id << ".apply();\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::DchainTableLookup *node) {
  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  DS_ID dchain_table_id                = node->get_id();
  klee::ref<klee::Expr> key            = node->get_key();
  std::optional<LibCore::symbol_t> hit = node->get_hit();

  const DchainTable *dchain_table = get_tofino_ds<DchainTable>(ep, dchain_table_id);

  LibBDD::node_id_t node_id = node->get_node()->get_id();
  const Table *table        = dchain_table->get_table(node_id);
  assert(table && "Table not found");

  code_t transpiled_key = transpiler.transpile(key, TRANSPILER_OPT_SWAP_HDR_ENDIANNESS);

  std::vector<var_t> keys_vars;
  if (declared_ds.find(table->id) == declared_ds.end()) {
    transpile_table_decl(ingress, table, {key}, {}, false, keys_vars);
    ingress << "\n";
    declared_ds.insert(table->id);
  }

  assert(keys_vars.size() == 1);
  var_t key_var = keys_vars[0];

  ingress_apply.indent();
  ingress_apply << key_var.name << " = " << transpiled_key << ";\n";

  if (hit) {
    var_t hit_var = alloc_var("hit", hit->expr, LOCAL | FORCE_BOOL);
    hit_var.declare(ingress_apply, table->id + ".apply().hit");
  } else {
    ingress_apply.indent();
    ingress_apply << table->id << ".apply();\n";
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterLookup *node) {
  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  DS_ID id                    = node->get_id();
  klee::ref<klee::Expr> index = node->get_index();
  klee::ref<klee::Expr> value = node->get_value();

  const VectorRegister *vector_register = get_tofino_ds<VectorRegister>(ep, id);

  std::vector<const Register *> regs;
  for (const Register &reg : vector_register->regs) {
    regs.push_back(&reg);
  }

  std::sort(regs.begin(), regs.end(), [](const Register *r0, const Register *r1) { return natural_compare(r0->id, r1->id); });

  for (const Register *reg : regs) {
    if (declared_ds.find(reg->id) == declared_ds.end()) {
      transpile_register_decl(ingress, reg, index, value);
      declared_ds.insert(reg->id);
    }
  }

  if (!regs.empty()) {
    ingress << "\n";
  }

  const std::string value_prefix_name = "vector_reg_value";
  const var_t value_var               = alloc_var(value_prefix_name, value, LOCAL);
  value_var.declare(ingress_apply, TofinoSynthesizer::Transpiler::transpile_literal(0, value->getWidth()));

  int i         = 0;
  bits_t offset = 0;
  for (const Register *reg : regs) {
    const code_t action_name = build_register_action_name(ep_node, reg, RegisterActionType::READ);
    transpile_register_read_action_decl(ingress, reg, action_name);
    ingress << "\n";

    const klee::ref<klee::Expr> entry_expr = LibCore::solver_toolbox.exprBuilder->Extract(value, offset, reg->value_size);
    std::optional<var_t> entry_var         = ingress_vars.get(entry_expr);
    assert(entry_var && "Register entry is not a variable");

    ingress_apply.indent();
    ingress_apply << entry_var->name << " = " << action_name + ".execute(" + transpiler.transpile(index) + ");\n";

    offset += reg->value_size;
    i++;
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterUpdate *node) {
  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  DS_ID id                          = node->get_id();
  klee::ref<klee::Expr> index       = node->get_index();
  klee::ref<klee::Expr> value       = node->get_read_value();
  klee::ref<klee::Expr> write_value = node->get_write_value();

  const VectorRegister *vector_register = get_tofino_ds<VectorRegister>(ep, id);

  std::vector<const Register *> regs;
  for (const Register &reg : vector_register->regs) {
    regs.push_back(&reg);
  }

  std::sort(regs.begin(), regs.end(), [](const Register *r0, const Register *r1) { return natural_compare(r0->id, r1->id); });

  for (const Register *reg : regs) {
    if (declared_ds.find(reg->id) == declared_ds.end()) {
      transpile_register_decl(ingress, reg, index, value);
      declared_ds.insert(reg->id);
    }
  }

  if (!regs.empty()) {
    ingress << "\n";
  }

  int i         = 0;
  bits_t offset = 0;
  for (const Register *reg : regs) {
    const klee::ref<klee::Expr> reg_write_expr = LibCore::solver_toolbox.exprBuilder->Extract(write_value, offset, reg->value_size);
    std::optional<var_t> reg_write_var         = ingress_vars.get(reg_write_expr);
    assert(reg_write_var && "Register write value is not a variable");

    const code_t action_name = build_register_action_name(ep_node, reg, RegisterActionType::WRITE);
    transpile_register_write_action_decl(ingress, reg, action_name, *reg_write_var);
    ingress << "\n";

    ingress_apply.indent();
    ingress_apply << action_name;
    ingress_apply << ".execute(" << transpiler.transpile(index) << ");\n";

    i++;
    offset += reg->value_size;
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableRead *node) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL);
  // coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const FCFSCachedTable *fcfs_cached_table = get_tofino_ds<FCFSCachedTable>(ep, node->get_cached_table_id());
  klee::ref<klee::Expr> key                = node->get_key();
  klee::ref<klee::Expr> value              = node->get_value();

  if (declared_ds.find(fcfs_cached_table->id) == declared_ds.end()) {
    transpile_fcfs_cached_table_decl(ingress, fcfs_cached_table, key, value);
    declared_ds.insert(fcfs_cached_table->id);
  }

  // const LibCore::symbol_t &map_has_this_key         = node->get_map_has_this_key();

  // transpile_fcfs_cached_table(ingress, fcfs_cached_table, key, value);
  // dbg();

  panic("TODO: FCFSCachedTableRead");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::LPMLookup *node) {
  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  DS_ID lpm_id                 = node->get_lpm_id();
  klee::ref<klee::Expr> addr   = node->get_addr();
  klee::ref<klee::Expr> device = node->get_device();
  klee::ref<klee::Expr> match  = node->get_match();

  const LPM *lpm = get_tofino_ds<LPM>(ep, lpm_id);

  code_t transpiled_key = transpiler.transpile(addr);

  if (declared_ds.find(lpm_id) == declared_ds.end()) {
    transpile_lpm_decl(ingress, lpm, addr, device);
    ingress << "\n";
    declared_ds.insert(lpm_id);
  }

  std::optional<var_t> key_var = ingress_vars.get(addr);
  assert(key_var && "Key is not a variable");

  ingress_apply.indent();
  ingress_apply << key_var->name << " = " << transpiled_key << ";\n";

  var_t hit_var = alloc_var("hit", match, LOCAL | FORCE_BOOL);
  hit_var.declare(ingress_apply, lpm_id + ".apply().hit");

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableReadWrite *node) {
  panic("TODO: FCFSCachedTableReadWrite");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableWrite *node) {
  panic("TODO: FCFSCachedTableWrite");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableDelete *node) {
  panic("TODO: FCFSCachedTableDelete");
  return EPVisitor::Action::doChildren;
}

TofinoSynthesizer::code_t TofinoSynthesizer::create_unique_name(const code_t &prefix) {
  if (var_prefix_usage.find(prefix) == var_prefix_usage.end()) {
    var_prefix_usage[prefix] = 0;
  }

  int &counter = var_prefix_usage[prefix];

  coder_t coder;
  coder << prefix << "_" << counter;

  counter++;

  return coder.dump();
}

void TofinoSynthesizer::dbg_vars() const {
  std::cerr << "================== Stack =================== \n";
  for (const Stack &stack : ingress_vars.get_all()) {
    std::cerr << "------------------------------------------\n";
    for (const var_t &var : stack.get_all()) {
      std::cerr << var.name;
      if (var.is_bool()) {
        std::cerr << " (bool)";
      }
      if (var.is_header_field) {
        std::cerr << " (header)";
      }
      std::cerr << ": ";
      std::cerr << LibCore::expr_to_string(var.expr, true);
      std::cerr << "\n";
    }
  }
  std::cerr << "============================================ \n";

  std::cerr << "================== Headers ================= \n";
  for (const Stack &stack : hdrs_stacks.get_all()) {
    std::cerr << "------------------------------------------\n";
    for (const var_t &var : stack.get_all()) {
      std::cerr << var.name;
      if (var.is_bool()) {
        std::cerr << " (bool)";
      }
      if (var.is_header_field) {
        std::cerr << " (header)";
      }
      std::cerr << ": ";
      std::cerr << LibCore::expr_to_string(var.expr, true);
      std::cerr << "\n";
    }
  }
  std::cerr << "======================================== \n";
}

} // namespace Tofino
} // namespace LibSynapse