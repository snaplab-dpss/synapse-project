#include <LibSynapse/Synthesizers/TofinoSynthesizer.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Strings.h>

namespace LibSynapse {
namespace Tofino {

using LibCore::expr_to_string;
using LibCore::get_unique_symbolic_reads;
using LibCore::is_conditional;
using LibCore::is_constant;
using LibCore::is_constant_signed;
using LibCore::match_endian_swap_pattern;
using LibCore::natural_compare;
using LibCore::simplify;
using LibCore::solver_toolbox;

namespace {

constexpr const u16 CUCKOO_CODE_PATH = 0xffff;

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
constexpr const char *const MARKER_INGRESS_DEPARSER_APPLY       = "INGRESS_DEPARSER_APPLY";
constexpr const char *const MARKER_EGRESS_HEADERS               = "EGRESS_HEADERS";
constexpr const char *const MARKER_EGRESS_METADATA              = "EGRESS_METADATA";
constexpr const char *const MARKER_CONTROL_BLOCKS               = "CONTROL_BLOCKS";

constexpr const char *const MARKER_CUCKOO_IDX_WIDTH       = "CUCKOO_IDX_WIDTH";
constexpr const char *const MARKER_CUCKOO_ENTRIES         = "CUCKOO_ENTRIES";
constexpr const char *const MARKER_CUCKOO_BLOOM_IDX_WIDTH = "CUCKOO_BLOOM_IDX_WIDTH";
constexpr const char *const MARKER_CUCKOO_BLOOM_ENTRIES   = "CUCKOO_BLOOM_ENTRIES";

constexpr const char *const TEMPLATE_FILENAME                   = "tofino.template.p4";
constexpr const char *const TEMPLATE_CUCKOO_HASH_TABLE_FILENAME = "cuckoo_hash_table.template.p4";

template <class T> const T *get_tofino_ds(const EP *ep, DS_ID id) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  const Tofino::DS *ds            = tofino_ctx->get_data_structures().get_ds_from_id(id);
  assert(ds && "DS not found");
  return dynamic_cast<const T *>(ds);
}

const Parser &get_tofino_parser(const EP *ep) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  const TNA &tna                  = tofino_ctx->get_tna();
  return tna.parser;
}

} // namespace

TofinoSynthesizer::Transpiler::Transpiler(TofinoSynthesizer *_synthesizer) : synthesizer(_synthesizer) {}

code_t TofinoSynthesizer::Transpiler::transpile_constant(klee::ref<klee::Expr> expr, bool swap_endianness) {
  assert(is_constant(expr) && "Expected a constant expression");

  const bytes_t width = expr->getWidth() / 8;

  coder_t code;
  code << width * 8 << "w0x";

  for (size_t byte = 0; byte < width; byte++) {
    const bytes_t offset          = swap_endianness ? byte : width - byte - 1;
    klee::ref<klee::Expr> extract = solver_toolbox.exprBuilder->Extract(expr, offset * 8, 8);
    const u64 byte_value          = solver_toolbox.value_from_expr(extract);

    std::stringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0') << byte_value;
    code << ss.str();
  }

  return code.dump();
}

code_t TofinoSynthesizer::Transpiler::swap_endianness(const code_t &expr, bits_t size) {
  coder_t code;

  if (size == 16) {
    code << "bswap16(" << expr << ")";
  } else if (size == 32) {
    code << "bswap32(" << expr << ")";
  } else if (size == 64) {
    code << "bswap64(" << expr << ")";
  } else {
    panic("FIXME: incompatible endian swap size %d", size);
  }

  return code.dump();
}

code_t TofinoSynthesizer::Transpiler::transpile(klee::ref<klee::Expr> expr, transpiler_opt_t opt) {
  loaded_opt = opt;

  std::cerr << "Transpiling: " << expr_to_string(expr) << "\n";
  expr = simplify(expr);
  std::cerr << "Simplified:  " << expr_to_string(expr) << "\n";

  coders.emplace();
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> endian_swap_target;

  if (is_constant(expr)) {
    coder << transpile_constant(expr, opt & TRANSPILER_OPT_SWAP_CONST_ENDIANNESS);
  } else if (match_endian_swap_pattern(expr, endian_swap_target)) {
    const bits_t size = endian_swap_target->getWidth();
    coder << swap_endianness(transpile(endian_swap_target, loaded_opt), size);
  } else if (std::optional<var_t> var = synthesizer->ingress_vars.get(expr, loaded_opt)) {
    coder << var->name;
  } else {
    visit(expr);

    // HACK: clear the visited map so we force the transpiler to revisit all expressions.
    visited.clear();
  }

  code_t code = coder.dump();
  coders.pop();

  assert(!code.empty() && "Empty code");
  return code;
}

code_t TofinoSynthesizer::Transpiler::type_from_size(bits_t size) {
  coder_t coder;
  coder << "bit<" << size << ">";
  return coder.dump();
}

code_t TofinoSynthesizer::Transpiler::type_from_expr(klee::ref<klee::Expr> expr) {
  klee::Expr::Width width = expr->getWidth();
  assert(width != klee::Expr::InvalidWidth && "Invalid width");

  if (is_conditional(expr)) {
    return "bool";
  }

  return type_from_size(width);
}

code_t TofinoSynthesizer::Transpiler::transpile_literal(u64 value, bits_t size, bool hex) {
  coder_t coder;

  coder << size << "w";

  if (!hex) {
    coder << value;
  } else {
    std::stringstream ss;
    ss << "0x";
    ss << std::hex;

    switch (size) {
    case 8: {
      ss << std::setw(2);
      ss << std::setfill('0');
      break;
    }
    case 16: {
      ss << std::setw(4);
      ss << std::setfill('0');
      break;
    }
    case 32: {
      ss << std::setw(8);
      ss << std::setfill('0');
      break;
    }
    case 64: {
      ss << std::setw(16);
      ss << std::setfill('0');
      break;
    }
    }

    ss << value;
    coder << ss.str();
  }

  return coder.dump();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitRead(const klee::ReadExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ReadExpr *>(&e);

  coder_t &coder = coders.top();

  if (std::optional<TofinoSynthesizer::var_t> var = synthesizer->ingress_vars.get(expr, loaded_opt)) {
    coder << var->name;
    return Action::skipChildren();
  }

  std::cerr << expr_to_string(expr) << "\n";
  synthesizer->dbg_vars();

  panic("TODO: visitRead: %s", expr_to_string(expr).c_str());
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

  std::cerr << expr_to_string(expr) << "\n";
  synthesizer->dbg_vars();

  panic("TODO: visitConcat: %s", expr_to_string(expr).c_str());
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
  panic("TODO: visitExtract: %s", expr_to_string(expr).c_str());
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

  if (is_constant(lhs)) {
    const_expr = lhs;
    var_expr   = rhs;
  } else {
    const_expr = rhs;
    var_expr   = lhs;
  }

  if (is_constant(const_expr)) {
    std::optional<TofinoSynthesizer::var_t> var = synthesizer->ingress_vars.get(var_expr, loaded_opt);
    if (var && var->is_bool()) {
      u64 value = solver_toolbox.value_from_expr(const_expr);
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

  if (is_constant(lhs)) {
    const_expr = lhs;
    var_expr   = rhs;
  } else {
    const_expr = rhs;
    var_expr   = lhs;
  }

  if (is_constant(const_expr)) {
    std::optional<TofinoSynthesizer::var_t> var = synthesizer->ingress_vars.get(var_expr);
    if (var && var->is_bool()) {
      u64 value = solver_toolbox.value_from_expr(const_expr);
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

code_t TofinoSynthesizer::get_parser_state_name(const ParserState *state, bool state_init) {
  coder_t coder;

  if (state_init) {
    coder << "parser_init";
    return coder.dump();
  }

  coder << "parser_";
  bool first = true;
  for (bdd_node_id_t id : state->ids) {
    if (!first) {
      coder << "_";
    } else {
      first = false;
    }
    coder << id;
  }

  return coder.dump();
}

void TofinoSynthesizer::declare_var_in_ingress_metadata(const var_t &var) {
  coder_t &ingress_metadata = get(MARKER_INGRESS_METADATA);
  if (!ingress_metadata_var_names.contains(var.name)) {
    var.declare(ingress_metadata);
  }
  ingress_metadata_var_names.insert(var.name);
}

code_t TofinoSynthesizer::build_register_action_name(const Register *reg, RegisterActionType action, const EPNode *node) const {
  coder_t coder;
  coder << reg->id;
  coder << "_";
  switch (action) {
  case RegisterActionType::Read:
    coder << "read";
    break;
  case RegisterActionType::Write:
    coder << "write";
    break;
  case RegisterActionType::Swap:
    coder << "update";
    break;
  case RegisterActionType::Increment:
    coder << "inc";
    break;
  case RegisterActionType::Decrement:
    coder << "dec";
    break;
  case RegisterActionType::SetToOneAndReturnOldValue:
    coder << "read_and_set";
    break;
  case RegisterActionType::IncrementAndReturnNewValue:
    coder << "inc_and_read";
    break;
  case RegisterActionType::ReadConditionalWrite:
    coder << "read_conditional_write";
    break;
  case RegisterActionType::CalculateDiff:
    coder << "diff";
    break;
  case RegisterActionType::SampleEveryFourth:
    coder << "sample_every_fourth";
    break;
  case RegisterActionType::QueryTimestamp:
    coder << "query_timestamp";
    break;
  case RegisterActionType::QueryAndRefreshTimestamp:
    coder << "query_and_refresh_timestamp";
    break;
  case RegisterActionType::CheckValue:
    coder << "check_value";
    break;
  }

  if (node) {
    coder << "_";
    coder << node->get_id();
  }

  return coder.dump();
}

void TofinoSynthesizer::transpile_action_decl(const code_t &action_name, const std::vector<code_t> &body) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL);

  ingress.indent();
  ingress << "action " << action_name << "() {\n";
  ingress.inc();
  for (const code_t &statement : body) {
    ingress.indent();
    ingress << statement;
    ingress << "\n";
  }
  ingress.dec();
  ingress.indent();
  ingress << "}\n";
}

void TofinoSynthesizer::transpile_action_decl(const code_t &action_name, const std::vector<klee::ref<klee::Expr>> &params, bool params_are_buffers) {
  assert(!params.empty() && "Empty action");

  coder_t &ingress = get(MARKER_INGRESS_CONTROL);

  std::vector<var_t> params_vars;

  for (klee::ref<klee::Expr> param : params) {
    const std::string param_name = action_name + "_param";
    const var_t param_value_var  = alloc_var(param_name, param, (params_are_buffers ? BUFFER : 0));

    params_vars.push_back(param_value_var);
    param_value_var.declare(ingress, TofinoSynthesizer::Transpiler::transpile_literal(0, param->getWidth()));
  }

  ingress.indent();
  ingress << "action " << action_name << "(";

  for (size_t i = 0; i < params.size(); i++) {
    const klee::ref<klee::Expr> param = params[i];

    if (i != 0) {
      ingress << ", ";
    }

    ingress << TofinoSynthesizer::Transpiler::type_from_expr(param);
    ingress << " ";
    ingress << "_" << params_vars[i].name;
  }

  ingress << ") {\n";

  ingress.inc();

  for (const var_t &param : params_vars) {
    ingress.indent();
    ingress << param.name;
    ingress << " = ";
    ingress << "_" << param.name;
    ingress << ";\n";
  }

  ingress.dec();
  ingress.indent();
  ingress << "}\n";

  ingress << "\n";
}

void TofinoSynthesizer::transpile_table_decl(const Table *table, const std::vector<klee::ref<klee::Expr>> &keys,
                                             const std::vector<klee::ref<klee::Expr>> &values, bool values_are_buffers,
                                             std::vector<var_t> &keys_vars) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL);

  if (declared_ds.find(table->id) != declared_ds.end()) {
    return;
  }

  declared_ds.insert(table->id);

  const code_t action_name = table->id + "_get_value";
  if (!values.empty()) {
    transpile_action_decl(action_name, values, values_are_buffers);
  }

  for (size_t i = 0; i < keys.size(); i++) {
    const std::string key_name = "key_" + std::to_string(keys[i]->getWidth()) + "b_" + std::to_string(i);
    const var_t key_var        = alloc_var(key_name, keys[i], SKIP_STACK_ALLOC | EXACT_NAME | IS_INGRESS_METADATA);
    keys_vars.push_back(key_var);
    declare_var_in_ingress_metadata(key_var);
  }

  ingress.indent();
  ingress << "table " << table->id << " {\n";
  ingress.inc();

  ingress.indent();
  ingress << "key = {\n";
  ingress.inc();

  for (const var_t &key : keys_vars) {
    ingress.indent();
    ingress << key.name << ": exact;\n";
  }

  ingress.dec();
  ingress.indent();
  ingress << "}\n";

  ingress.indent();
  ingress << "actions = {";

  if (!values.empty()) {
    ingress << "\n";
    ingress.inc();

    ingress.indent();
    ingress << action_name << ";\n";

    ingress.dec();
    ingress.indent();
  } else {
    ingress << "\n";
    ingress.inc();

    ingress.indent();
    ingress << " NoAction;\n";

    ingress.dec();
    ingress.indent();
  }

  ingress << "}\n";

  ingress.indent();
  ingress << "size = " << table->capacity << ";\n";

  if (table->time_aware == TimeAware::Yes) {
    ingress.indent();
    ingress << "idle_timeout = true;\n";
  }

  ingress.dec();
  ingress.indent();
  ingress << "}\n";
  ingress << "\n";
}

void TofinoSynthesizer::transpile_lpm_decl(const LPM *lpm, klee::ref<klee::Expr> addr, klee::ref<klee::Expr> device) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL);

  if (declared_ds.find(lpm->id) != declared_ds.end()) {
    return;
  }

  declared_ds.insert(lpm->id);

  const code_t action_name = lpm->id + "_get_device";
  transpile_action_decl(action_name, {device}, false);

  const std::string key_name = "ipv4_addr";
  const var_t key_var        = alloc_var(key_name, addr);

  key_var.declare(ingress, TofinoSynthesizer::Transpiler::transpile_literal(0, key_var.expr->getWidth()));

  ingress.indent();
  ingress << "table " << lpm->id << " {\n";
  ingress.inc();

  ingress.indent();
  ingress << "key = {\n";
  ingress.inc();

  ingress.indent();
  ingress << key_var.name << ": ternary;\n";

  ingress.dec();
  ingress.indent();
  ingress << "}\n";

  ingress.indent();
  ingress << "actions = { " << action_name << "; }\n";

  ingress.indent();
  ingress << "size = " << lpm->capacity << ";\n";

  ingress.dec();
  ingress.indent();
  ingress << "}\n";
}

