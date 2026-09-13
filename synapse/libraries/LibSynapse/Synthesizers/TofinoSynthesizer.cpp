#include <LibSynapse/Synthesizers/TofinoSynthesizer.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibBDD/Unroll.h>
#include <LibSynapse/Walk.h>
#include <LibCore/Strings.h>
#include <functional>
#include <limits>
#include <regex>

namespace LibSynapse {
namespace Tofino {

using LibCore::dbg_mode_active;
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

// Split a canonical ReadLSB into its first `keep` bytes (in packet order) and the rest.
//
// Rebuilt from the expression's own Read leaves, because both halves have to stay in the ReadLSB
// shape the transpiler recognises. Building them out of Extracts instead -- bytes_in_expr followed
// by concat_exprs -- yields byte-level Concats that visitConcat cannot render, which is how the
// first attempt at this died: "TODO: visitConcat: (Concat w32 (Concat w24 (ReadMSB w16 ...".
bool split_read(klee::ref<klee::Expr> expr, bytes_t keep, klee::ref<klee::Expr> &head, klee::ref<klee::Expr> &tail) {
  std::vector<klee::ref<klee::Expr>> leaves; // most significant byte first
  const std::function<void(klee::ref<klee::Expr>)> collect = [&](klee::ref<klee::Expr> e) {
    if (e->getKind() == klee::Expr::Concat) {
      collect(e->getKid(0));
      collect(e->getKid(1));
      return;
    }
    leaves.push_back(e);
  };
  collect(expr);

  const size_t n = expr->getWidth() / 8;
  if (leaves.size() != n || keep == 0 || keep >= n) {
    return false;
  }
  for (const klee::ref<klee::Expr> &leaf : leaves) {
    if (leaf->getWidth() != 8) {
      return false;
    }
  }

  const auto rebuild = [](std::vector<klee::ref<klee::Expr>>::const_iterator begin, std::vector<klee::ref<klee::Expr>>::const_iterator end) {
    klee::ref<klee::Expr> out = *(end - 1);
    for (auto it = end - 1; it != begin;) {
      --it;
      out = solver_toolbox.exprBuilder->Concat(*it, out);
    }
    return out;
  };

  // Packet order is the reverse of `leaves`, so the first `keep` packet bytes are the last `keep`
  // entries and the remainder is everything before them.
  tail = rebuild(leaves.begin(), leaves.end() - keep);
  head = rebuild(leaves.end() - keep, leaves.end());
  return true;
}

// An operation on an N-bit value zero-extended to the BDD's 32-bit int width, against a constant
// that fits in N bits, is really an N-bit operation. Emitted at 32 bits it needs a cast that
// violates Tofino2's action constraints; the ground truth writes the same thing narrow, as
// `hdr.hdr2.data4[7:0] | 8w0x12`. Bitwise only -- narrowing an add or a shift would lose a carry.
klee::ref<klee::Expr> narrow_widened_bitop(klee::ref<klee::Expr> e) {
  switch (e->getKind()) {
  case klee::Expr::And:
  case klee::Expr::Or:
  case klee::Expr::Xor:
    break;
  default:
    return nullptr;
  }

  klee::ref<klee::Expr> zext = e->getKid(0);
  klee::ref<klee::Expr> cnst = e->getKid(1);
  if (zext->getKind() == klee::Expr::Constant) {
    std::swap(zext, cnst);
  }
  if (zext->getKind() != klee::Expr::ZExt || cnst->getKind() != klee::Expr::Constant) {
    return nullptr;
  }

  const klee::ref<klee::Expr> inner = zext->getKid(0);
  const bits_t narrow               = inner->getWidth();
  if (narrow >= e->getWidth() || narrow > 64) {
    return nullptr;
  }

  const u64 value = LibCore::solver_toolbox.value_from_expr(cnst);
  if (narrow < 64 && (value >> narrow) != 0) {
    return nullptr;
  }

  const klee::ref<klee::Expr> narrowed_const = LibCore::solver_toolbox.exprBuilder->Constant(value, narrow);
  switch (e->getKind()) {
  case klee::Expr::And:
    return LibCore::solver_toolbox.exprBuilder->And(inner, narrowed_const);
  case klee::Expr::Or:
    return LibCore::solver_toolbox.exprBuilder->Or(inner, narrowed_const);
  default:
    return LibCore::solver_toolbox.exprBuilder->Xor(inner, narrowed_const);
  }
}

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
constexpr const char *const MARKER_EGRESS_STATE_HEADER          = "EGRESS_STATE_HEADER";
constexpr const char *const MARKER_INGRESS_EGRESS_STATE_FIELD   = "INGRESS_EGRESS_STATE_FIELD";
constexpr const char *const MARKER_EGRESS_EGRESS_STATE_FIELD    = "EGRESS_EGRESS_STATE_FIELD";
constexpr const char *const MARKER_INGRESS_EGRESS_DECISION      = "INGRESS_EGRESS_DECISION";
constexpr const char *const MARKER_EGRESS_PARSER_START          = "EGRESS_PARSER_START";
constexpr const char *const MARKER_EGRESS_PARSER                = "EGRESS_PARSER";
constexpr const char *const MARKER_EGRESS_CONTROL               = "EGRESS_CONTROL";
constexpr const char *const MARKER_EGRESS_CONTROL_HELPERS       = "EGRESS_CONTROL_HELPERS";
constexpr const char *const MARKER_EGRESS_CONTROL_APPLY         = "EGRESS_CONTROL_APPLY";
constexpr const char *const MARKER_EGRESS_DEPARSER              = "EGRESS_DEPARSER";
constexpr const char *const MARKER_EGRESS_DEPARSER_APPLY        = "EGRESS_DEPARSER_APPLY";
constexpr const char *const MARKER_CONTROL_BLOCKS               = "CONTROL_BLOCKS";
constexpr const char *const MARKER_PARSE_RECIRC                 = "PARSE_RECIRC";
constexpr const char *const MARKER_PARSE_CPU                    = "PARSE_CPU";
constexpr const char *const MARKER_LEAVE_TO_CPU                 = "LEAVE_TO_CPU";
constexpr const char *const MARKER_INGRESS_APPLY_START          = "INGRESS_APPLY_START";
constexpr const char *const MARKER_LEAVE_SWITCH                 = "LEAVE_SWITCH";

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
  return *tna.parser;
}

// Whether an ALU op's statement carries a constant bf-p4c turns into action data: anything past
// the smallest immediates, shift amounts and read offsets aside. A keyless table whose action
// computes in the hash unit takes the hit pathway and cannot carry action data ("the driver can
// only currently program the miss pathway"), so such statements go to a companion action.
bool carries_action_data(const compute_op_t &op) {
  if (op.in_hash || op.kind != ComputeOpKind::ALU || op.args.empty()) {
    return false;
  }
  bool found                                            = false;
  std::function<void(klee::ref<klee::Expr>, bool)> scan = [&](klee::ref<klee::Expr> expr, bool amount) {
    if (expr.isNull() || found || expr->getKind() == klee::Expr::Read) {
      return;
    }
    if (is_constant(expr)) {
      found = !amount && expr->getWidth() <= 64 && solver_toolbox.value_from_expr(expr) > 7;
      return;
    }
    const bool shift = expr->getKind() == klee::Expr::Shl || expr->getKind() == klee::Expr::LShr || expr->getKind() == klee::Expr::AShr;
    for (unsigned i = 0; i < expr->getNumKids(); i++) {
      scan(expr->getKid(i), shift && i == 1);
    }
  };
  scan(op.args.front(), false);
  return found;
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

code_t TofinoSynthesizer::Transpiler::transpile(klee::ref<klee::Expr> expr, transpiler_opt_t opt,
                                                std::map<klee::ref<klee::Expr>, code_t> _temporary_transpilations) {
  loaded_opt               = opt;
  temporary_transpilations = _temporary_transpilations;

  if (LibCore::dbg_mode_active) {
    std::cerr << "Transpiling: " << expr_to_string(expr) << "\n";
  }
  expr = simplify(expr);
  if (LibCore::dbg_mode_active) {
    std::cerr << "Simplified:  " << expr_to_string(expr) << "\n";
  }

  coders.emplace();
  coder_t &coder = coders.top();

  bool is_temp_expr = false;
  for (const auto &[temp_expr, temp_code] : temporary_transpilations) {
    if (solver_toolbox.are_exprs_always_equal(expr, temp_expr)) {
      coder << temp_code;
      is_temp_expr = true;
      break;
    }
  }

  klee::ref<klee::Expr> endian_swap_target;
  if (!is_temp_expr) {
    if (is_constant(expr)) {
      coder << transpile_constant(expr, opt & TRANSPILER_OPT_SWAP_CONST_ENDIANNESS);
    } else if (match_endian_swap_pattern(expr, endian_swap_target)) {
      const bits_t size = endian_swap_target->getWidth();
      coder << swap_endianness(transpile(endian_swap_target, loaded_opt, temporary_transpilations), size);
    } else if (std::optional<var_t> var = synthesizer->ingress_vars.get(expr, loaded_opt)) {
      coder << var->name;
    } else {
      visit(expr);

      // HACK: clear the visited map so we force the transpiler to revisit all expressions.
      visited.clear();
    }
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

code_t TofinoSynthesizer::Transpiler::type_from_register_out_value(RegisterActionOutValueSize out_value_size, bits_t stored_value_size) {
  bits_t width = 0;

  switch (out_value_size) {
  case RegisterActionOutValueSize::SameAsStoredValue:
    width = stored_value_size;
    break;
  case RegisterActionOutValueSize::Bool:
    return "bool";
  case RegisterActionOutValueSize::UInt8:
    width = 8;
    break;
  case RegisterActionOutValueSize::UInt16:
    width = 16;
    break;
  case RegisterActionOutValueSize::UInt32:
    width = 32;
    break;
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

  // Header fields are cut at natural widths, not at whatever boundary the program happens to read
  // (see guess_struct_fields_from_expr). So a read of part of a field arrives here as a concat of
  // its bytes with no variable of its own: render it as a slice of the field that contains it.
  for (const TofinoSynthesizer::var_t &hdr_var : synthesizer->hdr_vars.get_all()) {
    if (hdr_var.expr.isNull() || hdr_var.expr->getWidth() < expr->getWidth()) {
      continue;
    }
    for (unsigned lo = 0; lo + expr->getWidth() <= hdr_var.expr->getWidth(); lo += 8) {
      klee::ref<klee::Expr> slice = solver_toolbox.exprBuilder->Extract(hdr_var.expr, lo, expr->getWidth());
      if (!solver_toolbox.are_exprs_always_equal(expr, slice)) {
        continue;
      }
      coder << hdr_var.name;
      coder << "[" << (lo + expr->getWidth() - 1) << ":" << lo << "]";
      return Action::skipChildren();
    }
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

  // Render a bit-slice of an already-transpilable value: base[hi:lo].
  klee::ref<klee::Expr> base = e.getKid(0);
  if (synthesizer->ingress_vars.get(base, loaded_opt)) {
    coder << transpile(base, loaded_opt, temporary_transpilations);
    coder << "[" << (e.offset + e.width - 1) << ":" << e.offset << "]";
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
  coder << transpile(arg, loaded_opt, temporary_transpilations);
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

  coder << "(" << transpile(lhs, loaded_opt, temporary_transpilations) << ")";
  coder << " + ";
  coder << "(" << transpile(rhs, loaded_opt, temporary_transpilations) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitSub(const klee::SubExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt, temporary_transpilations) << ")";
  coder << " - ";
  coder << "(" << transpile(rhs, loaded_opt, temporary_transpilations) << ")";

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
  coder_t &coder            = coders.top();
  klee::ref<klee::Expr> arg = e.getKid(0);

  // Negating a comparison: emit the flipped relational operator (e.g. !(a <= b) => a > b)
  // rather than a logical `!`. This keeps the result a single plain comparison, which is
  // required inside stateful-ALU register actions (where the swap condition lives) and is
  // cleaner in gateways too. Only the unsigned comparisons and Eq transpile their operands
  // verbatim; signed comparisons (which cast their operands) and anything else fall back to
  // a logical/bitwise complement below.
  const char *flipped_op = nullptr;
  switch (arg->getKind()) {
  case klee::Expr::Eq:
    flipped_op = " != ";
    break;
  case klee::Expr::Ne:
    flipped_op = " == ";
    break;
  case klee::Expr::Ult:
    flipped_op = " >= ";
    break;
  case klee::Expr::Ule:
    flipped_op = " > ";
    break;
  case klee::Expr::Ugt:
    flipped_op = " <= ";
    break;
  case klee::Expr::Uge:
    flipped_op = " < ";
    break;
  default:
    break;
  }

  if (flipped_op) {
    klee::ref<klee::Expr> lhs = arg->getKid(0);
    klee::ref<klee::Expr> rhs = arg->getKid(1);
    coder << "(" << transpile(lhs, loaded_opt, temporary_transpilations) << ")";
    coder << flipped_op;
    coder << "(" << transpile(rhs, loaded_opt, temporary_transpilations) << ")";
    return Action::skipChildren();
  }

  // Boolean negation (width 1) vs bitwise complement (wider).
  coder << (e.getWidth() == 1 ? "!(" : "~(");
  coder << transpile(arg, loaded_opt, temporary_transpilations);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitAnd(const klee::AndExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt, temporary_transpilations) << ")";
  coder << " & ";
  coder << "(" << transpile(rhs, loaded_opt, temporary_transpilations) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitOr(const klee::OrExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt, temporary_transpilations) << ")";
  coder << " | ";
  coder << "(" << transpile(rhs, loaded_opt, temporary_transpilations) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitXor(const klee::XorExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt, temporary_transpilations) << ")";
  coder << " ^ ";
  coder << "(" << transpile(rhs, loaded_opt, temporary_transpilations) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitShl(const klee::ShlExpr &e) {
  panic("TODO: visitShl");
  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitLShr(const klee::LShrExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt, temporary_transpilations) << ")";
  coder << " >> ";
  coder << "(" << transpile(rhs, loaded_opt, temporary_transpilations) << ")";

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

  coder << "(" << transpile(lhs, loaded_opt, temporary_transpilations) << ")";
  coder << " == ";
  coder << "(" << transpile(rhs, loaded_opt, temporary_transpilations) << ")";

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

  coder << "(" << transpile(lhs, loaded_opt, temporary_transpilations) << ")";
  coder << " != ";
  coder << "(" << transpile(rhs, loaded_opt, temporary_transpilations) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitUlt(const klee::UltExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt, temporary_transpilations) << ")";
  coder << " < ";
  coder << "(" << transpile(rhs, loaded_opt, temporary_transpilations) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitUle(const klee::UleExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt, temporary_transpilations) << ")";
  coder << " <= ";
  coder << "(" << transpile(rhs, loaded_opt, temporary_transpilations) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitUgt(const klee::UgtExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt, temporary_transpilations) << ")";
  coder << " > ";
  coder << "(" << transpile(rhs, loaded_opt, temporary_transpilations) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitUge(const klee::UgeExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt, temporary_transpilations) << ")";
  coder << " >= ";
  coder << "(" << transpile(rhs, loaded_opt, temporary_transpilations) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitSlt(const klee::SltExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt, temporary_transpilations) << ")";
  coder << " < ";
  coder << "(" << transpile(rhs, loaded_opt, temporary_transpilations) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitSle(const klee::SleExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt, temporary_transpilations) << ")";
  coder << " <= ";
  coder << "(" << transpile(rhs, loaded_opt, temporary_transpilations) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitSgt(const klee::SgtExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt, temporary_transpilations) << ")";
  coder << " > ";
  coder << "(" << transpile(rhs, loaded_opt, temporary_transpilations) << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action TofinoSynthesizer::Transpiler::visitSge(const klee::SgeExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(" << transpile(lhs, loaded_opt, temporary_transpilations) << ")";
  coder << " >= ";
  coder << "(" << transpile(rhs, loaded_opt, temporary_transpilations) << ")";

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
  case RegisterActionType::AddValue:
    coder << "add_value";
    break;
  case RegisterActionType::SetToOne:
    coder << "set_to_one";
    break;
  case RegisterActionType::SetToOneAndReturnOldValue:
    coder << "read_and_set";
    break;
  case RegisterActionType::ConditionalSetToOneAndReturnOldValue:
    coder << "read_and_cond_set";
    break;
  case RegisterActionType::IncrementAndReturnNewValue:
    coder << "inc_and_read";
    break;
  case RegisterActionType::ConditionalIncrementAndReturnOldValue:
    coder << "cond_inc_and_read";
    break;
  case RegisterActionType::ReadConditionalWrite:
    coder << "read_conditional_write";
    break;
  case RegisterActionType::ReadConditionalWriteReturnOther:
    coder << "read_conditional_write_return_other";
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
  case RegisterActionType::IntegerAllocatorHeadReadAndUpdate:
    coder << "int_alloc_head_read_and_update";
    break;
  }

  if (node) {
    coder << "_";
    coder << node->get_id();
  }

  return coder.dump();
}

void TofinoSynthesizer::emit_register_execute(const code_t &lhs, const code_t &action_name, const klee::ref<klee::Expr> &index,
                                              const code_t &index_code, const EPNode *ep_node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const code_t call = action_name + ".execute(" + index_code + ")";

  // Constant index: a plain keyless table, which tolerates fused action data -> emit inline
  // exactly as before (no change for the common case / other NFs).
  if (is_constant(index)) {
    ingress_apply.indent();
    if (lhs.empty()) {
      ingress_apply << call << ";\n";
    } else {
      ingress_apply << lhs << " = " << call << ";\n";
    }
    return;
  }

  // Computed index -> hash_action table. Isolate the execute in its own named action so no
  // following statement can be fused into it (see header comment).
  const code_t exec_action = "regexec_" + action_name;

  // The action lives at control scope, so its index operand must be visible there. Metadata
  // (meta.*) and header (hdr.*) references are visible inside actions and keep their exact P4 type,
  // so use them directly. An apply-block local (e.g. a vector-borrow index `bit<32> index0 = ...`)
  // is not visible in the action, so materialize it into ingress metadata first and reference that.
  code_t action_index                = index_code;
  const bool index_visible_in_action = index_code.rfind("meta.", 0) == 0 || index_code.rfind("hdr.", 0) == 0;
  if (!index_visible_in_action) {
    // expr-based alloc_var (not the size-only one, whose null expr crashes the later is_bool pass);
    // it allocates a fresh uniquely-named metadata field and never collapses back onto the local.
    const var_t idx_var = alloc_var(exec_action + "_index", index, IS_INGRESS_METADATA);
    declare_var_in_ingress_metadata(idx_var);
    ingress_apply.indent();
    ingress_apply << idx_var.name << " = " << index_code << ";\n";
    action_index = idx_var.name;
  }

  const code_t action_call = action_name + ".execute(" + action_index + ")";

  coder_t &ingress = get(MARKER_INGRESS_CONTROL);
  ingress.indent();
  ingress << "action " << exec_action << "() {\n";
  ingress.inc();
  ingress.indent();
  if (lhs.empty()) {
    ingress << action_call << ";\n";
  } else {
    ingress << lhs << " = " << action_call << ";\n";
  }
  ingress.dec();
  ingress.indent();
  ingress << "}\n";

  ingress_apply.indent();
  ingress_apply << exec_action << "();\n";
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
  if (declared_ds.find(table->id) != declared_ds.end()) {
    return;
  }

  for (size_t i = 0; i < keys.size(); i++) {
    const std::string key_name = "key_" + std::to_string(keys[i]->getWidth()) + "b_" + std::to_string(i);
    const var_t key_var        = alloc_var(key_name, keys[i], SKIP_STACK_ALLOC | EXACT_NAME | IS_INGRESS_METADATA);
    keys_vars.push_back(key_var);
    declare_var_in_ingress_metadata(key_var);
  }

  transpile_table_decl(table, keys_vars, values, values_are_buffers);
}

void TofinoSynthesizer::transpile_table_decl(const Table *table, const std::vector<var_t> &keys_vars,
                                             const std::vector<klee::ref<klee::Expr>> &values, bool values_are_buffers) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL);

  if (declared_ds.find(table->id) != declared_ds.end()) {
    return;
  }

  declared_ds.insert(table->id);

  const code_t action_name = table->id + "_get_value";
  if (!values.empty()) {
    transpile_action_decl(action_name, values, values_are_buffers);
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

// A max/min swap that returns the displaced value can't be a single-value register
// action (the return would source from the register value in one branch and from an
// external input in the other -- illegal on one stateful ALU). Following the expert
// P4, such a register is a PAIR {lo = kept max, hi = returned shadow}, and the action
// always returns `hi` (a single register source). See the ReadConditionalWriteReturnOther
// case in transpile_register_action_decl.
static bool register_is_shadow_pair(const Register *reg) { return reg->actions.count(RegisterActionType::ReadConditionalWriteReturnOther) > 0; }

static code_t register_pair_type(const Register *reg) { return reg->id + "_pair_t"; }

// Bits needed to address a register of the given capacity (ceil(log2(capacity))). A
// register .execute() needs an index field of exactly this width; a wider one (e.g. a
// 32-bit hash-derived index into a 64-entry register) is rejected as "too complex".
static bits_t register_index_bits(u32 capacity) {
  if (capacity <= 1) {
    return 1;
  }
  bits_t bits = 0;
  u32 n       = capacity - 1;
  while (n) {
    bits++;
    n >>= 1;
  }
  return bits;
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

  if (register_is_shadow_pair(reg)) {
    // Emit the pair struct at top level, then a paired register (no scalar init).
    const code_t value_type = TofinoSynthesizer::Transpiler::type_from_size(reg->value_size);
    coder_t &headers        = get(MARKER_CUSTOM_HEADERS);
    headers << "struct " << register_pair_type(reg) << " {\n";
    headers << "  " << value_type << " lo;\n";
    headers << "  " << value_type << " hi;\n";
    headers << "}\n\n";

    ingress.indent();
    ingress << "Register<" << register_pair_type(reg) << ",_>(" << reg->capacity << ") " << reg->id << ";\n";
    return;
  }

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
                                                       std::optional<register_action_extras_t> extras) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL);

  const code_t value_type = TofinoSynthesizer::Transpiler::type_from_size(reg->value_size);
  const code_t index_type = TofinoSynthesizer::Transpiler::type_from_size(reg->index_size);
  const code_t out_value_type =
      register_action_types_with_out_value.contains(action_type)
          ? TofinoSynthesizer::Transpiler::type_from_register_out_value(register_action_types_with_out_value.at(action_type), reg->value_size)
          : "void";

  // Paired shadow-swap registers store a {lo, hi} pair; the action's value operand is
  // that pair, while the returned value stays a single field (out_value_type). Their
  // index is also narrowed to the register's addressing width (see register_index_bits).
  const code_t stored_type = register_is_shadow_pair(reg) ? register_pair_type(reg) : value_type;
  const code_t index_type_final =
      register_is_shadow_pair(reg) ? TofinoSynthesizer::Transpiler::type_from_size(register_index_bits(reg->capacity)) : index_type;

  ingress.indent();
  ingress << "RegisterAction<";
  ingress << stored_type;
  ingress << ", ";
  ingress << index_type_final;
  ingress << ", ";
  ingress << out_value_type;
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
    assert_or_panic(extras.has_value() && extras->external_var.has_value(), "Expected a write value");

    ingress.indent();
    ingress << "void apply(inout " << value_type << " value) {\n";

    ingress.inc();
    ingress.indent();
    ingress << "value = " << extras->external_var.value() << ";\n";

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
  case RegisterActionType::AddValue: {
    assert_or_panic(extras.has_value() && extras->external_var.has_value(), "Expected an increment value");

    ingress.indent();
    ingress << "void apply(inout " << value_type << " value, out " << value_type << " out_value) {\n";
    ingress.inc();

    ingress.indent();
    ingress << "value = value + " << extras->external_var.value() << ";\n";
    ingress.indent();
    ingress << "out_value = value;\n";

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
  case RegisterActionType::SetToOne: {
    ingress.indent();
    ingress << "void apply(inout " << value_type << " value) {\n";

    ingress.inc();
    ingress.indent();
    ingress << "value = 1;\n";

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
  case RegisterActionType::ConditionalSetToOneAndReturnOldValue: {
    assert_or_panic(extras.has_value() && extras->extra_condition.has_value(), "Expected a condition for SetToOneAndReturnOldValue register action");

    ingress.indent();
    ingress << "void apply(inout " << value_type << " value, out " << value_type << " out_value) {\n";
    ingress.inc();

    ingress.indent();
    ingress << "out_value = value;\n";

    ingress.indent();
    ingress << "if (" << transpiler.transpile(extras->extra_condition.value()) << ") {\n";
    ingress.inc();
    ingress.indent();
    ingress << "value = 1;\n";
    ingress.dec();
    ingress.indent();
    ingress << "}\n";

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
  case RegisterActionType::ConditionalIncrementAndReturnOldValue: {
    assert_or_panic(extras.has_value() && extras->extra_condition.has_value(), "Expected a condition for SetToOneAndReturnOldValue register action");

    ingress.indent();
    ingress << "void apply(inout " << value_type << " value, out " << value_type << " out_value) {\n";
    ingress.inc();

    ingress.indent();
    ingress << "out_value = value;\n";

    ingress.indent();
    ingress << "if (" << transpiler.transpile(extras->extra_condition.value(), TRANSPILER_OPT_NO_OPTION, extras->temporary_transpilations) << ") {\n";
    ingress.inc();
    ingress.indent();
    ingress << "value = value + 1;\n";
    ingress.dec();
    ingress.indent();
    ingress << "}\n";

    ingress.dec();
    ingress.indent();
    ingress << "}\n";
  } break;
  case RegisterActionType::ReadConditionalWrite: {
    assert_or_panic(extras.has_value() && extras->extra_condition.has_value(), "Expected a condition for SetToOneAndReturnOldValue register action");
    assert_or_panic(extras.has_value() && extras->write_value.has_value(), "Expected a write value for SetToOneAndReturnOldValue register action");

    ingress.indent();
    ingress << "void apply(inout " << value_type << " value, out " << value_type << " out_value) {\n";
    ingress.inc();

    ingress.indent();
    ingress << "out_value = value;\n";

    ingress.indent();
    ingress << "if (" << transpiler.transpile(extras->extra_condition.value(), TRANSPILER_OPT_NO_OPTION, extras->temporary_transpilations) << ") {\n";
    ingress.inc();
    ingress.indent();
    ingress << "value = " << transpiler.transpile(extras->write_value.value(), TRANSPILER_OPT_NO_OPTION, extras->temporary_transpilations) << ";\n";
    ingress.dec();
    ingress.indent();
    ingress << "}\n";

    ingress.dec();
    ingress.indent();
    ingress << "}\n";
  } break;
  case RegisterActionType::ReadConditionalWriteReturnOther: {
    assert_or_panic(extras.has_value() && extras->extra_condition.has_value(), "Expected a condition for ReadConditionalWriteReturnOther");
    assert_or_panic(extras.has_value() && extras->write_value.has_value(), "Expected a write value for ReadConditionalWriteReturnOther");

    // Keep the winner in the register and return the displaced "shadow" = the smaller
    // of the stored value and the candidate. A single-value register can't do this (the
    // return would come from the register value in one branch and the candidate in the
    // other -- incompatible outputs on one ALU), so use a {lo = kept max, hi = shadow}
    // pair and always return `hi`, exactly like the expert's swap_if_larger. The stored
    // value is read as `in_value.lo` (captured before the conditional writes).
    const code_t pair_type                                        = register_pair_type(reg);
    std::map<klee::ref<klee::Expr>, code_t> paired_transpilations = extras->temporary_transpilations;
    for (auto &[expr, name] : paired_transpilations) {
      if (name == "value") {
        name = "in_value.lo";
      }
    }
    const code_t cond_code  = transpiler.transpile(extras->extra_condition.value(), TRANSPILER_OPT_NO_OPTION, paired_transpilations);
    const code_t write_code = transpiler.transpile(extras->write_value.value(), TRANSPILER_OPT_NO_OPTION, paired_transpilations);

    ingress.indent();
    ingress << "void apply(inout " << pair_type << " value, out " << value_type << " out_value) {\n";
    ingress.inc();

    ingress.indent();
    ingress << pair_type << " in_value = value;\n";
    ingress.indent();
    ingress << "if (" << cond_code << ") {\n";
    ingress.inc();
    ingress.indent();
    ingress << "value.lo = " << write_code << ";\n";
    ingress.indent();
    ingress << "value.hi = in_value.lo;\n";
    ingress.dec();
    ingress.indent();
    ingress << "} else {\n";
    ingress.inc();
    ingress.indent();
    ingress << "value.lo = in_value.lo;\n";
    ingress.indent();
    ingress << "value.hi = " << write_code << ";\n";
    ingress.dec();
    ingress.indent();
    ingress << "}\n";
    ingress.indent();
    ingress << "out_value = value.hi;\n";

    ingress.dec();
    ingress.indent();
    ingress << "}\n";
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
    assert_or_panic(extras.has_value() && extras->extra_constant.has_value(), "Expected a global variable for the timeout value");
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

    ingress.indent();
    ingress << "alarm = meta.time + " << extras->extra_constant.value() << ";\n";

    ingress.dec();
    ingress.indent();
    ingress << "}\n";
  } break;
  case RegisterActionType::CheckValue: {
    assert_or_panic(extras.has_value() && extras->external_var.has_value(), "Expected a global variable for crosschecking the value");
    ingress.indent();
    ingress << "void apply(inout " << value_type << " curr_value, out bit<8> match) {\n";
    ingress.inc();

    ingress.indent();
    ingress << "if (curr_value == " << extras->external_var.value() << ") {\n";
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
  case RegisterActionType::IntegerAllocatorHeadReadAndUpdate:
    assert_or_panic(extras.has_value() && extras->external_var.has_value(), "Expected a global variable for the tail value");
    assert_or_panic(extras.has_value() && extras->extra_constant.has_value(), "Expected an extra constant for the maximum head value");
    ingress.indent();
    ingress << "void apply(inout " << value_type << " head, out " << value_type << " out_head) {\n";
    ingress.inc();

    ingress.indent();
    ingress << "out_head = head;\n";

    ingress.indent();
    ingress << "if (head == " << extras->extra_constant.value() << " - 1) {\n";
    ingress.inc();

    ingress.indent();
    ingress << "head = 0;\n";
    ingress.dec();
    ingress.indent();
    ingress << "} else {\n";
    ingress.inc();

    ingress.indent();
    ingress << "head = head + 1;\n";
    ingress.dec();
    ingress.indent();
    ingress << "}\n";

    ingress.dec();
    ingress.indent();
    ingress << "}\n";

    break;
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

  if (hash->size > 32) {
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

void TofinoSynthesizer::transpile_fcfs_ct_hash_calculation(const Hash *hash, const std::vector<code_t> &inputs, const var_t &fcfs_ct_value,
                                                           code_t &hash_calculator, code_t &output_hash) {
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

  hash_calculation_body.indent();
  hash_calculation_body << fcfs_ct_value.get_slice(0, hash->size).name << " = " << output_hash << ";\n";

  hash_calculator = hash->id + "_calc";
  transpile_action_decl(hash_calculator, hash_calculation_body.split_lines());
}

void TofinoSynthesizer::transpile_digest_decl(const Digest *digest) {
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

void TofinoSynthesizer::transpile_fcfs_ct_decl(const FCFSCachedTable *fcfs_ct, const EPNode *ep_node) {
  if (declared_ds.find(fcfs_ct->id) != declared_ds.end()) {
    return;
  }

  declared_ds.insert(fcfs_ct->id);

  const fcfs_ct_internals_t fcfs_ct_internals = fcfs_ct_get_internals(fcfs_ct);

  for (const Hash &hash : fcfs_ct->hashes) {
    transpile_hash_decl(&hash);
  }

  transpile_register_decl(&fcfs_ct->reg_liveness);
  transpile_register_action_decl(&fcfs_ct->reg_liveness, fcfs_ct_internals.liveness_query, RegisterActionType::QueryTimestamp);
  transpile_register_action_decl(&fcfs_ct->reg_liveness, fcfs_ct_internals.liveness_query_and_refresh, RegisterActionType::QueryAndRefreshTimestamp,
                                 register_action_extras_t{
                                     .external_var             = {},
                                     .extra_constant           = 16384, // 1s
                                     .extra_condition          = {},
                                     .write_value              = {},
                                     .temporary_transpilations = {},
                                 });

  for (size_t key_idx = 0; key_idx < fcfs_ct->cache_keys.size(); key_idx++) {
    const Register &reg_key = fcfs_ct->cache_keys.at(key_idx);
    transpile_register_decl(&reg_key);
    for (RegisterActionType action_type : reg_key.actions) {
      register_action_extras_t extras;
      extras.external_var = fcfs_ct_internals.keys.at(key_idx).name;
      transpile_register_action_decl(&reg_key, fcfs_ct_internals.keys_reg_actions.at({reg_key.id, action_type}), action_type, extras);
    }
  }
}

void TofinoSynthesizer::transpile_fcfs_cs_decl(const FCFSCachedSet *fcfs_cs, const EPNode *ep_node) {
  if (declared_ds.find(fcfs_cs->id) != declared_ds.end()) {
    return;
  }

  declared_ds.insert(fcfs_cs->id);

  const fcfs_cs_internals_t fcfs_cs_internals = fcfs_cs_get_internals(fcfs_cs);

  for (const Hash &hash : fcfs_cs->hashes) {
    transpile_hash_decl(&hash);
  }

  transpile_register_decl(&fcfs_cs->reg_liveness);
  transpile_register_action_decl(&fcfs_cs->reg_liveness, fcfs_cs_internals.liveness_query, RegisterActionType::QueryTimestamp);
  transpile_register_action_decl(&fcfs_cs->reg_liveness, fcfs_cs_internals.liveness_query_and_refresh, RegisterActionType::QueryAndRefreshTimestamp,
                                 register_action_extras_t{
                                     .external_var             = {},
                                     .extra_constant           = 16384, // 1s
                                     .extra_condition          = {},
                                     .write_value              = {},
                                     .temporary_transpilations = {},
                                 });

  for (size_t key_idx = 0; key_idx < fcfs_cs->cache_keys.size(); key_idx++) {
    const Register &reg_key = fcfs_cs->cache_keys.at(key_idx);
    transpile_register_decl(&reg_key);
    for (RegisterActionType action_type : reg_key.actions) {
      register_action_extras_t extras;
      extras.external_var = fcfs_cs_internals.keys.at(key_idx).name;
      transpile_register_action_decl(&reg_key, fcfs_cs_internals.keys_reg_actions.at({reg_key.id, action_type}), action_type, extras);
    }
  }
}

code_t TofinoSynthesizer::var_t::get_type() const { return force_bool ? "bool" : TofinoSynthesizer::Transpiler::type_from_size(size); }

bool TofinoSynthesizer::var_t::is_bool() const { return force_bool || (!expr.isNull() && is_conditional(expr)); }

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

  const code_t slice_name = name + "[" + std::to_string(hi) + ":" + std::to_string(lo) + "]";

  klee::ref<klee::Expr> slice_expr = expr;
  if (!expr.isNull()) {
    slice_expr = solver_toolbox.exprBuilder->Extract(expr, offset, slice_size);
  }

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

void TofinoSynthesizer::Stack::push(const var_t &var, bool allow_duplicated) {
  if (!allow_duplicated && names.contains(var.name)) {
    return;
  }
  frames.push_back(var);
  names.insert(var.name);
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
    if (!var.expr.isNull() && solver_toolbox.are_exprs_always_equal(var.expr, expr)) {
      return var;
    }
  }

  return {};
}

std::optional<TofinoSynthesizer::var_t> TofinoSynthesizer::Stack::get_exact_hdr(klee::ref<klee::Expr> expr) const {
  for (auto var_it = frames.rbegin(); var_it != frames.rend(); var_it++) {
    const var_t &var = *var_it;
    if (var.is_header_field && !var.expr.isNull() && solver_toolbox.are_exprs_always_equal(var.expr, expr)) {
      return var;
    }
  }

  return {};
}

std::optional<TofinoSynthesizer::var_t> TofinoSynthesizer::Stack::get(const code_t &name) const {
  for (auto var_it = frames.rbegin(); var_it != frames.rend(); var_it++) {
    const var_t &var = *var_it;
    if (var.name == name) {
      return var;
    }
  }

  return {};
}

bool TofinoSynthesizer::Stack::set_var_expr(const code_t &name, klee::ref<klee::Expr> expr) {
  for (auto var_it = frames.rbegin(); var_it != frames.rend(); var_it++) {
    var_t &var = *var_it;
    if (var.name == name) {
      var.expr = expr;
      return true;
    }
  }

  return false;
}

// A network-order read spanning several consecutive header fields of this frame (an NF
// byte-swapping its little-endian read of them, as for a hash input): their concatenation,
// which holds exactly those bytes in that order, as a var of its own.
std::optional<TofinoSynthesizer::var_t> TofinoSynthesizer::Stack::compose_hdr_fields(klee::ref<klee::Expr> expr) const {
  if (expr->getWidth() <= 8) {
    return {};
  }
  const std::optional<LibCore::consecutive_bytes_t> target = LibCore::get_consecutive_bytes(expr);
  if (!target) {
    return {};
  }

  std::vector<code_t> field_names;
  u32 next = target->lo;
  while (next <= target->hi) {
    bool found = false;
    for (const var_t &var : frames) {
      if (!var.is_header_field || var.expr.isNull()) {
        continue;
      }
      const std::optional<LibCore::consecutive_bytes_t> bytes = LibCore::get_consecutive_bytes(var.expr);
      if (bytes && !bytes->network_order && bytes->array == target->array && bytes->lo == next && bytes->hi <= target->hi) {
        field_names.push_back(var.name);
        next  = bytes->hi + 1;
        found = true;
        break;
      }
    }
    if (found) {
      continue;
    }

    // No field starts here. Since fields are coalesced to their natural width, a read can begin or
    // end inside one -- bytes 10..11 of a TCP header are the last byte of the acknowledgement
    // number and the first of the offset -- so take the part of the containing field that the read
    // needs. A P4 header field is network order, byte 0 at the top, so the slice counts down.
    for (const var_t &var : frames) {
      if (!var.is_header_field || var.expr.isNull() || var.is_slice()) {
        continue;
      }
      const std::optional<LibCore::consecutive_bytes_t> bytes = LibCore::get_consecutive_bytes(var.expr);
      if (!bytes || bytes->network_order || bytes->array != target->array || next < bytes->lo || next > bytes->hi) {
        continue;
      }
      const u32 end      = std::min(bytes->hi, target->hi);
      const bits_t width = (bytes->hi - bytes->lo + 1) * 8;
      // Where bytes `next..end` sit inside the field depends on how the field itself is read. A
      // network-order field has byte `lo` at the top, so the run counts down from the width; a
      // host-order one has byte `lo` at the bottom, so it counts up from it.
      bits_t high;
      bits_t low;
      if (bytes->network_order) {
        high = width - 1 - (next - bytes->lo) * 8;
        low  = width - (end - bytes->lo + 1) * 8;
      } else {
        high = (end - bytes->lo + 1) * 8 - 1;
        low  = (next - bytes->lo) * 8;
      }
      field_names.push_back(var.name + "[" + std::to_string(high) + ":" + std::to_string(low) + "]");
      next  = end + 1;
      found = true;
      break;
    }

    if (!found) {
      return {};
    }
  }
  if (field_names.empty()) {
    return {};
  }

  // The pieces were gathered by increasing address. A network-order value wants them in that
  // order, most significant first; a host-order one is the same bytes read the other way round, so
  // its pieces -- each already a host-order run -- go most significant last.
  if (!target->network_order) {
    std::reverse(field_names.begin(), field_names.end());
  }

  // One field covering the whole range is the common case now that adjacent fields are coalesced
  // at their natural width: a P4 header field holds its bytes in wire order, so the network-order
  // read *is* that field, with no concatenation to build.
  code_t name;
  if (field_names.size() == 1) {
    name = field_names[0];
  } else {
    name = "(";
    for (size_t i = 0; i < field_names.size(); i++) {
      name += (i > 0 ? " ++ " : "") + field_names[i];
    }
    name += ")";
  }
  return var_t(name, expr, expr->getWidth(), /*force_bool=*/false, /*is_header_field=*/true, /*is_buffer=*/false);
}

std::optional<TofinoSynthesizer::var_t> TofinoSynthesizer::Stack::get(klee::ref<klee::Expr> expr, transpiler_opt_t opt) const {
  if (std::optional<var_t> var = get_exact(expr)) {
    return var;
  }

  // No point in looking for a slice if the expression is 1 bit.
  if (expr->getWidth() == 1) {
    return {};
  }

  if (std::optional<var_t> var = compose_hdr_fields(expr)) {
    return var;
  }

  for (auto var_it = frames.rbegin(); var_it != frames.rend(); var_it++) {
    const var_t &var = *var_it;

    if (var.expr.isNull()) {
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
        return slice;
      }
    }
  }

  return {};
}

std::optional<TofinoSynthesizer::var_t> TofinoSynthesizer::Stack::get_hdr(klee::ref<klee::Expr> expr, transpiler_opt_t opt) const {
  if (std::optional<var_t> var = get_exact(expr)) {
    if (var->is_header_field) {
      return var;
    }
  }

  for (auto var_it = frames.rbegin(); var_it != frames.rend(); var_it++) {
    const var_t &var = *var_it;

    // A slot of the state header holding a temporary has no expression of its own.
    if (!var.is_header_field || var.expr.isNull()) {
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
        return slice;
      }
    }
  }

  return {};
}

std::vector<TofinoSynthesizer::var_t> TofinoSynthesizer::Stack::get_all() const { return frames; }

void TofinoSynthesizer::Stacks::push() { stacks.emplace_back(); }

void TofinoSynthesizer::Stacks::pop() { stacks.pop_back(); }

void TofinoSynthesizer::Stacks::insert_front(const var_t &var, bool allow_duplicates) { stacks.front().push(var, allow_duplicates); }
void TofinoSynthesizer::Stacks::insert_front(const Stack &stack) { stacks.front().push(stack); }

void TofinoSynthesizer::Stacks::insert_back(const var_t &var, bool allow_duplicates) { stacks.back().push(var, allow_duplicates); }
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

std::optional<TofinoSynthesizer::var_t> TofinoSynthesizer::Stacks::get(const code_t &name) const {
  for (auto stack_it = stacks.rbegin(); stack_it != stacks.rend(); stack_it++) {
    if (std::optional<var_t> var = stack_it->get(name)) {
      return var;
    }
  }

  return {};
}

bool TofinoSynthesizer::Stacks::set_var_expr(const code_t &name, klee::ref<klee::Expr> expr) {
  for (auto stack_it = stacks.rbegin(); stack_it != stacks.rend(); stack_it++) {
    if (stack_it->set_var_expr(name, expr)) {
      return true;
    }
  }

  return false;
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
                                             {MARKER_EGRESS_STATE_HEADER, 0},
                                             {MARKER_INGRESS_EGRESS_STATE_FIELD, 1},
                                             {MARKER_EGRESS_EGRESS_STATE_FIELD, 1},
                                             {MARKER_INGRESS_EGRESS_DECISION, 2},
                                             {MARKER_EGRESS_PARSER_START, 2},
                                             {MARKER_EGRESS_PARSER, 1},
                                             {MARKER_EGRESS_CONTROL, 1},
                                             {MARKER_EGRESS_CONTROL_HELPERS, 1},
                                             {MARKER_EGRESS_CONTROL_APPLY, 2},
                                             {MARKER_EGRESS_DEPARSER, 1},
                                             {MARKER_EGRESS_DEPARSER_APPLY, 2},
                                             {MARKER_CONTROL_BLOCKS, 0},
                                             {MARKER_PARSE_RECIRC, 2},
                                             {MARKER_PARSE_CPU, 2},
                                             {MARKER_LEAVE_TO_CPU, 2},
                                             {MARKER_INGRESS_APPLY_START, 2},
                                             {MARKER_LEAVE_SWITCH, 2},
                                         }),
      target_ep(_ep), transpiler(this) {}

coder_t &TofinoSynthesizer::get(const std::string &marker) {
  // Past a SendToEgress the plan runs in the other pipeline. Redirecting here rather than at
  // every call site means the existing emission code needs no changes to work in egress.
  if (in_egress) {
    if (marker == MARKER_INGRESS_CONTROL_APPLY) {
      return active_egress_code_path ? egress_coders[*active_egress_code_path] : code_template.get(MARKER_EGRESS_CONTROL_APPLY);
    }
    if (marker == MARKER_INGRESS_CONTROL) {
      return code_template.get(MARKER_EGRESS_CONTROL);
    }
    if (marker == MARKER_INGRESS_METADATA) {
      return code_template.get(MARKER_EGRESS_METADATA);
    }
  }
  if (marker == MARKER_INGRESS_CONTROL_APPLY && active_recirc_code_path) {
    return recirc_coders[*active_recirc_code_path];
  }
  return code_template.get(marker);
}

size_t TofinoSynthesizer::recirc_slot_for(const code_t &name, const code_t &slot_kind) {
  size_t slot;
  auto owned_it = recirc_slot_by_name.find(name);
  if (owned_it != recirc_slot_by_name.end() && owned_it->second.first == slot_kind) {
    slot = owned_it->second.second;
  } else {
    std::set<size_t> &owned = recirc_slots_owned[slot_kind];
    do {
      slot = recirc_slot_next[slot_kind]++;
    } while (owned.contains(slot));
    owned.insert(slot);
    recirc_slot_by_name[name] = {slot_kind, slot};
  }
  recirc_slots_used[slot_kind] = std::max(recirc_slots_used[slot_kind], slot + 1);
  return slot;
}

std::unordered_set<std::string> TofinoSynthesizer::live_symbols_past(const EP *ep, const BDDNode *cut_node, const EPNode *next) const {
  std::unordered_set<std::string> live_symbols;
  const BDDNode *node = ep->get_bdd()->get_node_by_id(cut_node->get_id());
  node->visit_nodes([&live_symbols](const BDDNode *future_node) {
    for (const symbol_t &symbol : future_node->get_used_symbols().get()) {
      live_symbols.insert(symbol.name);
    }
    return BDDNodeVisitAction::Continue;
  });
  std::vector<const EPNode *> pending;
  if (next) {
    pending.push_back(next);
  }
  while (!pending.empty()) {
    const EPNode *future_ep_node = pending.back();
    pending.pop_back();
    if (const Module *future_module = future_ep_node->get_module()) {
      if (future_module->get_type() == ModuleType::Tofino_SendToController) {
        for (const symbol_t &symbol : dynamic_cast<const Tofino::SendToController *>(future_module)->get_symbols().get()) {
          live_symbols.insert(symbol.name);
        }
      }
    }
    for (const EPNode *child : future_ep_node->get_children()) {
      pending.push_back(child);
    }
  }
  return live_symbols;
}

void TofinoSynthesizer::plan_value_homes(const EP *ep) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  const Pipeline &pipeline        = tofino_ctx->get_tna().pipeline;
  constexpr int INF               = 1 << 20;

  // The op whose variable holds an op's value: the original's, for an op that reused or was
  // unified with another path's.
  const auto canonical = [&](const std::string &op_id) -> std::string {
    if (const std::optional<compute_reuse_t> reuse = tofino_ctx->get_compute_reuse(op_id)) {
      return reuse->op.id;
    }
    if (const std::optional<klee::ref<klee::Expr>> theirs = tofino_ctx->get_output_alias(op_id)) {
      const std::optional<std::string> producer = tofino_ctx->get_producer(*theirs);
      assert(producer && "An output alias to a symbol no op produces");
      return *producer;
    }
    return op_id;
  };
  const auto stage_of = [&](const std::string &op_id) -> int { return pipeline.get_placed_stage(tofino_ctx->find_compute_action(op_id)); };

  // An input of an ALU op that some compute value may hold: a materialized operand of the module
  // (by canonical op id) or a symbol, resolved to the op producing it on each path. `rotation` is
  // where the input's bits land in the result: 0 for an aligned op, the amount for a rotate or a
  // left shift, width - amount for a right shift or a cast of the top bits.
  struct source_t {
    std::string op_id;
    std::string symbol;
    unsigned rotation;
  };
  // What a compute module defines: its own op (with the symbol it produces) and its temporaries.
  struct def_t {
    std::string op_id;         // Canonical.
    std::string symbol;        // The module's own output symbol; empty for a temporary.
    klee::ref<klee::Expr> out; // The canonical op's output (null for a temporary).
    bits_t width;
    bool clock_shift;              // The clock shifted right: kept as the 32 bits the data plane has (time_shift).
    std::vector<source_t> sources; // What an ALU op reads to write this one.
    std::string in_place_of;       // A shift rotate's or: written over the slot of this half of it (canonical op id).
  };
  // Whether the placed op runs in the hash unit, which reads its inputs with no alignment to keep.
  const auto in_hash_op = [&](const std::string &op_id) -> bool {
    for (const auto &[id, ds] : tofino_ctx->get_data_structures().get_data_per_id()) {
      const ComputeAction *action = dynamic_cast<const ComputeAction *>(ds);
      if (!action) {
        continue;
      }
      for (const compute_op_t &op : action->ops) {
        if (op.id == op_id) {
          return op.in_hash;
        }
      }
    }
    return false;
  };
  // The source an ALU op's input is, if it may be a compute value: a materialized operand of the
  // module, or a symbol (whose producer is a matter of the path: a path that reuses another's op
  // reads it by its own symbol).
  const auto source_of = [&](const std::vector<TofinoModuleFactory::compute_operand_t> &operands, klee::ref<klee::Expr> input,
                             unsigned rotation) -> std::optional<source_t> {
    for (const TofinoModuleFactory::compute_operand_t &operand : operands) {
      if (operand.expr->getWidth() != input->getWidth()) {
        continue;
      }
      if (solver_toolbox.are_exprs_always_equal(operand.expr, input) ||
          solver_toolbox.are_exprs_always_equal(tofino_ctx->apply_rewrites(operand.op_id, operand.expr), input)) {
        return source_t{canonical(operand.op_id), "", rotation};
      }
    }
    std::string symbol;
    if (LibCore::is_readLSB(input, symbol)) {
      return source_t{"", symbol, rotation};
    }
    return std::nullopt;
  };
  // The sources of an ALU expression: its inputs that may be compute values, each with the
  // rotation the op applies to it.
  const auto alu_sources = [&](const std::vector<TofinoModuleFactory::compute_operand_t> &operands,
                               klee::ref<klee::Expr> expr) -> std::vector<source_t> {
    std::vector<source_t> sources;
    if (expr.isNull()) {
      return sources;
    }
    const unsigned width = expr->getWidth();
    const auto add       = [&](klee::ref<klee::Expr> input, unsigned rotation) {
      // A cast of a field's top bits, (bit<w>)(f[hi:lo]), is emitted as f >> lo (operand_rhs).
      if (input->getKind() == klee::Expr::ZExt && input->getKid(0)->getKind() == klee::Expr::Extract) {
        const klee::ExtractExpr *extract = static_cast<const klee::ExtractExpr *>(input->getKid(0).get());
        if (extract->expr->getWidth() == width) {
          rotation = (rotation + width - extract->offset % width) % width;
          input    = extract->expr;
        }
      }
      if (const std::optional<source_t> source = source_of(operands, input, rotation)) {
        sources.push_back(*source);
      }
    };
    switch (expr->getKind()) {
    case klee::Expr::Add:
    case klee::Expr::Sub:
    case klee::Expr::And:
    case klee::Expr::Or:
    case klee::Expr::Xor:
      for (unsigned i = 0; i < expr->getNumKids(); i++) {
        add(expr->getKid(i), 0);
      }
      break;
    case klee::Expr::Shl:
      if (is_constant(expr->getKid(1))) {
        add(expr->getKid(0), solver_toolbox.value_from_expr(expr->getKid(1)) % width);
      }
      break;
    case klee::Expr::LShr:
      if (is_constant(expr->getKid(1))) {
        add(expr->getKid(0), (width - solver_toolbox.value_from_expr(expr->getKid(1)) % width) % width);
      }
      break;
    case klee::Expr::ZExt:
      add(expr, 0);
      break;
    default:
      break;
    }
    return sources;
  };
  std::unordered_map<const Module *, std::vector<def_t>> defs_cache; // defs_of asks the solver about operands; every path asks again.
  const auto defs_of = [&](const Module *module) -> const std::vector<def_t> & {
    auto cached_it = defs_cache.find(module);
    if (cached_it != defs_cache.end()) {
      return cached_it->second;
    }
    std::vector<def_t> defs;
    const auto own = [&](const std::string &op_id, klee::ref<klee::Expr> out) {
      std::string symbol;
      if (!out.isNull() && LibCore::is_readLSB(out, symbol)) {
        const std::string id                = canonical(op_id);
        klee::ref<klee::Expr> canonical_out = out;
        if (const std::optional<compute_reuse_t> reuse = tofino_ctx->get_compute_reuse(op_id)) {
          canonical_out = reuse->op.out.isNull() ? out : reuse->op.out;
        } else if (const std::optional<klee::ref<klee::Expr>> theirs = tofino_ctx->get_output_alias(op_id)) {
          canonical_out = *theirs;
        }
        defs.push_back({id, symbol, canonical_out, out->getWidth(), false, {}, ""});
      }
    };
    const auto temporaries = [&](const std::vector<TofinoModuleFactory::compute_operand_t> &operands) {
      for (const TofinoModuleFactory::compute_operand_t &operand : operands) {
        const std::string id = canonical(operand.op_id);
        defs.push_back({id, "", nullptr, operand.expr->getWidth(), false,
                        in_hash_op(id) ? std::vector<source_t>{} : alu_sources(operands, tofino_ctx->apply_rewrites(operand.op_id, operand.expr)),
                        ""});
      }
    };
    switch (module->get_type()) {
    case ModuleType::Tofino_ArithmeticOp: {
      const Tofino::ArithmeticOp *op = dynamic_cast<const Tofino::ArithmeticOp *>(module);
      temporaries(op->get_operands());
      own(op->get_op_id(), op->get_out());
      if (!defs.empty() && defs.back().op_id == canonical(op->get_op_id()) && !in_hash_op(defs.back().op_id)) {
        defs.back().sources = alu_sources(op->get_operands(), tofino_ctx->apply_rewrites(op->get_op_id(), op->get_value()));
      }
      // A shift of the clock by 16 or more: the data plane keeps 32 bits of it (emit_compute_run's
      // time_shift), so the slot is 32 bits wide whatever the symbol's width.
      klee::ref<klee::Expr> value = op->get_value();
      if (!defs.empty() && value->getKind() == klee::Expr::LShr && is_constant(value->getKid(1)) &&
          solver_toolbox.value_from_expr(value->getKid(1)) >= 16 &&
          symbol_t::get_symbols_names(value->getKid(0)) == std::unordered_set<std::string>{ep->get_bdd()->get_time().name}) {
        defs.back().width       = 32;
        defs.back().clock_shift = true;
      }
    } break;
    case ModuleType::Tofino_RotateLeft: {
      const Tofino::RotateLeft *rot = dynamic_cast<const Tofino::RotateLeft *>(module);
      temporaries(rot->get_operands());
      own(rot->get_op_id(), rot->get_out());
      if (!defs.empty() && defs.back().op_id == canonical(rot->get_op_id()) && !in_hash_op(defs.back().op_id)) {
        if (const auto source = source_of(rot->get_operands(), tofino_ctx->apply_rewrites(rot->get_op_id(), rot->get_x()), rot->get_amount())) {
          defs.back().sources.push_back(*source);
        }
      }
    } break;
    case ModuleType::Tofino_RotateLeftShifts: {
      const Tofino::RotateLeftShifts *rot = dynamic_cast<const Tofino::RotateLeftShifts *>(module);
      temporaries(rot->get_operands());
      // Both halves place the input's bits where the rotate does; the or reads the halves aligned.
      std::vector<source_t> halves;
      if (const auto source = source_of(rot->get_operands(), tofino_ctx->apply_rewrites(rot->get_or_op_id(), rot->get_x()), rot->get_amount())) {
        halves.push_back(*source);
      }
      defs.push_back({canonical(rot->get_shl_op_id()), "", nullptr, rot->get_out()->getWidth(), false, halves, ""});
      defs.push_back({canonical(rot->get_shr_op_id()), "", nullptr, rot->get_out()->getWidth(), false, halves, ""});
      own(rot->get_or_op_id(), rot->get_out());
      if (!defs.empty() && defs.back().op_id == canonical(rot->get_or_op_id())) {
        defs.back().sources = {{canonical(rot->get_shl_op_id()), "", 0}, {canonical(rot->get_shr_op_id()), "", 0}};
        // `x = x | y` reads x before it writes it, in one action: the or takes its shl half's
        // slot, one word fewer at the stage where a shift rotate holds the most.
        defs.back().in_place_of = canonical(rot->get_shl_op_id());
      }
    } break;
    default:
      break;
    }
    return defs_cache.emplace(module, std::move(defs)).first->second;
  };
  const auto reads_of = [&](const Module *module) -> std::unordered_set<std::string> {
    std::unordered_set<std::string> reads;
    for (const symbol_t &symbol : module->get_node()->get_used_symbols().get()) {
      reads.insert(symbol.name);
    }
    if (module->get_type() == ModuleType::Tofino_SendToController) {
      for (const symbol_t &symbol : dynamic_cast<const Tofino::SendToController *>(module)->get_symbols().get()) {
        reads.insert(symbol.name);
      }
    }
    return reads;
  };

  // Every root-to-leaf path, the ones that reuse the fewest ops first: a shared action's slots
  // are fixed by the path that owns it, and the paths that call it work around them.
  std::vector<std::vector<const EPNode *>> paths;
  std::vector<const EPNode *> current;
  std::function<void(const EPNode *)> collect = [&](const EPNode *ep_node) {
    current.push_back(ep_node);
    if (ep_node->get_children().empty()) {
      paths.push_back(current);
    }
    for (const EPNode *child : ep_node->get_children()) {
      collect(child);
    }
    current.pop_back();
  };
  collect(ep->get_root());
  const auto reused_count = [&](const std::vector<const EPNode *> &path) {
    size_t n = 0;
    for (const EPNode *ep_node : path) {
      for (const def_t &def : defs_of(ep_node->get_module())) {
        n += tofino_ctx->get_compute_reuse(def.op_id).has_value() ? 1 : 0;
      }
    }
    return n;
  };
  std::stable_sort(paths.begin(), paths.end(), [&](const auto &a, const auto &b) { return reused_count(a) < reused_count(b); });

  // A value of one path, with its live range in the path's time: pass * STRIDE + stage. A cut
  // starts a new pass, and the state header is extracted again on its far side, so a value live
  // past a cut keeps its slot into the next pass.
  constexpr int STRIDE = 64;
  // The 32-bit words one gress may write or read in the chain: one PHV group's normal containers
  // (tofino/exp-compute/README.md, "How many sliced fields fit").
  constexpr size_t WORD_BUDGET = 12;
  struct value_t {
    def_t def;
    int from;
    int to;
    bool fixed;                                            // Its slot was set by an earlier path.
    bool egress;                                           // Defined in the egress: the pairs of words it writes and reads are the egress's.
    std::vector<std::pair<std::string, unsigned>> sources; // The values an ALU op reads to write this one (canonical op id, rotation).
    bool read_elsewhere = false;                           // A table, a gateway or a hand-off reads it too: no op writes over it in its last stage.
  };

  // Slot allocation for one path: linear scan by definition time, in the pool of the value's
  // width, around the slots an earlier path fixed and the ones shared actions write on this
  // path. A pool owns its slots; a new one takes the next index of its width.
  const auto allocate = [&](std::vector<value_t> &values, const std::vector<std::pair<std::string, int>> &foreign_writes) {
    std::stable_sort(values.begin(), values.end(), [](const value_t &a, const value_t &b) { return a.from < b.from; });
    struct slot_t {
      size_t index;
      int busy_until = -1;
      std::vector<std::pair<int, int>> fixed; // Ranges an earlier path's value or a shared action's write occupies.
    };
    // Every slot allocated so far, free at the start of this path: a path reuses the earlier
    // paths' slots before taking new ones.
    std::map<bits_t, std::vector<slot_t>> pools;
    for (const auto &[width, count] : state_slots_used) {
      for (size_t index = 0; index < count; index++) {
        pools[width].push_back(slot_t{index, -1, {}});
      }
    }
    const auto slot_index = [&](const std::string &op_id) -> std::optional<size_t> {
      auto found_it = slot_fields.find(op_id);
      if (found_it == slot_fields.end()) {
        return {};
      }
      const code_t &name = found_it->second.name;
      return std::stoul(name.substr(name.rfind('_') + 1));
    };
    const auto entry_of = [&](bits_t width, size_t index) -> slot_t & {
      std::vector<slot_t> &pool = pools[width];
      for (slot_t &slot : pool) {
        if (slot.index == index) {
          return slot;
        }
      }
      pool.push_back(slot_t{index, -1, {}});
      return pool.back();
    };
    // A word written from another word takes it at one rotation, always. bf-p4c places the words
    // of a sliced cluster one at a time and, for a source not yet placed, records where its slices
    // must sit relative to the destination; a source one op takes aligned (xor, add, move) and
    // another byte-rotated, into the same destination, asks for two positions at once, and the
    // destination becomes unplaceable ("would (conservatively) need to be aligned at the same
    // position in the same container", ActionPhvConstraints::check_and_generate_conditional_
    // constraints). Whether it bites depends on the placement order -- the ground truth carries
    // four such pairs and allocates, the synthesized program did not -- so no such pair is made
    // (tofino/exp-compute/README.md, scy-*-one.p4).
    const auto slot_name = [](bits_t width, size_t index) { return "hdr.st.s" + std::to_string(width) + "_" + std::to_string(index); };
    // The pairs a value's slot takes part in: its sources' slots as the ones it is written from,
    // and, for a value allocated after one of its readers (a path that reuses another's op is
    // planned before it), the readers' slots as the ones it is read into.
    struct pair_t {
      bool egress;
      code_t word;   // Empty: the value's own slot, to be chosen.
      code_t source; // Empty: the value's own slot.
      unsigned rotation;
    };
    const auto pairs_of = [&](const value_t &value) {
      std::vector<pair_t> pairs;
      for (const auto &[op_id, rotation] : value.sources) {
        auto found_it = slot_fields.find(op_id);
        if (found_it != slot_fields.end()) {
          pairs.push_back({value.egress, "", found_it->second.name, rotation});
        }
      }
      auto readers_it = slot_readers.find(value.def.op_id);
      if (readers_it != slot_readers.end()) {
        for (const auto &[reader, rotation, egress] : readers_it->second) {
          auto found_it = slot_fields.find(reader);
          if (found_it != slot_fields.end()) {
            pairs.push_back({egress, found_it->second.name, "", rotation});
          }
        }
      }
      return pairs;
    };
    const auto pair_key = [](const pair_t &pair, const code_t &own) {
      return std::make_tuple(pair.egress, pair.word.empty() ? own : pair.word, pair.source.empty() ? own : pair.source);
    };
    const auto compatible = [&](const std::vector<pair_t> &pairs, const code_t &own) {
      for (const pair_t &pair : pairs) {
        auto rot_it = slot_rotations.find(pair_key(pair, own));
        if (rot_it != slot_rotations.end() && rot_it->second != pair.rotation) {
          return false;
        }
      }
      return true;
    };
    const auto record_rotations = [&](const value_t &value, const code_t &name) {
      for (const pair_t &pair : pairs_of(value)) {
        slot_rotations[pair_key(pair, name)] = pair.rotation;
      }
      std::vector<code_t> &planned = planned_sources[value.def.op_id];
      for (const auto &[op_id, rotation] : value.sources) {
        auto found_it = slot_fields.find(op_id);
        if (found_it != slot_fields.end()) {
          planned.push_back(found_it->second.name);
        }
      }
    };
    for (const value_t &value : values) {
      if (value.fixed) {
        const code_t name = slot_fields.at(value.def.op_id).name;
        entry_of(value.def.width, *slot_index(value.def.op_id)).fixed.emplace_back(value.from, value.to);
        record_rotations(value, name);
        words_used.insert({value.egress, name});
        for (const auto &[op_id, rotation] : value.sources) {
          auto found_it = slot_fields.find(op_id);
          if (found_it != slot_fields.end()) {
            words_used.insert({value.egress, found_it->second.name});
          }
        }
      }
    }
    for (const auto &[op_id, time] : foreign_writes) {
      if (const std::optional<size_t> slot = slot_index(op_id)) {
        entry_of(slot_fields.at(op_id).size, *slot).fixed.emplace_back(time, time);
      }
    }
    // The slots a value may take, best first: the slot of an operand dying at it, then the free
    // slots that take every pair, fewest new pairs first, then a fresh one.
    //
    // `x = x op y` reads x before it writes it, in one statement: the value may take the slot of
    // an operand whose last read is this very op, when no other op of the same stage reads that
    // operand (a statement after this one in the stage would read the new value) and nothing
    // but ops reads it at all; a shift rotate's or always may take its shl half's. Among the free
    // slots, the one that adds the fewest new pairs: a round of a hash that lands on the slots
    // the round before it used repeats its pairs instead of spending fresh ones, as the ground
    // truth's fixed roles do, and later values find more slots still compatible.
    std::unordered_map<std::string, size_t> by_op; // By canonical op id, into this path's slotted values.
    for (size_t i = 0; i < values.size(); i++) {
      by_op.insert({values[i].def.op_id, i});
    }
    struct candidate_t {
      size_t index;
      bool fresh;
    };
    const auto candidates = [&](const value_t &value, const std::vector<pair_t> &pairs) -> std::vector<candidate_t> {
      std::vector<candidate_t> out;
      std::set<size_t> in_place;
      std::vector<std::string> dying;
      if (!value.def.in_place_of.empty()) {
        dying.push_back(value.def.in_place_of);
      }
      for (const auto &[source, rotation] : value.sources) {
        auto source_it = by_op.find(source);
        if (source_it == by_op.end()) {
          continue;
        }
        const value_t &operand = values[source_it->second];
        if (operand.to != value.from || operand.read_elsewhere || operand.def.width != value.def.width) {
          continue;
        }
        size_t readers_now = 0;
        auto readers_it    = slot_readers.find(source);
        if (readers_it != slot_readers.end()) {
          for (const auto &[reader, rot, eg] : readers_it->second) {
            auto reader_it = by_op.find(reader);
            readers_now += reader_it != by_op.end() && values[reader_it->second].from == value.from ? 1 : 0;
          }
        }
        if (readers_now <= 1) {
          dying.push_back(source);
        }
      }
      for (const std::string &op_id : dying) {
        auto slot_it = slot_fields.find(op_id);
        if (slot_it == slot_fields.end() || slot_it->second.size != value.def.width || !compatible(pairs, slot_it->second.name)) {
          continue;
        }
        const size_t index = *slot_index(op_id);
        if (in_place.contains(index)) {
          continue;
        }
        const slot_t &slot = entry_of(value.def.width, index);
        bool free          = slot.busy_until <= value.from;
        for (const auto &[from, to] : slot.fixed) {
          free &= to <= value.from || value.to < from;
        }
        if (free) {
          in_place.insert(index);
          out.push_back({index, false});
        }
      }
      std::vector<std::pair<size_t, size_t>> free_slots; // (new pairs, index)
      for (const slot_t &candidate : pools[value.def.width]) {
        if (in_place.contains(candidate.index)) {
          continue;
        }
        // Strictly after the last read: a write in the stage of a read of the same slot would
        // sit in one action with it, where P4 reads the written value.
        bool free = candidate.busy_until < value.from;
        for (const auto &[from, to] : candidate.fixed) {
          free &= to < value.from || value.to < from;
        }
        const code_t name = slot_name(value.def.width, candidate.index);
        if (!free || !compatible(pairs, name)) {
          continue;
        }
        size_t new_pairs = 0;
        for (const pair_t &pair : pairs) {
          new_pairs += slot_rotations.contains(pair_key(pair, name)) ? 0 : 1;
        }
        free_slots.emplace_back(new_pairs, candidate.index);
      }
      std::sort(free_slots.begin(), free_slots.end());
      for (const auto &[new_pairs, index] : free_slots) {
        out.push_back({index, false});
      }
      out.push_back({state_slots_used[value.def.width], true});
      return out;
    };

    // Taking a slot, and giving it back: everything a choice touches is logged, so a search can
    // undo it.
    struct undo_t {
      bits_t width;
      size_t index;
      bool fresh;
      int old_busy;
      std::string op_id;
      std::vector<std::tuple<bool, code_t, code_t>> new_rotations;
      std::vector<std::pair<bool, code_t>> new_words;
    };
    const auto touched_words = [&](const value_t &value, const code_t &name) {
      std::vector<code_t> names{name};
      for (const auto &[op_id, rotation] : value.sources) {
        auto found_it = slot_fields.find(op_id);
        if (found_it != slot_fields.end()) {
          names.push_back(found_it->second.name);
        }
      }
      return names;
    };
    const auto take = [&](const value_t &value, const candidate_t &c) -> undo_t {
      undo_t undo{value.def.width, c.index, c.fresh, 0, value.def.op_id, {}, {}};
      if (c.fresh) {
        state_slots_used[undo.width]++;
        pools[undo.width].push_back(slot_t{c.index, -1, {}});
      }
      slot_t &slot      = entry_of(undo.width, c.index);
      undo.old_busy     = slot.busy_until;
      slot.busy_until   = value.to;
      const code_t name = slot_name(undo.width, c.index);
      slot_fields.insert({undo.op_id, var_t(name, value.def.out, undo.width, false, true, false)});
      for (const pair_t &pair : pairs_of(value)) {
        auto [it, inserted] = slot_rotations.emplace(pair_key(pair, name), pair.rotation);
        if (inserted) {
          undo.new_rotations.push_back(it->first);
        }
      }
      for (const code_t &word : touched_words(value, name)) {
        if (words_used.insert({value.egress, word}).second) {
          undo.new_words.emplace_back(value.egress, word);
        }
      }
      return undo;
    };
    const auto give_back = [&](const undo_t &undo) {
      for (const auto &word : undo.new_words) {
        words_used.erase(word);
      }
      for (const auto &key : undo.new_rotations) {
        slot_rotations.erase(key);
      }
      slot_fields.erase(undo.op_id);
      entry_of(undo.width, undo.index).busy_until = undo.old_busy;
      if (undo.fresh) {
        pools[undo.width].pop_back();
        state_slots_used[undo.width]--;
      }
    };
    const auto over_budget = [&](const value_t &value, const candidate_t &c) {
      size_t count = 0;
      for (const auto &[egress, word] : words_used) {
        count += egress == value.egress ? 1 : 0;
      }
      for (const code_t &word : touched_words(value, slot_name(value.def.width, c.index))) {
        count += words_used.contains({value.egress, word}) ? 0 : 1;
      }
      return count > WORD_BUDGET;
    };

    // A depth-first search over the path's values in their order, each taking its best slot
    // first, that backtracks when a gress would touch more words than one PHV group holds; the
    // greedy choice alone leaves words behind that the pairs then keep others out of (measured on
    // the SmartCookie plan: 14 egress words greedily, 11 by search). Past the node limit the
    // greedy choice stands.
    std::vector<size_t> todo;
    for (size_t i = 0; i < values.size(); i++) {
      if (!values[i].fixed) {
        todo.push_back(i);
      }
    }
    size_t nodes                       = 0;
    constexpr size_t NODE_LIMIT        = 200'000;
    std::function<bool(size_t)> search = [&](size_t k) -> bool {
      if (k == todo.size()) {
        return true;
      }
      if (++nodes > NODE_LIMIT) {
        return false;
      }
      const value_t &value = values[todo[k]];
      for (const candidate_t &c : candidates(value, pairs_of(value))) {
        if (over_budget(value, c)) {
          continue;
        }
        const undo_t undo = take(value, c);
        if (search(k + 1)) {
          return true;
        }
        give_back(undo);
        if (nodes > NODE_LIMIT) {
          return false;
        }
      }
      return false;
    };
    if (!search(0)) {
      if (Walk::enabled()) {
        std::cerr << "[homes] no assignment within " << WORD_BUDGET << " words per gress after " << nodes << " nodes; taking the greedy one\n";
      }
      for (size_t k : todo) {
        const value_t &value = values[k];
        take(value, candidates(value, pairs_of(value)).front());
      }
    } else if (Walk::enabled()) {
      std::cerr << "[homes] assignment within " << WORD_BUDGET << " words per gress after " << nodes << " nodes\n";
    }
    for (const value_t &value : values) {
      if (!value.fixed) {
        std::vector<code_t> &planned = planned_sources[value.def.op_id];
        planned.clear();
        for (const auto &[op_id, rotation] : value.sources) {
          auto found_it = slot_fields.find(op_id);
          if (found_it != slot_fields.end()) {
            planned.push_back(found_it->second.name);
          }
        }
      }
    }
  };

  for (const std::vector<const EPNode *> &path : paths) {
    std::vector<value_t> values;
    std::vector<std::pair<std::string, int>> foreign_writes;      // (op id, time): a shared action's other ops, at the call.
    std::unordered_map<std::string, size_t> value_index;          // By canonical op id, into `values`.
    std::unordered_map<std::string, std::string> symbol_producer; // A symbol -> the canonical op producing it, this path.
    int pass           = 0;
    bool egress_pass   = false;
    const auto time_of = [&](const std::string &op_id) { return pass * STRIDE + stage_of(op_id); };
    // A def's sources on this path: the operands as they are, the symbols by what produced them here.
    const auto resolve_sources = [&](const def_t &def) {
      std::vector<std::pair<std::string, unsigned>> sources;
      for (const source_t &source : def.sources) {
        std::string op_id = source.op_id;
        if (op_id.empty()) {
          auto producer_it = symbol_producer.find(source.symbol);
          if (producer_it == symbol_producer.end()) {
            continue; // Not a compute value on this path: a packet field, a register's value.
          }
          op_id = producer_it->second;
        }
        sources.emplace_back(op_id, source.rotation);
      }
      return sources;
    };

    for (size_t i = 0; i < path.size(); i++) {
      const Module *module = path[i]->get_module();
      if (!module) {
        continue;
      }
      // Only a hash chain's values live in slots; a compute op off every chain (the clock and
      // delta arithmetic feeding the cookie) keeps its metadata variable, where nothing slices
      // it, and reaches the chain through the hash unit like any other outside value. What it
      // reads of the chain is read all the same: an exit xor holds its words to its own stage,
      // or the next rotate takes one of them first.
      const bool off_core =
          TofinoModuleFactory::is_compute_module(module) && !TofinoModuleFactory::is_hash_chain_node(ep->get_bdd(), module->get_node());
      // A module that leaves its node to be processed again (a cut: the node's own module comes
      // next, on the far side) reads nothing itself; counting the node's reads here, at a module
      // with no placed data structure, would hold the values to the end of the path.
      const Module *next  = i + 1 < path.size() ? path[i + 1]->get_module() : nullptr;
      const bool cut_here = next && next->get_node() == module->get_node();
      // Reads first: a value read here lives at least to this module's time.
      const std::unordered_set<std::string> reads = cut_here ? std::unordered_set<std::string>{} : reads_of(module);
      const bool is_compute                       = TofinoModuleFactory::is_compute_module(module);
      for (const std::string &symbol : reads) {
        auto producer_it = symbol_producer.find(symbol);
        if (producer_it == symbol_producer.end()) {
          continue;
        }
        value_t &value = values[value_index.at(producer_it->second)];
        int use        = INF;
        if (is_compute) {
          // The value lives to the op that reads it: the def whose sources name it, not the
          // module's earliest def (a materialized operand computed a stage earlier reads only its
          // own inputs). An op whose reads are not tracked, in the hash unit, reads at its own time.
          const std::vector<def_t> &reader_defs = defs_of(module);
          for (const def_t &def : reader_defs) {
            for (const auto &[source, rotation] : resolve_sources(def)) {
              if (source == producer_it->second) {
                use = std::min(use, time_of(def.op_id));
              }
            }
          }
          if (use >= INF) {
            for (const def_t &def : reader_defs) {
              if (def.sources.empty()) {
                use = std::min(use, time_of(def.op_id));
              }
            }
          }
        }
        // A compute module with no value of its own (its result is not a plain symbol) reads at
        // its action's stage, like any other module with a placed data structure.
        if (use >= INF) {
          if (const TofinoModule *reader = dynamic_cast<const TofinoModule *>(module)) {
            // A table or a register reads the value at its stage. A module that placed nothing (a
            // gateway, a hand-off to the controller) reads it at a stage the pipeline does not
            // know: the value lives to the end of the path.
            const std::unordered_set<DS_ID> dss = reader->get_generated_ds();
            int last                            = -1;
            for (const DS_ID &ds : dss) {
              last = std::max(last, pipeline.get_placed_stage(ds));
            }
            if (!dss.empty() && last >= 0) {
              use = pass * STRIDE + last;
            }
          }
        }
        if (use >= INF && Walk::enabled()) {
          std::cerr << "[homes] " << module->get_name() << " at node " << module->get_node()->get_id() << " holds " << symbol << " to the end\n";
        }
        if (!is_compute) {
          value.read_elsewhere = true;
        }
        value.to = std::max(value.to, use);
      }
      if (off_core) {
        continue; // Its own values stay in metadata: no slot, no def here.
      }
      // A temporary is read by its own module's ops: it lives to the last of those that read it
      // (a shift rotate's operand to the halves, the halves to the or), or, for a hash op whose
      // reads are not tracked, to the module's last op.
      const std::vector<def_t> defs = defs_of(module);
      int module_time               = -1;
      std::unordered_map<std::string, int> read_within;
      for (const def_t &def : defs) {
        module_time = std::max(module_time, time_of(def.op_id));
        for (const auto &[source, rotation] : resolve_sources(def)) {
          read_within[source] = std::max(read_within.count(source) ? read_within.at(source) : -1, time_of(def.op_id));
        }
      }
      for (const def_t &def : defs) {
        const int time = time_of(def.op_id);
        auto index_it  = value_index.find(def.op_id);
        if (index_it != value_index.end()) {
          // Defined already: a shared action holding several of this module's ops, or the op
          // node of an expression a rotate's operand computed first, which names that value.
          value_t &value = values[index_it->second];
          if (value.def.symbol.empty() && !def.symbol.empty()) {
            value.def.symbol            = def.symbol;
            value.def.out               = def.out;
            symbol_producer[def.symbol] = def.op_id;
          }
          for (const auto &source : resolve_sources(def)) {
            if (std::find(value.sources.begin(), value.sources.end(), source) == value.sources.end()) {
              value.sources.push_back(source);
              slot_readers[source.first].emplace_back(def.op_id, source.second, egress_pass);
            }
          }
          continue;
        }
        value_index.insert({def.op_id, values.size()});
        values.push_back({def, time,
                          def.symbol.empty() ? std::max(time, read_within.count(def.op_id) ? read_within.at(def.op_id) : module_time) : time,
                          slot_fields.contains(def.op_id), egress_pass, resolve_sources(def)});
        for (const auto &[source, rotation] : values.back().sources) {
          slot_readers[source].emplace_back(def.op_id, rotation, egress_pass);
        }
        if (!def.symbol.empty()) {
          symbol_producer[def.symbol] = def.op_id;
        }
      }
      // The other ops of a shared action this module calls write their slots at the call.
      for (const def_t &def : defs) {
        if (const std::optional<compute_reuse_t> reuse = tofino_ctx->get_compute_reuse(def.op_id)) {
          const ComputeAction *action = dynamic_cast<const ComputeAction *>(tofino_ctx->get_data_structures().get_ds_from_id(reuse->action));
          for (const compute_op_t &op : action->ops) {
            if (op.id != reuse->op.id) {
              foreign_writes.emplace_back(op.id, pass * STRIDE + pipeline.get_placed_stage(reuse->action));
            }
          }
        }
      }
      if (module->get_type() == ModuleType::Tofino_SendToEgress || module->get_type() == ModuleType::Tofino_Recirculate) {
        pass++;
        egress_pass = module->get_type() == ModuleType::Tofino_SendToEgress;
      }
    }

    std::vector<value_t> slotted;
    for (const value_t &value : values) {
      if (value.def.width % 8 == 0 && value.def.width <= 64) {
        slotted.push_back(value);
      }
    }
    allocate(slotted, foreign_writes);

    if (Walk::enabled()) {
      std::set<code_t> ingress_slots, egress_slots;
      for (const value_t &value : slotted) {
        (value.egress ? egress_slots : ingress_slots).insert(slot_fields.at(value.def.op_id).name);
      }
      std::cerr << "[homes] path of " << path.size() << " nodes: " << slotted.size() << " slotted values, " << (values.size() - slotted.size())
                << " of other widths; slots: ingress " << ingress_slots.size() << ", egress " << egress_slots.size() << "\n";
      for (const value_t &value : slotted) {
        std::cerr << "  " << slot_fields.at(value.def.op_id).name << " <- " << value.def.op_id
                  << (value.def.symbol.empty() ? "" : " (" + value.def.symbol + ")") << " [" << value.from << ", "
                  << (value.to >= INF ? std::string("end") : std::to_string(value.to)) << "]" << (value.fixed ? " fixed" : "")
                  << (value.egress ? " egress" : "");
        for (const auto &[op_id, rotation] : value.sources) {
          auto found_it = slot_fields.find(op_id);
          std::cerr << " " << (found_it != slot_fields.end() ? found_it->second.name : op_id) << ":" << rotation;
        }
        std::cerr << "\n";
      }
    }
  }
  if (Walk::enabled()) {
    size_t ingress_words = 0, egress_words = 0;
    for (const auto &[egress, name] : words_used) {
      (egress ? egress_words : ingress_words)++;
    }
    std::cerr << "[homes] words touched over every path: ingress " << ingress_words << ", egress " << egress_words << " (budget " << WORD_BUDGET
              << " each)\n";
  }
}

void TofinoSynthesizer::synthesize() {
  const BDD *bdd = target_ep->get_bdd();

  symbol_t device = bdd->get_device();
  symbol_t time   = bdd->get_time();

  alloc_var("meta.dev", device.expr, EXACT_NAME);
  alloc_var("meta.time", solver_toolbox.exprBuilder->Extract(time.expr, 0, 64), EXACT_NAME);

  ingress_vars.push();

  // The chunks a ChecksumUpdate covers, for the extraction visitor's header layout: the headers
  // the call names by address are the nearest extractions of those addresses above it on its path.
  {
    std::vector<const EPNode *> pending{target_ep->get_root()};
    while (!pending.empty()) {
      const EPNode *ep_node = pending.back();
      pending.pop_back();
      pending.insert(pending.end(), ep_node->get_children().begin(), ep_node->get_children().end());

      const Module *module = ep_node->get_module();
      if (!module || module->get_type() != ModuleType::Tofino_ChecksumUpdate) {
        continue;
      }
      const Tofino::ChecksumUpdate *update = dynamic_cast<const Tofino::ChecksumUpdate *>(module);

      const Tofino::ParserExtraction *ip = nullptr;
      const Tofino::ParserExtraction *l4 = nullptr;
      for (const EPNode *above = ep_node->get_prev(); above; above = above->get_prev()) {
        const Module *above_module = above->get_module();
        if (!above_module || above_module->get_type() != ModuleType::Tofino_ParserExtraction) {
          continue;
        }
        const Tofino::ParserExtraction *extraction = dynamic_cast<const Tofino::ParserExtraction *>(above_module);
        if (extraction->get_hdr_addr() == update->get_ip_hdr_addr() && !ip) {
          ip = extraction;
        } else if (extraction->get_hdr_addr() == update->get_l4_hdr_addr() && !l4) {
          l4 = extraction;
        }
      }
      assert_or_panic(ip && l4, "Checksum update without its headers extracted above it");

      // The deparser checksum covers whole headers: an IPv4 header without options and a TCP or
      // UDP header. A call over anything else (nat borrows the four port bytes as its L4 header,
      // and carries a payload) is left as it was, the checksums untouched.
      if (ip->get_length() != 20 || (l4->get_length() != 20 && l4->get_length() != 8)) {
        std::cerr << "[checksum] BDD node " << module->get_node()->get_id() << ": L4 header of " << l4->get_length()
                  << " bytes, not recomputed in the deparser\n";
        continue;
      }

      const checksum_site_t site{chunk_key(ip->get_hdr()), chunk_key(l4->get_hdr())};
      checksummed_chunks[site.ip_hdr]        = {true, ip->get_length()};
      checksummed_chunks[site.l4_hdr]        = {false, l4->get_length()};
      checksum_headers_of[ep_node->get_id()] = site;
    }
  }

  plan_value_homes(target_ep);
  plan_shared_runs(target_ep);
  EPVisitor::visit(target_ep);
  emit_action_variants(target_ep->get_ctx().get_target_ctx<TofinoContext>());

  // The recirculation passes are mutually exclusive: the code path is read from the header the
  // previous pass wrote and is never reassigned, so they belong in one if / else-if chain.
  // Emitting them into a single coder nested each pass inside the previous one's block, which
  // left every pass after the first unreachable.
  {
    coder_t &recirc = code_template.get(MARKER_INGRESS_CONTROL_APPLY_RECIRC);
    for (code_path_t code_path = 0; code_path < recirc_coders.size(); code_path++) {
      recirc.indent();
      recirc << (code_path == 0 ? code_t("") : code_t("} else "));
      recirc << "if (hdr.recirc.code_path == ";
      recirc << (i64)code_path;
      recirc << ") {\n";
      recirc << recirc_coders[code_path].dump();
    }
    if (!recirc_coders.empty()) {
      recirc.indent();
      recirc << "}\n";
    }
  }

  // The template no longer hardcodes these: a solution that uses the egress pipeline must not
  // bypass it, and the egress parser needs a transition. Written here so a solution that does not
  // use egress emits exactly what the template used to contain.
  {
    coder_t &decision = code_template.get(MARKER_INGRESS_EGRESS_DECISION);
    decision.indent();
    if (uses_egress) {
      decision << "if (meta.to_egress == 1) {\n";
      decision.inc();
      decision.indent();
      decision << "ig_tm_md.bypass_egress = 0;\n";
      decision.dec();
      decision.indent();
      decision << "} else {\n";
      decision.inc();
      decision.indent();
      decision << "ig_tm_md.bypass_egress = 1;\n";
      decision.dec();
      decision.indent();
      decision << "}\n";
    } else {
      decision << "ig_tm_md.bypass_egress = 1;\n";
    }
  }

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
  std::unordered_set<code_t> declared_recirc_slots;
  for (const var_t &var : recirc_hdr_vars.get_all()) {
    if (!declared_recirc_slots.insert(var.get_stem()).second) {
      continue;
    }
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

  if (!uses_egress) {
    coder_t &eg_parser_start = code_template.get(MARKER_EGRESS_PARSER_START);
    eg_parser_start.indent();
    eg_parser_start << "transition accept;\n";
  }

  if (!state_slots_used.empty()) {
    coder_t &state_hdr = code_template.get(MARKER_EGRESS_STATE_HEADER);
    state_hdr << "// The computed values' home: slots reused by live range. A real header -- valid from the start,\n";
    state_hdr << "// carried with the packet across every pass, dropped where it leaves -- so its fields are exact\n";
    state_hdr << "// containers: the allocator then keeps each one's slices in one container, as it does for the\n";
    state_hdr << "// ground truth's recirc_state, instead of spreading them and running out of PHV sources.\n";
    state_hdr << "header state_h {\n";
    for (const auto &[width, slots] : state_slots_used) {
      for (size_t slot = 0; slot < slots; slot++) {
        state_hdr << "  bit<" << width << "> s" << width << "_" << slot << ";\n";
        handoff_layout.state_words.emplace_back("s" + std::to_string(width) + "_" + std::to_string(slot), width);
      }
    }
    state_hdr << "}\n\n";
  }

  // The crossings are mutually exclusive: the egress reads which one the packet took and runs
  // that block only, as the recirculation passes do.
  if (uses_egress) {
    coder_t &egress_apply = code_template.get(MARKER_EGRESS_CONTROL_APPLY);
    // Blocks calling one shared run are one arm (plan_shared_runs): what each does besides is
    // nested under its own code path, before and after the calls, as the marker splits it.
    std::unordered_map<code_path_t, size_t> run_of_block;
    for (size_t r = 0; r < shared_runs.size(); r++) {
      for (const EPNode *cut : shared_runs[r].egress_cuts) {
        if (auto it = egress_code_path_of.find(cut); it != egress_code_path_of.end()) {
          run_of_block[it->second] = r;
        }
      }
    }
    const code_t arm_indent = code_t((egress_apply.lvl + 1) * 2, ' ');
    const auto nest         = [&](code_path_t code_path, const code_t &body) {
      bool only_comments = true;
      std::stringstream scan(body);
      for (code_t line; only_comments && std::getline(scan, line);) {
        const size_t text = line.find_first_not_of(" \t");
        only_comments     = text == code_t::npos || line.compare(text, 2, "//") == 0;
      }
      if (only_comments) {
        return; // Nothing but the plan's notes: no block for them.
      }
      egress_apply << arm_indent << "if (hdr.egress_state.code_path == " << (i64)code_path << ") {\n";
      std::stringstream lines(body);
      code_t line;
      while (std::getline(lines, line)) {
        egress_apply << (line.empty() ? line : "  " + line) << "\n";
      }
      egress_apply << arm_indent << "}\n";
    };
    std::unordered_set<size_t> emitted_runs;
    bool first_arm = true;
    for (code_path_t code_path = 0; code_path < egress_coders.size(); code_path++) {
      const auto run_it = run_of_block.find(code_path);
      if (run_it == run_of_block.end()) {
        egress_apply.indent();
        egress_apply << (first_arm ? code_t("") : code_t("} else "));
        egress_apply << "if (hdr.egress_state.code_path == " << (i64)code_path << ") {\n";
        egress_apply << egress_coders[code_path].dump();
        first_arm = false;
        continue;
      }
      if (emitted_runs.contains(run_it->second)) {
        continue;
      }
      emitted_runs.insert(run_it->second);
      const shared_run_t &run = shared_runs[run_it->second];
      std::vector<code_path_t> arms;
      for (code_path_t other = 0; other < egress_coders.size(); other++) {
        if (auto it = run_of_block.find(other); it != run_of_block.end() && it->second == run_it->second) {
          arms.push_back(other);
        }
      }
      egress_apply.indent();
      egress_apply << (first_arm ? code_t("") : code_t("} else ")) << "if (";
      for (size_t i = 0; i < arms.size(); i++) {
        egress_apply << (i == 0 ? code_t("") : code_t(" || ")) << "hdr.egress_state.code_path == " << (i64)arms[i];
      }
      egress_apply << ") {\n";
      first_arm           = false;
      const code_t marker = "// @shared-run " + std::to_string(run_it->second);
      std::vector<std::pair<code_t, code_t>> halves; // Each arm's block before and after the marker.
      for (const code_path_t arm : arms) {
        const code_t block = egress_coders[arm].dump();
        const size_t at    = block.find(marker);
        assert(at != code_t::npos && "A merged egress block without its shared run's marker");
        const size_t line_end = block.find('\n', at);
        halves.emplace_back(block.substr(0, block.rfind('\n', at) + 1), line_end == code_t::npos ? code_t("") : block.substr(line_end + 1));
      }
      for (size_t i = 0; i < arms.size(); i++) {
        nest(arms[i], halves[i].first);
      }
      for (const code_t &call : run.calls) {
        egress_apply << arm_indent << call << "\n";
      }
      for (size_t i = 0; i < arms.size(); i++) {
        nest(arms[i], halves[i].second);
      }
    }
    if (!egress_coders.empty()) {
      egress_apply.indent();
      egress_apply << "}\n";
    }
  }

  if (uses_egress) {
    coder_t &eg_state_hdr = code_template.get(MARKER_EGRESS_STATE_HEADER);
    eg_state_hdr << "header egress_state_h {\n";
    eg_state_hdr << "  bit<16> code_path;\n";
    for (const var_t &var : egress_state_hdr_vars.get_all()) {
      const bits_t pad = var.is_bool() ? 7 : (8 - var.expr->getWidth()) % 8;
      if (pad > 0) {
        eg_state_hdr << "  @padding bit<" << pad << "> pad_" << var.get_stem() << ";\n";
      }
      eg_state_hdr << "  ";
      var.declare(eg_state_hdr);
    }
    eg_state_hdr << "}\n";

    // Helpers the ingress control has inline. Emitted only when the egress is used, so a solution
    // that stays in ingress comes out byte-identical. They go in their own marker ahead of the
    // control's body: this runs after the plan has been walked, and P4 wants a declaration before
    // its use, so appending to the body's coder would put them after the actions that call them.
    coder_t &eg_control = code_template.get(MARKER_EGRESS_CONTROL_HELPERS);
    eg_control << "  action swap(inout bit<8> a, inout bit<8> b) {\n";
    eg_control << "    bit<8> tmp = a;\n";
    eg_control << "    a = b;\n";
    eg_control << "    b = tmp;\n";
    eg_control << "  }\n\n";
    for (const bits_t width : {16, 24, 32}) {
      eg_control << "  action swap" << width << "(inout bit<" << width << "> a, inout bit<" << width << "> b) {\n";
      eg_control << "    bit<" << width << "> tmp = a;\n";
      eg_control << "    a = b;\n";
      eg_control << "    b = tmp;\n";
      eg_control << "  }\n\n";
    }
    eg_control << "  bit<1> diff_sign_bit;\n";
    eg_control << "  action calculate_diff_32b(bit<32> a, bit<32> b) { diff_sign_bit = (a - b)[31:31]; }\n";
    eg_control << "  action calculate_diff_16b(bit<16> a, bit<16> b) { diff_sign_bit = (a - b)[15:15]; }\n";
    eg_control << "  action calculate_diff_8b(bit<8> a, bit<8> b) { diff_sign_bit = (a - b)[7:7]; }\n";

    code_template.get(MARKER_INGRESS_EGRESS_STATE_FIELD) << "  egress_state_h egress_state;\n";
    code_template.get(MARKER_EGRESS_EGRESS_STATE_FIELD) << "  egress_state_h egress_state;\n";
    code_template.get(MARKER_INGRESS_METADATA) << "  bit<1> to_egress;\n";

    coder_t &eg_parser = code_template.get(MARKER_EGRESS_PARSER_START);
    // The crossing always recirculates -- the egress cannot choose a port -- so the ingress runs
    // build_recirc_hdr on this path and its deparser emits hdr.recirc, which precedes
    // egress_state in the header struct. Without extracting it here the egress parser reads
    // egress_state out of the recirculation header's bytes, hdr.recirc stays invalid in egress,
    // and the egress's writes to it are dead: bf-p4c then eliminates the whole egress compute
    // chain, which is how a plan that discards half its work looked like it fit.
    eg_parser.indent();
    eg_parser << "pkt.extract(hdr.recirc);\n";
    eg_parser.indent();
    eg_parser << "pkt.extract(hdr.egress_state);\n";
    if (!state_slots_used.empty()) {
      eg_parser.indent();
      eg_parser << "pkt.extract(hdr.st);\n"; // Travels after egress_state, in struct order.
    }
    coder_t &eg_hdrs = code_template.get(MARKER_EGRESS_HEADERS);
    for (const code_t &hdr_name : egress_parser_hdrs) {
      eg_parser.indent();
      eg_parser << "pkt.extract(" << hdr_name << ");\n";
      const code_t stem = hdr_name.substr(hdr_name.rfind('.') + 1);
      eg_hdrs << "  " << stem << "_h " << stem << ";\n";
    }
    eg_parser.indent();
    eg_parser << "transition accept;\n";

    // The state header has to live as long as the packet is going round. The ingress writes it
    // before the crossing and reads it again on the pass after the lap, so dropping it in the
    // egress -- or never extracting it on the way back in -- makes those reads return zero. It is
    // dropped where the packet actually leaves, alongside the recirculation header.
    coder_t &parse_recirc = code_template.get(MARKER_PARSE_RECIRC);
    parse_recirc.indent();
    parse_recirc << "pkt.extract(hdr.egress_state);\n";

    coder_t &leave_switch = code_template.get(MARKER_LEAVE_SWITCH);
    leave_switch.indent();
    leave_switch << "hdr.egress_state.setInvalid();\n";
    coder_t &leave_to_cpu = code_template.get(MARKER_LEAVE_TO_CPU);
    leave_to_cpu.indent();
    leave_to_cpu << "hdr.egress_state.setInvalid();\n";
  }

  if (!state_slots_used.empty()) {
    code_template.get(MARKER_INGRESS_EGRESS_STATE_FIELD) << "  state_h st;\n";
    code_template.get(MARKER_EGRESS_EGRESS_STATE_FIELD) << "  state_h st;\n";
    coder_t &apply_start = code_template.get(MARKER_INGRESS_APPLY_START);
    apply_start.indent();
    apply_start << "hdr.st.setValid();\n";
    coder_t &parse_recirc = code_template.get(MARKER_PARSE_RECIRC);
    parse_recirc.indent();
    parse_recirc << "pkt.extract(hdr.st);\n";
    coder_t &leave_switch = code_template.get(MARKER_LEAVE_SWITCH);
    leave_switch.indent();
    leave_switch << "hdr.st.setInvalid();\n";
    // To the controller the state header goes along, after the cpu header, and comes back with
    // it (handoff_layout); fwd_to_cpu keeps it.
    coder_t &parse_cpu = code_template.get(MARKER_PARSE_CPU);
    parse_cpu.indent();
    parse_cpu << "pkt.extract(hdr.st);\n";
  }

  emit_deparser_checksums(false);
  emit_deparser_checksums(true);

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

      // Nothing to select on (the condition only checked the packet length): the branch every
      // frame takes.
      if (select->selections.empty()) {
        const ParserState *taken = select->constant_branch ? select->on_true : select->on_false;
        assert(taken && "Next state not found");
        ingress_parser.indent();
        ingress_parser << "state " << state_name << " {\n";
        ingress_parser.inc();
        ingress_parser.indent();
        ingress_parser << "transition " << get_parser_state_name(taken, false) << ";\n";
        ingress_parser.dec();
        ingress_parser.indent();
        ingress_parser << "}\n";
        states.push_back(taken);
        break;
      }

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

        // parser_selection_t carries `negated` and this emitter used to ignore it, so a condition
        // like BDD node 5's `!(17 == protocol)` came out as `17: on_true`. Measured consequence on
        // SmartCookie: IPv4 protocol 0x11 (UDP) reached the 160-bit TCP header and 0x06 (TCP) fell
        // through to the 64-bit UDP one, so a TCP packet never got a TCP header at all.
        //
        // Un-negated, the chain is an OR: selection i matches -> on_true, else try the next.
        // Negated it is the De Morgan dual, an AND of nots: a match settles it as false, and only
        // falling off the end of the chain reaches on_true.
        const code_t on_match   = selection.negated ? get_parser_state_name(select->on_false, false) : next_true;
        const code_t on_default = !selection.negated ? next_false
                                                     : (i < select->selections.size() - 1 ? get_selection_state_name(i + 1)
                                                                                          : get_parser_state_name(select->on_true, false));

        // Labels take the selector's declared width, which can exceed the NF value's (the
        // 16-bit device selects on the 32-bit meta.dev).
        const std::optional<var_t> selector = ingress_vars.get(selection.target);
        const bits_t selector_width = selector ? (selector->name == "meta.dev" ? 32 : selector->size) : 0; // meta.dev: the template's bit<32>.
        for (klee::ref<klee::Expr> value : selection.values) {
          klee::ref<klee::Expr> label = value;
          if (selector_width > value->getWidth() && is_constant(value)) {
            label = solver_toolbox.exprBuilder->ZExt(value, selector_width);
          }
          ingress_parser.indent();
          ingress_parser << transpiler.transpile(label, TRANSPILER_OPT_SWAP_CONST_ENDIANNESS) << ": " << on_match << ";\n";
        }

        ingress_parser.indent();
        ingress_parser << "default: " << on_default << ";\n";

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
    // Past an egress crossing the metadata lives in the egress struct, which the Egress control
    // takes as eg_md; there is no `meta` on that side of the pipeline.
    name = (in_egress ? "eg_md." : "meta.") + name;
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

TofinoSynthesizer::var_t TofinoSynthesizer::alloc_var(const code_t &proposed_name, bits_t size, alloc_opt_t option) {
  code_t name = (option & EXACT_NAME) ? proposed_name : create_unique_name(proposed_name);

  if (option & IS_INGRESS_METADATA) {
    // Past an egress crossing the metadata lives in the egress struct, which the Egress control
    // takes as eg_md; there is no `meta` on that side of the pipeline.
    name = (in_egress ? "eg_md." : "meta.") + name;
  }

  const var_t var(name, nullptr, size, option & FORCE_BOOL, option & (HEADER | HEADER_FIELD), option & BUFFER);

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
  recirc_coders.emplace_back(code_template.get(MARKER_INGRESS_CONTROL_APPLY_RECIRC).lvl + 1);
  return size;
}

code_path_t TofinoSynthesizer::alloc_egress_coder() {
  const size_t size = egress_coders.size();
  egress_coders.emplace_back(code_template.get(MARKER_EGRESS_CONTROL_APPLY).lvl + 1);
  return size;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::SendToController *node) {
  flag_pending_checksum();

  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);
  const Symbols &symbols = node->get_symbols();

  ingress_apply.indent();
  ingress_apply << "fwd_op = fwd_op_t.FORWARD_TO_CPU;\n";
  ingress_apply.indent();
  ingress_apply << "build_cpu_hdr(" << ep->get_cpu_code_path(ep_node) << ");\n";

  for (const symbol_t &symbol : symbols.get()) {
    std::optional<var_t> var = ingress_vars.get(symbol.expr);

    if (!var) {
      dbg_vars();
      panic("Variable %s not found in stack", expr_to_string(symbol.expr, true).c_str());
    }

    if (var->is_header_field) {
      // A packet header field the controller reads from the packet itself. A state word travels
      // with the state header, whole: the controller reads it there (handoff_layout).
      if (var->name.rfind("hdr.st.", 0) == 0) {
        handoff_layout.symbol_word[ep_node->get_id()][symbol.name] = var->name.substr(std::string("hdr.st.").size());
      }
      continue;
    }

    var_t cpu_var = *var;
    cpu_var.name  = "hdr.cpu." + var->get_stem();

    // The controller lays this header out from the BDD symbol's width, so the data plane has to
    // declare the same width. A 64-bit symbol whose data plane value is 32 bits made the P4 header
    // 48 bytes against the controller's 56, and every field past it was read at the wrong offset.
    const bits_t symbol_width = symbol.expr->getWidth();
    const bool widened        = symbol_width > cpu_var.size;

    if (widened) {
      cpu_var.size = cpu_var.original_size = symbol_width;
    }

    cpu_hdr_vars.push(cpu_var);

    // A sliced write cannot go through the hash unit, so it stays a plain copy.
    const bool sliced = symbol.name == "next_time";
    // Every computed value, not just the ones a rotate cut directly. The controller header is
    // written on the slow path, all of its fields at once, and they all end up in the compute
    // chain's supercluster, so an unwrapped one drags the whole group back into the ALU. The
    // egress-state and recirculation writes below are different: those are few and on the fast
    // path, and wrapping the uncut ones there only spends hash-distribution units.
    // A widened write carries the cast, and 64 bits do not fit the hash unit's 32-bit immediate.
    const bool via_hash = !sliced && !widened && var->name.rfind("hdr.st.", 0) == 0;

    ingress_apply.indent();
    if (via_hash) {
      ingress_apply << "@in_hash { ";
    }
    ingress_apply << cpu_var.name;
    if (sliced) {
      ingress_apply << "[47:16]";
    }
    ingress_apply << " = ";
    if (widened) {
      ingress_apply << "(" << Transpiler::type_from_size(symbol_width) << ")";
    }
    ingress_apply << var->name;
    ingress_apply << ";";
    ingress_apply << (via_hash ? " }\n" : "\n");
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Recirculate *node) {
  assert(ep_node->get_children().size() == 1);
  const EPNode *next = ep_node->get_children()[0];

  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  // Reached past a crossing, the port and the header's ingress-only fields (ingress_port, dev)
  // still have to be written in the ingress; only the values this pass computed are written here.
  coder_t &pass_end = (in_egress && ingress_coder_at_cut) ? *ingress_coder_at_cut : ingress_apply;

  // 1. Allocate a new recirculation code path
  const code_path_t code_path = alloc_recirc_coder();

  // 2. Build the recirculation header and populate it with the current stack
  pass_end.indent();
  pass_end << "fwd_op = fwd_op_t.RECIRCULATE;\n";
  pass_end.indent();
  pass_end << "build_recirc_hdr(" << code_path << ");\n";

  Stacks stack_backup = ingress_vars;

  // The first stack contains the variables set up right at the beginning of the ingress processing.
  // Every other stack contains variables introduced by modules.
  Stack first_stack              = ingress_vars.get_first_stack();
  std::vector<var_t> recirc_vars = first_stack.get_all();
  // A recirculation from the egress comes back through the ingress, where the clock is meta.time
  // again, not the egress name a crossing gave it.
  for (var_t &var : recirc_vars) {
    if (var.name == "eg_md.time" || var.original_name == "eg_md.time") {
      var.name          = "meta.time";
      var.original_name = "meta.time";
    }
  }

  // A variable can be known under several expressions (an alias: the same value another
  // module computes, e.g. a register's returned value); each of them must resolve to the
  // recirculation header field after the recirculation, so aliases are kept, not squashed.
  std::unordered_map<code_t, var_t> local_recirc_vars_by_name;

  // Slot counters restart for every recirculation, so the fields this pass needs land on the same
  // header slots another pass uses for its own values.
  begin_recirc_slots();

  // Only what the rest of the processing needs goes around: a chain of computations leaves
  // many dead temporaries behind, and a header holding them all doesn't fit the PHV. Live:
  // every symbol a BDD node reachable from here uses (from the final BDD, not the module's
  // symbols, which the search computed before later reorderings), plus everything a later
  // hand-off to the controller ships for its replay of the data-plane decisions.
  const std::unordered_set<std::string> live_symbols = live_symbols_past(ep, node->get_node(), next);
  const auto is_live                                 = [&live_symbols](const var_t &var) {
    if (var.expr.isNull() || var.transient) {
      return false;
    }
    for (const std::string &name : symbol_t::get_symbols_names(var.expr)) {
      if (live_symbols.contains(name)) {
        return true;
      }
    }
    return false;
  };

  for (const Stack &stack : ingress_vars.get_all()) {
    for (const var_t &var : stack.get_all()) {
      if (first_stack.get_exact(var.expr)) {
        continue;
      }

      if (var.is_header_field) {
        recirc_vars.push_back(var);
        continue;
      }

      if (!is_live(var)) {
        continue;
      }

      var_t recirc_var = var;
      recirc_var.name  = recirc_var.flatten_name();

      auto found_it = local_recirc_vars_by_name.find(recirc_var.name);
      if (found_it != local_recirc_vars_by_name.end()) {
        var_t alias = found_it->second;
        alias.expr  = var.expr;
        alias.size  = var.size;
        recirc_vars.push_back(alias);
        continue;
      }

      const code_t slot_kind = recirc_var.is_bool() ? code_t("b") : std::to_string(recirc_var.expr->getWidth());
      const size_t slot      = recirc_slot_for(recirc_var.name, slot_kind);

      var_t local_recirc_var         = recirc_var;
      local_recirc_var.name          = "hdr.recirc.f" + slot_kind + "_" + std::to_string(slot);
      local_recirc_var.original_name = local_recirc_var.name;

      var_t slot_var = recirc_var;
      slot_var.name  = "f" + slot_kind + "_" + std::to_string(slot);
      recirc_hdr_vars.push(slot_var);
      recirc_vars.push_back(local_recirc_var);
      local_recirc_vars_by_name.insert({recirc_var.name, local_recirc_var});

      const bool via_hash = var.name.rfind("hdr.st.", 0) == 0; // A state word: the copy must not slice it on the ALU.
      ingress_apply.indent();
      if (via_hash) {
        ingress_apply << "@in_hash { ";
      }
      ingress_apply << local_recirc_var.name;
      ingress_apply << " = ";
      ingress_apply << var.name;
      ingress_apply << ";";
      ingress_apply << (via_hash ? " }\n" : "\n");
    }
  }

  // 3. Replace the ingress apply coder with the recirc coder (restored below: a recirculation
  // can sit inside another's code path, whose remaining branches must keep going there).
  const std::optional<code_path_t> enclosing_recirc_code_path = active_recirc_code_path;
  active_recirc_code_path                                     = code_path;

  // 4. Clear the stack, rebuild it with hdr.recirc fields, and setup the recirculation code block
  ingress_vars.clear();
  ingress_vars.push();
  for (const var_t &var : recirc_vars) {
    ingress_vars.insert_back(var, /*allow_duplicates=*/true);
  }

  // A recirculated packet re-enters through the ingress pipeline, so everything past this point
  // belongs there even when the recirculation itself was reached from the egress. Leaving the flag
  // set emits the whole tail of the program inside the egress control, where the ingress-only
  // helpers (fwd_op, build_cpu_hdr) are not in scope.
  const bool enclosing_in_egress     = in_egress;
  coder_t *const enclosing_cut_coder = ingress_coder_at_cut;
  in_egress                          = false;
  ingress_coder_at_cut               = nullptr;

  ingress_vars.push();
  visit(ep, next);
  ingress_vars.pop();

  // 5. Revert the state back to before the recirculation was made
  in_egress               = enclosing_in_egress;
  ingress_coder_at_cut    = enclosing_cut_coder;
  active_recirc_code_path = enclosing_recirc_code_path;
  ingress_vars            = stack_backup;

  return EPVisitor::Action::skipChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::SendToEgress *node) {
  assert(ep_node->get_children().size() == 1);
  const EPNode *next = ep_node->get_children()[0];

  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  uses_egress = true;

  // The egress parser has to see exactly what the ingress deparser emitted. Only packets that
  // crossed here reach it, and they all took this one path, so the headers extracted along the
  // way are the whole story.
  {
    std::vector<code_t> hdrs;
    for (const EPNode *prev = ep_node; prev; prev = prev->get_prev()) {
      const Module *prev_module = prev->get_module();
      if (!prev_module || prev_module->get_type() != ModuleType::Tofino_ParserExtraction) {
        continue;
      }
      const Tofino::ParserExtraction *extraction = dynamic_cast<const Tofino::ParserExtraction *>(prev_module);
      if (const std::optional<var_t> hdr_var = hdr_vars.get(extraction->get_hdr())) {
        hdrs.push_back(hdr_var->name);
      }
    }
    std::reverse(hdrs.begin(), hdrs.end());
    egress_parser_hdrs = hdrs;
  }

  const code_path_t egress_code_path = alloc_egress_coder();
  egress_code_path_of[ep_node]       = egress_code_path;
  ingress_apply.indent();
  ingress_apply << "meta.to_egress = 1;\n";
  ingress_apply.indent();
  ingress_apply << "hdr.egress_state.setValid();\n";
  ingress_apply.indent();
  ingress_apply << "hdr.egress_state.code_path = " << (i64)egress_code_path << ";\n";

  Stacks stack_backup = ingress_vars;

  // Unlike a recirculation, the values the ingress starts with (meta.time, meta.dev, the ingress
  // port) are *not* in scope on the far side of a crossing: the egress control takes eg_md, and
  // those fields belong to the ingress. So they travel like anything else the egress still needs,
  // rather than being assumed available -- referring to them by their ingress names emitted
  // `meta.time` inside control Egress, which does not compile.
  Stack first_stack = ingress_vars.get_first_stack();
  std::vector<var_t> egress_vars;
  std::unordered_map<code_t, var_t> local_egress_vars_by_name;

  // Same liveness rule as a recirculation: only what a BDD node reachable from here still uses
  // travels, or the header does not fit the PHV.
  // A hand-off to the controller past the cut ships the data-plane state it replays, so those
  // symbols have to cross too (live_symbols_past counts them).
  const std::unordered_set<std::string> live_symbols = live_symbols_past(ep, node->get_node(), next);
  const auto is_live                                 = [&live_symbols](const var_t &var) {
    if (var.expr.isNull() || var.transient) {
      return false;
    }
    for (const std::string &name : symbol_t::get_symbols_names(var.expr)) {
      if (live_symbols.contains(name)) {
        return true;
      }
    }
    return false;
  };

  for (const Stack &stack : ingress_vars.get_all()) {
    for (const var_t &var : stack.get_all()) {
      if (var.is_header_field) {
        egress_vars.push_back(var);
        continue;
      }

      // The egress reads the clock itself (see the template), so the same expression resolves
      // there without travelling -- and travelling would lose the [47:16] convention the backend
      // rewrites shifts against.
      if (var.original_name == "meta.time") {
        var_t egress_time         = var;
        egress_time.name          = "eg_md.time";
        egress_time.original_name = egress_time.name;
        egress_vars.push_back(egress_time);
        continue;
      }

      if (!is_live(var)) {
        continue;
      }

      var_t egress_var = var;
      egress_var.name  = egress_var.flatten_name();

      auto found_it = local_egress_vars_by_name.find(egress_var.name);
      if (found_it != local_egress_vars_by_name.end()) {
        var_t alias = found_it->second;
        alias.expr  = var.expr;
        alias.size  = var.size;
        egress_vars.push_back(alias);
        continue;
      }

      var_t local_egress_var         = egress_var;
      local_egress_var.name          = "hdr.egress_state." + egress_var.name;
      local_egress_var.original_name = local_egress_var.name;

      egress_state_hdr_vars.push(egress_var);
      egress_vars.push_back(local_egress_var);
      local_egress_vars_by_name.insert({egress_var.name, local_egress_var});

      const bool via_hash = var.name.rfind("hdr.st.", 0) == 0; // A state word: the copy must not slice it on the ALU.
      ingress_apply.indent();
      if (via_hash) {
        ingress_apply << "@in_hash { ";
      }
      ingress_apply << local_egress_var.name;
      ingress_apply << " = ";
      ingress_apply << var.name;
      ingress_apply << ";";
      ingress_apply << (via_hash ? " }\n" : "\n");
    }
  }

  ingress_vars.clear();
  ingress_vars.push();
  for (const var_t &var : egress_vars) {
    ingress_vars.insert_back(var, /*allow_duplicates=*/true);
  }

  coder_t *const enclosing_cut_coder = ingress_coder_at_cut;
  ingress_coder_at_cut               = &ingress_apply;

  const bool enclosing_in_egress                              = in_egress;
  const std::optional<code_path_t> enclosing_egress_code_path = active_egress_code_path;
  in_egress                                                   = true;
  active_egress_code_path                                     = egress_code_path;

  ingress_vars.push();
  visit(ep, next);
  ingress_vars.pop();

  in_egress               = enclosing_in_egress;
  active_egress_code_path = enclosing_egress_code_path;
  ingress_coder_at_cut    = enclosing_cut_coder;
  ingress_vars            = stack_backup;

  return EPVisitor::Action::skipChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Ignore *node) { return EPVisitor::Action::doChildren; }

void TofinoSynthesizer::transpile_if_condition(const If::condition_t &condition) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL_APPLY);

  switch (condition.phv_limitation_workaround.action_helper) {
  case If::ConditionActionHelper::None:
    // Materialize arithmetic operands the gateway can't evaluate (e.g. `count + 1`)
    // into metadata first; the transpiler then resolves them to those fields, so the
    // gateway compares simple fields/constants.
    for (const klee::ref<klee::Expr> &operand : condition.operands_to_materialize) {
      if (ingress_vars.get(operand)) {
        continue;
      }
      const code_t operand_code = transpiler.transpile(operand);
      const var_t operand_var   = alloc_var("cond_operand", operand, IS_INGRESS_METADATA);
      declare_var_in_ingress_metadata(operand_var);
      ingress.indent();
      ingress << operand_var.name << " = " << operand_code << ";\n";
    }
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

  if (!node->get_materialized_operands().empty()) {
    emit_compute_run(ep, ep_node);
  }

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

    emit_shared_runs_after(ep_node);

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

  emit_shared_runs_after(ep_node);

  return EPVisitor::Action::skipChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::ParserCondition *node) {
  parser_vars[node->get_node()->get_id()] = ingress_vars.squash_hdrs_only();
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Then *node) { return EPVisitor::Action::doChildren; }

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Else *node) { return EPVisitor::Action::doChildren; }

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Forward *node) {
  flag_pending_checksum();

  klee::ref<klee::Expr> dst_device = node->get_dst_device();
  // Past a crossing the port still has to be written in ingress. The crossing only happened where
  // this device was computable there, so the expression resolves.
  coder_t &ingress = (in_egress && ingress_coder_at_cut) ? *ingress_coder_at_cut : get(MARKER_INGRESS_CONTROL_APPLY);

  code_t dst_device_code = transpiler.transpile(dst_device);

  // Kind of a hack, and I'm not sure it will work all the time. But for now it does the trick.
  const std::optional<var_t> dst_device_var = ingress_vars.get(dst_device);
  if (dst_device_var.has_value() && dst_device_var->is_header_field) {
    dst_device_code = transpiler.swap_endianness(dst_device_code, dst_device->getWidth());
  }

  // A device taken from a packet word next to a hash chain goes through the hash unit: an ALU op
  // reading the word would tie it to the chain's sliced cluster (the ground truth reads its device
  // from a table).
  const bool via_hash = !state_slots_used.empty() && dst_device_code.find("hdr.") != code_t::npos;
  ingress.indent();
  if (via_hash) {
    ingress << "@in_hash { nf_dev[15:0] = " << dst_device_code << "; }\n";
  } else {
    ingress << "nf_dev[15:0] = " << dst_device_code << ";\n";
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Drop *node) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL_APPLY);

  ingress.indent();
  if (in_egress) {
    ingress << "ig_intr_dprs_md.drop_ctl = 1;\n";
    return EPVisitor::Action::doChildren;
  }
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

  // The field boundaries come from wherever the program happens to read the chunk, so a protocol
  // field read whole in one place and by the octet in another arrives split: on SmartCookie the
  // TCP sequence number and both IPv4 addresses came out as 24 + 8. A field split that way can
  // never be written in one container operation -- a Tofino ALU writes whole containers -- and
  // bf-p4c could not slice the resulting supercluster at all.
  //
  // Coalesce a run of adjacent fields back into one naturally-aligned 2 or 4 byte field. The
  // merged field's expression is the concatenation of the parts, which is exactly what the
  // program computes when it reads them together, so that read now resolves to the single field;
  // each part stays reachable as a slice of it.
  //
  // Bit numbering flips here: klee keeps field 0 in the low bits, while a P4 header field is in
  // network order with field 0 at the top, so a part's slice bounds count down from the width.
  struct emitted_field_t {
    std::vector<klee::ref<klee::Expr>> parts;
    bits_t width;
  };

  std::vector<emitted_field_t> emitted;
  {
    // Merging alone is not enough, because a guessed field can straddle a boundary a read needs.
    // Measured on SmartCookie's TCP header, whose guess is [16][16][24][8][24][24][48]: the 32-bit
    // acknowledgement occupies bytes 8..11 but the guess puts a field at bytes 11..13, so no run of
    // whole fields sums to an aligned 4 bytes there and the header came out ...[24][24][48]. The
    // acknowledgement then had to be read as `data2 ++ data3[23:16]`, and bf-p4c rejects a concat
    // as an ALU operand ("read in a way too complex for the compiler to currently handle").
    //
    // So try whole-field merges first, exactly as before, and only when none fits allow the last
    // contributing field to be split, carrying its remainder forward. On that header this yields
    // [32][32][32][16][48] -- the layout the hand-fixed solution needed.
    std::vector<klee::ref<klee::Expr>> work(hdr_fields_guess.begin(), hdr_fields_guess.end());

    // A checksummed chunk's guessed fields are cut at the boundaries the deparser checksum needs
    // (checksum_boundaries), and no merge below crosses one.
    std::set<bytes_t> boundaries;
    if (const auto chunk_it = checksummed_chunks.find(chunk_key(node->get_hdr())); chunk_it != checksummed_chunks.end()) {
      boundaries = checksum_boundaries(chunk_it->second);
      bytes_t at = 0;
      for (size_t k = 0; k < work.size(); k++) {
        const bytes_t width = work[k]->getWidth() / 8;
        for (const bytes_t boundary : boundaries) {
          if (boundary > at && boundary < at + width) {
            klee::ref<klee::Expr> head;
            klee::ref<klee::Expr> tail;
            assert_or_panic(split_read(work[k], boundary - at, head, tail), "Cannot cut a guessed header field at a checksum boundary");
            work[k] = head;
            work.insert(work.begin() + k + 1, tail);
            break; // The tail is next.
          }
        }
        at += work[k]->getWidth() / 8;
      }
    }
    const auto crosses_boundary = [&boundaries](bytes_t from, bytes_t width) -> bool {
      for (const bytes_t boundary : boundaries) {
        if (boundary > from && boundary < from + width) {
          return true;
        }
      }
      return false;
    };

    bytes_t offset = 0;
    size_t i       = 0;
    while (i < work.size()) {
      size_t best_len   = 1;
      bits_t best_width = work[i]->getWidth();
      bool merged       = false;

      for (const bytes_t target : {4u, 2u}) {
        if (offset % target != 0 || crosses_boundary(offset, target)) {
          continue;
        }
        bits_t acc = 0;
        for (size_t j = i; j < work.size() && acc < target * 8; j++) {
          acc += work[j]->getWidth();
          if (acc == target * 8) {
            best_len   = j - i + 1;
            best_width = acc;
            merged     = true;
            break;
          }
        }
        if (merged) {
          break;
        }
      }

      for (const bytes_t target : {4u, 2u}) {
        if (merged || offset % target != 0 || crosses_boundary(offset, target)) {
          continue;
        }
        bits_t acc = 0;
        size_t j   = i;
        for (; j < work.size() && acc < target * 8; j++) {
          acc += work[j]->getWidth();
        }
        if (acc <= target * 8 || j == i) {
          continue;
        }
        // Never cut a guessed field that is already a width the target can hold. The guess comes
        // from where the program reads, so such a field is a protocol field: kvs's 12-byte payload
        // is guessed 1,4,4,1,2 -- an opcode then a key and a value -- and aligning to four bytes
        // from the start of the header put the key and the value across two fields each. Reading
        // one then needs a slice, and a stateful ALU cannot write its result to an unaligned one
        // ("ATTACHED_OUTPUT_ILLEGAL_ALIGNMENT"). Leaving them whole costs only that the fields
        // after the opcode are not four-byte aligned, which PHV allocation is free to do.
        const bits_t victim = work[j - 1]->getWidth();
        if (victim == 8 || victim == 16 || victim == 32) {
          continue;
        }

        // work[j - 1] overshoots: keep the bytes that complete this field and push the rest back.
        const bits_t overshoot = acc - target * 8;
        const bits_t keep      = victim - overshoot;
        if (keep == 0 || keep % 8 != 0) {
          continue;
        }
        klee::ref<klee::Expr> head;
        klee::ref<klee::Expr> tail;
        if (!split_read(work[j - 1], keep / 8, head, tail)) {
          continue;
        }
        work[j - 1] = head;
        work.insert(work.begin() + j, tail);
        best_len   = j - i;
        best_width = target * 8;
        merged     = true;
        break;
      }

      emitted_field_t field{{}, best_width};
      for (size_t j = i; j < i + best_len; j++) {
        field.parts.push_back(work[j]);
      }
      emitted.push_back(field);
      offset += best_width / 8;
      i += best_len;
    }
  }

  std::vector<var_t> hdr_data;
  hdr_fields_by_hdr[chunk_key(node->get_hdr())].clear();
  bytes_t hdr_offset = 0;
  for (const emitted_field_t &field : emitted) {
    const code_t field_name    = "hdr." + hdr_name + ".data" + std::to_string(hdr_data.size());
    klee::ref<klee::Expr> full = LibCore::concat_exprs(field.parts);
    const var_t var            = alloc_var(field_name, full, EXACT_NAME | HEADER_FIELD);
    hdr_data.push_back(var);
    hdr_fields_by_hdr[chunk_key(node->get_hdr())].push_back({hdr_offset, field.width, var.name});
    hdr_offset += field.width / 8;

    if (field.parts.size() == 1) {
      continue;
    }

    // Each part, as a slice of the field that now contains it.
    bits_t high = field.width;
    for (const klee::ref<klee::Expr> &part : field.parts) {
      const bits_t low        = high - part->getWidth();
      const code_t slice_name = field_name + "[" + std::to_string(high - 1) + ":" + std::to_string(low) + "]";
      alloc_var(slice_name, part, EXACT_NAME | HEADER_FIELD);
      high = low;
    }
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

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::ChecksumUpdate *node) {
  // The deparser of the gress the packet leaves from recomputes both checksums when the flag is
  // set (emit_deparser_checksums), and the flag is set where the packet leaves
  // (flag_pending_checksum), since that can be another gress or pass: SmartCookie's SYN-ACK is
  // checksummed in the egress and rewritten and forwarded on the recirculated pass in the ingress,
  // whose deparser has to do the update, after the rewrite. The L4 checksum covers the header
  // alone, the pseudo-header's length being the header's: right for a packet the data plane made
  // itself, such as that SYN-ACK, not for one carrying a payload, which would need the incremental
  // update from a parser residual.
  const auto site_it = checksum_headers_of.find(ep_node->get_id());
  if (site_it == checksum_headers_of.end()) {
    return EPVisitor::Action::doChildren; // Not a header the deparser can checksum (synthesize).
  }

  assert(ep_node->get_children().size() == 1 && "ChecksumUpdate must have 1 child");
  pending_checksum = site_it->second;
  visit(ep, ep_node->get_children()[0]);
  pending_checksum.reset();
  return EPVisitor::Action::skipChildren;
}

void TofinoSynthesizer::flag_pending_checksum() {
  if (!pending_checksum) {
    return;
  }

  coder_t &apply  = get(MARKER_INGRESS_CONTROL_APPLY);
  const code_t md = in_egress ? "eg_md." : "meta.";

  apply.indent();
  apply << md << "redo_checksum = 1;\n";
  apply.indent();
  apply << md << "l4_len = 16w" << checksummed_chunks.at(pending_checksum->l4_hdr).length << ";\n";

  std::optional<checksum_site_t> &site = in_egress ? egress_checksum_site : ingress_checksum_site;
  if (site) {
    assert_or_panic(site->ip_hdr == pending_checksum->ip_hdr && site->l4_hdr == pending_checksum->l4_hdr,
                    "Checksum updates of different headers in one gress");
  } else {
    site = pending_checksum;
  }
}

// The byte offsets inside a checksummed chunk at which one field has to end and another start:
// around the checksum, which the deparser writes whole, and in the IPv4 header around the
// protocol, which the L4 pseudo-header reads whole (the ttl before it then comes out whole too).
std::set<bytes_t> TofinoSynthesizer::checksum_boundaries(const checksummed_chunk_t &chunk) {
  if (chunk.is_ip) {
    return {8, 9, 10, 12};
  }
  switch (chunk.length) {
  case 20:
    return {16, 18}; // TCP
  case 8:
    return {6, 8}; // UDP
  }
  panic("Checksummed L4 header of %u bytes: neither TCP nor UDP", chunk.length);
}

void TofinoSynthesizer::emit_deparser_checksums(bool egress) {
  const std::optional<checksum_site_t> &site = egress ? egress_checksum_site : ingress_checksum_site;
  if (!site) {
    return;
  }

  const code_t md                       = egress ? "eg_md." : "meta.";
  const std::vector<hdr_field_t> &ip    = hdr_fields_by_hdr.at(site->ip_hdr);
  const std::vector<hdr_field_t> &l4    = hdr_fields_by_hdr.at(site->l4_hdr);
  const std::set<bytes_t> l4_boundaries = checksum_boundaries(checksummed_chunks.at(site->l4_hdr));
  const bytes_t l4_csum                 = *l4_boundaries.begin();

  // The fields covering the bytes [from, to), whole, in order.
  const auto fields_between = [](const std::vector<hdr_field_t> &fields, bytes_t from, bytes_t to) -> std::vector<code_t> {
    std::vector<code_t> names;
    bytes_t at = from;
    for (const hdr_field_t &field : fields) {
      if (field.offset == at && field.offset + field.width / 8 <= to) {
        names.push_back(field.name);
        at = field.offset + field.width / 8;
      }
    }
    assert_or_panic(at == to, "No whole header fields for bytes [%u, %u) of a checksummed header", from, to);
    return names;
  };
  const auto list = [](const std::vector<code_t> &names) -> code_t {
    code_t joined;
    for (const code_t &name : names) {
      joined += (joined.empty() ? "" : ", ") + name;
    }
    return joined;
  };
  const auto concat = [](std::vector<code_t> a, const std::vector<code_t> &b) {
    a.insert(a.end(), b.begin(), b.end());
    return a;
  };

  const std::vector<code_t> ip_csum       = fields_between(ip, 10, 12);
  const std::vector<code_t> l4_csum_field = fields_between(l4, l4_csum, l4_csum + 2);
  assert(ip_csum.size() == 1 && l4_csum_field.size() == 1 && "Checksum fields not whole");

  coder_t &metadata = code_template.get(egress ? MARKER_EGRESS_METADATA : MARKER_INGRESS_METADATA);
  metadata.indent();
  metadata << "bit<1> redo_checksum;\n";
  metadata.indent();
  metadata << "bit<16> l4_len;\n";

  coder_t &init = code_template.get(egress ? MARKER_EGRESS_PARSER_START : MARKER_INGRESS_APPLY_START);
  init.indent();
  init << md << "redo_checksum = 0;\n";

  coder_t &decl = code_template.get(egress ? MARKER_EGRESS_DEPARSER : MARKER_INGRESS_DEPARSER);
  decl.indent();
  decl << "Checksum() ipv4_checksum;\n";
  decl.indent();
  decl << "Checksum() l4_checksum;\n";

  coder_t &apply = code_template.get(egress ? MARKER_EGRESS_DEPARSER_APPLY : MARKER_INGRESS_DEPARSER_APPLY);
  apply.indent();
  apply << "if (" << md << "redo_checksum == 1) {\n";
  apply.inc();
  apply.indent();
  apply << ip_csum[0] << " = ipv4_checksum.update({" << list(concat(fields_between(ip, 0, 10), fields_between(ip, 12, 20))) << "});\n";
  apply.indent();
  apply << l4_csum_field[0] << " = l4_checksum.update({" << list(fields_between(ip, 12, 20)) << ", 8w0, " << list(fields_between(ip, 9, 10)) << ", "
        << md << "l4_len, " << list(concat(fields_between(l4, 0, l4_csum), fields_between(l4, l4_csum + 2, l4.back().offset + l4.back().width / 8)))
        << "});\n";
  apply.dec();
  apply.indent();
  apply << "}\n";
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::ModifyHeader *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  klee::ref<klee::Expr> hdr                  = node->get_hdr();
  const std::vector<expr_mod_t> &changes     = node->get_changes();
  const std::vector<expr_byte_swap_t> &swaps = node->get_swaps();

  std::unordered_set<bytes_t> bytes_already_dealt_with;
  std::vector<code_t> swap_assignments;

  // The swaps arrive as byte pairs. Emitting one `swap()` per byte forces byte-granular PHV
  // slicing on both fields, which drags their neighbours into the same container group until
  // bf-p4c cannot satisfy the action constraints. Consecutive pairs that together make up two
  // whole header fields are one field swap instead; the rest stay byte-wise.
  std::vector<expr_byte_swap_t> ordered(swaps.begin(), swaps.end());
  std::sort(ordered.begin(), ordered.end(), [](const expr_byte_swap_t &a, const expr_byte_swap_t &b) { return a.byte0 < b.byte0; });

  for (size_t i = 0; i < ordered.size();) {
    size_t run = 1;
    while (i + run < ordered.size() && ordered[i + run].byte0 == ordered[i].byte0 + run && ordered[i + run].byte1 == ordered[i].byte1 + run) {
      run++;
    }

    // Longest prefix of the run that names two whole fields.
    size_t width = run;
    for (; width > 1; width--) {
      const klee::ref<klee::Expr> a    = solver_toolbox.exprBuilder->Extract(hdr, ordered[i].byte0 * 8, width * 8);
      const klee::ref<klee::Expr> b    = solver_toolbox.exprBuilder->Extract(hdr, ordered[i].byte1 * 8, width * 8);
      const std::optional<var_t> a_var = ingress_vars.get_hdr(a);
      const std::optional<var_t> b_var = ingress_vars.get_hdr(b);
      if (!a_var || !b_var || a_var->name.find('[') != code_t::npos || b_var->name.find('[') != code_t::npos) {
        continue; // Not whole fields: a sliced name means we only caught part of one.
      }
      coder_t swap_assignment;
      swap_assignment << "swap" << (width * 8) << "(" << a_var->name << ", " << b_var->name << ");";
      swap_assignments.push_back(swap_assignment.dump());
      break;
    }

    // Both halves inside one field (swapping the two ports of a TCP header, say) is not a swap of
    // two fields but a rotation of one: writing it that way keeps the field whole.
    if (width == 1 && ordered[i].byte1 == ordered[i].byte0 + run) {
      const bits_t full                = 2 * run * 8;
      const klee::ref<klee::Expr> both = solver_toolbox.exprBuilder->Extract(hdr, ordered[i].byte0 * 8, full);
      const std::optional<var_t> f     = ingress_vars.get_hdr(both);
      if (f && f->name.find('[') == code_t::npos) {
        coder_t swap_assignment;
        swap_assignment << f->name << " = " << f->name << "[" << (full / 2 - 1) << ":0] ++ " << f->name << "[" << (full - 1) << ":" << (full / 2)
                        << "];";
        swap_assignments.push_back(swap_assignment.dump());
        width = run;
      }
    }

    if (width == 1) {
      for (size_t k = 0; k < run; k++) {
        const klee::ref<klee::Expr> byte0_expr = solver_toolbox.exprBuilder->Extract(hdr, ordered[i + k].byte0 * 8, 8);
        const klee::ref<klee::Expr> byte1_expr = solver_toolbox.exprBuilder->Extract(hdr, ordered[i + k].byte1 * 8, 8);
        const std::optional<var_t> byte0_var   = ingress_vars.get_hdr(byte0_expr);
        const std::optional<var_t> byte1_var   = ingress_vars.get_hdr(byte1_expr);
        assert(byte0_var.has_value() && "Byte0 not found");
        assert(byte1_var.has_value() && "Byte1 not found");
        coder_t swap_assignment;
        swap_assignment << "swap(" << byte0_var->name << ", " << byte1_var->name << ");";
        swap_assignments.push_back(swap_assignment.dump());
      }
      width = run;
    }

    for (size_t k = 0; k < width; k++) {
      bytes_already_dealt_with.insert(ordered[i + k].byte0);
      bytes_already_dealt_with.insert(ordered[i + k].byte1);
    }
    i += width;
  }

  const code_t swap_action_name = "swap_action_" + std::to_string(node->get_node()->get_id());
  transpile_action_decl(swap_action_name, swap_assignments);

  ingress_apply.indent();
  ingress_apply << swap_action_name << "();\n";

  // Materialize computed values written to the header (e.g. the LC-corrected estimate
  // `lc_offset - ln`) into metadata vars first, so the byte-level field assignments
  // below reference a bound variable instead of trying to bit-slice an un-materialized
  // arithmetic expression. Values that are already bound vars (e.g. a divide quotient)
  // are skipped and coalesced directly by the loop.
  std::unordered_set<std::string> materialized_bases;
  for (const expr_mod_t &mod : changes) {
    if (mod.expr->getKind() != klee::Expr::Extract || ingress_vars.get(mod.expr)) {
      continue;
    }
    klee::ref<klee::Expr> base = mod.expr->getKid(0);
    if (ingress_vars.get(base)) {
      continue;
    }
    switch (base->getKind()) {
    case klee::Expr::Add:
    case klee::Expr::Sub:
    case klee::Expr::Mul:
    case klee::Expr::UDiv:
    case klee::Expr::Shl:
    case klee::Expr::LShr:
    case klee::Expr::And:
    case klee::Expr::Or:
    case klee::Expr::Xor:
      break;
    default:
      continue;
    }
    if (!materialized_bases.insert(expr_to_string(base)).second) {
      continue;
    }
    klee::ref<klee::Expr> emitted           = base;
    const klee::ref<klee::Expr> narrowed_op = narrow_widened_bitop(base);
    if (!narrowed_op.isNull()) {
      emitted = narrowed_op;
    }

    const code_t base_code = transpiler.transpile(emitted);
    const var_t base_var   = alloc_var("hdr_val", emitted, IS_INGRESS_METADATA);
    declare_var_in_ingress_metadata(base_var);
    // In an action of its own: for a bare assignment in the apply block bf-p4c synthesizes the
    // action itself and then refuses an add of a wide constant in it ("multiple action data
    // parameters", the cookie check's ack - 1 in the egress); the same statement in a named
    // action compiles (tofino/exp-compute/addc2.p4).
    {
      const code_t action_name = base_var.name.substr(base_var.name.rfind('.') + 1) + "_calc";
      coder_t &control         = get(MARKER_INGRESS_CONTROL);
      // A value that reads a slot of a hash chain is computed by the hash unit, as the ground
      // truth computes its cookie (`data2 = ctime ^ v0 ^ ...` in @in_hash): an ALU op would tie
      // this metadata field to the chain's sliced cluster. The hash unit takes an xor or a
      // layout of fields; an add of a slot stays an ALU op.
      const std::optional<std::string> op = LibBDD::unrolled_op_name(emitted);
      const bool via_hash                 = base_code.find("hdr.st.") != std::string::npos && (!op || *op == "op_xor");
      control.indent();
      control << "action " << action_name << "() { " << (via_hash ? "@in_hash { " : "") << base_var.name << " = " << base_code << ";"
              << (via_hash ? " }" : "") << " }\n";
      control << "\n";
      ingress_apply.indent();
      ingress_apply << action_name << "();\n";
    }

    // The field assembly below looks the value up by the low bytes of the original wide
    // expression, so give those the narrowed variable's name too.
    if (!narrowed_op.isNull()) {
      // EXACT_NAME only: base_var.name already carries the metadata prefix, and IS_INGRESS_METADATA
      // would prepend a second one.
      const klee::ref<klee::Expr> low_bytes = LibCore::solver_toolbox.exprBuilder->Extract(base, 0, emitted->getWidth());
      alloc_var(base_var.name, low_bytes, EXACT_NAME);
    }
  }

  std::vector<code_t> assignments;

  // Bytes that together fill a whole header field are emitted as one assignment. Writing a field
  // byte by byte makes bf-p4c slice its container, and that slicing then propagates into
  // everything computed from the field; on one NAT solution it crashed the compiler outright
  // (tofino/exp-compute/README.md), and on SmartCookie it left the ALU unable to write the field
  // at all, since an ALU writes whole containers. A field's klee offset 0 is its most significant
  // byte, so an all-constant field folds to a literal and any other one to the bytes concatenated
  // in klee order.
  for (const bits_t field_width : {64u, 48u, 40u, 32u, 24u, 16u}) {
    const bytes_t field_bytes = field_width / 8;

    for (const expr_mod_t &mod : changes) {
      if (mod.width != 8 || bytes_already_dealt_with.contains(mod.offset / 8)) {
        continue;
      }

      for (bits_t first = 0; first < field_width; first += 8) {
        if (first > mod.offset) {
          break;
        }
        const bits_t field_offset = mod.offset - first;
        if (field_offset + field_width > hdr->getWidth()) {
          continue;
        }

        const std::optional<var_t> field = ingress_vars.get_hdr(solver_toolbox.exprBuilder->Extract(hdr, field_offset, field_width));
        if (!field || field->size != field_width || field->is_slice()) {
          continue;
        }

        // Every byte of the field must be written here, and none of them already handled.
        std::vector<const expr_mod_t *> field_bytes_written(field_bytes, nullptr);
        for (const expr_mod_t &candidate : changes) {
          if (candidate.width != 8 || candidate.offset < field_offset || candidate.offset >= field_offset + field_width) {
            continue;
          }
          field_bytes_written[(candidate.offset - field_offset) / 8] = &candidate;
        }

        // A field written only in part is still emitted whole, reading its own untouched bytes
        // back: an ALU writes whole containers, so there is no way to write just a slice.
        bool any_written = false;
        bool usable      = true;
        for (const expr_mod_t *byte : field_bytes_written) {
          if (!byte) {
            continue;
          }
          any_written = true;
          usable &= !bytes_already_dealt_with.contains(byte->offset / 8);
        }
        if (!any_written || !usable) {
          continue;
        }

        bool all_constant = true;
        for (const expr_mod_t *byte : field_bytes_written) {
          all_constant &= byte != nullptr && is_constant(byte->expr);
        }

        // Byte at klee offset p sits at the field's bits [width-1-p : width-8-p], so offset 0 is
        // the field's most significant byte and the bytes concatenate in their klee order.
        // When every byte is written and they reassemble a value we already hold, write that value
        // rather than a concatenation of its own bytes: the concat is the identity, but bf-p4c
        // reads it as one PHV source per byte and rejects the action -- measured on SmartCookie as
        // `hdr.hdr2.data1 = meta.hdr_val0[31:24] ++ ... ++ meta.hdr_val0[7:0]`.
        std::optional<var_t> whole_var;
        if (!all_constant) {
          bool every_byte_written = true;
          for (bytes_t b = 0; b < field_bytes; b++) {
            every_byte_written &= field_bytes_written[b] != nullptr;
          }
          if (every_byte_written) {
            std::vector<klee::ref<klee::Expr>> parts;
            for (bytes_t b = 0; b < field_bytes; b++) {
              parts.push_back(field_bytes_written[b]->expr);
            }
            // parts[0] is the field's most significant byte, which concat_exprs puts at the high
            // end only with left_to_right = false.
            whole_var = ingress_vars.get(LibCore::concat_exprs(parts, false));
          }
        }

        // A packet header field is deparsed and `exact_containers`, so it cannot be split; copying
        // a state word into one needs a PHV source per slice the chain cut it into, against a
        // limit of two. Route it through the hash unit, as the ground truth does
        // (`@in_hash { hdr.hdr2.data2 = ctime ^ v0 ^ v1 ^ v2 ^ v3; }`). A metadata temporary or
        // a carried field is whole, and a plain move of it is an ALU move.
        const bool via_hash = whole_var.has_value() && whole_var->name.rfind("hdr.st.", 0) == 0;

        // Byte at klee offset p sits at the field's bits [width-1-p : width-8-p], so offset 0 is
        // the field's most significant byte and the bytes concatenate in their klee order.
        coder_t assignment;
        if (via_hash) {
          assignment << "@in_hash { ";
        }
        assignment << field->name;
        assignment << " = ";

        if (all_constant) {
          u64 value = 0;
          for (bytes_t b = 0; b < field_bytes; b++) {
            value |= solver_toolbox.value_from_expr(field_bytes_written[b]->expr) << (field_width - 8 - b * 8);
          }
          std::stringstream literal;
          literal << std::to_string(field_width) << "w0x" << std::hex << std::setfill('0') << std::setw(field_width / 4) << value;
          assignment << literal.str();
        } else {
          if (whole_var.has_value()) {
            assignment << whole_var->name;
          } else {
            for (bytes_t b = 0; b < field_bytes; b++) {
              assignment << (b > 0 ? " ++ " : "");
              if (field_bytes_written[b]) {
                assignment << transpiler.transpile(field_bytes_written[b]->expr);
              } else {
                const bits_t high = field_width - 1 - b * 8;
                assignment << field->name << "[" << high << ":" << (high - 7) << "]";
              }
            }
          }
        }

        assignment << ";";
        if (via_hash) {
          assignment << " }";
        }
        assignments.push_back(assignment.dump());

        for (const expr_mod_t *byte : field_bytes_written) {
          if (byte) {
            bytes_already_dealt_with.insert(byte->offset / 8);
          }
        }
        break;
      }
    }
  }

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

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::MapSetTableLookup *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const DS_ID map_set_table_id                   = node->get_id();
  const std::vector<klee::ref<klee::Expr>> &keys = node->get_keys();
  const std::optional<symbol_t> hit              = node->get_hit();

  const MapSetTable *map_set_table = get_tofino_ds<MapSetTable>(ep, map_set_table_id);

  const bdd_node_id_t node_id = node->get_node()->get_id();
  const Table *table          = map_set_table->get_table(node_id);
  assert(table && "Table not found");

  std::vector<var_t> keys_vars;
  transpile_table_decl(table, keys, {}, false, keys_vars);
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
  transpile_register_action_decl(&guard, guard_allow_read_action_name, action_type);

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

  const bdd_node_id_t node_id = node->get_node()->get_id();
  const Table *table          = vector_table->get_table(node_id);
  assert(table && "Table not found");

  const code_t transpiled_key = transpiler.transpile(key);

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

  const code_t transpiled_key = transpiler.transpile(key);

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
  coder_t &ingress = get(MARKER_INGRESS_CONTROL);

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
    transpile_register_action_decl(reg, action_name, RegisterActionType::Read);

    const klee::ref<klee::Expr> entry_expr = solver_toolbox.exprBuilder->Extract(value, offset, reg->value_size);

    // Bind the read value to ingress metadata rather than an apply-local: the value
    // may be consumed on a different branch than the read (e.g. a count read once and
    // tested in gateways on multiple paths), and a local would be out of scope there.
    const std::string value_prefix_name = "vector_reg_value";
    const var_t value_var               = alloc_var(value_prefix_name, entry_expr, IS_INGRESS_METADATA);
    declare_var_in_ingress_metadata(value_var);

    ingress << "\n";
    emit_register_execute(value_var.name, action_name, index, transpiler.transpile(index), ep_node);

    offset += reg->value_size;
    i++;
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterUpdate *node) {
  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const DS_ID id                          = node->get_id();
  const klee::ref<klee::Expr> index       = node->get_index();
  const klee::ref<klee::Expr> value       = node->get_read_value();
  const klee::ref<klee::Expr> write_value = node->get_write_value();

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

  // `*reg += delta` (increment by an expression that doesn't read the register's
  // own value): materialize the delta in a regular action, then let the stateful
  // ALU add it. Bind the register's new value so downstream nodes can use it.
  std::optional<klee::ref<klee::Expr>> increment_delta = TofinoModuleFactory::get_register_increment_delta(write_value, value);
  if (increment_delta.has_value() && regs.size() == 1) {
    const Register *reg = regs[0];

    const code_t delta_code = transpiler.transpile(*increment_delta);
    const var_t delta_var   = alloc_var("reg_incr", *increment_delta, IS_INGRESS_METADATA);
    declare_var_in_ingress_metadata(delta_var);

    ingress_apply.indent();
    ingress_apply << delta_var.name << " = " << delta_code << ";\n";

    const code_t action_name = build_register_action_name(reg, RegisterActionType::AddValue, ep_node);
    transpile_register_action_decl(reg, action_name, RegisterActionType::AddValue,
                                   register_action_extras_t{
                                       .external_var             = delta_var.name,
                                       .extra_constant           = {},
                                       .extra_condition          = {},
                                       .write_value              = {},
                                       .temporary_transpilations = {},
                                   });

    const var_t new_value_var = alloc_var("reg_new", write_value);
    new_value_var.declare(ingress_apply, action_name + ".execute(" + transpiler.transpile(index) + ")");

    return EPVisitor::Action::doChildren;
  }

  bits_t offset = 0;
  for (const Register *reg : regs) {
    const klee::ref<klee::Expr> reg_write_expr = solver_toolbox.exprBuilder->Extract(write_value, offset, reg->value_size);
    std::optional<var_t> reg_write_var         = ingress_vars.get(reg_write_expr);

    if (!reg_write_var) {
      const code_t write_expr       = transpiler.transpile(reg_write_expr);
      const var_t new_reg_write_var = alloc_var("reg_write", reg_write_expr, IS_INGRESS_METADATA);
      declare_var_in_ingress_metadata(new_reg_write_var);

      ingress_apply.indent();
      ingress_apply << new_reg_write_var.name << " = " << write_expr << ";\n";

      reg_write_var = new_reg_write_var;
    }

    const code_t action_name = build_register_action_name(reg, RegisterActionType::Write, ep_node);
    transpile_register_action_decl(reg, action_name, RegisterActionType::Write,
                                   register_action_extras_t{
                                       .external_var             = reg_write_var->name,
                                       .extra_constant           = {},
                                       .extra_condition          = {},
                                       .write_value              = {},
                                       .temporary_transpilations = {},
                                   });

    emit_register_execute("", action_name, index, transpiler.transpile(index), ep_node);

    offset += reg->value_size;
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterReadConditionalUpdate *node) {
  // Actually nothing to do here, this module creates other modules.
  panic("VectorRegisterReadConditionalUpdate should not be visited directly");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterReadConditionalUpdateSingleAction *node) {
  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const DS_ID id                    = node->get_id();
  klee::ref<klee::Expr> index       = node->get_index();
  klee::ref<klee::Expr> value       = node->get_read_value();
  klee::ref<klee::Expr> write_value = node->get_write_value();
  klee::ref<klee::Expr> condition   = node->get_condition();

  const VectorRegister *vector_register = get_tofino_ds<VectorRegister>(ep, id);

  std::vector<const Register *> regs;
  for (const Register &reg : vector_register->regs) {
    regs.push_back(&reg);
  }

  std::sort(regs.begin(), regs.end(), [](const Register *r0, const Register *r1) { return natural_compare(r0->id, r1->id); });

  // Max-swap that returns the displaced value (the shadow): keep the larger in the
  // register, return min(value, write). The register action does the compare and
  // conditional write internally, so there's no separate branch (already collapsed).
  if (node->get_returns_shadow()) {
    const Register *reg = regs.front();
    transpile_register_decl(reg);
    ingress << "\n";

    const klee::ref<klee::Expr> keep_larger = solver_toolbox.exprBuilder->Ult(value, write_value);
    const code_t action_name                = build_register_action_name(reg, RegisterActionType::ReadConditionalWriteReturnOther, ep_node);
    transpile_register_action_decl(reg, action_name, RegisterActionType::ReadConditionalWriteReturnOther,
                                   register_action_extras_t{
                                       .external_var             = {},
                                       .extra_constant           = {},
                                       .extra_condition          = keep_larger,
                                       .write_value              = write_value,
                                       .temporary_transpilations = {{value, "value"}},
                                   });

    // The register returns min(value, write) = the shadow. The NF exposes that shadow
    // as the return symbol of a min() call (which process_node consumed), so bind the
    // register output to that symbol; downstream uses then resolve to this register.
    klee::ref<klee::Expr> shadow_expr = node->get_shadow_symbol();
    if (shadow_expr.isNull()) {
      shadow_expr = solver_toolbox.exprBuilder->Select(solver_toolbox.exprBuilder->Ult(value, write_value), value, write_value);
    }
    // Bind the shadow to an ingress-metadata field (not an apply-local): downstream
    // consumers can be later register actions whose apply() bodies may reference
    // `meta.*` but not a control-apply local, and metadata sidesteps use-before-decl.
    const var_t shadow_var = alloc_var("vector_reg_shadow", shadow_expr, IS_INGRESS_METADATA);
    declare_var_in_ingress_metadata(shadow_var);

    // A register .execute() needs a simple index field of exactly the register's
    // addressing width, not an inline expression (e.g. `hash >> 26`) nor a wider field.
    // Materialize the index into a narrow ingress-metadata field, truncating to width.
    const bits_t index_bits     = register_index_bits(reg->capacity);
    const code_t index_type_str = TofinoSynthesizer::Transpiler::type_from_size(index_bits);
    const code_t index_code     = transpiler.transpile(index);
    const var_t index_var       = alloc_var("vector_reg_index", index_bits, IS_INGRESS_METADATA);
    declare_var_in_ingress_metadata(index_var);
    ingress_apply.indent();
    ingress_apply << index_var.name << " = (" << index_type_str << ")(" << index_code << ");\n";

    emit_register_execute(shadow_var.name, action_name, index, index_var.name, ep_node);

    return EPVisitor::Action::doChildren;
  }

  bits_t offset = 0;
  std::vector<var_t> value_vars;
  for (const Register *reg : regs) {
    const klee::ref<klee::Expr> entry_expr = solver_toolbox.exprBuilder->Extract(value, offset, reg->value_size);
    const std::string value_prefix_name    = "vector_reg_value";
    const var_t value_var                  = alloc_var(value_prefix_name, entry_expr);
    value_vars.push_back(value_var);
    offset += reg->value_size;
  }

  for (const Register *reg : regs) {
    transpile_register_decl(reg);
  }

  if (!regs.empty()) {
    ingress << "\n";
  }

  int i = 0;
  for (const Register *reg : regs) {
    const code_t action_name = build_register_action_name(reg, RegisterActionType::ReadConditionalWrite, ep_node);
    transpile_register_action_decl(reg, action_name, RegisterActionType::ReadConditionalWrite,
                                   register_action_extras_t{
                                       .external_var             = {},
                                       .extra_constant           = {},
                                       .extra_condition          = condition,
                                       .write_value              = write_value,
                                       .temporary_transpilations = {{value, "value"}},
                                   });

    const code_t assignment = action_name + ".execute(" + transpiler.transpile(index) + ")";
    const var_t value_var   = value_vars[i];

    ingress << "\n";
    value_var.declare(ingress_apply, assignment);

    i++;
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterReadConditionalIncrement *node) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL);

  const DS_ID id                    = node->get_id();
  klee::ref<klee::Expr> index       = node->get_index();
  klee::ref<klee::Expr> value       = node->get_read_value();
  klee::ref<klee::Expr> write_value = node->get_write_value();
  klee::ref<klee::Expr> condition   = node->get_condition();

  const VectorRegister *vector_register = get_tofino_ds<VectorRegister>(ep, id);

  std::vector<const Register *> regs;
  for (const Register &reg : vector_register->regs) {
    regs.push_back(&reg);
  }
  assert(regs.size() == 1 && "conditional-increment counter must be a single-field register");
  const Register *reg = regs.front();

  transpile_register_decl(reg);
  ingress << "\n";

  // Single register action: return the OLD value, then conditionally increment it.
  //   apply(inout value, out out_value) { out_value = value; if (cond) value = value + delta; }
  const code_t action_name = build_register_action_name(reg, RegisterActionType::ReadConditionalWrite, ep_node);
  transpile_register_action_decl(reg, action_name, RegisterActionType::ReadConditionalWrite,
                                 register_action_extras_t{
                                     .external_var             = {},
                                     .extra_constant           = {},
                                     .extra_condition          = condition,
                                     .write_value              = write_value,
                                     .temporary_transpilations = {{value, "value"}},
                                 });

  // Bind the returned OLD value to ingress metadata so both branch continuations can
  // use it (post-op count is `old` on the no-increment side, `old + delta` on the other).
  const var_t value_var = alloc_var("vector_reg_value", value, IS_INGRESS_METADATA);
  declare_var_in_ingress_metadata(value_var);

  emit_register_execute(value_var.name, action_name, index, transpiler.transpile(index), ep_node);

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
  const DS_ID fcfs_ct_id                         = node->get_fcfs_ct_id();
  const std::vector<klee::ref<klee::Expr>> &keys = node->get_keys();
  const klee::ref<klee::Expr> value              = node->get_value();
  const std::optional<symbol_t> hit              = node->get_map_has_this_key();

  const FCFSCachedTable *fcfs_ct = get_tofino_ds<FCFSCachedTable>(ep, fcfs_ct_id);
  const bdd_node_id_t node_id    = node->get_node()->get_id();

  const fcfs_ct_internals_t fcfs_ct_internals = fcfs_ct_get_internals(fcfs_ct);

  transpile_fcfs_ct_decl(fcfs_ct, ep_node);

  const Table *table = fcfs_ct->get_table(node_id);
  assert(table && "Table not found");
  transpile_table_decl(table, fcfs_ct_internals.keys, {value}, false);

  const std::optional<var_t> value_var = ingress_vars.get(value);
  assert_or_panic(value_var.has_value(), "Value variable from table not found");

  const Hash *hash = fcfs_ct->get_hash(node_id);
  assert(hash && "Hash not found");

  std::vector<code_t> hash_inputs;
  for (const var_t &key_var : fcfs_ct_internals.keys) {
    hash_inputs.push_back(key_var.name);
  }

  code_t hash_calculator;
  code_t hash_value;
  transpile_fcfs_ct_hash_calculation(hash, hash_inputs, value_var.value(), hash_calculator, hash_value);

  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  for (size_t i = 0; i < keys.size(); i++) {
    const klee::ref<klee::Expr> key_expr = keys[i];
    const var_t &key_var                 = fcfs_ct_internals.keys[i];
    ingress_apply.indent();
    ingress_apply << key_var.name << " = " << transpiler.transpile(key_expr) << ";\n";
  }

  const var_t hit_var = hit.has_value() ? alloc_var("hit", hit->expr, FORCE_BOOL) : alloc_var("hit", 32, EXACT_NAME | FORCE_BOOL);
  hit_var.declare(ingress_apply, table->id + ".apply().hit");

  ingress_apply.indent();
  ingress_apply << hash_calculator << "();\n";

  const code_t is_alive_var_name = create_unique_name("fcfs_ct_is_alive");
  ingress_apply.indent();
  ingress_apply << "bool " << is_alive_var_name << " = ";
  ingress_apply << fcfs_ct_internals.liveness_query << ".execute(" << value_var->name << ");\n";

  ingress_apply.indent();
  ingress_apply << "if (";
  ingress_apply << "!" << hit_var.name;
  ingress_apply << " && ";
  ingress_apply << is_alive_var_name;
  ingress_apply << ") {\n";
  ingress_apply.inc();

  const code_t match_counter_var_name = create_unique_name("match_counter");

  ingress.indent();
  ingress << "bit<8> " << match_counter_var_name << " = 0;\n";

  for (size_t i = 0; i < fcfs_ct->cache_keys.size(); i++) {
    coder_t action_body;
    action_body.indent();
    action_body << match_counter_var_name << " = " << match_counter_var_name << " + "
                << fcfs_ct_internals.keys_reg_actions.at({fcfs_ct->cache_keys[i].id, RegisterActionType::CheckValue}) << ".execute(" << hash_value
                << ");\n";

    const code_t action_name = fcfs_ct_id + "_check_key_" + std::to_string(i) + "_" + std::to_string(node_id);
    transpile_action_decl(action_name, action_body.split_lines());

    ingress_apply.indent();
    ingress_apply << action_name << "();\n";
  }

  ingress_apply.indent();
  ingress_apply << "if (" << match_counter_var_name << " == " << fcfs_ct->cache_keys.size() << ") {\n";
  ingress_apply.inc();

  ingress_apply.indent();
  ingress_apply << hit_var.name << " = true;\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableReadInsert *node) {
  const DS_ID fcfs_ct_id                         = node->get_fcfs_ct_id();
  const std::vector<klee::ref<klee::Expr>> &keys = node->get_keys();
  const klee::ref<klee::Expr> read_value         = node->get_read_value();
  const klee::ref<klee::Expr> write_value        = node->get_write_value();
  const std::optional<symbol_t> hit              = node->get_map_has_this_key();
  const symbol_t &cached_insert_success          = node->get_cached_insert_success();

  const FCFSCachedTable *fcfs_ct = get_tofino_ds<FCFSCachedTable>(ep, fcfs_ct_id);
  const bdd_node_id_t node_id    = node->get_node()->get_id();

  const fcfs_ct_internals_t fcfs_ct_internals = fcfs_ct_get_internals(fcfs_ct);

  transpile_fcfs_ct_decl(fcfs_ct, ep_node);

  const Table *table = fcfs_ct->get_table(node_id);
  assert(table && "Table not found");
  transpile_table_decl(table, fcfs_ct_internals.keys, {read_value}, false);

  const std::optional<var_t> value_var = ingress_vars.get(read_value);
  assert_or_panic(value_var.has_value(), "Value variable from table not found");

  const Hash *hash = fcfs_ct->get_hash(node_id);
  assert(hash && "Hash not found");

  std::vector<code_t> hash_inputs;
  for (const var_t &key_var : fcfs_ct_internals.keys) {
    hash_inputs.push_back(key_var.name);
  }

  code_t hash_calculator;
  code_t hash_value;
  transpile_fcfs_ct_hash_calculation(hash, hash_inputs, value_var.value(), hash_calculator, hash_value);

  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  for (size_t i = 0; i < keys.size(); i++) {
    const klee::ref<klee::Expr> key_expr = keys[i];
    const var_t &key_var                 = fcfs_ct_internals.keys[i];
    ingress_apply.indent();
    ingress_apply << key_var.name << " = " << transpiler.transpile(key_expr) << ";\n";
  }

  const var_t hit_var = hit.has_value() ? alloc_var("hit", hit->expr, FORCE_BOOL) : alloc_var("hit", 32, EXACT_NAME | FORCE_BOOL);
  hit_var.declare(ingress_apply, table->id + ".apply().hit");

  const var_t cached_insert_success_var = alloc_var("cached_insert_success", cached_insert_success.expr);
  cached_insert_success_var.declare(ingress_apply, "0");

  ingress_apply.indent();
  ingress_apply << "if (!" << hit_var.name << ") {\n";
  ingress_apply.inc();

  ingress_apply.indent();
  ingress_apply << hash_calculator << "();\n";

  const code_t is_alive_var_name = create_unique_name("fcfs_ct_is_alive");
  ingress_apply.indent();
  ingress_apply << "bool " << is_alive_var_name << " = ";
  ingress_apply << fcfs_ct_internals.liveness_query_and_refresh << ".execute(" << value_var->name << ");\n";

  ingress_apply.indent();
  ingress_apply << "if (" << is_alive_var_name << ") {\n";
  ingress_apply.inc();

  const code_t match_counter_var_name = create_unique_name("match_counter");

  ingress.indent();
  ingress << "bit<8> " << match_counter_var_name << " = 0;\n";

  for (size_t i = 0; i < fcfs_ct->cache_keys.size(); i++) {
    coder_t action_body;
    action_body.indent();
    action_body << match_counter_var_name << " = " << match_counter_var_name << " + "
                << fcfs_ct_internals.keys_reg_actions.at({fcfs_ct->cache_keys[i].id, RegisterActionType::CheckValue}) << ".execute(" << hash_value
                << ");\n";

    const code_t action_name = fcfs_ct_id + "_check_key_" + std::to_string(i) + "_" + std::to_string(node_id);
    transpile_action_decl(action_name, action_body.split_lines());

    ingress_apply.indent();
    ingress_apply << action_name << "();\n";
  }

  ingress_apply.indent();
  ingress_apply << "if (" << match_counter_var_name << " == " << fcfs_ct->cache_keys.size() << ") {\n";
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

  for (const Register &reg_key : fcfs_ct->cache_keys) {
    ingress_apply.indent();
    ingress_apply << fcfs_ct_internals.keys_reg_actions.at({reg_key.id, RegisterActionType::Write}) << ".execute(" << hash_value << ");\n";
  }

  // HACK
  var_t value_written_var = *value_var;
  value_written_var.expr  = write_value;
  ingress_vars.insert_back(value_written_var, true);

  ingress_apply.indent();
  ingress_apply << cached_insert_success_var.name << " = 1;\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableInsert *node) {
  const DS_ID fcfs_ct_id                         = node->get_fcfs_ct_id();
  const std::vector<klee::ref<klee::Expr>> &keys = node->get_keys();
  const klee::ref<klee::Expr> value              = node->get_value();
  const symbol_t &cached_insert_success          = node->get_success();

  const FCFSCachedTable *fcfs_ct = get_tofino_ds<FCFSCachedTable>(ep, fcfs_ct_id);
  const bdd_node_id_t node_id    = node->get_node()->get_id();

  const fcfs_ct_internals_t fcfs_ct_internals = fcfs_ct_get_internals(fcfs_ct);

  transpile_fcfs_ct_decl(fcfs_ct, ep_node);

  const Hash *hash = fcfs_ct->get_hash(node_id);
  assert(hash && "Hash not found");

  std::vector<code_t> hash_inputs;
  for (const var_t &key_var : fcfs_ct_internals.keys) {
    hash_inputs.push_back(key_var.name);
  }

  coder_t &ingress = get(MARKER_INGRESS_CONTROL);

  const var_t value_var = alloc_var("fcfs_ct_value", value);
  value_var.declare(ingress, "0");

  code_t hash_calculator;
  code_t hash_value;
  transpile_fcfs_ct_hash_calculation(hash, hash_inputs, value_var, hash_calculator, hash_value);

  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  for (size_t i = 0; i < keys.size(); i++) {
    const klee::ref<klee::Expr> key_expr = keys[i];
    const var_t &key_var                 = fcfs_ct_internals.keys[i];
    ingress_apply.indent();
    ingress_apply << key_var.name << " = " << transpiler.transpile(key_expr) << ";\n";
  }

  const var_t cached_insert_success_var = alloc_var("cached_insert_success", cached_insert_success.expr);
  cached_insert_success_var.declare(ingress_apply, "0");

  ingress_apply.indent();
  ingress_apply << hash_calculator << "();\n";

  const code_t is_alive_var_name = create_unique_name("fcfs_ct_is_alive");
  ingress_apply.indent();
  ingress_apply << "bool " << is_alive_var_name << " = ";
  ingress_apply << fcfs_ct_internals.liveness_query_and_refresh << ".execute((bit<32>)" << hash_value << ");\n";

  ingress_apply.indent();
  ingress_apply << "if (!" << is_alive_var_name << ") {\n";
  ingress_apply.inc();

  for (const Register &reg_key : fcfs_ct->cache_keys) {
    ingress_apply.indent();
    ingress_apply << fcfs_ct_internals.keys_reg_actions.at({reg_key.id, RegisterActionType::Write}) << ".execute(" << hash_value << ");\n";
  }

  ingress_apply.indent();
  ingress_apply << cached_insert_success_var.name << " = 1;\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableIsIndexAllocated *node) {
  const DS_ID fcfs_ct_id            = node->get_fcfs_ct_id();
  const klee::ref<klee::Expr> index = node->get_index();
  const symbol_t &is_allocated      = node->get_is_allocated();

  const FCFSCachedTable *fcfs_ct = get_tofino_ds<FCFSCachedTable>(ep, fcfs_ct_id);

  const fcfs_ct_internals_t fcfs_ct_internals = fcfs_ct_get_internals(fcfs_ct);

  transpile_fcfs_ct_decl(fcfs_ct, ep_node);

  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const code_t index_pre_transpilatation = transpiler.transpile(index);
  const var_t value_var                  = alloc_var("index", index);
  value_var.declare(ingress_apply, index_pre_transpilatation);

  const var_t is_allocated_var = alloc_var("is_allocated", is_allocated.expr);
  is_allocated_var.declare(ingress_apply, "0");

  ingress_apply.indent();
  ingress_apply << "if(" << fcfs_ct_internals.liveness_query + ".execute(" + value_var.name + ")"
                << ") {\n";
  ingress_apply.inc();

  ingress_apply.indent();
  ingress_apply << is_allocated_var.name << " = 1;\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  return EPVisitor::Action::doChildren;
}

TofinoSynthesizer::fcfs_cs_internals_t TofinoSynthesizer::fcfs_cs_get_internals(const FCFSCachedSet *fcfs_cs) {
  fcfs_cs_internals_t internals;

  internals.liveness_query             = build_register_action_name(&fcfs_cs->reg_liveness, RegisterActionType::QueryTimestamp);
  internals.liveness_query_and_refresh = build_register_action_name(&fcfs_cs->reg_liveness, RegisterActionType::QueryAndRefreshTimestamp);

  for (size_t i = 0; i < fcfs_cs->keys_sizes.size(); i++) {
    const code_t key_name = fcfs_cs->id + "_key_" + std::to_string(fcfs_cs->keys_sizes[i]) + "b_" + std::to_string(i);
    if (std::optional<var_t> key_var = ingress_vars.get(key_name)) {
      internals.keys.push_back(key_var.value());
      continue;
    }
    const var_t key_var = alloc_var(key_name, fcfs_cs->keys_sizes.at(i), EXACT_NAME | SKIP_STACK_ALLOC | IS_INGRESS_METADATA);
    declare_var_in_ingress_metadata(key_var);
    internals.keys.push_back(key_var);
  }

  for (const Register &reg : fcfs_cs->cache_keys) {
    for (const RegisterActionType &action_type : reg.actions) {
      const code_t action_name                          = build_register_action_name(&reg, action_type);
      internals.keys_reg_actions[{reg.id, action_type}] = action_name;
    }
  }

  return internals;
}

TofinoSynthesizer::fcfs_ct_internals_t TofinoSynthesizer::fcfs_ct_get_internals(const FCFSCachedTable *fcfs_ct) {
  fcfs_ct_internals_t internals;

  internals.liveness_query             = build_register_action_name(&fcfs_ct->reg_liveness, RegisterActionType::QueryTimestamp);
  internals.liveness_query_and_refresh = build_register_action_name(&fcfs_ct->reg_liveness, RegisterActionType::QueryAndRefreshTimestamp);

  for (size_t i = 0; i < fcfs_ct->keys_sizes.size(); i++) {
    const code_t key_name = fcfs_ct->id + "_key_" + std::to_string(fcfs_ct->keys_sizes[i]) + "b_" + std::to_string(i);
    if (std::optional<var_t> key_var = ingress_vars.get(key_name)) {
      internals.keys.push_back(key_var.value());
      continue;
    }
    const var_t key_var = alloc_var(key_name, fcfs_ct->keys_sizes.at(i), EXACT_NAME | SKIP_STACK_ALLOC | IS_INGRESS_METADATA);
    declare_var_in_ingress_metadata(key_var);
    internals.keys.push_back(key_var);
  }

  for (const Register &reg : fcfs_ct->cache_keys) {
    for (const RegisterActionType &action_type : reg.actions) {
      const code_t action_name                          = build_register_action_name(&reg, action_type);
      internals.keys_reg_actions[{reg.id, action_type}] = action_name;
    }
  }

  return internals;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedSetRead *node) {
  const DS_ID fcfs_cs_id                         = node->get_fcfs_cs_id();
  const std::vector<klee::ref<klee::Expr>> &keys = node->get_keys();
  const symbol_t hit                             = node->get_map_has_this_key();

  const FCFSCachedSet *fcfs_cs = get_tofino_ds<FCFSCachedSet>(ep, fcfs_cs_id);
  const bdd_node_id_t node_id  = node->get_node()->get_id();

  const fcfs_cs_internals_t fcfs_cs_internals = fcfs_cs_get_internals(fcfs_cs);

  transpile_fcfs_cs_decl(fcfs_cs, ep_node);

  const Table *table = fcfs_cs->get_table(node_id);
  assert(table && "Table not found");
  transpile_table_decl(table, fcfs_cs_internals.keys, {}, true);

  const Hash *hash = fcfs_cs->get_hash(node_id);
  assert(hash && "Hash not found");

  std::vector<code_t> hash_inputs;
  for (const var_t &key_var : fcfs_cs_internals.keys) {
    hash_inputs.push_back(key_var.name);
  }

  code_t hash_calculator;
  code_t hash_value;
  transpile_hash_calculation(hash, hash_inputs, hash_calculator, hash_value);

  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  for (size_t i = 0; i < keys.size(); i++) {
    const klee::ref<klee::Expr> key_expr = keys[i];
    const var_t &key_var                 = fcfs_cs_internals.keys[i];
    ingress_apply.indent();
    ingress_apply << key_var.name << " = " << transpiler.transpile(key_expr) << ";\n";
  }

  const var_t hit_var = alloc_var("hit", hit.expr, FORCE_BOOL);
  hit_var.declare(ingress_apply, table->id + ".apply().hit");

  ingress_apply.indent();
  ingress_apply << hash_calculator << "();\n";

  const code_t is_alive_var_name = create_unique_name("fcfs_cs_is_alive");
  ingress_apply.indent();
  ingress_apply << "bool " << is_alive_var_name << " = ";
  ingress_apply << fcfs_cs_internals.liveness_query << ".execute(" << hash_value << ");\n";

  ingress_apply.indent();
  ingress_apply << "if (";
  ingress_apply << "!" << hit_var.name;
  ingress_apply << " && ";
  ingress_apply << is_alive_var_name;
  ingress_apply << ") {\n";
  ingress_apply.inc();

  const code_t match_counter_var_name = create_unique_name("match_counter");

  ingress.indent();
  ingress << "bit<8> " << match_counter_var_name << " = 0;\n";

  for (size_t i = 0; i < fcfs_cs->cache_keys.size(); i++) {
    coder_t action_body;
    action_body.indent();
    action_body << match_counter_var_name << " = " << match_counter_var_name << " + "
                << fcfs_cs_internals.keys_reg_actions.at({fcfs_cs->cache_keys[i].id, RegisterActionType::CheckValue}) << ".execute(" << hash_value
                << ");\n";

    const code_t action_name = fcfs_cs_id + "_check_key_" + std::to_string(i) + "_" + std::to_string(node_id);
    transpile_action_decl(action_name, action_body.split_lines());

    ingress_apply.indent();
    ingress_apply << action_name << "();\n";
  }

  ingress_apply.indent();
  ingress_apply << "if (" << match_counter_var_name << " == " << fcfs_cs->cache_keys.size() << ") {\n";
  ingress_apply.inc();

  ingress_apply.indent();
  ingress_apply << hit_var.name << " = true;\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedSetReadInsert *node) {
  const DS_ID fcfs_cs_id                         = node->get_fcfs_cs_id();
  const std::vector<klee::ref<klee::Expr>> &keys = node->get_keys();
  const std::optional<symbol_t> hit              = node->get_map_has_this_key();
  const symbol_t &cached_insert_success          = node->get_cached_insert_success();

  const FCFSCachedSet *fcfs_cs = get_tofino_ds<FCFSCachedSet>(ep, fcfs_cs_id);
  const bdd_node_id_t node_id  = node->get_node()->get_id();

  const fcfs_cs_internals_t fcfs_cs_internals = fcfs_cs_get_internals(fcfs_cs);

  transpile_fcfs_cs_decl(fcfs_cs, ep_node);

  const Table *table = fcfs_cs->get_table(node_id);
  assert(table && "Table not found");
  transpile_table_decl(table, fcfs_cs_internals.keys, {}, true);

  const Hash *hash = fcfs_cs->get_hash(node_id);
  assert(hash && "Hash not found");

  std::vector<code_t> hash_inputs;
  for (const var_t &key_var : fcfs_cs_internals.keys) {
    hash_inputs.push_back(key_var.name);
  }

  code_t hash_calculator;
  code_t hash_value;
  transpile_hash_calculation(hash, hash_inputs, hash_calculator, hash_value);

  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  for (size_t i = 0; i < keys.size(); i++) {
    const klee::ref<klee::Expr> key_expr = keys[i];
    const var_t &key_var                 = fcfs_cs_internals.keys[i];
    ingress_apply.indent();
    ingress_apply << key_var.name << " = " << transpiler.transpile(key_expr) << ";\n";
  }

  const var_t hit_var = hit.has_value() ? alloc_var("hit", hit->expr, FORCE_BOOL) : alloc_var("hit", 32, EXACT_NAME | FORCE_BOOL);
  hit_var.declare(ingress_apply, table->id + ".apply().hit");

  const var_t cached_insert_success_var = alloc_var("cached_insert_success", cached_insert_success.expr);
  cached_insert_success_var.declare(ingress_apply, "0");

  ingress_apply.indent();
  ingress_apply << "if (!" << hit_var.name << ") {\n";
  ingress_apply.inc();

  ingress_apply.indent();
  ingress_apply << hash_calculator << "();\n";

  const code_t is_alive_var_name = create_unique_name("fcfs_cs_is_alive");
  ingress_apply.indent();
  ingress_apply << "bool " << is_alive_var_name << " = ";
  ingress_apply << fcfs_cs_internals.liveness_query_and_refresh << ".execute(" << hash_value << ");\n";

  ingress_apply.indent();
  ingress_apply << "if (" << is_alive_var_name << ") {\n";
  ingress_apply.inc();

  const code_t match_counter_var_name = create_unique_name("match_counter");

  ingress.indent();
  ingress << "bit<8> " << match_counter_var_name << " = 0;\n";

  for (size_t i = 0; i < fcfs_cs->cache_keys.size(); i++) {
    coder_t action_body;
    action_body.indent();
    action_body << match_counter_var_name << " = " << match_counter_var_name << " + "
                << fcfs_cs_internals.keys_reg_actions.at({fcfs_cs->cache_keys[i].id, RegisterActionType::CheckValue}) << ".execute(" << hash_value
                << ");\n";

    const code_t action_name = fcfs_cs_id + "_check_key_" + std::to_string(i) + "_" + std::to_string(node_id);
    transpile_action_decl(action_name, action_body.split_lines());

    ingress_apply.indent();
    ingress_apply << action_name << "();\n";
  }

  ingress_apply.indent();
  ingress_apply << "if (" << match_counter_var_name << " == " << fcfs_cs->cache_keys.size() << ") {\n";
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

  for (const Register &reg_key : fcfs_cs->cache_keys) {
    ingress_apply.indent();
    ingress_apply << fcfs_cs_internals.keys_reg_actions.at({reg_key.id, RegisterActionType::Write}) << ".execute(" << hash_value << ");\n";
  }

  ingress_apply.indent();
  ingress_apply << cached_insert_success_var.name << " = 1;\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedSetInsert *node) {
  const DS_ID fcfs_cs_id                         = node->get_fcfs_cs_id();
  const std::vector<klee::ref<klee::Expr>> &keys = node->get_keys();
  const symbol_t &cached_insert_success          = node->get_success();

  const FCFSCachedSet *fcfs_cs = get_tofino_ds<FCFSCachedSet>(ep, fcfs_cs_id);
  const bdd_node_id_t node_id  = node->get_node()->get_id();

  const fcfs_cs_internals_t fcfs_cs_internals = fcfs_cs_get_internals(fcfs_cs);

  transpile_fcfs_cs_decl(fcfs_cs, ep_node);

  const Hash *hash = fcfs_cs->get_hash(node_id);
  assert(hash && "Hash not found");

  std::vector<code_t> hash_inputs;
  for (const var_t &key_var : fcfs_cs_internals.keys) {
    hash_inputs.push_back(key_var.name);
  }

  code_t hash_calculator;
  code_t hash_value;
  transpile_hash_calculation(hash, hash_inputs, hash_calculator, hash_value);

  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  for (size_t i = 0; i < keys.size(); i++) {
    const klee::ref<klee::Expr> key_expr = keys[i];
    const var_t &key_var                 = fcfs_cs_internals.keys[i];
    ingress_apply.indent();
    ingress_apply << key_var.name << " = " << transpiler.transpile(key_expr) << ";\n";
  }

  const var_t cached_insert_success_var = alloc_var("cached_insert_success", cached_insert_success.expr);
  cached_insert_success_var.declare(ingress_apply, "0");

  ingress_apply.indent();
  ingress_apply << hash_calculator << "();\n";

  const code_t is_alive_var_name = create_unique_name("fcfs_cs_is_alive");
  ingress_apply.indent();
  ingress_apply << "bool " << is_alive_var_name << " = ";
  ingress_apply << fcfs_cs_internals.liveness_query_and_refresh << ".execute(" << hash_value << ");\n";

  ingress_apply.indent();
  ingress_apply << "if (!" << is_alive_var_name << ") {\n";
  ingress_apply.inc();

  for (const Register &reg_key : fcfs_cs->cache_keys) {
    ingress_apply.indent();
    ingress_apply << fcfs_cs_internals.keys_reg_actions.at({reg_key.id, RegisterActionType::Write}) << ".execute(" << hash_value << ");\n";
  }

  ingress_apply.indent();
  ingress_apply << cached_insert_success_var.name << " = 1;\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "}\n";

  return EPVisitor::Action::doChildren;
}

void TofinoSynthesizer::transpile_digest(const Digest &digest, const std::vector<code_t> &fields) {
  coder_t &ingress_deparser_apply = get(MARKER_INGRESS_DEPARSER_APPLY);

  ingress_deparser_apply.indent();
  ingress_deparser_apply << "if (";
  ingress_deparser_apply << "ig_dprsr_md.digest_type == " << digest.digest_type;
  ingress_deparser_apply << ") {\n";

  ingress_deparser_apply.inc();

  ingress_deparser_apply.indent();
  ingress_deparser_apply << digest.id << ".pack({\n";
  ingress_deparser_apply.inc();

  for (const code_t &field : fields) {
    ingress_deparser_apply.indent();
    ingress_deparser_apply << field << ",\n";
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
  transpile_register_action_decl(&hh_table->cached_counters, cached_counters_action_name, cached_counters_action_type);

  transpile_register_decl(&hh_table->packet_sampler);
  assert(hh_table->packet_sampler.actions.size() == 1);
  const RegisterActionType packet_sampler_action_type = *hh_table->packet_sampler.actions.begin();
  const code_t packet_sampler_action_name             = build_register_action_name(&hh_table->packet_sampler, packet_sampler_action_type, ep_node);
  transpile_register_action_decl(&hh_table->packet_sampler, packet_sampler_action_name, packet_sampler_action_type);

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
    transpile_register_action_decl(&cms_row, cms_row_action_name, cms_row_action_type);
    cms_rows_actions.push_back(cms_row_action_name);
  }

  transpile_register_decl(&hh_table->threshold);
  assert(hh_table->threshold.actions.size() == 1);
  const RegisterActionType threshold_action_type = *hh_table->threshold.actions.begin();
  const code_t threshold_action_name             = build_register_action_name(&hh_table->threshold, threshold_action_type, ep_node);
  const code_t threshold_value_cmp               = threshold_action_name + "_cmp";
  ingress.indent();
  ingress << Transpiler::type_from_size(32) << " " << threshold_value_cmp << ";\n";
  transpile_register_action_decl(&hh_table->threshold, threshold_action_name, threshold_action_type);

  const bits_t cms_value_size   = hh_table->count_min_sketch[0].value_size;
  const var_t cms_min_value_var = alloc_var(hh_table_id + "_cms_min", cms_value_size, EXACT_NAME | SKIP_STACK_ALLOC | IS_INGRESS_METADATA);
  declare_var_in_ingress_metadata(cms_min_value_var);

  transpile_digest_decl(&hh_table->digest);

  for (const var_t &key_var : keys_vars) {
    ingress_apply.indent();
    ingress_apply << key_var.name << " = " << transpiler.transpile(key_var.expr) << ";\n";
  }

  const var_t hit_var = alloc_var("hit", hit->expr, FORCE_BOOL);
  hit_var.declare(ingress_apply, table->id + ".apply().hit");

  const code_t packet_sampler_out_value = packet_sampler_action_name + "_out_value";
  ingress_apply.indent();
  ingress_apply << Transpiler::type_from_size(hh_table->packet_sampler.value_size) << " " << packet_sampler_out_value;
  ingress_apply << " = ";
  ingress_apply << packet_sampler_action_name << ".execute(0);\n";

  ingress_apply.indent();
  ingress_apply << "if (" << packet_sampler_out_value << " == 1) {\n";
  ingress_apply.inc();

  ingress_apply.indent();
  ingress_apply << "if (" << hit_var.name << ") {\n";
  ingress_apply.inc();

  ingress_apply.indent();
  ingress_apply << cached_counters_action_name << ".execute(" << transpiler.transpile(index) << ");\n";

  ingress_apply.dec();
  ingress_apply.indent();
  ingress_apply << "} else {\n";
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

  for (size_t i = 0; i < cms_rows_values.size(); i++) {
    const code_t &cms_row_value = cms_rows_values[i];

    ingress_apply.indent();
    if (i == 0) {
      ingress_apply << cms_min_value_var.name << " = " << cms_row_value << ";\n";
    } else {
      ingress_apply << cms_min_value_var.name << " = min(" << cms_min_value_var.name << ", " << cms_row_value << ");\n";
    }
  }

  ingress_apply.indent();
  ingress_apply << threshold_value_cmp << " = " << cms_min_value_var.name << ";\n";

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

  std::vector<code_t> digest_fields;
  for (const var_t &key_var : keys_vars) {
    digest_fields.push_back(key_var.name);
  }
  digest_fields.push_back(cms_min_value_var.name);
  transpile_digest(hh_table->digest, digest_fields);

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

std::unordered_map<RegisterActionType, std::vector<code_t>> TofinoSynthesizer::bf_get_rows_reg_actions(const BloomFilter *bf) {
  std::unordered_map<RegisterActionType, std::vector<code_t>> bf_rows_reg_actions;

  for (const Register &bf_row : bf->rows) {
    for (const RegisterActionType &action_type : bf_row.actions) {
      const code_t bf_row_action_name = build_register_action_name(&bf_row, action_type);
      bf_rows_reg_actions[action_type].push_back(bf_row_action_name);
    }
  }

  return bf_rows_reg_actions;
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

std::unordered_map<RegisterActionType, std::vector<code_t>> TofinoSynthesizer::bf_get_rows_actions(const BloomFilter *bf) {
  std::unordered_map<RegisterActionType, std::vector<code_t>> bf_rows_actions;

  const std::unordered_map<RegisterActionType, std::vector<code_t>> bf_rows_reg_actions = bf_get_rows_reg_actions(bf);

  for (const auto &[action_type, reg_actions] : bf_rows_reg_actions) {
    for (const code_t &reg_action : reg_actions) {
      const code_t bf_row_action = reg_action + "_execute";
      bf_rows_actions[action_type].push_back(bf_row_action);
    }
  }

  return bf_rows_actions;
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

std::unordered_map<RegisterActionType, std::vector<code_t>> TofinoSynthesizer::bf_get_rows_values(const BloomFilter *bf) {
  std::unordered_map<RegisterActionType, std::vector<code_t>> bf_rows_values;

  const std::unordered_map<RegisterActionType, std::vector<code_t>> bf_rows_reg_actions = bf_get_rows_reg_actions(bf);

  for (const auto &[action_type, reg_actions] : bf_rows_reg_actions) {
    for (const code_t &reg_action : reg_actions) {
      const code_t bf_row_value = reg_action + "_value";
      bf_rows_values[action_type].push_back(bf_row_value);
    }
  }

  return bf_rows_values;
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

TofinoSynthesizer::var_t TofinoSynthesizer::bf_get_estimate_value(const BloomFilter *bf) {
  const var_t estimate_var = alloc_var(bf->id + "_estimate", 32, EXACT_NAME | IS_INGRESS_METADATA);
  declare_var_in_ingress_metadata(estimate_var);
  return estimate_var;
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

        transpile_register_action_decl(&row, reg_action, action_type);

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

std::vector<code_t> TofinoSynthesizer::transpile_bf_decl(const BloomFilter *bf, const EPNode *ep_node, const std::vector<code_t> &key_inputs,
                                                         RegisterActionType action_type) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL);

  const std::unordered_map<RegisterActionType, std::vector<code_t>> reg_actions = bf_get_rows_reg_actions(bf);
  const std::unordered_map<RegisterActionType, std::vector<code_t>> actions     = bf_get_rows_actions(bf);
  const std::unordered_map<RegisterActionType, std::vector<code_t>> values      = bf_get_rows_values(bf);
  const var_t estimate_value                                                    = bf_get_estimate_value(bf);

  if (!declared_ds.contains(bf->id)) {
    for (size_t i = 0; i < bf->height; i++) {
      transpile_register_decl(&bf->rows[i]);
    }
    ingress << "\n";
    declared_ds.insert(bf->id);
  }

  // The site's hash instances, ahead of the actions computing them.
  std::vector<code_t> hash_ids;
  for (Hash hash : bf->hashes) {
    hash.id = hash.id + "_" + std::to_string(ep_node->get_id());
    transpile_hash_decl(&hash);
    hash_ids.push_back(hash.id);
  }
  ingress << "\n";

  assert(reg_actions.contains(action_type) && actions.contains(action_type) && values.contains(action_type));
  std::vector<code_t> site_actions;
  for (size_t i = 0; i < bf->height; i++) {
    assert(i < BloomFilter::HASH_SALTS.size());
    const Register &row      = bf->rows[i];
    code_t reg_action        = reg_actions.at(action_type)[i];
    code_t action            = actions.at(action_type)[i];
    code_t value             = values.at(action_type)[i];
    const bool returns_value = register_action_types_with_out_value.contains(action_type);

    if (const auto inputs_it = bf_action_inputs.find(action); inputs_it != bf_action_inputs.end() && inputs_it->second != key_inputs) {
      const code_t site = "_" + std::to_string(ep_node->get_id());
      reg_action += site;
      action += site;
      value += site;
    }
    site_actions.push_back(action);
    if (bf_action_inputs.contains(action)) {
      continue; // Declared at an earlier site, hashing the same inputs.
    }
    bf_action_inputs[action] = key_inputs;

    transpile_register_action_decl(&row, reg_action, action_type);
    if (returns_value) {
      ingress.indent();
      ingress << Transpiler::type_from_size(row.value_size) << " " << value << ";\n";
    }

    coder_t body;
    body.indent();
    if (returns_value) {
      body << value << " = ";
    }
    body << reg_action << ".execute(" << hash_ids[i] << ".get({\n";
    body.inc();
    for (const code_t &input : key_inputs) {
      body.indent();
      body << input << ",\n";
    }
    body.indent();
    body << Transpiler::transpile_literal(HHTable::HASH_SALTS[i], sizeof(BloomFilter::HASH_SALTS[i]) * 8, true) << "\n";
    body.dec();
    body.indent();
    body << "}));\n";
    if (returns_value) {
      body.indent();
      body << estimate_value.get_slice(i, 1).name << " = " << value << "[0:0];\n";
    }
    transpile_action_decl(action, body.split_lines());
    ingress << "\n";
  }
  return site_actions;
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

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::BloomFilterSet *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const DS_ID bf_id                              = node->get_bf_id();
  const std::vector<klee::ref<klee::Expr>> &keys = node->get_keys();

  const BloomFilter *bf = get_tofino_ds<BloomFilter>(ep, bf_id);

  std::vector<code_t> key_inputs;
  for (const klee::ref<klee::Expr> &key : keys) {
    key_inputs.push_back(transpiler.transpile(key));
  }
  const std::vector<code_t> row_actions = transpile_bf_decl(bf, ep_node, key_inputs, RegisterActionType::SetToOne);

  for (const code_t &action : row_actions) {
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

  assert(values.find(RegisterActionType::IncrementAndReturnNewValue) != values.end());
  const std::vector<code_t> &read_values = values.at(RegisterActionType::IncrementAndReturnNewValue);

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

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::BloomFilterQueryAndSet *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const DS_ID bf_id                              = node->get_bf_id();
  const std::vector<klee::ref<klee::Expr>> &keys = node->get_keys();
  const klee::ref<klee::Expr> estimate           = node->get_estimate();

  const BloomFilter *bf = get_tofino_ds<BloomFilter>(ep, bf_id);

  std::vector<code_t> key_inputs;
  for (const klee::ref<klee::Expr> &key : keys) {
    key_inputs.push_back(transpiler.transpile(key));
  }
  const std::vector<code_t> row_actions = transpile_bf_decl(bf, ep_node, key_inputs, RegisterActionType::SetToOneAndReturnOldValue);

  const var_t estimate_value = bf_get_estimate_value(bf);
  ingress_apply.indent();
  ingress_apply << estimate_value.name << " = 0;\n";

  for (const code_t &action : row_actions) {
    ingress_apply.indent();
    ingress_apply << action << "();\n";
  }

  ingress_vars.set_var_expr(estimate_value.name, estimate);

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

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::BloomFilterQuery *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const DS_ID bf_id                              = node->get_bf_id();
  const std::vector<klee::ref<klee::Expr>> &keys = node->get_keys();
  const klee::ref<klee::Expr> estimate           = node->get_estimate();

  const BloomFilter *bf = get_tofino_ds<BloomFilter>(ep, bf_id);

  std::vector<code_t> key_inputs;
  for (const klee::ref<klee::Expr> &key : keys) {
    key_inputs.push_back(transpiler.transpile(key));
  }
  const std::vector<code_t> row_actions = transpile_bf_decl(bf, ep_node, key_inputs, RegisterActionType::Read);

  const var_t estimate_value = bf_get_estimate_value(bf);
  ingress_apply.indent();
  ingress_apply << estimate_value.name << " = 0;\n";

  for (const code_t &action : row_actions) {
    ingress_apply.indent();
    ingress_apply << action << "();\n";
  }

  ingress_vars.set_var_expr(estimate_value.name, estimate);

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

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::HashObj *node) {
  const DS_ID hash_id          = node->get_hash_id();
  klee::ref<klee::Expr> in     = node->get_in();
  klee::ref<klee::Expr> hash_r = node->get_hash();

  const Hash *hash = get_tofino_ds<Hash>(ep, hash_id);
  transpile_hash_decl(hash);

  const std::vector<code_t> hash_inputs = {transpiler.transpile(in)};

  code_t hash_calculator;
  code_t hash_value;
  transpile_hash_calculation(hash, hash_inputs, hash_calculator, hash_value);

  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  ingress_apply.indent();
  ingress_apply << hash_calculator << "();\n";

  const var_t hash_var = alloc_var("hll_hash", hash_r);
  hash_var.declare(ingress_apply, hash_value);

  return EPVisitor::Action::doChildren;
}

void TofinoSynthesizer::emit_compute_table(const EP *ep, DS_ID table_id, klee::ref<klee::Expr> in, klee::ref<klee::Expr> out) {
  const Table *table = get_tofino_ds<Table>(ep, table_id);

  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const var_t key_var = alloc_var(table_id + "_key", in, SKIP_STACK_ALLOC | EXACT_NAME | IS_INGRESS_METADATA);
  declare_var_in_ingress_metadata(key_var);

  const var_t out_var = alloc_var(table_id + "_out", out, EXACT_NAME | IS_INGRESS_METADATA);
  declare_var_in_ingress_metadata(out_var);

  if (declared_ds.find(table_id) == declared_ds.end()) {
    declared_ds.insert(table_id);

    const code_t action_name = table_id + "_get_value";
    const bits_t key_w       = in->getWidth();
    const bool ternary       = (table->match == TableMatch::Ternary);

    ingress.indent();
    ingress << "action " << action_name << "(" << Transpiler::type_from_size(out->getWidth()) << " v) {\n";
    ingress.inc();
    ingress.indent();
    ingress << out_var.name << " = v;\n";
    ingress.dec();
    ingress.indent();
    ingress << "}\n";

    ingress.indent();
    ingress << "table " << table_id << " {\n";
    ingress.inc();

    ingress.indent();
    ingress << "key = { " << key_var.name << ": " << (ternary ? "ternary" : "exact") << "; }\n";

    ingress.indent();
    ingress << "actions = { " << action_name << "; }\n";

    ingress.indent();
    ingress << "size = " << table->capacity << ";\n";

    ingress.indent();
    ingress << "default_action = " << action_name << "(0);\n";

    // Const entries computed from the math function (ctz/ffs/power_of_two) or from
    // the profiler-observed values (ln).
    if (!table->const_entries.empty()) {
      ingress.indent();
      ingress << "const entries = {\n";
      ingress.inc();
      for (const table_entry_t &e : table->const_entries) {
        ingress.indent();
        if (ternary) {
          ingress << key_w << "w" << e.key << " &&& " << key_w << "w" << e.mask << " : " << action_name << "(" << e.value << ");\n";
        } else {
          ingress << key_w << "w" << e.key << " : " << action_name << "(" << e.value << ");\n";
        }
      }
      ingress.dec();
      ingress.indent();
      ingress << "}\n";
    }

    ingress.dec();
    ingress.indent();
    ingress << "}\n";
    ingress << "\n";
  }

  ingress_apply.indent();
  ingress_apply << key_var.name << " = " << transpiler.transpile(in) << ";\n";

  ingress_apply.indent();
  ingress_apply << table_id << ".apply();\n";
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::CountTrailingZeros *node) {
  emit_compute_table(ep, node->get_table_id(), node->get_in(), node->get_out());
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::FindFirstSetBit *node) {
  emit_compute_table(ep, node->get_table_id(), node->get_in(), node->get_out());
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::PowerOfTwo *node) {
  emit_compute_table(ep, node->get_table_id(), node->get_in(), node->get_out());
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Ln *node) {
  emit_compute_table(ep, node->get_table_id(), node->get_in(), node->get_out());
  return EPVisitor::Action::doChildren;
}

// An unrolled arithmetic operation: a keyless table whose only action computes the value into
// metadata, so the P4 carries the same one-stage step the placer charged for it.
EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::ArithmeticOp *node) {
  emit_compute_run(ep, ep_node);
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::RotateLeft *node) {
  emit_compute_run(ep, ep_node);
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::RotateLeftShifts *node) {
  emit_compute_run(ep, ep_node);
  return EPVisitor::Action::doChildren;
}

std::vector<const EPNode *> TofinoSynthesizer::compute_run_steps(const EPNode *first) const {
  std::vector<const EPNode *> steps;
  for (const EPNode *ep_node = first; ep_node && ep_node->get_module();) {
    const Module *module = ep_node->get_module();
    if (TofinoModuleFactory::is_compute_module(module)) {
      steps.push_back(ep_node);
    } else if (module->get_type() == ModuleType::Tofino_Ignore) {
      // Transparent: nothing in the data plane, and no break in the run, as in the search.
    } else if (module->get_type() == ModuleType::Tofino_If && !dynamic_cast<const Tofino::If *>(module)->get_materialized_operands().empty()) {
      steps.push_back(ep_node);
      break;
    } else {
      break;
    }
    const std::vector<EPNode *> &children = ep_node->get_children();
    if (children.size() != 1) {
      break;
    }
    ep_node = children[0];
  }
  return steps;
}

std::vector<code_t> TofinoSynthesizer::action_calls(const code_t &name, const std::vector<compute_op_t> &ops) const {
  size_t hash_ops  = 0;
  bool action_data = false;
  for (const compute_op_t &op : ops) {
    hash_ops += op.in_hash ? 1 : 0;
    action_data |= carries_action_data(op);
  }
  std::vector<code_t> calls{name + "();"};
  for (size_t i = 1; i < hash_ops; i++) {
    calls.push_back(name + "_h" + std::to_string(i) + "();");
  }
  if (hash_ops > 0 && action_data) {
    calls.push_back(name + "_k();");
  }
  return calls;
}

std::vector<code_t> TofinoSynthesizer::compute_action_calls(const TofinoContext *tofino_ctx, const DS_ID &action_id) const {
  const Tofino::ComputeAction *action = dynamic_cast<const Tofino::ComputeAction *>(tofino_ctx->get_data_structures().get_ds_from_id(action_id));
  assert(action && "Compute step placed outside a ComputeAction");
  return action_calls(action_id, action->ops);
}

void TofinoSynthesizer::declare_compute_action(coder_t &coder, const code_t &name, const std::vector<compute_op_t> &ops,
                                               const std::unordered_map<std::string, action_statement_t> &statements) const {
  // bf-p4c allows 32 bits through a table's immediate pathway, which is one 32-bit @in_hash op; a
  // second one in the same action is "the number of bits required to go through the immediate
  // pathway 64 ... is greater than the available bits 32". The hand-written ground truth never has
  // more than one per action. Keep the first here and give each of the rest an action of its own,
  // invoked straight after: the statements in an action are independent by construction, so
  // moving one to the next stage cannot change what it reads.
  // Likewise, an action computing in the hash unit carries no action data (carries_action_data):
  // its statements with a wide constant go to a `_k` companion.
  // Split by the ops, not the statements there are, so the companions are the ones action_calls
  // derives from the ops.
  std::vector<const compute_op_t *> kept;
  std::vector<const compute_op_t *> spilled;
  std::vector<const compute_op_t *> constants;
  bool hash_taken = false;
  bool has_hash   = false;
  for (const compute_op_t &op : ops) {
    has_hash |= op.in_hash;
  }
  for (const compute_op_t &op : ops) {
    if (op.in_hash && hash_taken) {
      spilled.push_back(&op);
    } else if (has_hash && !op.in_hash && carries_action_data(op)) {
      constants.push_back(&op);
    } else {
      hash_taken |= op.in_hash;
      kept.push_back(&op);
    }
  }
  const auto declare = [&](const code_t &action_name, const std::vector<const compute_op_t *> &body) {
    coder.indent();
    coder << "action " << action_name << "() {\n";
    coder.inc();
    for (const compute_op_t *op : body) {
      const auto statement_it = statements.find(op->id);
      if (statement_it == statements.end()) {
        continue; // A value held elsewhere already: nothing to compute.
      }
      coder.indent();
      if (statement_it->second.in_hash) {
        coder << "@in_hash { " << statement_it->second.statement << " }\n";
      } else {
        coder << statement_it->second.statement << "\n";
      }
    }
    coder.dec();
    coder.indent();
    coder << "}\n";
    coder << "\n";
  };
  declare(name, kept);
  for (size_t i = 0; i < spilled.size(); i++) {
    declare(name + "_h" + std::to_string(i + 1), {spilled[i]});
  }
  if (has_hash && !constants.empty()) {
    declare(name + "_k", constants);
  }
}

void TofinoSynthesizer::emit_action_variants(const TofinoContext *tofino_ctx) {
  for (const auto &[action_id, parts] : action_variants) {
    const Tofino::ComputeAction *action = dynamic_cast<const Tofino::ComputeAction *>(tofino_ctx->get_data_structures().get_ds_from_id(action_id));
    assert(action && "Compute step placed outside a ComputeAction");
    const auto statements_it = action_statements.find(action_id);
    if (statements_it == action_statements.end()) {
      panic("The action %s is called in part by a path, but no path declared it", action_id.c_str());
    }
    coder_t &control = code_template.get(action_in_egress.at(action_id) ? MARKER_EGRESS_CONTROL : MARKER_INGRESS_CONTROL);
    for (size_t k = 0; k < parts.size(); k++) {
      std::vector<compute_op_t> ops;
      for (const compute_op_t &op : action->ops) {
        if (std::find(parts[k].begin(), parts[k].end(), op.id) != parts[k].end()) {
          ops.push_back(op);
        }
      }
      declare_compute_action(control, action_id + "_v" + std::to_string(k), ops, statements_it->second);
    }
  }
}

void TofinoSynthesizer::plan_shared_runs(const EP *ep) {
  using compute_operand_t         = TofinoModuleFactory::compute_operand_t;
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  const Pipeline &pipeline        = tofino_ctx->get_tna().pipeline;

  // A site: a compute run, by its first step, with the actions of its own ops, the actions of
  // other paths' ops it calls, and the pass it runs in (the cut it starts at; none for the first).
  struct site_t {
    const EPNode *first;
    const EPNode *cut;
    bool egress;
    std::vector<DS_ID> own;
    std::vector<DS_ID> reused;
  };
  std::vector<site_t> sites;
  std::unordered_map<const EPNode *, size_t> site_of;
  std::unordered_set<const EPNode *> in_a_run;
  std::vector<std::vector<size_t>> paths; // The sites on each root-to-leaf path, in order.

  const auto push_unique = [](std::vector<DS_ID> &ids, const DS_ID &id) {
    if (std::find(ids.begin(), ids.end(), id) == ids.end()) {
      ids.push_back(id);
    }
  };
  const auto classify = [&](site_t &site, const std::string &op_id, const DS_ID &action) {
    const std::optional<compute_reuse_t> reuse = tofino_ctx->get_compute_reuse(op_id);
    if (reuse && !tofino_ctx->is_own_path_reuse(op_id)) {
      push_unique(site.reused, reuse->action);
    } else {
      push_unique(site.own, action);
    }
  };
  const auto classify_operands = [&](site_t &site, const std::vector<compute_operand_t> &operands) {
    for (const compute_operand_t &operand : operands) {
      classify(site, operand.op_id, operand.action_id);
    }
  };
  const auto new_site = [&](const EPNode *first, const EPNode *cut, bool egress) {
    site_t site{first, cut, egress, {}, {}};
    for (const EPNode *step : compute_run_steps(first)) {
      in_a_run.insert(step);
      const Module *module = step->get_module();
      switch (module->get_type()) {
      case ModuleType::Tofino_ArithmeticOp: {
        const Tofino::ArithmeticOp *op = dynamic_cast<const Tofino::ArithmeticOp *>(module);
        classify(site, op->get_op_id(), op->get_action_id());
        classify_operands(site, op->get_operands());
      } break;
      case ModuleType::Tofino_RotateLeft: {
        const Tofino::RotateLeft *rot = dynamic_cast<const Tofino::RotateLeft *>(module);
        classify(site, rot->get_op_id(), rot->get_action_id());
        classify_operands(site, rot->get_operands());
      } break;
      case ModuleType::Tofino_RotateLeftShifts: {
        const Tofino::RotateLeftShifts *rot = dynamic_cast<const Tofino::RotateLeftShifts *>(module);
        classify(site, rot->get_shl_op_id(), rot->get_shl_action_id());
        classify(site, rot->get_shr_op_id(), rot->get_shr_action_id());
        classify(site, rot->get_or_op_id(), rot->get_or_action_id());
        classify_operands(site, rot->get_operands());
      } break;
      case ModuleType::Tofino_If: {
        classify_operands(site, dynamic_cast<const Tofino::If *>(module)->get_materialized_operands());
      } break;
      default:
        break;
      }
    }
    site_of[first] = sites.size();
    sites.push_back(site);
  };

  std::function<void(const EPNode *, const EPNode *, bool, std::vector<size_t>)> walk = [&](const EPNode *node, const EPNode *cut, bool egress,
                                                                                            std::vector<size_t> on_path) {
    while (node) {
      if (const Module *module = node->get_module()) {
        switch (module->get_type()) {
        case ModuleType::Tofino_Recirculate:
          cut    = node;
          egress = false;
          break;
        case ModuleType::Tofino_SendToEgress:
          cut    = node;
          egress = true;
          break;
        default:
          if (TofinoModuleFactory::is_compute_module(module) && !in_a_run.contains(node)) {
            new_site(node, cut, egress);
          }
          if (auto it = site_of.find(node); it != site_of.end()) {
            on_path.push_back(it->second);
          }
          break;
        }
      }
      const std::vector<EPNode *> &children = node->get_children();
      if (children.empty()) {
        paths.push_back(on_path);
        return;
      }
      if (children.size() > 1) {
        for (const EPNode *child : children) {
          walk(child, cut, egress, on_path);
        }
        return;
      }
      node = children[0];
    }
  };
  walk(ep->get_root(), nullptr, false, {});

  // A run: a site's own actions, every one of them called by another path of the same pass (the
  // reused actions of that path's sites cover them). Grouped by the actions, so a chain several
  // paths share is one run.
  struct group_t {
    std::vector<DS_ID> actions;
    std::set<size_t> site_ids;
  };
  std::map<std::vector<DS_ID>, group_t> groups;
  for (size_t x = 0; x < sites.size(); x++) {
    const site_t &X = sites[x];
    if (X.own.empty()) {
      continue;
    }
    for (const std::vector<size_t> &path : paths) {
      if (std::find(path.begin(), path.end(), x) != path.end()) {
        continue;
      }
      std::unordered_set<DS_ID> covered;
      std::vector<size_t> callers;
      for (const size_t y : path) {
        const site_t &Y = sites[y];
        if (Y.egress != X.egress || (!X.egress && Y.cut != X.cut)) {
          continue;
        }
        bool calls = false;
        for (const DS_ID &action : Y.reused) {
          if (std::find(X.own.begin(), X.own.end(), action) != X.own.end()) {
            covered.insert(action);
            calls = true;
          }
        }
        if (calls) {
          callers.push_back(y);
        }
      }
      if (covered.size() != X.own.size()) {
        continue;
      }
      std::vector<DS_ID> key = X.own;
      std::sort(key.begin(), key.end());
      group_t &group = groups[key];
      group.actions  = X.own;
      group.site_ids.insert(x);
      group.site_ids.insert(callers.begin(), callers.end());
    }
  }

  // The If the sites diverge at: the deepest ancestor they all have.
  const auto join_of = [](const std::set<const EPNode *> &nodes) -> const EPNode * {
    std::unordered_map<const EPNode *, size_t> shared;
    for (const EPNode *node : nodes) {
      for (const EPNode *ancestor = node->get_prev(); ancestor; ancestor = ancestor->get_prev()) {
        shared[ancestor]++;
      }
    }
    for (const EPNode *ancestor = (*nodes.begin())->get_prev(); ancestor; ancestor = ancestor->get_prev()) {
      if (shared[ancestor] == nodes.size()) {
        return ancestor;
      }
    }
    return nullptr;
  };
  // Whether nothing branches between a site and the cut its block starts at.
  const auto at_block_top = [](const site_t &site) -> bool {
    for (const EPNode *ancestor = site.first->get_prev(); ancestor && ancestor != site.cut; ancestor = ancestor->get_prev()) {
      const Module *module = ancestor->get_module();
      if (module && (module->get_type() == ModuleType::Tofino_If || module->get_type() == ModuleType::Tofino_Then ||
                     module->get_type() == ModuleType::Tofino_Else)) {
        return false;
      }
    }
    return true;
  };

  for (auto &[key, group] : groups) {
    shared_run_t run;
    run.actions = group.actions;
    std::stable_sort(run.actions.begin(), run.actions.end(),
                     [&pipeline](const DS_ID &a, const DS_ID &b) { return pipeline.get_placed_stage(a) < pipeline.get_placed_stage(b); });
    std::set<const EPNode *> firsts;
    for (const size_t id : group.site_ids) {
      firsts.insert(sites[id].first);
    }
    run.sites.insert(firsts.begin(), firsts.end());
    if (!sites[*group.site_ids.begin()].egress) {
      run.join = join_of(firsts);
      if (!run.join || !run.join->get_module() || run.join->get_module()->get_type() != ModuleType::Tofino_If) {
        continue;
      }
      const var_t flag = alloc_var("shared_run_" + std::to_string(shared_runs.size()), 1, EXACT_NAME | IS_INGRESS_METADATA | SKIP_STACK_ALLOC);
      declare_var_in_ingress_metadata(flag);
      run.flag             = flag.name;
      coder_t &apply_start = code_template.get(MARKER_INGRESS_APPLY_START);
      apply_start.indent();
      apply_start << run.flag << " = 0;\n";
    } else {
      // Each site the whole of its block's top level, each block once: the ladder can then nest
      // what else the block does under its own code path.
      std::set<const EPNode *> cuts;
      bool mergeable = true;
      for (const size_t id : group.site_ids) {
        const site_t &site = sites[id];
        mergeable &= site.cut && !cuts.contains(site.cut) && at_block_top(site);
        cuts.insert(site.cut);
      }
      if (!mergeable) {
        continue;
      }
      run.egress_cuts.assign(cuts.begin(), cuts.end());
    }
    for (const DS_ID &action : run.actions) {
      const std::vector<code_t> calls = compute_action_calls(tofino_ctx, action);
      run.calls.insert(run.calls.end(), calls.begin(), calls.end());
    }
    const size_t index = shared_runs.size();
    for (const EPNode *site : run.sites) {
      shared_runs_by_site[site].push_back(index);
    }
    if (run.join) {
      shared_runs_by_join[run.join].push_back(index);
    }
    shared_runs.push_back(run);
  }
  if (Walk::enabled()) {
    for (size_t r = 0; r < shared_runs.size(); r++) {
      std::cerr << "[runs] shared run " << r << ": " << shared_runs[r].actions.size() << " actions, " << shared_runs[r].sites.size() << " sites, "
                << (shared_runs[r].join ? "joined under a flag" : "one egress arm") << "\n";
    }
  }
}

void TofinoSynthesizer::emit_shared_runs_after(const EPNode *join) {
  const auto runs_it = shared_runs_by_join.find(join);
  if (runs_it == shared_runs_by_join.end()) {
    return;
  }
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);
  for (const size_t r : runs_it->second) {
    const shared_run_t &run = shared_runs[r];
    ingress_apply.indent();
    ingress_apply << "if (" << run.flag << " == 1) {\n";
    ingress_apply.inc();
    for (const code_t &call : run.calls) {
      ingress_apply.indent();
      ingress_apply << call << "\n";
    }
    ingress_apply.dec();
    ingress_apply.indent();
    ingress_apply << "}\n";
  }
}

namespace {

code_t arithmetic_code(klee::ref<klee::Expr> value, const std::vector<code_t> &operands) {
  switch (value->getKind()) {
  case klee::Expr::Add:
    return operands[0] + " + " + operands[1];
  case klee::Expr::Sub:
    return operands[0] + " - " + operands[1];
  case klee::Expr::Mul:
    return operands[0] + " * " + operands[1];
  case klee::Expr::UDiv:
  case klee::Expr::SDiv:
    return operands[0] + " / " + operands[1];
  case klee::Expr::URem:
  case klee::Expr::SRem:
    return operands[0] + " % " + operands[1];
  case klee::Expr::And:
    return operands[0] + " & " + operands[1];
  case klee::Expr::Or:
    return operands[0] + " | " + operands[1];
  case klee::Expr::Xor:
    return operands[0] + " ^ " + operands[1];
  case klee::Expr::Not:
    return "~" + operands[0];
  case klee::Expr::Shl:
    return operands[0] + " << " + operands[1];
  case klee::Expr::LShr:
  case klee::Expr::AShr:
    return operands[0] + " >> " + operands[1];
  default:
    panic("Not an arithmetic operation: %s", expr_to_string(value).c_str());
  }
}

// rotl(a, n) = a[w-1-n:0] ++ a[w-1:w-n] on a plain field.
code_t rotation_code(const code_t &a, u32 n, bits_t width) {
  if (n == 0) {
    return a;
  }
  return a + "[" + std::to_string(width - 1 - n) + ":0] ++ " + a + "[" + std::to_string(width - 1) + ":" + std::to_string(width - n) + "]";
}

} // namespace

// The run of consecutive compute steps starting at `first` is emitted here, all at once: one
// bare action per ComputeAction, in stage order, holding the assignments of every op it took
// (from any step of the run), so each value is computed after its producers and before its
// consumers whatever the steps' order in the plan. Later steps of the run then emit nothing.
void TofinoSynthesizer::emit_compute_run(const EP *ep, const EPNode *first) {
  using compute_operand_t = TofinoModuleFactory::compute_operand_t;

  struct op_emission_t {
    DS_ID action_id;
    std::string op_id;
    code_t statement; // Empty for an aliased value (already held elsewhere).
    bool in_hash;
  };
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  const Pipeline &pipeline        = tofino_ctx->get_tna().pipeline;

  // (time >> k) with k >= 16: the data plane keeps time as ingress_mac_tstamp[47:16] (32 bits
  // of 2^16 ns units, see the parser), so the value is meta.time >> (k - 16); that remaining
  // shift, when `value` is such a shift.
  const auto time_shift = [&](klee::ref<klee::Expr> value) -> std::optional<u64> {
    if (value->getKind() != klee::Expr::LShr || !is_constant(value->getKid(1))) {
      return {};
    }
    const std::optional<var_t> var = ingress_vars.get(value->getKid(0));
    if (!var || (var->name != "meta.time" && var->name != "eg_md.time")) {
      return {};
    }
    const u64 shift = solver_toolbox.value_from_expr(value->getKid(1));
    if (shift < 16) {
      return {};
    }
    return shift - 16;
  };

  // 1. The steps of the run.
  const std::vector<const EPNode *> steps = compute_run_steps(first);

  if (steps.empty() || emitted_compute_steps.contains(first)) {
    return; // Emitted with an earlier step of this run.
  }
  emitted_compute_steps.insert(steps.begin(), steps.end());

  // Ops that reuse another path's action (TofinoContext::reuse_compute_op): nothing to declare
  // or compute here, their output is the original's variable, but the shared action is called
  // from this branch too.
  std::unordered_set<std::string> reused_op_ids;
  std::vector<DS_ID> reused_actions;

  // The ops of this run, by id: a move (TofinoContext::get_compute_moves) anchored on one of
  // them is this run's to emit, first.
  std::unordered_set<std::string> run_op_ids;
  for (const EPNode *step : steps) {
    const Module *module = step->get_module();
    switch (module->get_type()) {
    case ModuleType::Tofino_ArithmeticOp: {
      const Tofino::ArithmeticOp *op = dynamic_cast<const Tofino::ArithmeticOp *>(module);
      run_op_ids.insert(op->get_op_id());
      for (const compute_operand_t &operand : op->get_operands()) {
        run_op_ids.insert(operand.op_id);
      }
    } break;
    case ModuleType::Tofino_RotateLeft: {
      const Tofino::RotateLeft *rot = dynamic_cast<const Tofino::RotateLeft *>(module);
      run_op_ids.insert(rot->get_op_id());
      for (const compute_operand_t &operand : rot->get_operands()) {
        run_op_ids.insert(operand.op_id);
      }
    } break;
    case ModuleType::Tofino_RotateLeftShifts: {
      const Tofino::RotateLeftShifts *rot = dynamic_cast<const Tofino::RotateLeftShifts *>(module);
      run_op_ids.insert(rot->get_shl_op_id());
      run_op_ids.insert(rot->get_shr_op_id());
      run_op_ids.insert(rot->get_or_op_id());
      for (const compute_operand_t &operand : rot->get_operands()) {
        run_op_ids.insert(operand.op_id);
      }
    } break;
    case ModuleType::Tofino_If: {
      for (const compute_operand_t &operand : dynamic_cast<const Tofino::If *>(module)->get_materialized_operands()) {
        run_op_ids.insert(operand.op_id);
      }
    } break;
    default:
      break;
    }
  }

  // 2. Every op gets its output variable first, so the statements can refer to each other's
  // results whatever the order.
  std::vector<op_emission_t> ops;
  std::unordered_map<std::string, var_t> out_vars; // By op id.

  // Both idempotent by name: a shared op's variable is declared by whichever path is emitted
  // first, and found by the other.
  const auto existing_out_var = [&](const std::string &op_id) -> std::optional<var_t> {
    return ingress_vars.get((in_egress ? "eg_md." : "meta.") + op_id + "_out");
  };
  // A value's slot of the state header (plan_value_homes), declared on this path if not yet.
  const auto slot_var = [&](const std::string &op_id, klee::ref<klee::Expr> expr) -> std::optional<var_t> {
    auto found_it = slot_fields.find(op_id);
    if (found_it == slot_fields.end()) {
      return {};
    }
    var_t var         = found_it->second;
    var.expr          = expr.isNull() ? var.expr : expr;
    var.original_expr = var.expr; // Not a slice of anything: is_slice() compares the two, and a null one crashes.
    if (!var.expr.isNull()) {
      if (const std::optional<var_t> existing = ingress_vars.get(var.expr)) {
        if (existing->name == var.name) {
          out_vars.insert({op_id, *existing});
          return *existing;
        }
      }
    }
    ingress_vars.insert_back(var, /*allow_duplicates=*/true);
    out_vars.insert({op_id, var});
    return var;
  };
  const auto out_var = [&](const std::string &op_id, klee::ref<klee::Expr> expr, bool transient = false) -> var_t {
    if (const std::optional<var_t> slot = slot_var(op_id, expr)) {
      return *slot;
    }
    if (const std::optional<var_t> existing = existing_out_var(op_id)) {
      out_vars.insert({op_id, *existing});
      return *existing;
    }
    var_t var     = alloc_var(op_id + "_out", expr, EXACT_NAME | IS_INGRESS_METADATA | SKIP_STACK_ALLOC);
    var.transient = transient;
    ingress_vars.insert_back(var);
    declare_var_in_ingress_metadata(var);
    out_vars.insert({op_id, var});
    return var;
  };
  const auto out_var_sized = [&](const std::string &op_id, bits_t width) -> var_t {
    if (const std::optional<var_t> slot = slot_var(op_id, nullptr)) {
      return *slot;
    }
    if (const std::optional<var_t> existing = existing_out_var(op_id)) {
      out_vars.insert({op_id, *existing});
      return *existing;
    }
    var_t var     = alloc_var(op_id + "_out", width, EXACT_NAME | IS_INGRESS_METADATA | SKIP_STACK_ALLOC);
    var.transient = true;
    ingress_vars.insert_back(var);
    declare_var_in_ingress_metadata(var);
    out_vars.insert({op_id, var});
    return var;
  };
  // The variable of a shift of the clock: the 32-bit time the data plane keeps, shifted by the
  // rest (see time_shift), whatever the width of the symbol. Idempotent by name, like out_var.
  const auto time_shift_out_var = [&](const std::string &op_id, klee::ref<klee::Expr> out) -> var_t {
    if (const std::optional<var_t> slot = slot_var(op_id, out)) {
      return *slot;
    }
    const code_t name = (in_egress ? "eg_md." : "meta.") + op_id + "_out";
    if (const std::optional<var_t> existing = ingress_vars.get(name)) {
      out_vars.insert({op_id, *existing});
      return *existing;
    }
    const var_t var(name, out, 32, false, false, false);
    ingress_vars.insert_back(var);
    declare_var_in_ingress_metadata(var);
    out_vars.insert({op_id, var});
    return var;
  };
  // An op whose output was unified with another path's symbol writes that symbol's variable
  // (declared here if that path has not been emitted yet); its own symbol is another name for it.
  const auto bind_output_alias = [&](const std::string &op_id, klee::ref<klee::Expr> module_out) -> bool {
    const std::optional<klee::ref<klee::Expr>> theirs = tofino_ctx->get_output_alias(op_id);
    if (!theirs) {
      return false;
    }
    const std::optional<std::string> producer = tofino_ctx->get_producer(*theirs);
    assert(producer && "An output alias to a symbol no op produces");
    const var_t var = out_var(*producer, *theirs);
    out_vars.insert({op_id, var});
    if (!module_out.isNull()) {
      const var_t alias(var.name, module_out, module_out->getWidth(), false, var.is_header_field, false);
      ingress_vars.insert_back(alias, /*allow_duplicates=*/true);
    }
    return true;
  };
  // Whether an op's value is computed by an action this run calls anyway: its output was unified
  // with another path's value, and that value's op is in one of the actions reused here. The
  // step would run twice (the hand ladder's path B ran its first round step next to the shared
  // chain's), so its own statement is not emitted.
  const auto computed_by_a_called_action = [&](const std::string &op_id) -> bool {
    const std::optional<klee::ref<klee::Expr>> theirs = tofino_ctx->get_output_alias(op_id);
    if (!theirs) {
      return false;
    }
    const std::optional<std::string> producer = tofino_ctx->get_producer(*theirs);
    if (!producer) {
      return false;
    }
    for (const DS_ID &action_id : reused_actions) {
      const Tofino::ComputeAction *action = dynamic_cast<const Tofino::ComputeAction *>(tofino_ctx->get_data_structures().get_ds_from_id(action_id));
      for (const compute_op_t &op : action->ops) {
        if (op.id == *producer) {
          return true;
        }
      }
    }
    return false;
  };
  // A reused op: its variable is the original's (declared here if the original's path has not
  // been emitted yet), and the module's own output symbol is another name for it.
  const auto bind_reused = [&](const std::string &op_id, klee::ref<klee::Expr> module_out) -> bool {
    const std::optional<compute_reuse_t> reuse = tofino_ctx->get_compute_reuse(op_id);
    if (!reuse) {
      return false;
    }
    const compute_op_t &original = reuse->op;
    const bool shifts_time       = original.fn.rfind("op_", 0) == 0 && !original.args.empty() && time_shift(original.args.at(0)).has_value();
    const bool is_operand        = original.out.isNull() && original.fn != "shl" && original.fn != "shr";
    var_t var                    = shifts_time              ? time_shift_out_var(original.id, original.out)
                                   : !original.out.isNull() ? out_var(original.id, original.out)
                                   : is_operand             ? out_var(original.id, original.args.at(0), /*transient=*/true)
                                                            : out_var_sized(original.id, original.width);
    out_vars.insert({op_id, var});
    if (!module_out.isNull()) {
      const var_t alias(var.name, module_out, module_out->getWidth(), false, var.is_header_field, false);
      ingress_vars.insert_back(alias, /*allow_duplicates=*/true);
    }
    reused_op_ids.insert(op_id);
    // An action of another path is called here and declared with that path; one of this very
    // path (the op named a value computed by a rotate's operand) is this run's own.
    if (!tofino_ctx->is_own_path_reuse(op_id) && std::find(reused_actions.begin(), reused_actions.end(), reuse->action) == reused_actions.end()) {
      reused_actions.push_back(reuse->action);
    }
    return true;
  };
  // The right-hand side of each operand's definition, transpiled before its variable exists.
  //
  // out_var -> alloc_var registers the variable keyed on the very expression it computes, and the
  // statements are emitted in a second pass over the steps, by which point the lookup finds that
  // variable and the definition comes out as `X = X`. Measured on SmartCookie: 88 values -- every
  // rotate operand, i.e. the whole SipHash chain -- were never computed, so the chain evaluated to
  // zero and every cookie was zero. Transpiling here is also the correct scope: only earlier steps
  // have been declared, and an operand can only depend on those.
  std::unordered_map<std::string, code_t> out_var_rhs; // By op id.

  // What the placed op `op_id` reads now: an operand a shape match rewrote to a shared field
  // (TofinoContext::rewrite_compute_op) is transpiled as that field.
  const auto rw = [&](const std::string &op_id, klee::ref<klee::Expr> expr) { return tofino_ctx->apply_rewrites(op_id, expr); };
  // The code computing an operand. A zero-extension of the top bits of a field as wide as the
  // result, (bit<32>)(f[31:16]), comes out as f >> 16: the cast is a deposit of a rotated source
  // with a constant fill, which Tofino 2 refuses once a hash-unit rotate has cut the destination
  // into slices; the shift is one plain ALU op.
  // Whether the placed op `op_id` of `action_id` is computed by the hash unit (emitted in @in_hash).
  const auto op_in_hash = [&](const DS_ID &action_id, const std::string &op_id) -> bool {
    const Tofino::ComputeAction *action = dynamic_cast<const Tofino::ComputeAction *>(tofino_ctx->get_data_structures().get_ds_from_id(action_id));
    if (!action) {
      return false;
    }
    for (const compute_op_t &op : action->ops) {
      if (op.id == op_id) {
        return op.in_hash;
      }
    }
    return false;
  };
  const auto operand_rhs = [&](const compute_operand_t &operand) -> code_t {
    klee::ref<klee::Expr> expr = rw(operand.op_id, operand.expr);
    const code_t code          = transpiler.transpile(expr);
    if (expr->getKind() != klee::Expr::ZExt || op_in_hash(operand.action_id, operand.op_id)) {
      return code; // The hash unit takes the cast as it is.
    }
    static const std::regex top_slice(R"(^\(bit<(\d+)>\)\(([A-Za-z_][\w.]*)\[(\d+):(\d+)\]\)$)");
    std::smatch m;
    if (!std::regex_match(code, m, top_slice)) {
      return code;
    }
    const bits_t width               = std::stoul(m[1]);
    const bits_t hi                  = std::stoul(m[3]);
    const bits_t lo                  = std::stoul(m[4]);
    const std::optional<var_t> field = ingress_vars.get(m[2].str());
    if (!field || field->size != width || hi + 1 != width || lo == 0) {
      return code;
    }
    return m[2].str() + " >> " + std::to_string(lo);
  };
  const auto computed_operands = [&](const std::vector<compute_operand_t> &operands) {
    for (const compute_operand_t &operand : operands) {
      if (bind_reused(operand.op_id, nullptr)) {
        continue;
      }
      // A value some field holds already needs no computing -- unless the operand is a load by
      // the hash unit, whose whole point is the copy of that field into a slot of the chain. The
      // slot is the chain's alone: it is not registered under the value's expression, or a
      // header write or a condition elsewhere would find the slot for that value and tie its own
      // metadata to the chain's cluster with an ALU op.
      // An operand with a slot of its own is computed into it for the same reason as an op's
      // value above: the planner's word pairs hold only if the statements read the words it chose.
      if (op_in_hash(operand.action_id, operand.op_id)) {
        out_var_rhs[operand.op_id] = operand_rhs(operand);
        out_var_sized(operand.op_id, operand.expr->getWidth());
      } else if (slot_fields.contains(operand.op_id) || !ingress_vars.get(operand.expr)) {
        out_var_rhs[operand.op_id] = operand_rhs(operand);
        out_var(operand.op_id, operand.expr, /*transient=*/true);
      }
    }
  };
  // The code reading `expr` as an operand: the variable computed for it in this run, else the
  // plain value (a constant or a variable).
  const auto operand_code = [&](klee::ref<klee::Expr> expr, const std::vector<compute_operand_t> &operands) -> code_t {
    for (const compute_operand_t &operand : operands) {
      if (solver_toolbox.are_exprs_always_equal(operand.expr, expr) || solver_toolbox.are_exprs_always_equal(rw(operand.op_id, operand.expr), expr)) {
        auto found_it = out_vars.find(operand.op_id);
        if (found_it != out_vars.end()) {
          return found_it->second.name;
        }
      }
    }
    return transpiler.transpile(expr);
  };
  const auto computed_operand_statements = [&](const std::vector<compute_operand_t> &operands) {
    for (const compute_operand_t &operand : operands) {
      auto found_it = out_vars.find(operand.op_id);
      if (found_it == out_vars.end() || reused_op_ids.contains(operand.op_id)) {
        continue; // Held elsewhere already.
      }
      const auto rhs_it = out_var_rhs.find(operand.op_id);
      const code_t rhs  = rhs_it != out_var_rhs.end() ? rhs_it->second : operand_rhs(operand);
      ops.push_back({operand.action_id, operand.op_id, found_it->second.name + " = " + rhs + ";", op_in_hash(operand.action_id, operand.op_id)});
    }
  };

  // The moves this run owns: the shared field, declared here if its producer's path has not
  // been emitted yet, takes the plain value, ahead of everything the run computes.
  for (const compute_move_t &move : tofino_ctx->get_compute_moves()) {
    if (!run_op_ids.contains(move.anchor)) {
      continue;
    }
    const std::optional<std::string> producer = tofino_ctx->get_producer(move.to);
    assert(producer && "A move into a field no op produces");
    const var_t to_var = out_var(*producer, move.to);
    ops.push_back({move.action, move.op_id, to_var.name + " = " + transpiler.transpile(move.from) + ";", op_in_hash(move.action, move.op_id)});
  }

  for (const EPNode *step : steps) {
    const Module *module = step->get_module();
    switch (module->get_type()) {
    case ModuleType::Tofino_ArithmeticOp: {
      const Tofino::ArithmeticOp *op = dynamic_cast<const Tofino::ArithmeticOp *>(module);
      computed_operands(op->get_operands());
      if (bind_reused(op->get_op_id(), op->get_out()) || bind_output_alias(op->get_op_id(), op->get_out())) {
        break;
      }
      if (time_shift(op->get_value())) {
        time_shift_out_var(op->get_op_id(), op->get_out());
        break;
      }
      // Another module may already hold this very value (e.g. a register's returned new
      // value); the result is then just another name for it. Unless the action is shared with
      // another path, which calls it for this very op and reads its variable -- or the value has
      // a slot of the state header: the planner chose that slot knowing which words the op reads
      // and which read it (plan_value_homes), and a reader sent to another word that happens to
      // hold the same value would take it at a rotation the planner never saw.
      const bool planned = slot_fields.contains(op->get_op_id());
      if (std::optional<var_t> held =
              tofino_ctx->is_shared_compute_action(op->get_action_id()) || planned ? std::nullopt : ingress_vars.get(op->get_value())) {
        const var_t alias(held->name, op->get_out(), op->get_out()->getWidth(), false, held->is_header_field, false);
        ingress_vars.insert_back(alias, /*allow_duplicates=*/true);
        continue;
      }
      out_var(op->get_op_id(), op->get_out());
    } break;
    case ModuleType::Tofino_RotateLeft: {
      const Tofino::RotateLeft *rot = dynamic_cast<const Tofino::RotateLeft *>(module);
      computed_operands(rot->get_operands());
      if (bind_reused(rot->get_op_id(), rot->get_out()) || bind_output_alias(rot->get_op_id(), rot->get_out())) {
        break;
      }
      out_var(rot->get_op_id(), rot->get_out());
    } break;
    case ModuleType::Tofino_RotateLeftShifts: {
      const Tofino::RotateLeftShifts *rot = dynamic_cast<const Tofino::RotateLeftShifts *>(module);
      computed_operands(rot->get_operands());
      if (!bind_reused(rot->get_shl_op_id(), nullptr)) {
        out_var_sized(rot->get_shl_op_id(), rot->get_out()->getWidth());
      }
      if (!bind_reused(rot->get_shr_op_id(), nullptr)) {
        out_var_sized(rot->get_shr_op_id(), rot->get_out()->getWidth());
      }
      if (!bind_reused(rot->get_or_op_id(), rot->get_out()) && !bind_output_alias(rot->get_or_op_id(), rot->get_out())) {
        out_var(rot->get_or_op_id(), rot->get_out());
      }
    } break;
    case ModuleType::Tofino_If: {
      computed_operands(dynamic_cast<const Tofino::If *>(module)->get_materialized_operands());
    } break;
    default:
      panic("Not a compute step: %s", module->get_name().c_str());
    }
  }

  // 3. The statements.
  for (const EPNode *step : steps) {
    const Module *module = step->get_module();
    switch (module->get_type()) {
    case ModuleType::Tofino_ArithmeticOp: {
      const Tofino::ArithmeticOp *op = dynamic_cast<const Tofino::ArithmeticOp *>(module);
      computed_operand_statements(op->get_operands());
      auto found_it = out_vars.find(op->get_op_id());
      if (found_it == out_vars.end() || reused_op_ids.contains(op->get_op_id()) || computed_by_a_called_action(op->get_op_id())) {
        continue; // Aliased, or computed by another path's action.
      }
      if (const std::optional<u64> shift = time_shift(op->get_value())) {
        // The copy of the clock reads it through the hash unit, as the ground truth does: an ALU
        // op reading the intrinsic's metadata field counts one PHV source per slice once the hash
        // chain the clock feeds has cut its destination into slices (Tofino::copies_clock).
        const code_t time = in_egress ? "eg_md.time" : "meta.time";
        const code_t rhs  = *shift == 0 ? time : time + " >> " + std::to_string(*shift);
        ops.push_back({op->get_action_id(), op->get_op_id(), found_it->second.name + " = " + rhs + ";", *shift == 0});
        break;
      }
      std::vector<code_t> operands;
      for (unsigned i = 0; i < op->get_value()->getNumKids(); i++) {
        operands.push_back(operand_code(rw(op->get_op_id(), op->get_value()->getKid(i)), op->get_operands()));
      }
      ops.push_back({op->get_action_id(), op->get_op_id(), found_it->second.name + " = " + arithmetic_code(op->get_value(), operands) + ";",
                     op_in_hash(op->get_action_id(), op->get_op_id())});
    } break;
    case ModuleType::Tofino_RotateLeft: {
      const Tofino::RotateLeft *rot = dynamic_cast<const Tofino::RotateLeft *>(module);
      computed_operand_statements(rot->get_operands());
      if (reused_op_ids.contains(rot->get_op_id()) || computed_by_a_called_action(rot->get_op_id())) {
        continue; // Computed by another path's action.
      }
      const var_t &out          = out_vars.at(rot->get_op_id());
      const bits_t width        = rot->get_out()->getWidth();
      const u32 n               = rot->get_amount();
      klee::ref<klee::Expr> x_r = rw(rot->get_op_id(), rot->get_x());
      if (is_constant(x_r)) {
        // Rotating a constant is a constant.
        const u64 x    = solver_toolbox.value_from_expr(x_r);
        const u64 mask = (width == 64) ? ~0ull : ((1ull << width) - 1);
        const u64 r    = n == 0 ? x : (((x << n) | (x >> (width - n))) & mask);
        ops.push_back({rot->get_action_id(), rot->get_op_id(), out.name + " = " + std::to_string(width) + "w" + std::to_string(r) + ";", false});
        break;
      }
      const code_t a = operand_code(x_r, rot->get_operands());
      // A concat rotate cuts its operand's container. Remember which values that happens to, so a
      // later copy of one into a deparsed header field can be routed through the hash unit while
      // the uncut ones stay ordinary moves -- each @in_hash costs hash-distribution units, and
      // only three 32-bit ones fit per stage, so wrapping indiscriminately buys PHV with stages.
      cut_values.insert(a);
      ops.push_back({rot->get_action_id(), rot->get_op_id(), out.name + " = " + rotation_code(a, n, width) + ";", rot->uses_hash_unit()});
    } break;
    case ModuleType::Tofino_RotateLeftShifts: {
      const Tofino::RotateLeftShifts *rot = dynamic_cast<const Tofino::RotateLeftShifts *>(module);
      computed_operand_statements(rot->get_operands());
      const bits_t width = rot->get_out()->getWidth();
      const u32 n        = rot->get_amount();
      const code_t a     = operand_code(rw(rot->get_or_op_id(), rot->get_x()), rot->get_operands());
      const var_t &shl   = out_vars.at(rot->get_shl_op_id());
      const var_t &shr   = out_vars.at(rot->get_shr_op_id());
      const var_t &out   = out_vars.at(rot->get_or_op_id());
      if (!reused_op_ids.contains(rot->get_shl_op_id())) {
        ops.push_back({rot->get_shl_action_id(), rot->get_shl_op_id(), shl.name + " = " + a + " << " + std::to_string(n) + ";", false});
      }
      if (!reused_op_ids.contains(rot->get_shr_op_id())) {
        ops.push_back({rot->get_shr_action_id(), rot->get_shr_op_id(), shr.name + " = " + a + " >> " + std::to_string(width - n) + ";", false});
      }
      if (!reused_op_ids.contains(rot->get_or_op_id())) {
        ops.push_back({rot->get_or_action_id(), rot->get_or_op_id(), out.name + " = " + shl.name + " | " + shr.name + ";", false});
      }
    } break;
    case ModuleType::Tofino_If: {
      computed_operand_statements(dynamic_cast<const Tofino::If *>(module)->get_materialized_operands());
    } break;
    default:
      break;
    }
  }

  // 3b. A chain of xors leaving the chain through the hash unit is one hash op: xor is
  // associative, a hash-unit op reads as many words as it likes, and a chain of them costs a
  // level and two hash-distribution units a link (the ground truth writes its cookie as one
  // @in_hash xor of the whole state). An intermediate folds into its reader when nothing else
  // reads it: no other statement here, no module after the run.
  {
    static const std::regex xor_chain(R"(^([\w.]+) = ([\w.\[\]:]+(?: \^ [\w.\[\]:]+)*);$)");
    const auto symbols_of = [&](const std::string &op_id) -> std::vector<std::string> {
      std::vector<std::string> names;
      for (const EPNode *step : steps) {
        const Module *module = step->get_module();
        if (module->get_type() != ModuleType::Tofino_ArithmeticOp || dynamic_cast<const Tofino::ArithmeticOp *>(module)->get_op_id() != op_id) {
          continue;
        }
        if (const LibBDD::Call *call = dynamic_cast<const LibBDD::Call *>(module->get_node())) {
          for (const symbol_t &symbol : call->get_local_symbols().get()) {
            names.push_back(symbol.name);
          }
        }
      }
      return names;
    };
    const auto read_after_the_run = [&](const std::vector<std::string> &names) -> bool {
      std::vector<const EPNode *> pending(steps.back()->get_children().begin(), steps.back()->get_children().end());
      while (!pending.empty()) {
        const EPNode *ep_node = pending.back();
        pending.pop_back();
        if (const Module *module = ep_node->get_module()) {
          Symbols used = module->get_node() ? module->get_node()->get_used_symbols() : Symbols();
          if (module->get_type() == ModuleType::Tofino_SendToController) {
            for (const symbol_t &symbol : dynamic_cast<const Tofino::SendToController *>(module)->get_symbols().get()) {
              used.add(symbol);
            }
          }
          for (const std::string &name : names) {
            if (used.has(name)) {
              return true;
            }
          }
        }
        pending.insert(pending.end(), ep_node->get_children().begin(), ep_node->get_children().end());
      }
      return false;
    };
    for (size_t i = 0; i < ops.size(); i++) {
      std::smatch m;
      if (!ops[i].in_hash || !std::regex_match(ops[i].statement, m, xor_chain) || m[2].str().find(" ^ ") == std::string::npos) {
        continue;
      }
      const code_t out = m[1].str();
      if (out.rfind("hdr.st.", 0) == 0) {
        continue; // A state word: the chain reads it.
      }
      size_t reader = ops.size();
      size_t reads  = 0;
      for (size_t j = 0; j < ops.size(); j++) {
        if (j == i) {
          continue;
        }
        const code_t &s = ops[j].statement;
        for (size_t at = s.find(out); at != std::string::npos; at = s.find(out, at + out.size())) {
          const bool whole = (at == 0 || !(std::isalnum(s[at - 1]) || s[at - 1] == '_' || s[at - 1] == '.')) &&
                             (at + out.size() >= s.size() || !(std::isalnum(s[at + out.size()]) || s[at + out.size()] == '_'));
          if (whole) {
            reads++;
            reader = j;
          }
        }
      }
      if (reads != 1 || !ops[reader].in_hash || !std::regex_match(ops[reader].statement, xor_chain) || read_after_the_run(symbols_of(ops[i].op_id))) {
        continue;
      }
      code_t &target  = ops[reader].statement;
      const size_t at = target.find(out);
      target.replace(at, out.size(), m[2].str());
      ops.erase(ops.begin() + i);
      i = (size_t)-1; // Start over: a folded reader may fold on.
    }
  }

  // 4. One bare action per ComputeAction, in stage order, its ops in the order they were
  // appended (the data structure's), called from the apply block in that same order.
  std::vector<DS_ID> action_ids;
  if (Walk::enabled()) {
    // The words a statement reads must be the ones the planner chose for the op: its word pairs
    // hold only then. A difference here is an emitter lookup the planner did not foresee.
    static const std::regex word(R"(hdr\.st\.s\d+_\d+)");
    for (const op_emission_t &op : ops) {
      auto planned_it = planned_sources.find(op.op_id);
      if (planned_it == planned_sources.end() || op.statement.empty() || op.in_hash) {
        continue;
      }
      const size_t eq = op.statement.find(" = ");
      std::set<code_t> emitted, planned(planned_it->second.begin(), planned_it->second.end());
      for (std::sregex_iterator it(op.statement.begin() + (eq == std::string::npos ? 0 : eq), op.statement.end(), word), end; it != end; ++it) {
        emitted.insert(it->str());
      }
      auto own_it      = slot_fields.find(op.op_id);
      const code_t own = own_it != slot_fields.end() ? own_it->second.name : "";
      if (emitted != planned || (eq != std::string::npos && !own.empty() && op.statement.substr(0, eq) != own)) {
        std::cerr << "[homes] mismatch " << op.op_id << " planned " << own << " <-";
        for (const code_t &name : planned) {
          std::cerr << " " << name;
        }
        std::cerr << " | emitted: " << op.statement << "\n";
      }
    }
  }
  for (const op_emission_t &op : ops) {
    if (std::find(action_ids.begin(), action_ids.end(), op.action_id) == action_ids.end()) {
      action_ids.push_back(op.action_id);
    }
  }
  for (const DS_ID &action_id : reused_actions) {
    if (std::find(action_ids.begin(), action_ids.end(), action_id) == action_ids.end()) {
      action_ids.push_back(action_id);
    }
  }
  std::stable_sort(action_ids.begin(), action_ids.end(),
                   [&pipeline](const DS_ID &a, const DS_ID &b) { return pipeline.get_placed_stage(a) < pipeline.get_placed_stage(b); });

  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  // A site of a shared run: the run's actions are declared here when they are this path's, but
  // called once, at the run's join (plan_shared_runs). The chain's first action marks the site:
  // the flag the join tests, or the marker the ladder splits the block at.
  std::unordered_set<DS_ID> hoisted;
  const auto site_runs_it = shared_runs_by_site.find(first);
  if (site_runs_it != shared_runs_by_site.end()) {
    for (const size_t r : site_runs_it->second) {
      hoisted.insert(shared_runs[r].actions.begin(), shared_runs[r].actions.end());
    }
  }

  for (const DS_ID &action_id : action_ids) {
    const Tofino::ComputeAction *action = dynamic_cast<const Tofino::ComputeAction *>(tofino_ctx->get_data_structures().get_ds_from_id(action_id));
    assert(action && "Compute step placed outside a ComputeAction");

    const bool hoist = hoisted.contains(action_id);
    if (hoist) {
      for (const size_t r : site_runs_it->second) {
        const shared_run_t &run = shared_runs[r];
        if (run.actions.front() != action_id) {
          continue;
        }
        ingress_apply.indent();
        if (run.join) {
          ingress_apply << run.flag << " = 1;\n";
        } else {
          ingress_apply << "// @shared-run " << (i64)r << "\n";
        }
      }
    }

    if (std::find(reused_actions.begin(), reused_actions.end(), action_id) != reused_actions.end()) {
      // Another path's action, declared with that path (before or after this one: declarations
      // and the apply block are separate sections). Called here with the same spill into
      // one-@in_hash companions its declaration makes, derived from the action's ops.
      if (!hoist) {
        // The part of the action this run runs: its reused ops, by the ids the original path gave
        // them. The rest is the other path's, and does not run here.
        std::unordered_set<std::string> canonical;
        for (const std::string &op_id : reused_op_ids) {
          if (const std::optional<compute_reuse_t> reuse = tofino_ctx->get_compute_reuse(op_id)) {
            canonical.insert(reuse->op.id);
          }
        }
        std::vector<compute_op_t> part;
        std::vector<std::string> part_ids;
        for (const compute_op_t &op : action->ops) {
          if (canonical.contains(op.id)) {
            part.push_back(op);
            part_ids.push_back(op.id);
          }
        }
        code_t name = action_id;
        if (!part.empty() && part.size() < action->ops.size()) {
          std::vector<std::vector<std::string>> &parts = action_variants[action_id];
          const auto part_it                           = std::find(parts.begin(), parts.end(), part_ids);
          const size_t k                               = part_it - parts.begin();
          if (part_it == parts.end()) {
            parts.push_back(part_ids);
          }
          name = action_id + "_v" + std::to_string(k);
        } else {
          part = action->ops;
        }
        for (const code_t &call : action_calls(name, part)) {
          ingress_apply.indent();
          ingress_apply << call << "\n";
        }
      }
      continue;
    }

    declared_ds.insert(action_id);

    // This path's action: its statements, kept for the variants other paths call in part
    // (emit_action_variants), declared here and called unless a shared run calls it at its join.
    // A statement touching no state word is an ALU statement, whatever the op's kind: the hash
    // unit's point is to keep an outside value or a hash-unit output off the chain's sliced
    // cluster, and a statement between metadata and carried fields touches that cluster nowhere.
    std::unordered_map<std::string, action_statement_t> &statements = action_statements[action_id];
    for (const op_emission_t &emission : ops) {
      if (emission.action_id == action_id && !emission.statement.empty()) {
        statements[emission.op_id] = {emission.statement, emission.in_hash && emission.statement.find("hdr.st.") != code_t::npos};
      }
    }
    action_in_egress[action_id] = in_egress;
    if (statements.empty()) {
      continue;
    }
    declare_compute_action(ingress, action_id, action->ops, statements);
    if (!hoist) {
      for (const code_t &call : action_calls(action_id, action->ops)) {
        ingress_apply.indent();
        ingress_apply << call << "\n";
      }
    }
  }

  for (const DS_ID &action_id : action_ids) {
    declared_ds.insert(action_id);
  }
}

EPVisitor::Action TofinoSynthesizer::visit(const EP *ep, const EPNode *ep_node, const Tofino::Divide *node) {
  const DS_ID reg_id          = node->get_reg_id();
  klee::ref<klee::Expr> denom = node->get_denominator();
  const u64 numer             = node->get_numerator();
  klee::ref<klee::Expr> quot  = node->get_quotient();

  const Register *reg = get_tofino_ds<Register>(ep, reg_id);

  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const code_t value_type = Transpiler::type_from_size(reg->value_size);
  const code_t index_type = Transpiler::type_from_size(reg->index_size);

  const var_t denom_var = alloc_var(reg_id + "_denom", denom, SKIP_STACK_ALLOC | EXACT_NAME | IS_INGRESS_METADATA);
  declare_var_in_ingress_metadata(denom_var);

  const code_t math_unit = reg_id + "_mu";
  const code_t action    = reg_id + "_calc";

  ingress.indent();
  ingress << "MathUnit<" << value_type << ">(MathOp_t.DIV, " << numer << ") " << math_unit << ";\n";

  transpile_register_decl(reg);

  ingress.indent();
  ingress << "RegisterAction<" << value_type << ", " << index_type << ", " << value_type << ">(" << reg_id << ") " << action << " = {\n";
  ingress.inc();
  ingress.indent();
  ingress << "void apply(inout " << value_type << " value, out " << value_type << " out_value) {\n";
  ingress.inc();
  ingress.indent();
  ingress << "value = " << math_unit << ".execute(" << denom_var.name << ");\n";
  ingress.indent();
  ingress << "out_value = value;\n";
  ingress.dec();
  ingress.indent();
  ingress << "}\n";
  ingress.dec();
  ingress.indent();
  ingress << "};\n";
  ingress << "\n";

  ingress_apply.indent();
  ingress_apply << denom_var.name << " = " << transpiler.transpile(denom) << ";\n";

  const var_t quot_var = alloc_var("quotient", quot);
  quot_var.declare(ingress_apply, action + ".execute(0)");

  return EPVisitor::Action::doChildren;
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