void TofinoSynthesizer::transpile_register_decl(const Register *reg) {
  // * Template:
  // Register<{VALUE_WIDTH}, _>({CAPACITY}, {INIT_VALUE}) {NAME};
  // * Example:
  // Register<bit<32>, _>(1024, 0) my_register;

  if (declared_ds.find(reg->id) != declared_ds.end()) {
    return;
  }

  declared_ds.insert(reg->id);

  coder_t &ingress = get(MARKER_INGRESS_CONTROL);

  const u64 init_value = 0;

  ingress.indent();
  ingress << "Register<";
  ingress << TofinoSynthesizer::Transpiler::type_from_size(reg->value_size);
  ingress << ",_>";

  ingress << "(";
  ingress << reg->capacity;
  ingress << ", ";
  ingress << init_value;
  ingress << ")";

  ingress << " ";

  ingress << reg->id;
  ingress << ";\n";
}

void TofinoSynthesizer::transpile_register_action_decl(const Register *reg, const code_t &action_name, RegisterActionType action_type,
                                                       std::optional<code_t> external_var) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL);

  const code_t value_type = TofinoSynthesizer::Transpiler::type_from_size(reg->value_size);
  const code_t index_type = TofinoSynthesizer::Transpiler::type_from_size(reg->index_size);

  ingress.indent();
  ingress << "RegisterAction<";
  ingress << value_type;
  ingress << ", ";
  ingress << index_type;
  ingress << ", ";
  if (register_action_types_with_out_value.find(action_type) != register_action_types_with_out_value.end()) {
    ingress << value_type;
  } else {
    ingress << "void";
  }
  ingress << ">";

  ingress << "(";
  ingress << reg->id;
  ingress << ")";

  ingress << " ";
  ingress << action_name;
  ingress << " = {\n";
  ingress.inc();

  switch (action_type) {
  case RegisterActionType::Read: {
    ingress.indent();
    ingress << "void apply(inout " << value_type << " value, out " << value_type << " out_value) {\n";
    ingress.inc();

    ingress.indent();
    ingress << "out_value = value;\n";

    ingress.dec();
    ingress.indent();
    ingress << "}\n";
  } break;
  case RegisterActionType::Write: {
    assert_or_panic(external_var.has_value(), "Expected a write value");

    ingress.indent();
    ingress << "void apply(inout " << value_type << " value) {\n";

    ingress.inc();
    ingress.indent();
    ingress << "value = " << external_var.value() << ";\n";

    ingress.dec();
    ingress.indent();
    ingress << "}\n";
  } break;
  case RegisterActionType::Swap: {
    panic("TODO: transpile_register_actions_decl(RegisterActionType::Swap)");
  } break;
  case RegisterActionType::Increment: {
    ingress.indent();
    ingress << "void apply(inout " << value_type << " value) {\n";

    ingress.inc();
    ingress.indent();
    ingress << "value = value + 1;\n";

    ingress.dec();
    ingress.indent();
    ingress << "}\n";
  } break;
  case RegisterActionType::Decrement: {
    ingress.indent();
    ingress << "void apply(inout " << value_type << " value) {\n";

    ingress.inc();
    ingress.indent();
    ingress << "value = value - 1;\n";

    ingress.dec();
    ingress.indent();
    ingress << "}\n";
  } break;
  case RegisterActionType::SetToOneAndReturnOldValue: {
    ingress.indent();
    ingress << "void apply(inout " << value_type << " value, out " << value_type << " out_value) {\n";
    ingress.inc();

    ingress.indent();
    ingress << "out_value = value;\n";
    ingress.indent();
    ingress << "value = 1;\n";

    ingress.dec();
    ingress.indent();
    ingress << "}\n";
  } break;
  case RegisterActionType::IncrementAndReturnNewValue: {
    ingress.indent();
    ingress << "void apply(inout " << value_type << " value, out " << value_type << " out_value) {\n";
    ingress.inc();

    ingress.indent();
    ingress << "value = value + 1;\n";
    ingress.indent();
    ingress << "out_value = value;\n";

    ingress.dec();
    ingress.indent();
    ingress << "}\n";
  } break;
  case RegisterActionType::ReadConditionalWrite: {
    todo();
  } break;
  case RegisterActionType::CalculateDiff: {
    const code_t value_cmp = action_name + "_cmp";

    ingress.indent();
    ingress << "void apply(inout " << value_type << " value, out " << value_type << " out_value) {\n";
    ingress.inc();

    ingress.indent();
    ingress << "out_value = " << value_cmp << " - value;\n";

    ingress.dec();
    ingress.indent();
    ingress << "}\n";
  } break;
  case RegisterActionType::SampleEveryFourth: {
    ingress.indent();
    ingress << "void apply(inout " << value_type << " value, out " << value_type << " out_value) {\n";
    ingress.inc();

    ingress.indent();
    ingress << "out_value = 0;\n";

    ingress.indent();
    ingress << "if (value < 3) {\n";
    ingress.inc();

    ingress.indent();
    ingress << "value = value + 1;\n";

    ingress.dec();
    ingress.indent();
    ingress << "} else {\n";
    ingress.inc();

    ingress.indent();
    ingress << "value = 0;\n";
    ingress.indent();
    ingress << "out_value = 1;\n";

    ingress.dec();
    ingress.indent();
    ingress << "}\n";

    ingress.dec();
    ingress.indent();
    ingress << "}\n";
  } break;
  case RegisterActionType::QueryTimestamp: {
    assert_or_panic(external_var.has_value(), "Expected a global variable for the timeout value");
    ingress.indent();
    ingress << "void apply(inout bit<32> alarm, out bool was_alive) {\n";
    ingress.inc();

    ingress.indent();
    ingress << "if (meta.time > alarm) {\n";
    ingress.inc();

    ingress.indent();
    ingress << "was_alive = false;\n";

    ingress.dec();
    ingress.indent();
    ingress << "} else {\n";
    ingress.inc();

    ingress.indent();
    ingress << "was_alive = true;\n";

    ingress.dec();
    ingress.indent();
    ingress << "}\n";

    ingress.dec();
    ingress.indent();
    ingress << "}\n";
  } break;
  case RegisterActionType::QueryAndRefreshTimestamp: {
    assert_or_panic(external_var.has_value(), "Expected a global variable for the timeout value");
    ingress.indent();
    ingress << "void apply(inout bit<32> alarm, out bool was_alive) {\n";
    ingress.inc();

    ingress.indent();
    ingress << "if (meta.time > alarm) {\n";
    ingress.inc();

    ingress.indent();
    ingress << "was_alive = false;\n";
    ingress.indent();
    ingress << "alarm = meta.time + " << external_var.value() << ";\n";

    ingress.dec();
    ingress.indent();
    ingress << "} else {\n";
    ingress.inc();

    ingress.indent();
    ingress << "was_alive = true;\n";
    ingress.dec();
    ingress.indent();
    ingress << "}\n";

    ingress.dec();
    ingress.indent();
    ingress << "}\n";
  } break;
  case RegisterActionType::CheckValue: {
    assert_or_panic(external_var.has_value(), "Expected a global variable for crosschecking the value");
    ingress.indent();
    ingress << "void apply(inout " << value_type << " curr_value, out bit<8> match) {\n";
    ingress.inc();

    ingress.indent();
    ingress << "if (curr_value == " << external_var.value() << ") {\n";
    ingress.inc();

    ingress.indent();
    ingress << "match = 1;\n";
    ingress.dec();
    ingress.indent();
    ingress << "} else {\n";
    ingress.inc();

    ingress.indent();
    ingress << "match = 0;\n";
    ingress.dec();
    ingress.indent();
    ingress << "}\n";

    ingress.dec();
    ingress.indent();
    ingress << "}\n";
  } break;
  }

  ingress.dec();
  ingress.indent();
  ingress << "};\n";

  ingress << "\n";
}

void TofinoSynthesizer::transpile_hash_decl(const Hash *hash) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL);

  if (declared_ds.find(hash->id) != declared_ds.end()) {
    return;
  }

  declared_ds.insert(hash->id);

  if (hash->size >= 32) {
    panic("Hash size too large: %u", hash->size);
  }

  const code_t hash_algo{"CRC32"};

  ingress.indent();
  ingress << "Hash<";
  ingress << TofinoSynthesizer::Transpiler::type_from_size(hash->size);
  ingress << ">(HashAlgorithm_t." << hash_algo << ")";
  ingress << " ";
  ingress << hash->id;
  ingress << ";\n";
}

void TofinoSynthesizer::transpile_hash_calculation(const Hash *hash, const std::vector<code_t> &inputs, code_t &hash_calculator,
                                                   code_t &output_hash) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL);

  output_hash = hash->id + "_value";

  ingress.indent();
  ingress << Transpiler::type_from_size(hash->size) << " " << output_hash << ";\n";

  coder_t hash_calculation_body;

  hash_calculation_body.indent();
  hash_calculation_body << output_hash << " = " << hash->id << ".get({\n";

  hash_calculation_body.inc();

  for (size_t i = 0; i < inputs.size(); i++) {
    const code_t &input = inputs[i];
    hash_calculation_body.indent();
    hash_calculation_body << input;
    if (i != inputs.size() - 1) {
      hash_calculation_body << ",";
    }
    hash_calculation_body << "\n";
  }

  hash_calculation_body.indent();
  hash_calculation_body << "});\n";

  hash_calculator = hash->id + "_calc";
  transpile_action_decl(hash_calculator, hash_calculation_body.split_lines());
}

void TofinoSynthesizer::transpile_digest_decl(const Digest *digest, const std::vector<klee::ref<klee::Expr>> &keys) {
  coder_t &ingress_deparser = get(MARKER_INGRESS_DEPARSER);
  coder_t &custom_headers   = get(MARKER_CUSTOM_HEADERS);

  if (declared_ds.find(digest->id) != declared_ds.end()) {
    return;
  }

  declared_ds.insert(digest->id);

  const code_t digest_hdr = digest->id + "_hdr";

  custom_headers.indent();
  custom_headers << "header " << digest_hdr << " {\n";
  custom_headers.inc();

  for (size_t i = 0; i < digest->fields.size(); i++) {
    custom_headers.indent();
    custom_headers << Transpiler::type_from_size(digest->fields[i]);
    custom_headers << " ";
    custom_headers << "data" + std::to_string(i);
    custom_headers << ";\n";
  }

  custom_headers.dec();
  custom_headers.indent();
  custom_headers << "}\n";
  custom_headers << "\n";

  ingress_deparser.indent();
  ingress_deparser << "Digest<" << digest_hdr << ">() " << digest->id << ";\n";
}

void TofinoSynthesizer::transpile_fcfs_cached_table_decl(const FCFSCachedTable *fcfs_cached_table, const std::vector<klee::ref<klee::Expr>> &keys) {
  if (declared_ds.find(fcfs_cached_table->id) != declared_ds.end()) {
    return;
  }

  declared_ds.insert(fcfs_cached_table->id);

  transpile_hash_decl(&fcfs_cached_table->hash);
  transpile_register_decl(&fcfs_cached_table->cache_expirator);

  for (const Register &reg_key : fcfs_cached_table->cache_keys) {
    transpile_register_decl(&reg_key);
  }
}

code_t TofinoSynthesizer::var_t::get_type() const { return force_bool ? "bool" : TofinoSynthesizer::Transpiler::type_from_expr(expr); }

bool TofinoSynthesizer::var_t::is_bool() const { return force_bool || is_conditional(expr); }

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

TofinoSynthesizer::var_t TofinoSynthesizer::var_t::get_slice(bits_t offset, bits_t slice_size, transpiler_opt_t opt) const {
  assert(offset + slice_size <= size && "Invalid slice");

  bits_t lo;
  bits_t hi;

  if (!is_header_field && !is_buffer) {
    lo = offset;
    hi = offset + slice_size - 1;
  } else {
    lo = size - (offset + slice_size);
    hi = size - offset - 1;
  }

  const code_t slice_name          = name + "[" + std::to_string(hi) + ":" + std::to_string(lo) + "]";
  klee::ref<klee::Expr> slice_expr = solver_toolbox.exprBuilder->Extract(expr, offset, slice_size);

  return var_t(original_name, original_expr, original_size, slice_name, slice_expr, slice_size, force_bool, is_header_field, is_buffer);
}

code_t TofinoSynthesizer::var_t::get_stem() const {
  size_t pos = name.find_last_of('.');
  if (pos == std::string::npos) {
    return name;
  }
  return name.substr(pos + 1);
}

code_t TofinoSynthesizer::var_t::flatten_name() const {
  const std::vector<char> forbidden_chars{'(', ')'};

  bool found_forbidden_char = false;
  for (char c : forbidden_chars) {
    if (name.find(c) != std::string::npos) {
      found_forbidden_char = true;
      break;
    }
  }

  if (!found_forbidden_char) {
    return get_stem();
  }

  const std::vector<char> escape_chars{'(', ')', '.'};
  code_t flat_name = name;
  for (char c : escape_chars) {
    std::replace(flat_name.begin(), flat_name.end(), c, '_');
  }

  return flat_name;
}

std::vector<code_t> TofinoSynthesizer::var_t::split_by_dot() const {
  std::vector<code_t> subnames;

  code_t name_copy = original_name;
  while (true) {
    const size_t pos     = name_copy.find('.');
    const code_t subname = name_copy.substr(0, pos);
    subnames.push_back(subname);
    name_copy = name_copy.substr(pos + 1);
    if (name_copy.find('.') == std::string::npos) {
      subnames.push_back(name_copy);
      break;
    }
  }

  return subnames;
}

std::string TofinoSynthesizer::var_t::to_string() const {
  std::stringstream ss;
  ss << "var_t{";
  ss << "oname: " << original_name << ", ";
  ss << "oexpr: " << expr_to_string(original_expr, true) << ", ";
  ss << "osize: " << original_size << ", ";
  ss << "name: " << name << ", ";
  ss << "expr: " << expr_to_string(expr, true) << ", ";
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
    if (solver_toolbox.are_exprs_always_equal(var.expr, expr)) {
      return var;
    }
  }

  return {};
}

std::optional<TofinoSynthesizer::var_t> TofinoSynthesizer::Stack::get_exact_hdr(klee::ref<klee::Expr> expr) const {
  for (auto var_it = frames.rbegin(); var_it != frames.rend(); var_it++) {
    const var_t &var = *var_it;
    if (var.is_header_field && solver_toolbox.are_exprs_always_equal(var.expr, expr)) {
      return var;
    }
  }

  return {};
}

std::optional<TofinoSynthesizer::var_t> TofinoSynthesizer::Stack::get(klee::ref<klee::Expr> expr, transpiler_opt_t opt) const {
  if (std::optional<var_t> var = get_exact(expr)) {
    if (var->is_header_field && (opt & TRANSPILER_OPT_SWAP_HDR_ENDIANNESS)) {
      var->name = Transpiler::swap_endianness(var->name, var->size);
    }

    return var;
  }

  // No point in looking for a slice if the expression is 1 bit.
  if (expr->getWidth() == 1) {
    return {};
  }

  for (auto var_it = frames.rbegin(); var_it != frames.rend(); var_it++) {
    const var_t &var = *var_it;

    const bits_t expr_size = expr->getWidth();
    const bits_t var_size  = var.size;

    if (expr_size > var_size) {
      continue;
    }

    for (bits_t offset = 0; offset + expr_size <= var_size; offset += 8) {
      klee::ref<klee::Expr> var_slice = solver_toolbox.exprBuilder->Extract(var.expr, offset, expr_size);

      if (solver_toolbox.are_exprs_always_equal(var_slice, expr)) {
        var_t slice = var.get_slice(offset, expr_size, opt);

        if (slice.is_header_field && (opt & TRANSPILER_OPT_SWAP_HDR_ENDIANNESS)) {
          slice.name = Transpiler::swap_endianness(slice.name, slice.size);
        }

        return slice;
      }
    }
  }

  return {};
}

std::optional<TofinoSynthesizer::var_t> TofinoSynthesizer::Stack::get_hdr(klee::ref<klee::Expr> expr, transpiler_opt_t opt) const {
  if (std::optional<var_t> var = get_exact(expr)) {
    if (var->is_header_field) {
      if (opt & TRANSPILER_OPT_SWAP_HDR_ENDIANNESS) {
        var->name = Transpiler::swap_endianness(var->name, var->size);
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
      klee::ref<klee::Expr> var_slice = solver_toolbox.exprBuilder->Extract(var.expr, offset, expr_size);

      if (solver_toolbox.are_exprs_always_equal(var_slice, expr)) {
        var_t slice = var.get_slice(offset, expr_size, opt);

        if (slice.is_header_field && (opt & TRANSPILER_OPT_SWAP_HDR_ENDIANNESS)) {
          slice.name = Transpiler::swap_endianness(slice.name, slice.size);
        }

        return slice;
      }
    }
  }

  return {};
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
    if (std::optional<var_t> var = stack_it->get(expr, opt)) {
      return var;
    }
  }

  return {};
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

  return {};
}

void TofinoSynthesizer::Stacks::clear() { stacks.clear(); }

std::vector<TofinoSynthesizer::Stack> TofinoSynthesizer::Stacks::get_all() const { return stacks; }

TofinoSynthesizer::TofinoSynthesizer(const EP *_ep, std::filesystem::path _out_file)
    : out_file(_out_file), code_template(std::filesystem::path(__FILE__).parent_path() / "Templates" / TEMPLATE_FILENAME,
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
                                             {MARKER_INGRESS_DEPARSER, 1},
                                             {MARKER_INGRESS_DEPARSER_APPLY, 2},
                                             {MARKER_EGRESS_HEADERS, 1},
                                             {MARKER_EGRESS_METADATA, 1},
                                             {MARKER_CONTROL_BLOCKS, 0},
                                         }),
      target_ep(_ep), transpiler(this) {}

coder_t &TofinoSynthesizer::get(const std::string &marker) {
  if (marker == MARKER_INGRESS_CONTROL_APPLY && active_recirc_code_path) {
    return code_template.get(MARKER_INGRESS_CONTROL_APPLY_RECIRC);
  }
  return code_template.get(marker);
}

void TofinoSynthesizer::synthesize() {
  const BDD *bdd = target_ep->get_bdd();

  symbol_t device = bdd->get_device();
  symbol_t time   = bdd->get_time();

  alloc_var("meta.dev", device.expr, EXACT_NAME);
  alloc_var("meta.time", solver_toolbox.exprBuilder->Extract(time.expr, 0, 64), EXACT_NAME);

  ingress_vars.push();

  EPVisitor::visit(target_ep);

  // Transpile the parser after the whole EP has been visited so we have all the headers available.

  ingress_vars.clear();
  ingress_vars.push();

  alloc_var("meta.dev", solver_toolbox.exprBuilder->Extract(device.expr, 0, 16), EXACT_NAME);
  alloc_var("meta.time", solver_toolbox.exprBuilder->Extract(time.expr, 0, 64), EXACT_NAME);

  ingress_vars.push();

  transpile_parser(get_tofino_parser(target_ep));

  coder_t &cpu_hdr = get(MARKER_CPU_HEADER);
  for (const var_t &var : cpu_hdr_vars.get_all()) {
    const bits_t pad = var.is_bool() ? var.expr->getWidth() - 1 : (8 - var.expr->getWidth()) % 8;

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

  coder_t &ingress_deparser = get(MARKER_INGRESS_DEPARSER_APPLY);
  ingress_deparser.indent();
  ingress_deparser << "pkt.emit(hdr);";

  std::ofstream ofs(out_file);
  ofs << code_template.dump();
  ofs.close();
}

void TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const Module *module = ep_node->get_module();
  if (module->get_target() == TargetType::Tofino) {
    ingress_apply.indent();
    ingress_apply << "// EP node  " << ep_node->get_id() << ":" << module->get_name() << "\n";
    ingress_apply.indent();
    ingress_apply << "// BDD node " << module->get_node()->dump(true, true) << "\n";
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

    for (bdd_node_id_t id : state->ids) {
      if (parser_vars.find(id) != parser_vars.end()) {
        const Stack &vars = parser_vars.at(id);
        ingress_vars.insert_back(vars);
      }
    }

    switch (state->type) {
    case ParserStateType::Extract: {
      const ParserStateExtract *extract = dynamic_cast<const ParserStateExtract *>(state);
      const code_t state_name           = get_parser_state_name(state, state_init);

      std::optional<var_t> hdr_var = hdr_vars.get(extract->hdr);
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
    case ParserStateType::Select: {
      const ParserStateSelect *select = dynamic_cast<const ParserStateSelect *>(state);
      const code_t state_name         = get_parser_state_name(state, state_init);

      auto get_selection_state_name = [state_name](size_t i) -> code_t { return state_name + "_" + std::to_string(i); };

      ingress_parser.indent();
      ingress_parser << "state " << state_name << " {\n";

      ingress_parser.inc();
      ingress_parser.indent();
      ingress_parser << "transition " << get_selection_state_name(0) << ";\n";

      ingress_parser.dec();
      ingress_parser.indent();
      ingress_parser << "}\n";

      for (size_t i = 0; i < select->selections.size(); i++) {
        const parser_selection_t &selection = select->selections[i];
        const code_t selection_state_name   = state_name + "_" + std::to_string(i);

        ingress_parser.indent();
        ingress_parser << "state " << get_selection_state_name(i) << " {\n";

        ingress_parser.inc();
        ingress_parser.indent();
        ingress_parser << "transition select (" << transpiler.transpile(selection.target) << ") {\n";

        ingress_parser.inc();

        const code_t next_true = get_parser_state_name(select->on_true, false);
        const code_t next_false =
            i < select->selections.size() - 1 ? get_selection_state_name(i + 1) : get_parser_state_name(select->on_false, false);

        for (klee::ref<klee::Expr> value : selection.values) {
          ingress_parser.indent();
          ingress_parser << transpiler.transpile(value, TRANSPILER_OPT_SWAP_CONST_ENDIANNESS) << ": " << next_true << ";\n";
        }

        ingress_parser.indent();
        ingress_parser << "default: " << next_false << ";\n";

        ingress_parser.dec();
        ingress_parser.indent();
        ingress_parser << "}\n";

        ingress_parser.dec();
        ingress_parser.indent();
        ingress_parser << "}\n";
      }

      states.push_back(select->on_true);
      states.push_back(select->on_false);
    } break;
    case ParserStateType::Terminate: {
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

    state_init = false;
  }
}

TofinoSynthesizer::var_t TofinoSynthesizer::alloc_var(const code_t &proposed_name, klee::ref<klee::Expr> expr, alloc_opt_t option) {
  assert(!expr.isNull());

  code_t name = (option & EXACT_NAME) ? proposed_name : create_unique_name(proposed_name);

  if (option & IS_INGRESS_METADATA) {
    name = "meta." + name;
  }

  const var_t var(name, expr, expr->getWidth(), option & FORCE_BOOL, option & (HEADER | HEADER_FIELD), option & BUFFER);

  if (!(option & SKIP_STACK_ALLOC)) {
    if (option & HEADER) {
      hdr_vars.push(var);
    } else {
      ingress_vars.insert_back(var);
    }
  }

  return var;
}

code_path_t TofinoSynthesizer::alloc_recirc_coder() {
  const size_t size = recirc_coders.size();
  recirc_coders.emplace_back();
  return size;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::SendToController *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);
  const Symbols &symbols = node->get_symbols();

  ingress_apply.indent();
  ingress_apply << "fwd_op = fwd_op_t.FORWARD_TO_CPU;\n";
  ingress_apply.indent();
  ingress_apply << "build_cpu_hdr(" << ep_node->get_id() << ");\n";

  for (const symbol_t &symbol : symbols.get()) {
    std::optional<var_t> var = ingress_vars.get(symbol.expr);

    if (!var) {
      dbg_vars();
      panic("Variable %s not found in stack", expr_to_string(symbol.expr, true).c_str());
    }

    if (var->is_header_field) {
      // Header fields are not sent to the controller, so we skip them.
      continue;
    }

    var_t cpu_var = *var;
    cpu_var.name  = "hdr.cpu." + var->get_stem();
    cpu_hdr_vars.push(cpu_var);

    ingress_apply.indent();
    ingress_apply << cpu_var.name;

    // Hack
    if (symbol.name == "next_time") {
      ingress_apply << "[47:16]";
    }

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
  const code_path_t code_path = alloc_recirc_coder();

  // 2. Build the recirculation header and populate it with the current stack
  ingress_apply.indent();
  ingress_apply << "fwd_op = fwd_op_t.RECIRCULATE;\n";
  ingress_apply.indent();
  ingress_apply << "build_recirc_hdr(" << code_path << ");\n";

  Stacks stack_backup = ingress_vars;

  // The first stack contains the variables set up right at the beginning of the ingress processing.
  // Every other stack contains variables introduced by modules.
  Stack first_stack              = ingress_vars.get_first_stack();
  std::vector<var_t> recirc_vars = first_stack.get_all();

  for (const var_t &var : ingress_vars.squash().get_all()) {
    if (first_stack.get_exact(var.expr)) {
      continue;
    }

    if (var.is_header_field) {
      recirc_vars.push_back(var);
      continue;
    }

    var_t recirc_var = var;
    recirc_var.name  = recirc_var.flatten_name();

    var_t local_recirc_var = recirc_var;
    local_recirc_var.name  = "hdr.recirc." + recirc_var.name;

    recirc_hdr_vars.push(recirc_var);
    recirc_vars.push_back(local_recirc_var);

    ingress_apply.indent();
    ingress_apply << local_recirc_var.name;
    ingress_apply << " = ";
    ingress_apply << var.name;
    ingress_apply << ";\n";
  }

  // 3. Replace the ingress apply coder with the recirc coder
  active_recirc_code_path = code_path;

  // 4. Clear the stack, rebuild it with hdr.recirc fields, and setup the recirculation code block
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

  // 5. Revert the state back to before the recirculation was made
  active_recirc_code_path.reset();
  ingress_vars = stack_backup;

  return EPVisitor::Action::skipChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Ignore *node) { return EPVisitor::Action::doChildren; }

void TofinoSynthesizer::transpile_if_condition(const If::condition_t &condition) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL_APPLY);

  switch (condition.phv_limitation_workaround.action_helper) {
  case If::ConditionActionHelper::None:
    ingress.indent();
    ingress << "if (";
    ingress << transpiler.transpile(condition.expr);
    ingress << ")";
    break;
  case If::ConditionActionHelper::CheckSignBitForLessThan32b:
    // a < b <=> (a - b)[31:31] == 1
    ingress.indent();
    ingress << "calculate_diff_32b(";
    ingress << transpiler.transpile(condition.phv_limitation_workaround.lhs);
    ingress << ", ";
    ingress << transpiler.transpile(condition.phv_limitation_workaround.rhs);
    ingress << ");\n";

    ingress.indent();
    ingress << "if (diff_sign_bit == 1)";
    break;
  case If::ConditionActionHelper::CheckSignBitForLessThanOrEqual32b:
    // a <= b <=> (b - a)[31:31] == 0
    ingress.indent();
    ingress << "calculate_diff_32b(";
    ingress << transpiler.transpile(condition.phv_limitation_workaround.rhs);
    ingress << ", ";
    ingress << transpiler.transpile(condition.phv_limitation_workaround.lhs);
    ingress << ");\n";

    ingress.indent();
    ingress << "if (diff_sign_bit == 0)";
    break;
  case If::ConditionActionHelper::CheckSignBitForGreaterThan32b:
    // a > b <=> (b - a)[31:31] == 1
    ingress.indent();
    ingress << "calculate_diff_32b(";
    ingress << transpiler.transpile(condition.phv_limitation_workaround.rhs);
    ingress << ", ";
    ingress << transpiler.transpile(condition.phv_limitation_workaround.lhs);
    ingress << ");\n";

    ingress.indent();
    ingress << "if (diff_sign_bit == 1)";
    break;
  case If::ConditionActionHelper::CheckSignBitForGreaterThanOrEqual32b:
    // a >= b <=> (a - b)[31:31] == 0
    ingress.indent();
    ingress << "calculate_diff_32b(";
    ingress << transpiler.transpile(condition.phv_limitation_workaround.lhs);
    ingress << ", ";
    ingress << transpiler.transpile(condition.phv_limitation_workaround.rhs);
    ingress << ");\n";

    ingress.indent();
    ingress << "if (diff_sign_bit == 0)";
    break;
  }
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::If *node) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL_APPLY);

  const std::vector<If::condition_t> &conditions = node->get_conditions();
  const std::vector<EPNode *> &children          = ep_node->get_children();
  assert(children.size() == 2 && "If node must have 2 children");

  const EPNode *then_node = children[0];
  const EPNode *else_node = children[1];

  if (conditions.size() == 1) {
    const If::condition_t &condition = conditions[0];

    transpile_if_condition(condition);
    ingress << "{\n";

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

  const var_t cond_var = alloc_var("cond", solver_toolbox.exprBuilder->True(), FORCE_BOOL);
  cond_var.declare(ingress, "false");

  for (const If::condition_t &condition : conditions) {
    transpile_if_condition(condition);
    ingress << "{\n";
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
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Then *node) { return EPVisitor::Action::doChildren; }

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Else *node) { return EPVisitor::Action::doChildren; }

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Forward *node) {
  klee::ref<klee::Expr> dst_device = node->get_dst_device();
  coder_t &ingress                 = get(MARKER_INGRESS_CONTROL_APPLY);

  ingress.indent();
  ingress << "nf_dev[15:0] = " << transpiler.transpile(dst_device, TRANSPILER_OPT_SWAP_HDR_ENDIANNESS) << ";\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Drop *node) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL_APPLY);

  ingress.indent();
  ingress << "fwd_op = fwd_op_t.DROP;\n";

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
  const klee::ref<klee::Expr> hdr_expr                       = node->get_hdr();
  const std::vector<klee::ref<klee::Expr>> &hdr_fields_guess = node->get_hdr_fields_guess();

  ingress_vars.push();

  var_t hdr;
  bool already_allocated = false;

  if (std::optional<var_t> allocated_hdr = hdr_vars.get_exact(hdr_expr)) {
    hdr               = *allocated_hdr;
    already_allocated = true;
  } else {
    const code_t hdr_name = create_unique_name("hdr");
    hdr                   = alloc_var("hdr." + hdr_name, hdr_expr, EXACT_NAME | HEADER);
  }

  const code_t hdr_name = hdr.split_by_dot()[1];

  std::vector<var_t> hdr_data;
  for (klee::ref<klee::Expr> field : hdr_fields_guess) {
    const var_t var = alloc_var("hdr." + hdr_name + ".data" + std::to_string(hdr_data.size()), field, EXACT_NAME | HEADER_FIELD);
    hdr_data.push_back(var);
  }

  if (!already_allocated) {
    coder_t &custom_hdrs = get(MARKER_CUSTOM_HEADERS);
    custom_hdrs.indent();
    custom_hdrs << "header " << hdr_name << "_h {\n";

    custom_hdrs.inc();
    for (const var_t &field : hdr_data) {
      custom_hdrs.indent();
      custom_hdrs << TofinoSynthesizer::Transpiler::type_from_expr(field.expr) << " " << field.get_stem() << ";\n";
    }

    custom_hdrs.dec();
    custom_hdrs.indent();
    custom_hdrs << "}\n";

    coder_t &ingress_hdrs = get(MARKER_INGRESS_HEADERS);
    ingress_hdrs.indent();
    ingress_hdrs << hdr_name << "_h " << hdr_name << ";\n";
  }

  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);
  ingress_apply.indent();
  ingress_apply << "if(" << hdr.name << ".isValid()) {\n";

  ingress_apply.inc();

  parser_vars[node->get_node()->get_id()] = ingress_vars.squash_hdrs_only();

  assert(ep_node->get_children().size() == 1 && "ParserExtraction must have 1 child");
  visit(ep, ep_node->get_children()[0]);

  ingress_apply.dec();

  ingress_apply.indent();
  ingress_apply << "}\n";

  ingress_vars.pop();

  return EPVisitor::Action::skipChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::ModifyHeader *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  klee::ref<klee::Expr> hdr                  = node->get_hdr();
  const std::vector<expr_mod_t> &changes     = node->get_changes();
  const std::vector<expr_byte_swap_t> &swaps = node->get_swaps();

  std::unordered_set<bytes_t> bytes_already_dealt_with;
  for (const expr_byte_swap_t &byte_swap : swaps) {
    klee::ref<klee::Expr> byte0_expr = solver_toolbox.exprBuilder->Extract(hdr, byte_swap.byte0 * 8, 8);
    klee::ref<klee::Expr> byte1_expr = solver_toolbox.exprBuilder->Extract(hdr, byte_swap.byte1 * 8, 8);

    std::optional<var_t> byte0_var = ingress_vars.get_hdr(byte0_expr);
    std::optional<var_t> byte1_var = ingress_vars.get_hdr(byte1_expr);

    assert(byte0_var.has_value() && "Byte0 not found");
    assert(byte1_var.has_value() && "Byte1 not found");

    ingress_apply.indent();
    ingress_apply << "swap(";
    ingress_apply << byte0_var->name;
    ingress_apply << ", ";
    ingress_apply << byte1_var->name;
    ingress_apply << ");\n";

    bytes_already_dealt_with.insert(byte_swap.byte0);
    bytes_already_dealt_with.insert(byte_swap.byte1);
  }

  std::vector<code_t> assignments;

  for (size_t i = 0; i < changes.size(); i++) {
    const expr_mod_t &mod = changes[i];

    if (bytes_already_dealt_with.find(mod.offset / 8) != bytes_already_dealt_with.end()) {
      continue;
    }

    if (std::optional<var_t> var = ingress_vars.get(mod.expr)) {
      bytes_t matching_bytes = 0;
      for (bytes_t b = 0; b < var->original_size / 8; b++) {
        if (b + i >= changes.size() || changes[b + i].width != 8 || changes[b + i].offset != mod.offset + b * 8) {
          break;
        }

        std::optional<var_t> next_var = ingress_vars.get(changes[b + i].expr);
        if (!next_var.has_value() || next_var->original_name != var->original_name) {
          break;
        }

        matching_bytes++;
      }

      if (matching_bytes == var->original_size / 8) {
        klee::ref<klee::Expr> target_hdr_expr = solver_toolbox.exprBuilder->Extract(hdr, mod.offset, var->original_size);
        std::optional<var_t> target_hdr_var   = ingress_vars.get_hdr(target_hdr_expr);

        if (target_hdr_var.has_value()) {
          coder_t assignment;
          assignment << target_hdr_var->name;
          assignment << " = ";
          assignment << var->original_name;
          assignment << ";";

          assignments.push_back(assignment.dump());

          for (bytes_t b = 0; b < matching_bytes; b++) {
            bytes_already_dealt_with.insert(changes[b + i].offset / 8);
          }

          continue;
        }
      }
    }

    klee::ref<klee::Expr> target_hdr_expr = solver_toolbox.exprBuilder->Extract(hdr, mod.offset, mod.width);
    std::optional<var_t> target_hdr_var   = ingress_vars.get_hdr(target_hdr_expr);
    assert(target_hdr_var.has_value() && "Target hdr field not found");

    coder_t assignment;
    assignment << target_hdr_var->name;
    assignment << " = ";
    assignment << transpiler.transpile(mod.expr);
    assignment << ";";

    assignments.push_back(assignment.dump());

    bytes_already_dealt_with.insert(mod.offset / 8);
  }

  // I have no idea why this is necessary, but it is...
  // For the cases where we have a lot of register accesses, interleaving these accesses with each other helps the compiler find a
  // placement solution.
  const size_t steps = 4;
  for (size_t step = 0; step < steps; step++) {
    for (size_t i = step; i < assignments.size(); i += steps) {
      ingress_apply.indent();
      ingress_apply << assignments[i] << "\n";
    }
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::MapTableLookup *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const DS_ID map_table_id                       = node->get_id();
  const std::vector<klee::ref<klee::Expr>> &keys = node->get_keys();
  const klee::ref<klee::Expr> value              = node->get_value();
  const std::optional<symbol_t> hit              = node->get_hit();

  const MapTable *map_table = get_tofino_ds<MapTable>(ep, map_table_id);

  const bdd_node_id_t node_id = node->get_node()->get_id();
  const Table *table          = map_table->get_table(node_id);
  assert(table && "Table not found");

  std::vector<var_t> keys_vars;
  transpile_table_decl(table, keys, {value}, false, keys_vars);
  assert(keys_vars.size() == keys.size());

  for (const var_t &key_var : keys_vars) {
    ingress_apply.indent();
    ingress_apply << key_var.name << " = " << transpiler.transpile(key_var.expr) << ";\n";
  }

  if (hit) {
    const var_t hit_var = alloc_var("hit", hit->expr, FORCE_BOOL);
    hit_var.declare(ingress_apply, table->id + ".apply().hit");
  } else {
    ingress_apply.indent();
    ingress_apply << table->id << ".apply();\n";
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::GuardedMapTableLookup *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const DS_ID id                                 = node->get_id();
  const std::vector<klee::ref<klee::Expr>> &keys = node->get_keys();
  const klee::ref<klee::Expr> value              = node->get_value();
  const std::optional<symbol_t> hit              = node->get_hit();

  const GuardedMapTable *guarded_map_table = get_tofino_ds<GuardedMapTable>(ep, id);

  const bdd_node_id_t node_id = node->get_node()->get_id();
  const Table *table          = guarded_map_table->get_table(node_id);
  assert(table && "Table not found");

  std::vector<var_t> keys_vars;
  transpile_table_decl(table, keys, {value}, false, keys_vars);
  assert(keys_vars.size() == keys.size());

  const Register &guard = guarded_map_table->guard;
  transpile_register_decl(&guard);

  for (const var_t &key_var : keys_vars) {
    ingress_apply.indent();
    ingress_apply << key_var.name << " = " << transpiler.transpile(key_var.expr) << ";\n";
  }

  if (hit) {
    const var_t hit_var = alloc_var("hit", hit->expr, FORCE_BOOL | IS_INGRESS_METADATA);
    declare_var_in_ingress_metadata(hit_var);
    ingress_apply.indent();
    ingress_apply << hit_var.name << " = " << table->id << ".apply().hit;\n";
  } else {
    ingress_apply.indent();
    ingress_apply << table->id << ".apply();\n";
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::GuardedMapTableGuardCheck *node) {
  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const bdd_node_id_t node_id                 = node->get_node()->get_id();
  const DS_ID id                              = node->get_id();
  const symbol_t &guard_allow                 = node->get_guard_allow();
  klee::ref<klee::Expr> guard_allow_condition = node->get_guard_allow_condition();

  const GuardedMapTable *guarded_map_table = get_tofino_ds<GuardedMapTable>(ep, id);
  const Register &guard                    = guarded_map_table->guard;

  assert(guard.actions.size() == 1);
  const RegisterActionType action_type      = *guard.actions.begin();
  const code_t guard_allow_read_action_name = build_register_action_name(&guard, action_type, ep_node);
  transpile_register_action_decl(&guard, guard_allow_read_action_name, action_type, {});

  const code_t guard_value    = id + "_guard_value_" + std::to_string(node_id);
  const var_t guard_value_var = alloc_var(guard_value, guard_allow.expr, SKIP_STACK_ALLOC);

  guard_value_var.declare(ingress, "0");

  coder_t guard_allow_check_body;
  guard_allow_check_body.indent();
  guard_allow_check_body << guard_value_var.name << " = " << guard_allow_read_action_name << ".execute(0);\n";
  const code_t guard_allow_check = id + "_guard_check_" + std::to_string(node_id);
  transpile_action_decl(guard_allow_check, guard_allow_check_body.split_lines());

  ingress_apply.indent();
  ingress_apply << guard_allow_check << "();\n";

  const code_t guard_allow_value = id + "_guard_allow";
  const var_t guard_allow_var    = alloc_var(guard_allow_value, guard_allow.expr, FORCE_BOOL | IS_INGRESS_METADATA);

  declare_var_in_ingress_metadata(guard_allow_var);

  ingress_apply.indent();
  ingress_apply << guard_allow_var.name << " = false;\n";

  ingress_apply.indent();
  ingress_apply << "if (" << guard_value_var.name << " != 0) {\n";

  ingress_apply.inc();
  ingress_apply.indent();
  ingress_apply << guard_allow_var.name << " = true;\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorTableLookup *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const DS_ID vector_table_id = node->get_id();
  klee::ref<klee::Expr> key   = node->get_key();
  klee::ref<klee::Expr> value = node->get_value();

  const VectorTable *vector_table = get_tofino_ds<VectorTable>(ep, vector_table_id);

  bdd_node_id_t node_id = node->get_node()->get_id();
  const Table *table    = vector_table->get_table(node_id);
  assert(table && "Table not found");

  code_t transpiled_key = transpiler.transpile(key, TRANSPILER_OPT_SWAP_HDR_ENDIANNESS);

  std::vector<var_t> keys_vars;
  transpile_table_decl(table, {key}, {value}, true, keys_vars);

  assert(keys_vars.size() == 1);
  var_t key_var = keys_vars[0];

  ingress_apply.indent();
  ingress_apply << key_var.name << " = " << transpiled_key << ";\n";

  ingress_apply.indent();
  ingress_apply << table->id << ".apply();\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::DchainTableLookup *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const DS_ID dchain_table_id = node->get_id();
  klee::ref<klee::Expr> key   = node->get_key();
  std::optional<symbol_t> hit = node->get_hit();

  const DchainTable *dchain_table = get_tofino_ds<DchainTable>(ep, dchain_table_id);

  bdd_node_id_t node_id = node->get_node()->get_id();
  const Table *table    = dchain_table->get_table(node_id);
  assert(table && "Table not found");

  const code_t transpiled_key = transpiler.transpile(key, TRANSPILER_OPT_SWAP_HDR_ENDIANNESS);

  std::vector<var_t> keys_vars;
  transpile_table_decl(table, {key}, {}, false, keys_vars);

  assert(keys_vars.size() == 1);
  var_t key_var = keys_vars[0];

  ingress_apply.indent();
  ingress_apply << key_var.name << " = " << transpiled_key << ";\n";

  if (hit) {
    var_t hit_var = alloc_var("hit", hit->expr, FORCE_BOOL);
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

  const DS_ID id              = node->get_id();
  klee::ref<klee::Expr> index = node->get_index();
  klee::ref<klee::Expr> value = node->get_value();

  const VectorRegister *vector_register = get_tofino_ds<VectorRegister>(ep, id);

  std::vector<const Register *> regs;
  for (const Register &reg : vector_register->regs) {
    regs.push_back(&reg);
  }

  std::sort(regs.begin(), regs.end(), [](const Register *r0, const Register *r1) { return natural_compare(r0->id, r1->id); });

  for (const Register *reg : regs) {
    transpile_register_decl(reg);
  }

  if (!regs.empty()) {
    ingress << "\n";
  }

  int i         = 0;
  bits_t offset = 0;
  for (const Register *reg : regs) {
    const code_t action_name = build_register_action_name(reg, RegisterActionType::Read, ep_node);
    transpile_register_action_decl(reg, action_name, RegisterActionType::Read, {});

    const klee::ref<klee::Expr> entry_expr = solver_toolbox.exprBuilder->Extract(value, offset, reg->value_size);
    const code_t assignment                = action_name + ".execute(" + transpiler.transpile(index, TRANSPILER_OPT_SWAP_HDR_ENDIANNESS) + ")";

    const std::string value_prefix_name = "vector_reg_value";
    const var_t value_var               = alloc_var(value_prefix_name, entry_expr);

    ingress << "\n";
    value_var.declare(ingress_apply, assignment);

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
    transpile_register_decl(reg);
  }

  if (!regs.empty()) {
    ingress << "\n";
  }

  bits_t offset = 0;
  for (const Register *reg : regs) {
    const klee::ref<klee::Expr> reg_write_expr = solver_toolbox.exprBuilder->Extract(write_value, offset, reg->value_size);
    std::optional<var_t> reg_write_var         = ingress_vars.get(reg_write_expr);

    if (!reg_write_var) {
      const code_t write_expr       = transpiler.transpile(reg_write_expr);
      const var_t new_reg_write_var = alloc_var("reg_write", reg_write_expr);

      ingress_apply.indent();
      ingress_apply << new_reg_write_var.name << " = " << write_expr << ";\n";

      reg_write_var = new_reg_write_var;
    }

    const code_t action_name = build_register_action_name(reg, RegisterActionType::Write, ep_node);
    transpile_register_action_decl(reg, action_name, RegisterActionType::Write, reg_write_var->name);

    ingress_apply.indent();
    ingress_apply << action_name;
    ingress_apply << ".execute(" << transpiler.transpile(index) << ");\n";

    offset += reg->value_size;
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::LPMLookup *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  DS_ID lpm_id                 = node->get_lpm_id();
  klee::ref<klee::Expr> addr   = node->get_addr();
  klee::ref<klee::Expr> device = node->get_device();
  klee::ref<klee::Expr> match  = node->get_match();

  const LPM *lpm = get_tofino_ds<LPM>(ep, lpm_id);

  transpile_lpm_decl(lpm, addr, device);

  const code_t transpiled_key = transpiler.transpile(addr);

  std::optional<var_t> key_var = ingress_vars.get(addr);
  assert(key_var && "Key is not a variable");

  ingress_apply.indent();
  ingress_apply << key_var->name << " = " << transpiled_key << ";\n";

  var_t hit_var = alloc_var("hit", match, FORCE_BOOL);
  hit_var.declare(ingress_apply, lpm_id + ".apply().hit");

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableRead *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const DS_ID fcfscached_table_id                = node->get_fcfs_cached_table_id();
  const std::vector<klee::ref<klee::Expr>> &keys = node->get_keys();
  const symbol_t &hit                            = node->get_map_has_this_key();

  const FCFSCachedTable *fcfs_cached_table = get_tofino_ds<FCFSCachedTable>(ep, fcfscached_table_id);
  const DS_ID table_id                     = node->get_used_table_id();
  fcfs_cached_table->debug();
  const Table *table = fcfs_cached_table->get_table(table_id);
  assert(table && "Table not found");

  transpile_fcfs_cached_table_decl(fcfs_cached_table, keys);

  std::vector<var_t> keys_vars;
  transpile_table_decl(table, keys, {}, true, keys_vars);

  const code_t cache_expirator_action_name =
      build_register_action_name(&fcfs_cached_table->cache_expirator, RegisterActionType::QueryTimestamp, ep_node);

  transpile_register_action_decl(&fcfs_cached_table->cache_expirator, cache_expirator_action_name, RegisterActionType::QueryTimestamp,
                                 std::to_string(FCFSCachedTable::ENTRY_TIMEOUT));

  assert(fcfs_cached_table->cache_keys.size() == keys_vars.size());
  std::vector<code_t> reg_keys_read_actions;
  for (size_t i = 0; i < keys_vars.size(); i++) {
    const Register &reg                   = fcfs_cached_table->cache_keys[i];
    const code_t reg_key_read_action_name = build_register_action_name(&reg, RegisterActionType::CheckValue, ep_node);
    reg_keys_read_actions.push_back(reg_key_read_action_name);
    transpile_register_action_decl(&reg, reg_key_read_action_name, RegisterActionType::CheckValue, keys_vars[i].name);
  }

  std::vector<code_t> hash_inputs;
  for (const var_t &key_var : keys_vars) {
    hash_inputs.push_back(key_var.name);
  }
  code_t hash_calculator;
  code_t output_hash;
  transpile_hash_calculation(&fcfs_cached_table->hash, hash_inputs, hash_calculator, output_hash);

  for (const var_t &key_var : keys_vars) {
    ingress_apply.indent();
    ingress_apply << key_var.name << " = " << transpiler.transpile(key_var.expr) << ";\n";
  }

  const var_t hit_var = alloc_var("hit", hit.expr, FORCE_BOOL);
  hit_var.declare(ingress_apply, table->id + ".apply().hit");

  ingress_apply.indent();
  ingress_apply << "if (!" << hit_var.name << ") {\n";
  ingress_apply.inc();

  ingress_apply.indent();
  ingress_apply << hash_calculator << "();\n";

  ingress_apply.indent();
  ingress_apply << "if (" << cache_expirator_action_name << ".execute(" << output_hash << ")) {\n";
  ingress_apply.inc();

  const var_t match_counter_var = alloc_var("match_counter", hit.expr, FORCE_BOOL);
  match_counter_var.declare(ingress_apply, "0");

  for (const code_t &reg_key_read_action_name : reg_keys_read_actions) {
    ingress_apply.indent();
    ingress_apply << match_counter_var.name << " = " << match_counter_var.name << " + " << reg_key_read_action_name << ".execute(" << output_hash
                  << ");\n";
  }

  ingress_apply.indent();
  ingress_apply << "if (" << match_counter_var.name << " == " << keys_vars.size() << ") {\n";
  ingress_apply.inc();

  ingress_apply.indent();
  ingress_apply << hit_var.name << " == true;\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableReadWrite *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const DS_ID fcfscached_table_id                = node->get_fcfs_cached_table_id();
  const std::vector<klee::ref<klee::Expr>> &keys = node->get_keys();
  const symbol_t &hit                            = node->get_map_has_this_key();
  const symbol_t &cache_hit                      = node->get_cache_hit();

  const FCFSCachedTable *fcfs_cached_table = get_tofino_ds<FCFSCachedTable>(ep, fcfscached_table_id);
  const bdd_node_id_t node_id              = node->get_node()->get_id();
  const Table *table                       = fcfs_cached_table->get_table(node_id);
  assert(table && "Table not found");

  transpile_fcfs_cached_table_decl(fcfs_cached_table, keys);

  std::vector<var_t> keys_vars;
  transpile_table_decl(table, keys, {}, true, keys_vars);

  const code_t cache_expirator_action_name =
      build_register_action_name(&fcfs_cached_table->cache_expirator, RegisterActionType::QueryAndRefreshTimestamp, ep_node);

  transpile_register_action_decl(&fcfs_cached_table->cache_expirator, cache_expirator_action_name, RegisterActionType::QueryAndRefreshTimestamp,
                                 std::to_string(FCFSCachedTable::ENTRY_TIMEOUT));

  assert(fcfs_cached_table->cache_keys.size() == keys_vars.size());
  std::vector<code_t> reg_keys_read_actions;
  for (size_t i = 0; i < keys_vars.size(); i++) {
    const Register &reg                   = fcfs_cached_table->cache_keys[i];
    const code_t reg_key_read_action_name = build_register_action_name(&reg, RegisterActionType::CheckValue, ep_node);
    reg_keys_read_actions.push_back(reg_key_read_action_name);
    transpile_register_action_decl(&reg, reg_key_read_action_name, RegisterActionType::CheckValue, keys_vars[i].name);
  }

  std::vector<code_t> reg_keys_write_actions;
  for (size_t i = 0; i < keys_vars.size(); i++) {
    const Register &reg                    = fcfs_cached_table->cache_keys[i];
    const code_t reg_key_write_action_name = build_register_action_name(&reg, RegisterActionType::Write, ep_node);
    reg_keys_write_actions.push_back(reg_key_write_action_name);
    transpile_register_action_decl(&reg, reg_key_write_action_name, RegisterActionType::Write, keys_vars[i].name);
  }

  transpile_digest_decl(&fcfs_cached_table->digest, keys);

  std::vector<code_t> hash_inputs;
  for (const var_t &key_var : keys_vars) {
    hash_inputs.push_back(key_var.name);
  }
  code_t hash_calculator;
  code_t output_hash;
  transpile_hash_calculation(&fcfs_cached_table->hash, hash_inputs, hash_calculator, output_hash);

  for (const var_t &key_var : keys_vars) {
    ingress_apply.indent();
    ingress_apply << key_var.name << " = " << transpiler.transpile(key_var.expr) << ";\n";
  }

  const var_t fcfs_ct_cache_hit_var = alloc_var("fcfs_ct_cache_hit", cache_hit.expr, FORCE_BOOL);
  fcfs_ct_cache_hit_var.declare(ingress_apply, "true");

  const var_t hit_var = alloc_var("hit", hit.expr, FORCE_BOOL);
  hit_var.declare(ingress_apply, table->id + ".apply().hit");

  ingress_apply.indent();
  ingress_apply << "if (!" << hit_var.name << ") {\n";
  ingress_apply.inc();

  ingress_apply.indent();
  ingress_apply << hash_calculator << "();\n";

  ingress_apply.indent();
  ingress_apply << "if (" << cache_expirator_action_name << ".execute(" << output_hash << ")) {\n";
  ingress_apply.inc();

  const code_t match_counter_var_name = create_unique_name("match_counter");
  ingress_apply.indent();
  ingress_apply << Transpiler::type_from_size(8) << match_counter_var_name << " = 0;\n";

  for (const code_t &reg_key_read_action_name : reg_keys_read_actions) {
    ingress_apply.indent();
    ingress_apply << match_counter_var_name << " = " << match_counter_var_name << " + " << reg_key_read_action_name << ".execute(" << output_hash
                  << ");\n";
  }

  ingress_apply.indent();
  ingress_apply << "if (" << match_counter_var_name << " != " << keys_vars.size() << ") {\n";
  ingress_apply.inc();

  ingress_apply.indent();
  ingress_apply << fcfs_ct_cache_hit_var.name << " = false;\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "} else {\n";
  ingress_apply.inc();

  ingress_apply.indent();
  ingress_apply << hit_var.name << " = true;\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "} else {\n";
  ingress_apply.inc();

  ingress_apply.indent();
  ingress_apply << hit_var.name << " = true;\n";

  for (const code_t &reg_key_write_action_name : reg_keys_write_actions) {
    ingress_apply.indent();
    ingress_apply << reg_key_write_action_name << ".execute(" << output_hash << ");\n";
  }

  ingress_apply.indent();
  ingress_apply << "ig_dprsr_md.digest_type = " << fcfs_cached_table->digest.digest_type << ";\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  transpile_digest(fcfs_cached_table->digest, keys);

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableWrite *node) {
  panic("TODO: FCFSCachedTableWrite");
  return EPVisitor::Action::doChildren;
}

void TofinoSynthesizer::transpile_digest(const Digest &digest, const std::vector<klee::ref<klee::Expr>> &keys) {
  coder_t &ingress_deparser_apply = get(MARKER_INGRESS_DEPARSER_APPLY);

  ingress_deparser_apply.indent();
  ingress_deparser_apply << "if (";
  ingress_deparser_apply << "ig_dprsr_md.digest_type == " << digest.digest_type;
  ingress_deparser_apply << ") {\n";

  ingress_deparser_apply.inc();

  ingress_deparser_apply.indent();
  ingress_deparser_apply << digest.id << ".pack({\n";
  ingress_deparser_apply.inc();

  for (klee::ref<klee::Expr> key : keys) {
    std::optional<var_t> key_var = ingress_vars.get_hdr(key);

    if (!key_var.has_value()) {
      dbg_vars();
      panic("Key is not a header field variable:\n%s", expr_to_string(key).c_str());
    }

    ingress_deparser_apply.indent();
    ingress_deparser_apply << key_var->name << ",\n";
  }

  ingress_deparser_apply.dec();
  ingress_deparser_apply.indent();
  ingress_deparser_apply << "});\n";

  ingress_deparser_apply.dec();
  ingress_deparser_apply.indent();
  ingress_deparser_apply << "}\n";
  ingress_deparser_apply << "\n";
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::HHTableRead *node) {
  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const DS_ID hh_table_id                        = node->get_hh_table_id();
  const std::vector<klee::ref<klee::Expr>> &keys = node->get_keys();
  klee::ref<klee::Expr> index                    = node->get_value();
  std::optional<symbol_t> hit                    = node->get_hit();
  assert(hit.has_value() && "Hit is not a variable");

  const HHTable *hh_table = get_tofino_ds<HHTable>(ep, hh_table_id);

  const bdd_node_id_t node_id = node->get_node()->get_id();
  const Table *table          = hh_table->get_table(node_id);
  assert(table && "Table not found");

  std::vector<var_t> keys_vars;
  transpile_table_decl(table, keys, {index}, false, keys_vars);
  assert(keys_vars.size() == keys.size());

  transpile_register_decl(&hh_table->cached_counters);
  assert(hh_table->cached_counters.actions.size() == 1);
  const RegisterActionType cached_counters_action_type = *hh_table->cached_counters.actions.begin();
  const code_t cached_counters_action_name             = build_register_action_name(&hh_table->cached_counters, cached_counters_action_type, ep_node);
  transpile_register_action_decl(&hh_table->cached_counters, cached_counters_action_name, cached_counters_action_type, {});

  transpile_register_decl(&hh_table->packet_sampler);
  assert(hh_table->packet_sampler.actions.size() == 1);
  const RegisterActionType packet_sampler_action_type = *hh_table->packet_sampler.actions.begin();
  const code_t packet_sampler_action_name             = build_register_action_name(&hh_table->packet_sampler, packet_sampler_action_type, ep_node);
  transpile_register_action_decl(&hh_table->packet_sampler, packet_sampler_action_name, packet_sampler_action_type, {});

  for (const Hash &hash : hh_table->hashes) {
    transpile_hash_decl(&hash);
  }

  ingress << "\n";

  std::vector<code_t> cms_rows_actions;
  for (const Register &cms_row : hh_table->count_min_sketch) {
    transpile_register_decl(&cms_row);
    assert(cms_row.actions.size() == 1);
    const RegisterActionType cms_row_action_type = *cms_row.actions.begin();
    const code_t cms_row_action_name             = build_register_action_name(&cms_row, cms_row_action_type, ep_node);
    transpile_register_action_decl(&cms_row, cms_row_action_name, cms_row_action_type, {});
    cms_rows_actions.push_back(cms_row_action_name);
  }

  transpile_register_decl(&hh_table->threshold);
  assert(hh_table->threshold.actions.size() == 1);
  const RegisterActionType threshold_action_type = *hh_table->threshold.actions.begin();
  const code_t threshold_action_name             = build_register_action_name(&hh_table->threshold, threshold_action_type, ep_node);
  const code_t threshold_value_cmp               = threshold_action_name + "_cmp";
  ingress.indent();
  ingress << Transpiler::type_from_size(32) << " " << threshold_value_cmp << ";\n";
  transpile_register_action_decl(&hh_table->threshold, threshold_action_name, threshold_action_type, {});

  transpile_digest_decl(&hh_table->digest, keys);

  for (const var_t &key_var : keys_vars) {
    ingress_apply.indent();
    ingress_apply << key_var.name << " = " << transpiler.transpile(key_var.expr) << ";\n";
  }

  const var_t hit_var = alloc_var("hit", hit->expr, FORCE_BOOL);
  hit_var.declare(ingress_apply, table->id + ".apply().hit");

  ingress_apply.indent();
  ingress_apply << "if (" << hit_var.name << ") {\n";
  ingress_apply.inc();

  ingress_apply.indent();
  ingress_apply << cached_counters_action_name << ".execute(" << transpiler.transpile(index) << ");\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "} else {\n";
  ingress_apply.inc();

  const code_t packet_sampler_out_value = packet_sampler_action_name + "_out_value";
  ingress_apply.indent();
  ingress_apply << Transpiler::type_from_size(hh_table->packet_sampler.value_size) << " " << packet_sampler_out_value;
  ingress_apply << " = ";
  ingress_apply << packet_sampler_action_name << ".execute(0);\n";

  ingress_apply.indent();
  ingress_apply << "if (" << packet_sampler_out_value << " == 0) {\n";
  ingress_apply.inc();

  std::vector<code_t> hashes;
  std::vector<code_t> hash_calculators;
  for (size_t i = 0; i < hh_table->hashes.size(); i++) {
    std::vector<code_t> hash_inputs;
    for (const var_t &key_var : keys_vars) {
      hash_inputs.push_back(key_var.name);
    }

    assert(i < HHTable::HASH_SALTS.size());
    const bits_t hash_salt_size = sizeof(HHTable::HASH_SALTS[i]) * 8;
    hash_inputs.push_back(Transpiler::transpile_literal(HHTable::HASH_SALTS[i], hash_salt_size, true));

    code_t hash_calculator;
    code_t output_hash;
    transpile_hash_calculation(&hh_table->hashes[i], hash_inputs, hash_calculator, output_hash);

    hashes.push_back(output_hash);
    hash_calculators.push_back(hash_calculator);
  }

  assert(hashes.size() == cms_rows_actions.size());
  const bits_t cms_value_size = hh_table->count_min_sketch[0].value_size;

  std::vector<code_t> cms_rows_values;
  std::vector<code_t> cms_rows_values_calculators;
  for (size_t i = 0; i < cms_rows_actions.size(); i++) {
    const Register &cms_row      = hh_table->count_min_sketch[i];
    const code_t &cms_row_action = cms_rows_actions[i];
    const code_t &hash           = hashes[i];

    const code_t cms_row_value = cms_row.id + "_value";

    ingress.indent();
    ingress << Transpiler::type_from_size(cms_row.value_size) << " " << cms_row_value << ";\n";

    cms_rows_values.push_back(cms_row_value);

    coder_t cms_row_calculation_body;

    cms_row_calculation_body.indent();
    cms_row_calculation_body << cms_row_value << " = " << cms_row_action << ".execute(" << hash << ");\n";

    const code_t cms_row_calculation = cms_row_action + "_execute";
    transpile_action_decl(cms_row_calculation, cms_row_calculation_body.split_lines());

    cms_rows_values_calculators.push_back(cms_row_calculation);
  }

  for (const code_t &hash_calculator : hash_calculators) {
    ingress_apply.indent();
    ingress_apply << hash_calculator << "();\n";
  }

  for (const code_t &cms_row_value_calculator : cms_rows_values_calculators) {
    ingress_apply.indent();
    ingress_apply << cms_row_value_calculator << "();\n";
  }

  const code_t min_value = hh_table_id + "_cms_min";
  for (size_t i = 0; i < cms_rows_values.size(); i++) {
    const code_t &cms_row_value = cms_rows_values[i];

    ingress_apply.indent();
    if (i == 0) {
      ingress_apply << Transpiler::type_from_size(hh_table->count_min_sketch[i].value_size) << " " << min_value << " = " << cms_row_value << ";\n";
    } else {
      ingress_apply << min_value << " = min(" << min_value << ", " << cms_row_value << ");\n";
    }
  }

  ingress_apply.indent();
  ingress_apply << threshold_value_cmp << " = " << min_value << ";\n";

  const code_t threshold_diff = hh_table_id + "_threshold_diff";
  ingress_apply.indent();
  ingress_apply << Transpiler::type_from_size(cms_value_size) << " " << threshold_diff << " = ";
  ingress_apply << threshold_action_name << ".execute(0);\n";

  ingress_apply.indent();
  ingress_apply << "if (" << threshold_value_cmp << "[31:31] == 0) {\n";
  ingress_apply.inc();

  ingress_apply.indent();
  ingress_apply << "ig_dprsr_md.digest_type = " << hh_table->digest.digest_type << ";\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  transpile_digest(hh_table->digest, keys);

  return EPVisitor::Action::doChildren;
}

std::unordered_map<RegisterActionType, std::vector<code_t>> TofinoSynthesizer::cms_get_rows_reg_actions(const CountMinSketch *cms) {
  std::unordered_map<RegisterActionType, std::vector<code_t>> cms_rows_reg_actions;

  for (const Register &cms_row : cms->rows) {
    for (const RegisterActionType &action_type : cms_row.actions) {
      const code_t cms_row_action_name = build_register_action_name(&cms_row, action_type);
      cms_rows_reg_actions[action_type].push_back(cms_row_action_name);
    }
  }

  return cms_rows_reg_actions;
}

std::unordered_map<RegisterActionType, std::vector<code_t>> TofinoSynthesizer::cms_get_rows_actions(const CountMinSketch *cms) {
  std::unordered_map<RegisterActionType, std::vector<code_t>> cms_rows_actions;

  const std::unordered_map<RegisterActionType, std::vector<code_t>> cms_rows_reg_actions = cms_get_rows_reg_actions(cms);

  for (const auto &[action_type, reg_actions] : cms_rows_reg_actions) {
    for (const code_t &reg_action : reg_actions) {
      const code_t cms_row_action = reg_action + "_execute";
      cms_rows_actions[action_type].push_back(cms_row_action);
    }
  }

  return cms_rows_actions;
}

std::unordered_map<RegisterActionType, std::vector<code_t>> TofinoSynthesizer::cms_get_rows_values(const CountMinSketch *cms) {
  std::unordered_map<RegisterActionType, std::vector<code_t>> cms_rows_values;

  const std::unordered_map<RegisterActionType, std::vector<code_t>> cms_rows_reg_actions = cms_get_rows_reg_actions(cms);

  for (const auto &[action_type, reg_actions] : cms_rows_reg_actions) {
    for (const code_t &reg_action : reg_actions) {
      const code_t cms_row_value = reg_action + "_value";
      cms_rows_values[action_type].push_back(cms_row_value);
    }
  }

  return cms_rows_values;
}

std::vector<code_t> TofinoSynthesizer::cms_get_hashes_values(const CountMinSketch *cms) {
  std::vector<code_t> hashes;

  for (size_t i = 0; i < cms->height; i++) {
    const code_t &hash      = cms->hashes[i].id;
    const code_t hash_value = hash + "_value";
    hashes.push_back(hash_value);
  }

  return hashes;
}

std::vector<code_t> TofinoSynthesizer::cms_get_hashes_calculators(const CountMinSketch *cms, const EPNode *ep_node) {
  std::vector<code_t> hash_calculators;

  for (size_t i = 0; i < cms->height; i++) {
    const code_t &hash           = cms->hashes[i].id + "_" + std::to_string(ep_node->get_id());
    const code_t hash_calculator = hash + "_calc_" + std::to_string(ep_node->get_id());
    hash_calculators.push_back(hash_calculator);
  }

  return hash_calculators;
}

void TofinoSynthesizer::transpile_cms_hash_calculator_decl(const CountMinSketch *cms, const EPNode *ep_node, const std::vector<var_t> &keys_vars) {
  const std::vector<code_t> hashes_calculators = cms_get_hashes_calculators(cms, ep_node);
  const std::vector<code_t> hashes_values      = cms_get_hashes_values(cms);

  for (size_t i = 0; i < cms->height; i++) {
    assert(i < CountMinSketch::HASH_SALTS.size());

    const bits_t hash_salt_size   = sizeof(CountMinSketch::HASH_SALTS[i]) * 8;
    const code_t &hash            = cms->hashes[i].id + "_" + std::to_string(ep_node->get_id());
    const code_t &hash_calculator = hashes_calculators[i];
    const code_t &hash_value      = hashes_values[i];

    coder_t hash_calculation_body;

    hash_calculation_body.indent();
    hash_calculation_body << hash_value << " = " << hash << ".get({\n";

    hash_calculation_body.inc();
    for (const var_t &key_var : keys_vars) {
      hash_calculation_body.indent();
      hash_calculation_body << key_var.name << ",\n";
    }
    hash_calculation_body.indent();
    hash_calculation_body << Transpiler::transpile_literal(HHTable::HASH_SALTS[i], hash_salt_size, true) << "\n";
    hash_calculation_body.dec();

    hash_calculation_body.indent();
    hash_calculation_body << "});\n";

    transpile_action_decl(hash_calculator, hash_calculation_body.split_lines());
  }
}

void TofinoSynthesizer::transpile_cms_decl(const CountMinSketch *cms, const EPNode *ep_node) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL);

  const std::unordered_map<RegisterActionType, std::vector<code_t>> reg_actions = cms_get_rows_reg_actions(cms);
  const std::unordered_map<RegisterActionType, std::vector<code_t>> actions     = cms_get_rows_actions(cms);
  const std::unordered_map<RegisterActionType, std::vector<code_t>> values      = cms_get_rows_values(cms);
  const std::vector<code_t> hashes_values                                       = cms_get_hashes_values(cms);

  if (!declared_ds.contains(cms->id)) {
    for (size_t i = 0; i < cms->height; i++) {
      const Register &row = cms->rows[i];
      transpile_register_decl(&row);
    }

    ingress << "\n";

    for (size_t i = 0; i < cms->height; i++) {
      const code_t &hash_value = hashes_values[i];
      ingress.indent();
      ingress << Transpiler::type_from_size(cms->hash_size) << " " << hash_value << ";\n";
    }

    ingress << "\n";

    for (size_t i = 0; i < cms->height; i++) {
      const Register &row = cms->rows[i];
      const code_t &hash  = hashes_values[i];

      for (const RegisterActionType &action_type : row.actions) {
        assert(reg_actions.find(action_type) != reg_actions.end());
        assert(actions.find(action_type) != actions.end());
        assert(values.find(action_type) != values.end());

        assert(i < reg_actions.at(action_type).size());
        assert(i < actions.at(action_type).size());
        assert(i < values.at(action_type).size());

        const code_t &reg_action = reg_actions.at(action_type)[i];
        const code_t &action     = actions.at(action_type)[i];
        const code_t &value      = values.at(action_type)[i];

        transpile_register_action_decl(&row, reg_action, action_type, {});

        if (register_action_types_with_out_value.contains(action_type)) {
          ingress.indent();
          ingress << Transpiler::type_from_size(row.value_size) << " " << value << ";\n";
        }

        coder_t row_action_body;
        row_action_body.indent();

        if (register_action_types_with_out_value.contains(action_type)) {
          row_action_body << value << " = ";
        }
        row_action_body << reg_action << ".execute(" << hash << ");\n";
        transpile_action_decl(action, row_action_body.split_lines());
        ingress << "\n";
      }
    }

    declared_ds.insert(cms->id);
  }

  for (Hash hash : cms->hashes) {
    hash.id = hash.id + "_" + std::to_string(ep_node->get_id());
    transpile_hash_decl(&hash);
  }

  ingress << "\n";
}

void TofinoSynthesizer::transpile_cuckoo_hash_table_decl(const CuckooHashTable *cuckoo_hash_table) {
  if (declared_ds.contains(cuckoo_hash_table->id)) {
    return;
  }

  declared_ds.insert(cuckoo_hash_table->id);

  Template cuckoo_hash_table_template(std::filesystem::path(__FILE__).parent_path() / "Templates" / TEMPLATE_CUCKOO_HASH_TABLE_FILENAME,
                                      {
                                          {MARKER_CUCKOO_IDX_WIDTH, 0},
                                          {MARKER_CUCKOO_ENTRIES, 0},
                                          {MARKER_CUCKOO_BLOOM_IDX_WIDTH, 0},
                                          {MARKER_CUCKOO_BLOOM_ENTRIES, 0},
                                      });

  cuckoo_hash_table_template.get(MARKER_CUCKOO_IDX_WIDTH) << cuckoo_hash_table->cuckoo_index_size;
  cuckoo_hash_table_template.get(MARKER_CUCKOO_ENTRIES) << cuckoo_hash_table->entries_per_cuckoo_table;
  cuckoo_hash_table_template.get(MARKER_CUCKOO_BLOOM_IDX_WIDTH) << cuckoo_hash_table->cuckoo_bloom_index_size;
  cuckoo_hash_table_template.get(MARKER_CUCKOO_BLOOM_ENTRIES) << cuckoo_hash_table->BLOOM_WIDTH;

  coder_t &control_blocks = get(MARKER_CONTROL_BLOCKS);
  control_blocks << cuckoo_hash_table_template.dump() << "\n";

  coder_t &ingress = get(MARKER_INGRESS_CONTROL);
  ingress.indent();
  ingress << "CuckooHashTable() cuckoo_hash_table;\n";
  ingress.indent();
  ingress << "CuckooHashBloomFilter() cuckoo_bloom_filter;\n";
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::CMSIncrement *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const DS_ID cms_id                             = node->get_cms_id();
  const std::vector<klee::ref<klee::Expr>> &keys = node->get_keys();

  const CountMinSketch *cms = get_tofino_ds<CountMinSketch>(ep, cms_id);

  const std::unordered_map<RegisterActionType, std::vector<code_t>> actions = cms_get_rows_actions(cms);
  const std::vector<code_t> hashes_calculators                              = cms_get_hashes_calculators(cms, ep_node);

  transpile_cms_decl(cms, ep_node);

  std::vector<var_t> keys_vars;
  for (size_t i = 0; i < keys.size(); i++) {
    const std::string key_name = "key_" + std::to_string(keys[i]->getWidth()) + "b_" + std::to_string(i);
    const var_t key_var        = alloc_var(key_name, keys[i], SKIP_STACK_ALLOC | EXACT_NAME | IS_INGRESS_METADATA);
    keys_vars.push_back(key_var);

    declare_var_in_ingress_metadata(key_var);

    ingress_apply.indent();
    ingress_apply << key_var.name << " = " << transpiler.transpile(key_var.expr) << ";\n";
  }

  transpile_cms_hash_calculator_decl(cms, ep_node, keys_vars);

  for (const code_t &hash_calc : hashes_calculators) {
    ingress_apply.indent();
    ingress_apply << hash_calc << "();\n";
  }

  assert(actions.find(RegisterActionType::Increment) != actions.end());
  const std::vector<code_t> &increment_actions = actions.at(RegisterActionType::Increment);

  for (const code_t &action : increment_actions) {
    ingress_apply.indent();
    ingress_apply << action << "();\n";
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::CMSIncAndQuery *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const DS_ID cms_id                             = node->get_cms_id();
  const std::vector<klee::ref<klee::Expr>> &keys = node->get_keys();
  const klee::ref<klee::Expr> min_estimate       = node->get_min_estimate();

  const CountMinSketch *cms = get_tofino_ds<CountMinSketch>(ep, cms_id);

  const std::unordered_map<RegisterActionType, std::vector<code_t>> actions = cms_get_rows_actions(cms);
  const std::unordered_map<RegisterActionType, std::vector<code_t>> values  = cms_get_rows_values(cms);
  const std::vector<code_t> hashes_calculators                              = cms_get_hashes_calculators(cms, ep_node);

  transpile_cms_decl(cms, ep_node);

  std::vector<var_t> keys_vars;
  for (size_t i = 0; i < keys.size(); i++) {
    const std::string key_name = "key_" + std::to_string(keys[i]->getWidth()) + "b_" + std::to_string(i);
    const var_t key_var        = alloc_var(key_name, keys[i], SKIP_STACK_ALLOC | EXACT_NAME | IS_INGRESS_METADATA);
    keys_vars.push_back(key_var);

    declare_var_in_ingress_metadata(key_var);

    ingress_apply.indent();
    ingress_apply << key_var.name << " = " << transpiler.transpile(key_var.expr) << ";\n";
  }

  transpile_cms_hash_calculator_decl(cms, ep_node, keys_vars);

  for (const code_t &hash_calc : hashes_calculators) {
    ingress_apply.indent();
    ingress_apply << hash_calc << "();\n";
  }

  assert(actions.find(RegisterActionType::IncrementAndReturnNewValue) != actions.end());
  const std::vector<code_t> &inc_and_query_actions = actions.at(RegisterActionType::IncrementAndReturnNewValue);

  for (const code_t &action : inc_and_query_actions) {
    ingress_apply.indent();
    ingress_apply << action << "();\n";
  }

  assert(values.find(RegisterActionType::Read) != values.end());
  const std::vector<code_t> &read_values = values.at(RegisterActionType::Read);

  const var_t min_value = alloc_var(cms->id + "_min", min_estimate);
  for (size_t i = 0; i < cms->height; i++) {
    assert(i < read_values.size());
    const code_t &value = read_values[i];

    if (i == 0) {
      min_value.declare(ingress_apply, value);
    } else {
      ingress_apply.indent();
      ingress_apply << min_value.name << " = min(" << min_value.name << ", " << value << ");\n";
    }
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::CMSQuery *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const DS_ID cms_id                             = node->get_cms_id();
  const std::vector<klee::ref<klee::Expr>> &keys = node->get_keys();
  const klee::ref<klee::Expr> min_estimate       = node->get_min_estimate();

  const CountMinSketch *cms = get_tofino_ds<CountMinSketch>(ep, cms_id);

  const std::unordered_map<RegisterActionType, std::vector<code_t>> actions = cms_get_rows_actions(cms);
  const std::unordered_map<RegisterActionType, std::vector<code_t>> values  = cms_get_rows_values(cms);
  const std::vector<code_t> hashes_calculators                              = cms_get_hashes_calculators(cms, ep_node);

  transpile_cms_decl(cms, ep_node);

  std::vector<var_t> keys_vars;
  for (size_t i = 0; i < keys.size(); i++) {
    const std::string key_name = "key_" + std::to_string(keys[i]->getWidth()) + "b_" + std::to_string(i);
    const var_t key_var        = alloc_var(key_name, keys[i], SKIP_STACK_ALLOC | EXACT_NAME | IS_INGRESS_METADATA);
    keys_vars.push_back(key_var);

    declare_var_in_ingress_metadata(key_var);

    ingress_apply.indent();
    ingress_apply << key_var.name << " = " << transpiler.transpile(key_var.expr) << ";\n";
  }

  transpile_cms_hash_calculator_decl(cms, ep_node, keys_vars);

  for (const code_t &hash_calc : hashes_calculators) {
    ingress_apply.indent();
    ingress_apply << hash_calc << "();\n";
  }

  assert(actions.find(RegisterActionType::Read) != actions.end());
  const std::vector<code_t> &read_actions = actions.at(RegisterActionType::Read);

  for (const code_t &action : read_actions) {
    ingress_apply.indent();
    ingress_apply << action << "();\n";
  }

  assert(values.find(RegisterActionType::Read) != values.end());
  const std::vector<code_t> &read_values = values.at(RegisterActionType::Read);

  const var_t min_value = alloc_var(cms->id + "_min", min_estimate);
  for (size_t i = 0; i < cms->height; i++) {
    assert(i < read_values.size());
    const code_t &value = read_values[i];

    if (i == 0) {
      min_value.declare(ingress_apply, value);
    } else {
      ingress_apply.indent();
      ingress_apply << min_value.name << " = min(" << min_value.name << ", " << value << ");\n";
    }
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::HHTableOutOfBandUpdate *node) {
  // Ignore this one, as this is done out of band (as the name implies).
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::CuckooHashTableReadWrite *node) {
  const DS_ID cms_id                            = node->get_cuckoo_hash_table_id();
  klee::ref<klee::Expr> key                     = node->get_key();
  klee::ref<klee::Expr> read_value              = node->get_read_value();
  klee::ref<klee::Expr> write_value             = node->get_write_value();
  klee::ref<klee::Expr> cuckoo_update_condition = node->get_cuckoo_update_condition();
  const symbol_t &cuckoo_success                = node->get_cuckoo_success();

  assert_or_panic(ep_node->get_children().size() == 1, "Expected exactly one child");
  const EPNode *next_ep_node = ep_node->get_children().at(0);

  const CuckooHashTable *cuckoo_hash_table = get_tofino_ds<CuckooHashTable>(ep, cms_id);

  transpile_cuckoo_hash_table_decl(cuckoo_hash_table);

  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);
  ingress_apply.indent();
  ingress_apply << "if (!hdr.cuckoo.isValid()) {\n";
  ingress_apply.inc();
  ingress_apply.indent();
  ingress_apply << "build_cuckoo_hdr(";
  ingress_apply << transpiler.transpile(key) << ", " << transpiler.transpile(write_value);
  ingress_apply << ");\n";
  ingress_apply.indent();
  ingress_apply << "if (" << transpiler.transpile(cuckoo_update_condition) << ") {\n";
  ingress_apply.inc();
  ingress_apply.indent();
  ingress_apply << "hdr.cuckoo.op = cuckoo_ops_t.UPDATE;\n";
  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "} else {\n";
  ingress_apply.inc();
  ingress_apply.indent();
  ingress_apply << "hdr.cuckoo.op = cuckoo_ops_t.LOOKUP;\n";
  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";
  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  const var_t success_var = alloc_var(cuckoo_success.name, cuckoo_success.expr, FORCE_BOOL);

  success_var.declare(ingress_apply);

  ingress_apply.indent();
  ingress_apply << "cuckoo_hash_table.apply(meta.time, hdr.cuckoo, " << success_var.name << ");\n";
  ingress_apply.indent();
  ingress_apply << "cuckoo_bloom_filter.apply(hdr.cuckoo, fwd_op);\n";

  alloc_var("hdr.cuckoo.val", read_value, EXACT_NAME);

  ingress_apply.indent();
  ingress_apply << "if (hdr.cuckoo.op != cuckoo_ops_t.DONE) {\n";
  ingress_apply.inc();
  ingress_apply.indent();
  ingress_apply << "build_recirc_hdr(CUCKOO_CODE_PATH);\n";
  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "} else {\n";
  ingress_apply.inc();

  visit(ep, next_ep_node);

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  code_template.dbg_code();

  return EPVisitor::Action::skipChildren;
}

code_t TofinoSynthesizer::create_unique_name(const code_t &prefix) {
  if (var_prefix_usage.find(prefix) == var_prefix_usage.end()) {
    var_prefix_usage[prefix] = 0;
  }

  int &counter = var_prefix_usage[prefix];

  coder_t coder;
  coder << prefix << counter;

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
      std::cerr << expr_to_string(var.expr, true);
      std::cerr << "\n";
    }
  }
  std::cerr << "============================================ \n";

  std::cerr << "================== Headers ================= \n";
  for (const var_t &var : hdr_vars.get_all()) {
    std::cerr << var.name;
    if (var.is_bool()) {
      std::cerr << " (bool)";
    }
    if (var.is_header_field) {
      std::cerr << " (header)";
    }
    std::cerr << ": ";
    std::cerr << expr_to_string(var.expr, true);
    std::cerr << "\n";
  }
  std::cerr << "======================================== \n";
}

void TofinoSynthesizer::log(const EPNode *node) const {
  std::cerr << "[TofinoSynthesizer] ";
  EPVisitor::log(node);
}

} // namespace Tofino
} // namespace LibSynapse