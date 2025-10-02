#include <LibCore/Expr.h>
#include <LibCore/Symbol.h>
#include <LibCore/Solver.h>
#include <LibCore/Debug.h>
#include <LibCore/Types.h>

#include <iostream>
#include <regex>
#include <unordered_map>

#include <klee/util/ExprVisitor.h>

namespace LibCore {

namespace {

const std::unordered_set<klee::Expr::Kind> conditional_expr_kinds{
    klee::Expr::Or,  klee::Expr::And, klee::Expr::Eq,  klee::Expr::Ne,  klee::Expr::Ult, klee::Expr::Ule,
    klee::Expr::Ugt, klee::Expr::Uge, klee::Expr::Slt, klee::Expr::Sle, klee::Expr::Sgt, klee::Expr::Sge,
};

struct expr_info_t {
  bool has_allowed;
  bool has_not_allowed;
};

class ExpressionFilter : public klee::ExprVisitor::ExprVisitor {
private:
  std::vector<std::string> allowed_symbols;

public:
  ExpressionFilter(const std::vector<std::string> &_allowed_symbols) : ExprVisitor(true), allowed_symbols(_allowed_symbols) {}

  expr_info_t check_symbols(klee::ref<klee::Expr> expr) const {
    expr_info_t info{false, false};

    std::unordered_set<std::string> symbols = symbol_t::get_symbols_names(expr);

    for (auto s : symbols) {
      auto found_it = std::find(allowed_symbols.begin(), allowed_symbols.end(), s);

      if (found_it != allowed_symbols.end()) {
        info.has_allowed = true;
      } else {
        info.has_not_allowed = true;
      }
    }

    return info;
  }

  // Acts only if either lhs or rhs has **only** allowed symbols, while the
  // other has not allowed symbols. Returns the expression with allowed symbols.
  klee::ref<klee::Expr> pick_filtered_expression(klee::ref<klee::Expr> lhs, klee::ref<klee::Expr> rhs) const {
    expr_info_t lhs_info = check_symbols(lhs);
    expr_info_t rhs_info = check_symbols(rhs);

    if (lhs_info.has_allowed && !lhs_info.has_not_allowed && rhs_info.has_not_allowed) {
      return lhs;
    }

    if (rhs_info.has_allowed && !rhs_info.has_not_allowed && lhs_info.has_not_allowed) {
      return rhs;
    }

    return klee::ref<klee::Expr>();
  }

  Action visitAnd(const klee::AndExpr &e) {
    if (e.getNumKids() != 2)
      return Action::doChildren();
    klee::ref<klee::Expr> lhs      = e.getKid(0);
    klee::ref<klee::Expr> rhs      = e.getKid(1);
    klee::ref<klee::Expr> new_expr = pick_filtered_expression(lhs, rhs);
    if (new_expr.isNull())
      return Action::doChildren();
    return Action::changeTo(new_expr);
  }

  Action visitAnd(const klee::OrExpr &e) {
    if (e.getNumKids() != 2)
      return Action::doChildren();
    klee::ref<klee::Expr> lhs      = e.getKid(0);
    klee::ref<klee::Expr> rhs      = e.getKid(1);
    klee::ref<klee::Expr> new_expr = pick_filtered_expression(lhs, rhs);
    if (new_expr.isNull())
      return Action::doChildren();
    return Action::changeTo(new_expr);
  }
};

class SymbolicReadsRetriever : public klee::ExprVisitor::ExprVisitor {
private:
  std::optional<std::string> filter;
  symbolic_reads_t symbolic_reads;

public:
  SymbolicReadsRetriever() {}
  SymbolicReadsRetriever(std::optional<std::string> _filter) : filter(_filter) {}

  Action visitRead(const klee::ReadExpr &e) {
    assert(e.index->getKind() == klee::Expr::Kind::Constant && "Non-constant index");

    const klee::ConstantExpr *index_const = dynamic_cast<klee::ConstantExpr *>(e.index.get());
    const bytes_t byte                    = index_const->getZExtValue();
    const std::string name                = e.updates.root->name;

    if (!filter.has_value() || name == filter.value()) {
      symbolic_reads.insert({byte, name});
    }

    return Action::doChildren();
  }

  const symbolic_reads_t &get_symbolic_reads() const { return symbolic_reads; }
};

klee::ref<klee::Expr> concat_lsb(klee::ref<klee::Expr> expr, klee::ref<klee::Expr> byte) {
  if (expr->getKind() != klee::Expr::Concat) {
    return solver_toolbox.exprBuilder->Concat(expr, byte);
  }

  klee::ref<klee::Expr> lhs = expr->getKid(0);
  klee::ref<klee::Expr> rhs = expr->getKid(1);

  klee::ref<klee::Expr> new_kids[] = {lhs, concat_lsb(rhs, byte)};
  return expr->rebuild(new_kids);
}

class ExprPrettyPrinter : public klee::ExprVisitor::ExprVisitor {
private:
  std::string result;
  bool use_signed;

public:
  ExprPrettyPrinter(bool _use_signed) : ExprVisitor(false) { use_signed = _use_signed; }

  ExprPrettyPrinter() : ExprPrettyPrinter(false) {}

  static std::string print(klee::ref<klee::Expr> expr, bool use_signed = true) {
    assert(!expr.isNull() && "Null expr");

    if (expr->getKind() != klee::Expr::Kind::Constant) {
      ExprPrettyPrinter printer(use_signed);
      printer.visit(expr);
      return printer.get_result();
    }

    klee::ConstantExpr *constant = dynamic_cast<klee::ConstantExpr *>(expr.get());
    std::stringstream ss;

    if (use_signed) {
      switch (constant->getWidth()) {
      case klee::Expr::Bool: {
        bool value = constant->getZExtValue(1);
        ss << value;
        break;
      };
      case klee::Expr::Int8: {
        i8 value = constant->getZExtValue(8);
        ss << (int)value;
        break;
      };
      case klee::Expr::Int16: {
        i16 value = constant->getZExtValue(16);
        ss << (int)value;
        break;
      };
      case klee::Expr::Int32: {
        i32 value = constant->getZExtValue(32);
        ss << (int)value;
        break;
      };
      case klee::Expr::Int64: {
        i64 value = constant->getZExtValue(64);
        ss << (int)value;
        break;
      };
      default: {
        ss << expr_to_string(expr, true);
        break;
      };
      }
    } else {
      if (constant->getWidth() <= 64) {
        u64 value = constant->getZExtValue(constant->getWidth());
        ss << value;
      } else {
        ss << expr_to_string(expr, true);
      }
    }

    return ss.str();
  }

  const std::string &get_result() const { return result; }

  Action visitRead(const klee::ReadExpr &e) {
    klee::UpdateList ul         = e.updates;
    const klee::Array *root     = ul.root;
    klee::ref<klee::Expr> index = e.index;

    assert(index->getKind() == klee::Expr::Kind::Constant && "Non-constant index");
    klee::ConstantExpr *index_const = dynamic_cast<klee::ConstantExpr *>(index.get());
    u64 i                           = index_const->getZExtValue();

    std::stringstream ss;
    ss << root->name;
    ss << "[" << i << "]";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitSelect(const klee::SelectExpr &e) {
    std::stringstream ss;

    const std::string cond   = ExprPrettyPrinter::print(e.getKid(0), use_signed);
    const std::string first  = ExprPrettyPrinter::print(e.getKid(1), use_signed);
    const std::string second = ExprPrettyPrinter::print(e.getKid(2), use_signed);

    ss << cond << " ? " << first << " : " << second;
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitConcat(const klee::ConcatExpr &e) {
    klee::ref<klee::Expr> eref = const_cast<klee::ConcatExpr *>(&e);

    expr_groups_t groups = get_expr_groups(eref);

    if (groups.size() == 1 && groups[0].has_symbol && groups[0].offset == 0 && groups[0].size == (eref->getWidth() / 8)) {
      result = groups[0].symbol;
      return Action::skipChildren();
    }

    std::stringstream ss;

    for (auto i = 0u; i < groups.size(); i++) {
      if (i > 0) {
        ss << "++";
      }

      const expr_group_t &group = groups[i];

      if (!group.has_symbol) {
        ss << pretty_print_expr(group.expr);
      } else {
        ss << group.symbol;

        if (group.size == 1) {
          ss << "[" << group.offset << "]";
        } else {
          ss << "[";
          ss << group.offset;
          ss << ":";
          ss << (group.offset + group.size);
          ss << "]";
        }
      }
    }

    result = ss.str();
    return Action::skipChildren();
  }

  Action visitExtract(const klee::ExtractExpr &e) {
    klee::ref<klee::Expr> expr = e.getKid(0);
    const u32 offset_value     = e.offset;

    const std::string arg = ExprPrettyPrinter::print(expr, use_signed);

    if (offset_value == 0) {
      result = arg;
      return Action::skipChildren();
    }

    std::stringstream ss;
    ss << "(Extract " << offset_value << " " << arg << " )";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitZExt(const klee::ZExtExpr &e) {
    result = ExprPrettyPrinter::print(e.getKid(0), use_signed);
    return Action::skipChildren();
  }

  Action visitSExt(const klee::SExtExpr &e) {
    result = ExprPrettyPrinter::print(e.getKid(0), true);
    return Action::skipChildren();
  }

  Action visitAdd(const klee::AddExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), use_signed);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), use_signed);

    ss << "(" << left << " + " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitSub(const klee::SubExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), use_signed);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), use_signed);

    ss << "(" << left << " - " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitMul(const klee::MulExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), use_signed);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), use_signed);

    ss << "(" << left << " * " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitUDiv(const klee::UDivExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), use_signed);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), use_signed);

    ss << "(" << left << " / " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitSDiv(const klee::SDivExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), true);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), true);

    ss << "(" << left << " + " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitURem(const klee::URemExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), use_signed);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), use_signed);

    ss << "(" << left << " % " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitSRem(const klee::SRemExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), true);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), true);

    ss << "(" << left << " % " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitNot(const klee::NotExpr &e) {
    std::stringstream ss;

    const std::string arg = ExprPrettyPrinter::print(e.getKid(0), use_signed);

    ss << "!" << arg;
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitAnd(const klee::AndExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), use_signed);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), use_signed);

    ss << "(" << left << " & " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitOr(const klee::OrExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), use_signed);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), use_signed);

    ss << "(" << left << " | " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitXor(const klee::XorExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), use_signed);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), use_signed);

    ss << "(" << left << " ^ " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitShl(const klee::ShlExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), use_signed);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), use_signed);

    ss << "(" << left << " << " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitLShr(const klee::LShrExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), use_signed);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), use_signed);

    ss << "(" << left << " >> " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitAShr(const klee::AShrExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), true);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), true);

    ss << "(" << left << " >> " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitEq(const klee::EqExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), use_signed);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), use_signed);

    std::regex p0(R"(\(0 == (.+)\))");
    std::regex p1(R"(!(.+))");
    std::smatch m;

    if (left == "0" && (std::regex_match(right, m, p0) || std::regex_match(right, m, p1))) {
      ss << m.str(1);
    } else if (left == "0") {
      ss << "!" << right;
    } else {
      ss << "(" << left << " == " << right << ")";
    }

    result = ss.str();
    return Action::skipChildren();
  }

  Action visitNe(const klee::NeExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), use_signed);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), use_signed);

    ss << "(" << left << " != " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitUlt(const klee::UltExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), use_signed);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), use_signed);

    ss << "(" << left << " < " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitUle(const klee::UleExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), use_signed);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), use_signed);

    ss << "(" << left << " <= " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitUgt(const klee::UgtExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), use_signed);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), use_signed);

    ss << "(" << left << " > " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitUge(const klee::UgeExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), use_signed);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), use_signed);

    ss << "(" << left << " >= " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitSlt(const klee::SltExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), true);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), true);

    ss << "(" << left << " < " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitSle(const klee::SleExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), true);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), true);

    ss << "(" << left << " <= " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitSgt(const klee::SgtExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), true);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), true);

    ss << "(" << left << " > " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }

  Action visitSge(const klee::SgeExpr &e) {
    std::stringstream ss;

    const std::string left  = ExprPrettyPrinter::print(e.getKid(0), true);
    const std::string right = ExprPrettyPrinter::print(e.getKid(1), true);

    ss << "(" << left << " >= " << right << ")";
    result = ss.str();

    return Action::skipChildren();
  }
};

void remove_expr_str_labels(std::string &expr_str) {
  while (1) {
    size_t delim = expr_str.find(":");

    if (delim == std::string::npos) {
      break;
    }

    size_t start = delim;
    size_t end   = delim;

    while (expr_str[--start] != 'N') {
      assert(start > 0 && start < expr_str.size() && "Invalid start");
    }

    std::string pre  = expr_str.substr(0, start);
    std::string post = expr_str.substr(end + 1);

    std::string label_name = expr_str.substr(start, end - start);
    std::string label_expr;

    expr_str = pre + post;

    int parenthesis_lvl = 0;

    for (char c : post) {
      if (c == '(') {
        parenthesis_lvl++;
      } else if (c == ')') {
        parenthesis_lvl--;
      }

      label_expr += c;

      if (parenthesis_lvl == 0) {
        break;
      }
    }

    while (1) {
      delim = expr_str.find(label_name);

      if (delim == std::string::npos) {
        break;
      }

      size_t label_sz = label_name.size();

      if (delim + label_sz < expr_str.size() && expr_str[delim + label_sz] == ':') {
        pre  = expr_str.substr(0, delim);
        post = expr_str.substr(delim + label_sz + 1);

        expr_str = pre + post;
        continue;
      }

      pre  = expr_str.substr(0, delim);
      post = expr_str.substr(delim + label_sz);

      expr_str = pre + label_expr + post;
    }
  }
}

} // namespace

bool is_readLSB(klee::ref<klee::Expr> expr) {
  std::string symbol;
  return is_readLSB(expr, symbol);
}

bool is_readLSB(klee::ref<klee::Expr> expr, std::string &symbol) {
  assert(!expr.isNull() && "Null expr");

  if (expr->getKind() == klee::Expr::Read) {
    return true;
  }

  if (expr->getKind() != klee::Expr::Concat) {
    return false;
  }

  expr_groups_t groups = get_expr_groups(expr);

  if (groups.empty()) {
    return false;
  }

  const expr_group_t &group = groups[0];
  assert(group.has_symbol && "Group has no symbol");
  symbol = group.symbol;

  return true;
}

bool is_packet_readLSB(klee::ref<klee::Expr> expr, bytes_t &offset, bytes_t &size) {
  assert(!expr.isNull() && "Null expr");

  if (expr->getKind() == klee::Expr::Read) {
    klee::ReadExpr *read = dyn_cast<klee::ReadExpr>(expr);
    if (read->updates.root->name != "packet_chunks") {
      return false;
    }

    klee::ref<klee::Expr> index = read->index;
    if (index->getKind() == klee::Expr::Constant) {
      klee::ConstantExpr *index_const = dynamic_cast<klee::ConstantExpr *>(index.get());
      offset                          = index_const->getZExtValue();
      size                            = read->getWidth() / 8;
      return true;
    }
  }

  if (expr->getKind() != klee::Expr::Concat) {
    return false;
  }

  expr_groups_t groups = get_expr_groups(expr);

  if (groups.size() != 1) {
    return false;
  }

  const expr_group_t &group = groups[0];
  assert(group.has_symbol && "Group has no symbol");

  if (group.symbol != "packet_chunks") {
    return false;
  }

  offset = group.offset;
  size   = group.size;

  return true;
}

bool is_packet_readLSB(klee::ref<klee::Expr> expr) {
  bytes_t offset;
  bytes_t size;
  return is_packet_readLSB(expr, offset, size);
}

bool is_bool(klee::ref<klee::Expr> expr) {
  assert(!expr.isNull() && "Null expr");

  if (expr->getWidth() == 1) {
    return true;
  }

  if (expr->getKind() == klee::Expr::ZExt || expr->getKind() == klee::Expr::SExt || expr->getKind() == klee::Expr::Not) {
    return is_bool(expr->getKid(0));
  }

  if (expr->getKind() == klee::Expr::Or || expr->getKind() == klee::Expr::And) {
    return is_bool(expr->getKid(0)) && is_bool(expr->getKid(1));
  }

  return expr->getKind() == klee::Expr::Eq || expr->getKind() == klee::Expr::Uge || expr->getKind() == klee::Expr::Ugt ||
         expr->getKind() == klee::Expr::Ule || expr->getKind() == klee::Expr::Ult || expr->getKind() == klee::Expr::Sge ||
         expr->getKind() == klee::Expr::Sgt || expr->getKind() == klee::Expr::Sle || expr->getKind() == klee::Expr::Slt;
}

bool is_constant(klee::ref<klee::Expr> expr) {
  if (expr->getKind() == klee::Expr::Kind::Constant) {
    return true;
  }

  const std::unordered_set<std::string> symbols_names = symbol_t::get_symbols_names(expr);
  if (!symbols_names.empty()) {
    return false;
  }

  const u64 value                   = solver_toolbox.value_from_expr(expr);
  const bits_t width                = expr->getWidth();
  klee::ref<klee::Expr> const_value = solver_toolbox.exprBuilder->Constant(value, width);
  const bool is_always_eq           = solver_toolbox.are_exprs_always_equal(const_value, expr);

  return is_always_eq;
}

bool is_constant_signed(klee::ref<klee::Expr> expr) {
  const bits_t size = expr->getWidth();

  if (!is_constant(expr)) {
    return false;
  }

  assert(size <= 64 && "Size too big");

  const u64 value    = solver_toolbox.value_from_expr(expr);
  const u64 sign_bit = value >> (size - 1);

  return sign_bit == 1;
}

bool is_conditional(klee::ref<klee::Expr> expr) { return conditional_expr_kinds.find(expr->getKind()) != conditional_expr_kinds.end(); }

int64_t get_constant_signed(klee::ref<klee::Expr> expr) {
  if (!is_constant_signed(expr)) {
    return false;
  }

  u64 value    = 0;
  bits_t width = expr->getWidth();

  if (expr->getKind() == klee::Expr::Kind::Constant) {
    klee::ConstantExpr *constant = dynamic_cast<klee::ConstantExpr *>(expr.get());
    assert(width <= 64 && "Width too big");
    value = constant->getZExtValue(width);
  } else {
    value = solver_toolbox.value_from_expr(expr);
  }

  u64 mask = 0;
  for (u64 i = 0u; i < width; i++) {
    mask <<= 1;
    mask |= 1;
  }

  return -((~value + 1) & mask);
}

bool manager_contains(const klee::ConstraintManager &constraints, klee::ref<klee::Expr> expr) {
  auto found_it =
      std::find_if(constraints.begin(), constraints.end(), [&](klee::ref<klee::Expr> e) { return solver_toolbox.are_exprs_always_equal(e, expr); });
  return found_it != constraints.end();
}

klee::ConstraintManager join_managers(const klee::ConstraintManager &m1, const klee::ConstraintManager &m2) {
  klee::ConstraintManager m;

  for (klee::ref<klee::Expr> c : m1) {
    m.addConstraint(c);
  }

  for (klee::ref<klee::Expr> c : m2) {
    if (!manager_contains(m, c))
      m.addConstraint(c);
  }

  return m;
}

addr_t expr_addr_to_obj_addr(klee::ref<klee::Expr> obj_addr) {
  assert(!obj_addr.isNull() && "Null obj_addr");
  assert(is_constant(obj_addr) && "Non-constant obj_addr");
  return solver_toolbox.value_from_expr(obj_addr);
}

klee::ref<klee::Expr> constraint_from_expr(klee::ref<klee::Expr> expr) {
  klee::ref<klee::Expr> constraint;

  switch (expr->getKind()) {
  // Comparisons
  case klee::Expr::Eq:
  case klee::Expr::Ne:
  case klee::Expr::Ult:
  case klee::Expr::Ule:
  case klee::Expr::Ugt:
  case klee::Expr::Uge:
  case klee::Expr::Slt:
  case klee::Expr::Sle:
  case klee::Expr::Sgt:
  case klee::Expr::Sge:
    constraint = expr;
    break;
  // Constant
  case klee::Expr::Constant:
  // Reads
  case klee::Expr::Read:
  case klee::Expr::Select:
  case klee::Expr::Concat:
  case klee::Expr::Extract:
  // Bit
  case klee::Expr::Not:
  case klee::Expr::And:
  case klee::Expr::Or:
  case klee::Expr::Xor:
  case klee::Expr::Shl:
  case klee::Expr::LShr:
  case klee::Expr::AShr:
  // Casting
  case klee::Expr::ZExt:
  case klee::Expr::SExt:
  // Arithmetic
  case klee::Expr::Add:
  case klee::Expr::Sub:
  case klee::Expr::Mul:
  case klee::Expr::UDiv:
  case klee::Expr::SDiv:
  case klee::Expr::URem:
  case klee::Expr::SRem:
    constraint = solver_toolbox.exprBuilder->Ne(expr, solver_toolbox.exprBuilder->Constant(0, expr->getWidth()));
    break;
  default:
    panic("TODO: Constraint from expr");
  }

  return constraint;
}

klee::ref<klee::Expr> filter(klee::ref<klee::Expr> expr, const std::vector<std::string> &allowed_symbols) {
  ExpressionFilter filter(allowed_symbols);

  expr_info_t data = filter.check_symbols(expr);
  if (!data.has_allowed && data.has_not_allowed) {
    return solver_toolbox.exprBuilder->True();
  }

  klee::ref<klee::Expr> filtered = filter.visit(expr);
  assert(filter.check_symbols(filtered).has_not_allowed == 0 && "Invalid filter");

  return filtered;
}

std::vector<expr_mod_t> build_expr_mods(klee::ref<klee::Expr> before, klee::ref<klee::Expr> after) {
  assert(before->getWidth() == after->getWidth() && "Different widths");

  if (solver_toolbox.are_exprs_always_equal(before, after)) {
    return {};
  }

  if (after->getKind() == klee::Expr::Concat) {
    bits_t msb_width = after->getKid(0)->getWidth();
    bits_t lsb_width = after->getWidth() - msb_width;

    std::vector<expr_mod_t> msb_groups = build_expr_mods(solver_toolbox.exprBuilder->Extract(before, lsb_width, msb_width), after->getKid(0));
    std::vector<expr_mod_t> lsb_groups = build_expr_mods(solver_toolbox.exprBuilder->Extract(before, 0, lsb_width), after->getKid(1));
    std::vector<expr_mod_t> &groups    = lsb_groups;

    for (const expr_mod_t &group : msb_groups) {
      expr_mod_t mod(group.offset + lsb_width, group.width, group.expr);
      groups.push_back(mod);
    }

    return groups;
  }

  return {expr_mod_t(0, after->getWidth(), after)};
}

std::vector<std::optional<symbolic_read_t>> break_expr_by_reads(klee::ref<klee::Expr> expr) {
  std::vector<std::optional<symbolic_read_t>> groups;

  if (expr->getKind() == klee::Expr::Concat) {
    std::vector<std::optional<symbolic_read_t>> msb_bytes = break_expr_by_reads(expr->getKid(0));
    std::vector<std::optional<symbolic_read_t>> lsb_bytes = break_expr_by_reads(expr->getKid(1));

    groups.insert(groups.end(), lsb_bytes.begin(), lsb_bytes.end());
    groups.insert(groups.end(), msb_bytes.begin(), msb_bytes.end());
  } else if (expr->getKind() == klee::Expr::Read) {
    const klee::ReadExpr *read        = dynamic_cast<const klee::ReadExpr *>(expr.get());
    const std::string name            = read->updates.root->name;
    const klee::ref<klee::Expr> index = read->index;

    assert(index->getKind() == klee::Expr::Constant && "Non-constant index");
    klee::ConstantExpr *index_const = dynamic_cast<klee::ConstantExpr *>(index.get());
    bytes_t offset                  = index_const->getZExtValue();

    symbolic_read_t symbolic_read{offset, name};
    groups.push_back(symbolic_read);
  } else {
    for (bytes_t i = 0; i < expr->getWidth() / 8; i++) {
      groups.emplace_back();
    }
  }

  return groups;
}

std::vector<expr_byte_swap_t> get_expr_byte_swaps(klee::ref<klee::Expr> before, klee::ref<klee::Expr> after) {
  // If you're asking yourself why we didn't use the solver to check for this instead of using this clunky method, the
  // answer is that we did, but it was painfully slow.

  assert(before->getWidth() == after->getWidth() && "Different widths");

  std::vector<std::optional<symbolic_read_t>> lhs_bytes = break_expr_by_reads(before);
  std::vector<std::optional<symbolic_read_t>> rhs_bytes = break_expr_by_reads(after);

  assert(lhs_bytes.size() == rhs_bytes.size() && "Different number of groups");

  std::vector<expr_byte_swap_t> byte_swaps;
  std::unordered_set<bytes_t> matched;

  for (bytes_t i = 0; i < lhs_bytes.size(); i++) {
    const std::optional<symbolic_read_t> &lhs = lhs_bytes[i];

    if (!lhs.has_value()) {
      continue;
    }

    for (bytes_t j = 0; j < rhs_bytes.size(); j++) {
      if (j == i || matched.count(j)) {
        continue;
      }

      const std::optional<symbolic_read_t> &rhs = rhs_bytes[j];

      if (!rhs.has_value() || lhs->symbol != rhs->symbol || lhs->byte != rhs->byte) {
        continue;
      }

      const std::optional<symbolic_read_t> &lhs_swapped = lhs_bytes[j];
      const std::optional<symbolic_read_t> &rhs_swapped = rhs_bytes[i];

      if (!lhs_swapped.has_value() || !rhs_swapped.has_value()) {
        continue;
      }

      if (lhs_swapped->symbol != rhs_swapped->symbol || lhs_swapped->byte != rhs_swapped->byte) {
        continue;
      }

      expr_byte_swap_t byte_swap{i, j};
      matched.insert(i);
      matched.insert(j);
      byte_swaps.push_back(byte_swap);
    }
  }

  return byte_swaps;
}

std::vector<klee::ref<klee::Expr>> bytes_in_expr(klee::ref<klee::Expr> expr, bool force_big_endian_for_constants) {
  std::vector<klee::ref<klee::Expr>> bytes;

  bool is_const = is_constant(expr);

  bytes_t size = expr->getWidth() / 8;
  for (bytes_t i = 0; i < size; i++) {
    bits_t offset = i * 8;

    if (is_const && force_big_endian_for_constants) {
      offset = (size - i - 1) * 8;
    }

    bytes.push_back(solver_toolbox.exprBuilder->Extract(expr, offset, 8));
  }

  return bytes;
}

expr_groups_t get_expr_groups(klee::ref<klee::Expr> expr) {
  expr_groups_t groups;

  if (expr.isNull()) {
    return groups;
  }

  auto process_read = [&groups](klee::ref<klee::Expr> read_expr) {
    assert(read_expr->getKind() == klee::Expr::Read && "Not a read");
    const klee::ReadExpr *read = dyn_cast<klee::ReadExpr>(read_expr);

    klee::ref<klee::Expr> index = read->index;
    const std::string symbol    = read->updates.root->name;

    assert(index->getKind() == klee::Expr::Kind::Constant && "Non-constant index");
    const klee::ConstantExpr *index_const = dynamic_cast<klee::ConstantExpr *>(index.get());
    const bytes_t current_byte            = index_const->getZExtValue();

    bool appended_to_group = false;
    if (groups.size()) {
      expr_group_t &last_group = groups.back();
      if (last_group.has_symbol && last_group.symbol == symbol && last_group.offset == current_byte + 1) {
        last_group.size++;
        last_group.offset = current_byte;
        last_group.expr   = concat_lsb(last_group.expr, read_expr);
        appended_to_group = true;
      }
    }

    if (!appended_to_group) {
      const bytes_t size = read_expr->getWidth() / 8;
      groups.emplace_back(true, symbol, current_byte, size, read_expr);
    }
  };

  auto process_not_read = [&groups](klee::ref<klee::Expr> not_read_expr) {
    assert(not_read_expr->getKind() != klee::Expr::Read && "Non read is actually a read");
    const bits_t size = not_read_expr->getWidth();
    if (size % 8 == 0) {
      groups.emplace_back(false, "", 0, size / 8, not_read_expr);
    }
  };

  if (expr->getKind() == klee::Expr::Extract) {
    expr = simplify(expr);
  }

  while (expr->getKind() == klee::Expr::Concat) {
    klee::ref<klee::Expr> lhs = expr->getKid(0);
    klee::ref<klee::Expr> rhs = expr->getKid(1);

    assert(lhs->getKind() != klee::Expr::Concat && "Nested concats");

    if (lhs->getKind() == klee::Expr::Read) {
      process_read(lhs);
    } else {
      process_not_read(lhs);
    }

    expr = rhs;
  }

  if (expr->getKind() == klee::Expr::Read) {
    process_read(expr);
  } else {
    process_not_read(expr);
  }

  return groups;
}

std::vector<expr_group_t> get_expr_groups_from_condition(klee::ref<klee::Expr> expr) {
  expr_groups_t groups;

  if (expr.isNull()) {
    return groups;
  }

  const std::unordered_set<klee::Expr::Kind> conditional_expr_kinds = {
      klee::Expr::Eq,  klee::Expr::Ne,  klee::Expr::Ult, klee::Expr::Ule, klee::Expr::Ugt, klee::Expr::Uge, klee::Expr::Slt,
      klee::Expr::Sle, klee::Expr::Sgt, klee::Expr::Sge, klee::Expr::And, klee::Expr::Or,  klee::Expr::Not,
  };

  auto is_conditional = [&conditional_expr_kinds](klee::ref<klee::Expr> target_expr) {
    return (conditional_expr_kinds.find(target_expr->getKind()) != conditional_expr_kinds.end()) || (target_expr->getWidth() == 1);
  };

  auto process_read = [&groups](klee::ref<klee::Expr> target_expr) {
    assert(target_expr->getKind() == klee::Expr::Read && "Not a read");
    const klee::ReadExpr *read = dyn_cast<klee::ReadExpr>(target_expr);

    klee::ref<klee::Expr> index = read->index;
    const std::string symbol    = read->updates.root->name;

    assert(index->getKind() == klee::Expr::Kind::Constant && "Non-constant index");
    const klee::ConstantExpr *index_const = dynamic_cast<klee::ConstantExpr *>(index.get());
    const bytes_t current_byte            = index_const->getZExtValue();

    bool appended_to_group = false;
    if (groups.size()) {
      expr_group_t &last_group = groups.back();
      if (last_group.has_symbol && last_group.symbol == symbol && last_group.offset == current_byte + 1) {
        last_group.size++;
        last_group.offset = current_byte;
        last_group.expr   = concat_lsb(last_group.expr, target_expr);
        appended_to_group = true;
      }
    }

    if (!appended_to_group) {
      const bytes_t size = target_expr->getWidth() / 8;
      groups.emplace_back(true, symbol, current_byte, size, target_expr);
    }
  };

  auto process_conditional = [&groups](klee::ref<klee::Expr> target_expr) {
    for (size_t i = 0; i < target_expr->getNumKids(); i++) {
      klee::ref<klee::Expr> kid      = target_expr->getKid(i);
      const expr_groups_t kid_groups = get_expr_groups_from_condition(kid);
      groups.insert(groups.end(), kid_groups.begin(), kid_groups.end());
    }
  };

  auto process_not_read_not_conditional = [&groups](klee::ref<klee::Expr> target_expr) {
    assert(target_expr->getKind() != klee::Expr::Read && "Non read is actually a read");
    assert(target_expr->getWidth() % 8 == 0 && "Not a byte aligned expr");
  };

  if (expr->getKind() == klee::Expr::Extract) {
    expr = simplify(expr);
  }

  while (expr->getKind() == klee::Expr::Concat) {
    klee::ref<klee::Expr> lhs = expr->getKid(0);
    klee::ref<klee::Expr> rhs = expr->getKid(1);

    assert(lhs->getKind() != klee::Expr::Concat && "Nested concats");

    if (lhs->getKind() == klee::Expr::Read) {
      process_read(lhs);
    } else if (is_conditional(lhs)) {
      process_conditional(lhs);
    } else {
      process_not_read_not_conditional(lhs);
    }

    expr = rhs;
  }

  if (expr->getKind() == klee::Expr::Read) {
    process_read(expr);
  } else if (is_conditional(expr)) {
    process_conditional(expr);
  } else {
    process_not_read_not_conditional(expr);
  }

  return groups;
}

std::size_t symbolic_read_hash_t::operator()(const symbolic_read_t &s) const noexcept {
  std::hash<std::string> hash_fn;
  return hash_fn(s.symbol + std::to_string(s.byte));
}

bool symbolic_read_equal_t::operator()(const symbolic_read_t &a, const symbolic_read_t &b) const noexcept {
  return a.symbol == b.symbol && a.byte == b.byte;
}

symbolic_reads_t get_unique_symbolic_reads(klee::ref<klee::Expr> expr, std::optional<std::string> symbol_filter) {
  if (expr.isNull()) {
    return {};
  }
  SymbolicReadsRetriever retriever(symbol_filter);
  retriever.visit(expr);
  return retriever.get_symbolic_reads();
}

bool simplify_extract_0_zext_conditional(klee::ref<klee::Expr> extract_expr, klee::ref<klee::Expr> &out) {
  if (extract_expr->getKind() != klee::Expr::Extract) {
    return false;
  }

  klee::ExtractExpr *extract = dynamic_cast<klee::ExtractExpr *>(extract_expr.get());

  if (extract->offset != 0 || extract->width != 1) {
    return false;
  }

  if (extract->expr->getKind() != klee::Expr::ZExt) {
    return false;
  }

  klee::ZExtExpr *zext = dynamic_cast<klee::ZExtExpr *>(extract->expr.get());

  if (!is_conditional(zext->src)) {
    return false;
  }

  out = zext->src;
  return true;
}

bool simplify_extract_0_same_width(klee::ref<klee::Expr> extract_expr, klee::ref<klee::Expr> &out) {
  if (extract_expr->getKind() != klee::Expr::Extract) {
    return false;
  }

  klee::ExtractExpr *extract = dynamic_cast<klee::ExtractExpr *>(extract_expr.get());

  if (extract->offset != 0) {
    return false;
  }

  klee::ref<klee::Expr> arg = extract->expr;

  if (arg->getWidth() != extract->width) {
    return false;
  }

  out = arg;
  return true;
}

bool simplify_extract_of_concats(klee::ref<klee::Expr> extract_expr, klee::ref<klee::Expr> &out) {
  if (extract_expr->getKind() != klee::Expr::Extract) {
    return false;
  }

  const klee::ExtractExpr *extract = dynamic_cast<klee::ExtractExpr *>(extract_expr.get());
  const bits_t offset              = extract->offset;
  klee::ref<klee::Expr> expr       = extract->expr;
  const bits_t size                = extract->width;

  if (size % 8 != 0) {
    return false;
  }

  if (expr->getKind() != klee::Expr::Kind::Concat) {
    return false;
  }

  std::vector<klee::ref<klee::Expr>> concatenated_exprs;

  while (expr->getKind() == klee::Expr::Kind::Concat) {
    klee::ref<klee::Expr> lhs = expr->getKid(0);
    klee::ref<klee::Expr> rhs = expr->getKid(1);
    concatenated_exprs.push_back(lhs);
    expr = rhs;
  }

  // Adding the final rhs
  concatenated_exprs.push_back(expr);

  std::vector<klee::ref<klee::Expr>> target_exprs;
  bits_t current_offset = 0;
  for (auto it = concatenated_exprs.rbegin(); it != concatenated_exprs.rend(); it++) {
    klee::ref<klee::Expr> current_expr = *it;

    if (current_offset >= offset && current_offset + current_expr->getWidth() <= offset + size) {
      target_exprs.push_back(current_expr);
    }

    current_offset += current_expr->getWidth();
  }

  out = nullptr;
  for (klee::ref<klee::Expr> target_expr : target_exprs) {
    if (out.isNull()) {
      out = target_expr;
    } else {
      out = solver_toolbox.exprBuilder->Concat(target_expr, out);
    }
  }

  return !out.isNull();
}

bool simplify_extract_ext(klee::ref<klee::Expr> extract_expr, klee::ref<klee::Expr> &out) {
  if (extract_expr->getKind() != klee::Expr::Extract) {
    return false;
  }

  const klee::ExtractExpr *extract = dynamic_cast<klee::ExtractExpr *>(extract_expr.get());
  bits_t offset                    = extract->offset;
  klee::ref<klee::Expr> expr       = extract->expr;
  bits_t size                      = extract->width;

  if (offset != 0) {
    return false;
  }

  if (expr->getKind() != klee::Expr::Kind::ZExt && expr->getKind() != klee::Expr::Kind::SExt) {
    return false;
  }

  klee::ref<klee::Expr> src;
  klee::Expr::Width ext_size;
  if (expr->getKind() == klee::Expr::Kind::ZExt) {
    klee::ZExtExpr *zext = dynamic_cast<klee::ZExtExpr *>(expr.get());
    src                  = zext->src;
    ext_size             = zext->width;
  } else {
    klee::SExtExpr *sext = dynamic_cast<klee::SExtExpr *>(expr.get());
    src                  = sext->src;
    ext_size             = sext->width;
  }

  if (size > ext_size) {
    return false;
  }

  if (size == src->getWidth()) {
    out = src;
    return true;
  }

  out = solver_toolbox.exprBuilder->Extract(src, offset, size);

  return true;
}

bool simplify_extract_read(klee::ref<klee::Expr> extract_expr, klee::ref<klee::Expr> &out) {
  if (extract_expr->getKind() != klee::Expr::Extract) {
    return false;
  }

  const klee::ExtractExpr *extract = dynamic_cast<klee::ExtractExpr *>(extract_expr.get());
  const bits_t offset              = extract->offset;
  const bits_t size                = extract->width;
  klee::ref<klee::Expr> expr       = extract->expr;

  if (offset != 0 || size != 8) {
    return false;
  }

  if (expr->getKind() != klee::Expr::Kind::Read) {
    return false;
  }

  out = expr;

  return true;
}

bool is_cmp_0(klee::ref<klee::Expr> expr, klee::ref<klee::Expr> &not_const_kid) {
  assert(!expr.isNull() && "Null expr");

  if (expr->getNumKids() != 2) {
    return false;
  }

  klee::ref<klee::Expr> lhs = expr->getKid(0);
  klee::ref<klee::Expr> rhs = expr->getKid(1);

  bool lhs_is_constant = is_constant(lhs);
  bool rhs_is_constant = is_constant(rhs);

  if (!lhs_is_constant && !rhs_is_constant) {
    return false;
  }

  klee::ref<klee::Expr> constant = lhs_is_constant ? lhs : rhs;
  klee::ref<klee::Expr> other    = lhs_is_constant ? rhs : lhs;

  const std::vector<klee::Expr::Kind> allowed_exprs{
      klee::Expr::Or,  klee::Expr::And, klee::Expr::Eq,  klee::Expr::Ne,  klee::Expr::Ult, klee::Expr::Ule,
      klee::Expr::Ugt, klee::Expr::Uge, klee::Expr::Slt, klee::Expr::Sle, klee::Expr::Sgt, klee::Expr::Sge,
  };

  if (std::find(allowed_exprs.begin(), allowed_exprs.end(), other->getKind()) == allowed_exprs.end()) {
    return false;
  }

  u64 constant_value = solver_toolbox.value_from_expr(constant);
  not_const_kid      = other;

  return constant_value == 0;
}

// E.g.
// (Extract 0
//   (ZExt w8
//      (Eq (w32 0) (ReadLSB w32 (w32 0) out_of_space__64))))
bool is_extract_0_cond(klee::ref<klee::Expr> expr, klee::ref<klee::Expr> &cond_expr) {
  assert(!expr.isNull() && "Null expr");

  if (expr->getKind() != klee::Expr::Extract) {
    return false;
  }

  auto extract = dynamic_cast<klee::ExtractExpr *>(expr.get());

  if (extract->offset != 0) {
    return false;
  }

  auto kid = extract->expr;

  if (kid->getKind() == klee::Expr::ZExt) {
    kid = kid->getKid(0);
  }

  assert(kid->getKind() != klee::Expr::ZExt && "Invalid expr");

  if (is_conditional(extract->expr)) {
    cond_expr = kid;
    return true;
  }

  return false;
}

bool can_be_negated(klee::ref<klee::Expr> expr) {
  if (is_conditional(expr)) {
    return true;
  }

  const std::unordered_set<klee::Expr::Kind> transparent{
      {klee::Expr::ZExt},
      {klee::Expr::SExt},
  };

  if (transparent.find(expr->getKind()) != transparent.end()) {
    assert(expr->getNumKids() == 1 && "Invalid expr");
    klee::ref<klee::Expr> kid = expr->getKid(0);
    return can_be_negated(kid);
  }

  return false;
}

klee::ref<klee::Expr> negate(klee::ref<klee::Expr> expr) {
  klee::ref<klee::Expr> other;

  if (is_cmp_0(expr, other) && is_bool(other)) {
    return other;
  }

  const std::unordered_map<klee::Expr::Kind, klee::Expr::Kind> negate_map{
      {klee::Expr::Or, klee::Expr::And},    {klee::Expr::And, klee::Expr::Or},    {klee::Expr::Eq, klee::Expr::Ne},
      {klee::Expr::Ne, klee::Expr::Eq},     {klee::Expr::Ult, klee::Expr::Uge},   {klee::Expr::Uge, klee::Expr::Ult},
      {klee::Expr::Ule, klee::Expr::Ugt},   {klee::Expr::Ugt, klee::Expr::Ule},   {klee::Expr::Slt, klee::Expr::Sge},
      {klee::Expr::Sge, klee::Expr::Slt},   {klee::Expr::Sle, klee::Expr::Sgt},   {klee::Expr::Sgt, klee::Expr::Sle},
      {klee::Expr::ZExt, klee::Expr::ZExt}, {klee::Expr::SExt, klee::Expr::SExt},
  };

  auto kind     = expr->getKind();
  auto found_it = negate_map.find(kind);
  assert(found_it != negate_map.end() && "Invalid kind");

  switch (found_it->second) {
  case klee::Expr::Or: {
    klee::ref<klee::Expr> lhs = expr->getKid(0);
    klee::ref<klee::Expr> rhs = expr->getKid(1);

    if (can_be_negated(lhs) && can_be_negated(rhs)) {
      lhs = negate(lhs);
      rhs = negate(rhs);
      return solver_toolbox.exprBuilder->Or(lhs, rhs);
    }

    return solver_toolbox.exprBuilder->Not(expr);
  } break;
  case klee::Expr::And: {
    klee::ref<klee::Expr> lhs = expr->getKid(0);
    klee::ref<klee::Expr> rhs = expr->getKid(1);

    if (can_be_negated(lhs) && can_be_negated(rhs)) {
      lhs = negate(lhs);
      rhs = negate(rhs);
      return solver_toolbox.exprBuilder->And(lhs, rhs);
    }

    return solver_toolbox.exprBuilder->Not(expr);
  } break;
  case klee::Expr::Eq: {
    klee::ref<klee::Expr> lhs = expr->getKid(0);
    klee::ref<klee::Expr> rhs = expr->getKid(1);
    return solver_toolbox.exprBuilder->Eq(lhs, rhs);
  } break;
  case klee::Expr::Ne: {
    klee::ref<klee::Expr> lhs = expr->getKid(0);
    klee::ref<klee::Expr> rhs = expr->getKid(1);
    return solver_toolbox.exprBuilder->Ne(lhs, rhs);
  } break;
  case klee::Expr::Ult: {
    klee::ref<klee::Expr> lhs = expr->getKid(0);
    klee::ref<klee::Expr> rhs = expr->getKid(1);
    return solver_toolbox.exprBuilder->Ult(lhs, rhs);
  } break;
  case klee::Expr::Ule: {
    klee::ref<klee::Expr> lhs = expr->getKid(0);
    klee::ref<klee::Expr> rhs = expr->getKid(1);
    return solver_toolbox.exprBuilder->Ule(lhs, rhs);
  } break;
  case klee::Expr::Ugt: {
    klee::ref<klee::Expr> lhs = expr->getKid(0);
    klee::ref<klee::Expr> rhs = expr->getKid(1);
    return solver_toolbox.exprBuilder->Ugt(lhs, rhs);
  } break;
  case klee::Expr::Uge: {
    klee::ref<klee::Expr> lhs = expr->getKid(0);
    klee::ref<klee::Expr> rhs = expr->getKid(1);
    return solver_toolbox.exprBuilder->Uge(lhs, rhs);
  } break;
  case klee::Expr::Slt: {
    klee::ref<klee::Expr> lhs = expr->getKid(0);
    klee::ref<klee::Expr> rhs = expr->getKid(1);
    return solver_toolbox.exprBuilder->Slt(lhs, rhs);
  } break;
  case klee::Expr::Sle: {
    klee::ref<klee::Expr> lhs = expr->getKid(0);
    klee::ref<klee::Expr> rhs = expr->getKid(1);
    return solver_toolbox.exprBuilder->Sle(lhs, rhs);
  } break;
  case klee::Expr::Sgt: {
    klee::ref<klee::Expr> lhs = expr->getKid(0);
    klee::ref<klee::Expr> rhs = expr->getKid(1);
    return solver_toolbox.exprBuilder->Sgt(lhs, rhs);
  } break;
  case klee::Expr::Sge: {
    klee::ref<klee::Expr> lhs = expr->getKid(0);
    klee::ref<klee::Expr> rhs = expr->getKid(1);
    return solver_toolbox.exprBuilder->Sge(lhs, rhs);
  } break;
  case klee::Expr::ZExt: {
    auto kid   = expr->getKid(0);
    auto width = expr->getWidth();

    if (can_be_negated(kid)) {
      kid = negate(kid);
      return solver_toolbox.exprBuilder->ZExt(kid, width);
    }

    return solver_toolbox.exprBuilder->Not(expr);
  } break;
  case klee::Expr::SExt: {
    auto kid   = expr->getKid(0);
    auto width = expr->getWidth();

    if (can_be_negated(kid)) {
      kid = negate(kid);
      return solver_toolbox.exprBuilder->SExt(kid, width);
    }

    return solver_toolbox.exprBuilder->Not(expr);
  } break;
  default:
    panic("TODO: Negate expression");
  }

  return expr;
}

// Simplify expressions like (Ne (0) (And X Y)) which could be (And X Y)
bool simplify_cmp_ne_0(klee::ref<klee::Expr> expr, klee::ref<klee::Expr> &out) {
  klee::ref<klee::Expr> other;

  if (expr->getKind() != klee::Expr::Ne) {
    return false;
  }

  if (!is_cmp_0(expr, other)) {
    return false;
  }

  if (!is_bool(other)) {
    return false;
  }

  out = negate(other);
  return true;
}

// Simplify expressions like (Eq 0 (And X Y)) which could be (Or !X !Y),
// or (Eq 0 (Eq A B)) => (Ne A B)
bool simplify_cmp_eq_0(klee::ref<klee::Expr> expr, klee::ref<klee::Expr> &out) {
  klee::ref<klee::Expr> other;

  if (expr->getKind() != klee::Expr::Eq) {
    return false;
  }

  if (!is_cmp_0(expr, other)) {
    return false;
  }

  if (!is_bool(other)) {
    return false;
  }

  out = negate(other);
  return true;
}

// (Not (Eq X Y)) => (Ne X Y)
bool simplify_not_eq(klee::ref<klee::Expr> expr, klee::ref<klee::Expr> &out) {
  if (expr->getKind() != klee::Expr::Not) {
    return false;
  }

  assert(expr->getNumKids() == 1 && "Invalid expr");
  expr = expr->getKid(0);

  if (expr->getKind() != klee::Expr::Eq) {
    return false;
  }

  assert(expr->getNumKids() == 2 && "Invalid expr");

  klee::ref<klee::Expr> lhs = expr->getKid(0);
  klee::ref<klee::Expr> rhs = expr->getKid(1);

  out = solver_toolbox.exprBuilder->Ne(lhs, rhs);
  return true;
}

bool simplify_cmp_zext_eq_size(klee::ref<klee::Expr> expr, klee::ref<klee::Expr> &out) {
  auto cmp_ops = std::vector<klee::Expr::Kind>{
      klee::Expr::Or,  klee::Expr::And, klee::Expr::Eq,  klee::Expr::Ne,  klee::Expr::Ult, klee::Expr::Ule,
      klee::Expr::Ugt, klee::Expr::Uge, klee::Expr::Slt, klee::Expr::Sle, klee::Expr::Sgt, klee::Expr::Sge,
  };

  auto found_it = std::find(cmp_ops.begin(), cmp_ops.end(), expr->getKind());

  if (found_it == cmp_ops.end()) {
    return false;
  }

  assert(expr->getNumKids() == 2 && "Invalid expr");

  klee::ref<klee::Expr> lhs = expr->getKid(0);
  klee::ref<klee::Expr> rhs = expr->getKid(1);

  auto detected = false;

  while (lhs->getKind() == klee::Expr::ZExt) {
    detected = true;
    lhs      = lhs->getKid(0);
  }

  while (rhs->getKind() == klee::Expr::ZExt) {
    detected = true;
    rhs      = rhs->getKid(0);
  }

  if (!detected) {
    return false;
  }

  if (lhs->getWidth() != rhs->getWidth()) {
    return false;
  }

  klee::ref<klee::Expr> kids[2] = {lhs, rhs};
  out                           = expr->rebuild(kids);

  return true;
}

bool simplify_add_neg_sext(klee::ref<klee::Expr> expr, klee::ref<klee::Expr> &out) {
  if (expr->getKind() != klee::Expr::Add) {
    return false;
  }

  assert(expr->getNumKids() == 2 && "Invalid expr");
  klee::ref<klee::Expr> lhs = expr->getKid(0);
  klee::ref<klee::Expr> rhs = expr->getKid(1);

  klee::ref<klee::Expr> negative_constant;
  klee::ref<klee::Expr> sext;

  if (is_constant_signed(lhs)) {
    negative_constant = lhs;
    sext              = rhs;
  } else {
    negative_constant = rhs;
    sext              = lhs;
  }

  if (!is_constant_signed(negative_constant) || sext->getKind() != klee::Expr::SExt) {
    return false;
  }

  const i64 n = get_constant_signed(negative_constant);

  klee::ref<klee::Expr> sext_src     = sext->getKid(0);
  klee::ref<klee::Expr> new_constant = solver_toolbox.exprBuilder->Constant(-n, sext_src->getWidth());

  if (solver_toolbox.value_from_expr(new_constant) != static_cast<u64>(-n)) {
    return false;
  }

  out = solver_toolbox.exprBuilder->ZExt(solver_toolbox.exprBuilder->Sub(sext_src, new_constant), expr->getWidth());

  return true;
}

bool simplify_add_non_neg_sext(klee::ref<klee::Expr> expr, klee::ref<klee::Expr> &out) {
  if (expr->getKind() != klee::Expr::Add) {
    return false;
  }

  assert(expr->getNumKids() == 2 && "Invalid expr");
  klee::ref<klee::Expr> lhs = expr->getKid(0);
  klee::ref<klee::Expr> rhs = expr->getKid(1);

  klee::ref<klee::Expr> non_negative_constant;
  klee::ref<klee::Expr> sext;

  if (is_constant(lhs)) {
    non_negative_constant = lhs;
    sext                  = rhs;
  } else {
    non_negative_constant = rhs;
    sext                  = lhs;
  }

  if (!is_constant(non_negative_constant) || sext->getKind() != klee::Expr::SExt) {
    return false;
  }

  if (is_constant_signed(non_negative_constant)) {
    return false;
  }

  const u64 n = solver_toolbox.value_from_expr(non_negative_constant);

  klee::ref<klee::Expr> sext_src     = sext->getKid(0);
  klee::ref<klee::Expr> new_constant = solver_toolbox.exprBuilder->Constant(n, sext_src->getWidth());

  if (solver_toolbox.value_from_expr(new_constant) != n) {
    return false;
  }

  out = solver_toolbox.exprBuilder->ZExt(solver_toolbox.exprBuilder->Add(sext_src, new_constant), expr->getWidth());

  return true;
}

using simplifier_fn = std::function<bool(klee::ref<klee::Expr>, klee::ref<klee::Expr> &)>;

enum class simplifier_type_t {
  ExtractZeroZExtConditional,
  ExtractZeroSameWidth,
  ExtractConcats,
  ExtractRead,
  ExtractExt,
  CompareEqZero,
  CompareZExtEqSize,
  NotEq,
  CompareNeZero,
  AddNegativeSExt,
  AddNonNegativeSExt,
};

std::ostream &operator<<(std::ostream &os, const simplifier_type_t &type) {
  switch (type) {
  case simplifier_type_t::ExtractZeroZExtConditional:
    os << "ExtractZeroZExtConditional";
    break;
  case simplifier_type_t::ExtractZeroSameWidth:
    os << "ExtractZeroSameWidth";
    break;
  case simplifier_type_t::ExtractConcats:
    os << "ExtractConcats";
    break;
  case simplifier_type_t::ExtractRead:
    os << "ExtractRead";
    break;
  case simplifier_type_t::ExtractExt:
    os << "ExtractExt";
    break;
  case simplifier_type_t::CompareEqZero:
    os << "CompareEqZero";
    break;
  case simplifier_type_t::CompareZExtEqSize:
    os << "CompareZExtEqSize";
    break;
  case simplifier_type_t::NotEq:
    os << "NotEq";
    break;
  case simplifier_type_t::CompareNeZero:
    os << "CompareNeZero";
    break;
  case simplifier_type_t::AddNegativeSExt:
    os << "AddNegativeSExt";
    break;
  case simplifier_type_t::AddNonNegativeSExt:
    os << "AddNonNegativeSExt";
    break;
  }

  return os;
}

struct simplifier_op {
  simplifier_type_t type;
  simplifier_fn apply;
};

typedef std::vector<simplifier_op> simplifiers_t;

std::vector<simplifier_type_t> apply_simplifiers(const simplifiers_t &simplifiers, klee::ref<klee::Expr> expr,
                                                 klee::ref<klee::Expr> &simplified_expr) {
  std::vector<simplifier_type_t> simplifications_applied;

  simplified_expr = expr;

  for (auto op : simplifiers) {
    if (op.apply(expr, simplified_expr)) {
      simplifications_applied.push_back(op.type);
      return simplifications_applied;
    }
  }

  const u32 num_kids = expr->getNumKids();
  std::vector<klee::ref<klee::Expr>> new_kids(num_kids);

  bool simplified                 = false;
  klee::Expr::Width max_kid_width = 0;

  for (u32 i = 0; i < num_kids; i++) {
    klee::ref<klee::Expr> kid            = expr->getKid(i);
    klee::ref<klee::Expr> simplified_kid = kid;

    std::vector<simplifier_type_t> local_simplifications = apply_simplifiers(simplifiers, kid, simplified_kid);
    max_kid_width                                        = std::max(max_kid_width, simplified_kid->getWidth());
    new_kids[i]                                          = simplified_kid;

    simplified |= !local_simplifications.empty();

    for (auto simplification : local_simplifications) {
      simplifications_applied.push_back(simplification);
    }
  }

  if (!simplified) {
    return simplifications_applied;
  }

  for (u32 i = 0; i < num_kids; i++) {
    klee::ref<klee::Expr> simplified_kid = new_kids[i];

    if (simplified_kid->getWidth() < max_kid_width) {
      simplified_kid = solver_toolbox.exprBuilder->ZExt(simplified_kid, max_kid_width);
    }

    assert(simplified_kid->getWidth() == max_kid_width && "Invalid width");
    new_kids[i] = simplified_kid;
  }

  simplified_expr = expr->rebuild(&new_kids[0]);
  return simplifications_applied;
}

klee::ref<klee::Expr> simplify(klee::ref<klee::Expr> expr) {
  if (expr.isNull()) {
    return expr;
  }

  if (expr->getKind() == klee::Expr::Constant) {
    return expr;
  }

  const simplifiers_t simplifiers{
      {simplifier_type_t::ExtractZeroZExtConditional, simplify_extract_0_zext_conditional},
      {simplifier_type_t::ExtractZeroSameWidth, simplify_extract_0_same_width},
      {simplifier_type_t::ExtractConcats, simplify_extract_of_concats},
      {simplifier_type_t::ExtractRead, simplify_extract_read},
      {simplifier_type_t::ExtractExt, simplify_extract_ext},
      {simplifier_type_t::CompareEqZero, simplify_cmp_eq_0},
      {simplifier_type_t::CompareZExtEqSize, simplify_cmp_zext_eq_size},
      {simplifier_type_t::NotEq, simplify_not_eq},
      {simplifier_type_t::CompareNeZero, simplify_cmp_ne_0},
      {simplifier_type_t::AddNegativeSExt, simplify_add_neg_sext},
      {simplifier_type_t::AddNonNegativeSExt, simplify_add_non_neg_sext},
      {simplifier_type_t::AddNonNegativeSExt, simplify_add_non_neg_sext},
  };

  klee::ref<klee::Expr> original_expr = expr;
  std::vector<klee::ref<klee::Expr>> prev_exprs{expr};

  while (true) {
    klee::ref<klee::Expr> simplified = expr;

    std::vector<simplifier_type_t> simplifications_applied = apply_simplifiers(simplifiers, expr, simplified);
    assert(!simplified.isNull() && "Null expr");

    // for (simplifier_type_t type : simplifications_applied) {
    //   std::cerr << "Applied simplification: " << type << std::endl;
    //   std::cerr << "New expr: " << expr_to_string(simplified) << std::endl;
    // }

    bool changed = true;
    for (klee::ref<klee::Expr> prev : prev_exprs) {
      if (!simplified->compare(*prev.get())) {
        changed = false;
        break;
      }
    }

    if (!changed) {
      break;
    }

    prev_exprs.push_back(simplified);
    expr = simplified;
  }

  if (!solver_toolbox.are_exprs_always_equal(original_expr, expr)) {
    panic("*** Bug in simplification! ***\nOriginal: %s\nSimplified: %s\n", expr_to_string(original_expr).c_str(), expr_to_string(expr).c_str());
  }

  return expr;
}

klee::ref<klee::Expr> simplify_conditional(klee::ref<klee::Expr> expr) {
  assert(!expr.isNull() && "Null condition");

  if (!is_conditional(expr)) {
    return expr;
  }

  const simplifiers_t simplifiers{
      {simplifier_type_t::ExtractConcats, simplify_extract_of_concats},
      {simplifier_type_t::ExtractZeroSameWidth, simplify_extract_0_same_width},
      {simplifier_type_t::ExtractExt, simplify_extract_ext},
      {simplifier_type_t::CompareEqZero, simplify_cmp_eq_0},
      {simplifier_type_t::CompareZExtEqSize, simplify_cmp_zext_eq_size},
      {simplifier_type_t::NotEq, simplify_not_eq},
      {simplifier_type_t::CompareNeZero, simplify_cmp_ne_0},
      {simplifier_type_t::AddNegativeSExt, simplify_add_neg_sext},
  };

  std::vector<klee::ref<klee::Expr>> prev_exprs{expr};

  while (true) {
    klee::ref<klee::Expr> simplified = expr;

    std::vector<simplifier_type_t> simplifications_applied = apply_simplifiers(simplifiers, expr, simplified);
    assert(!simplified.isNull() && "Null expr");

    for (klee::ref<klee::Expr> prev : prev_exprs) {
      if (!simplified->compare(*prev.get())) {
        return expr;
      }
    }

    prev_exprs.push_back(simplified);
    expr = simplified;
  }

  return expr;
}

std::string pretty_print_expr(klee::ref<klee::Expr> expr, bool use_signed) { return ExprPrettyPrinter::print(expr, use_signed); }

std::string expr_to_string(klee::ref<klee::Expr> expr, bool one_liner) {
  std::string expr_str;
  if (expr.isNull()) {
    expr_str = "(null)";
    return expr_str;
  }
  llvm::raw_string_ostream os(expr_str);
  expr->print(os);
  os.str();

  remove_expr_str_labels(expr_str);

  if (one_liner) {
    // remove new lines
    expr_str.erase(std::remove(expr_str.begin(), expr_str.end(), '\n'), expr_str.end());

    // remove duplicated whitespaces
    auto bothAreSpaces            = [](char lhs, char rhs) -> bool { return (lhs == rhs) && (lhs == ' '); };
    std::string::iterator new_end = std::unique(expr_str.begin(), expr_str.end(), bothAreSpaces);
    expr_str.erase(new_end, expr_str.end());
  }

  return expr_str;
}

std::string expr_to_ascii(klee::ref<klee::Expr> expr) {
  assert(!expr.isNull() && "Null expr");

  std::string str;
  for (klee::ref<klee::Expr> byte_expr : bytes_in_expr(expr)) {
    u8 value = solver_toolbox.value_from_expr(byte_expr);
    str += static_cast<char>(value);
  }

  return str;
}
bool match_endian_swap_16_pattern(klee::ref<klee::Expr> expr, klee::ref<klee::Expr> &target) {
  /*
  (Extract w16 0
    (Or w32
        (Shl w32 (And w32 (ZExt w32 (ReadLSB w16 (w32 514) packet_chunks)) (w32 255)) (w32 8))
        (AShr w32 (And w32 (ZExt w32 (ReadLSB w16 (w32 514) packet_chunks)) (w32 65280)) (w32 8))
    )
  )
  */
  if (expr->getKind() != klee::Expr::Extract) {
    return false;
  }

  klee::ExtractExpr *extract = dynamic_cast<klee::ExtractExpr *>(expr.get());
  if (extract->width != 16 || extract->expr->getKind() != klee::Expr::Or) {
    return false;
  }

  klee::OrExpr *or_expr = dynamic_cast<klee::OrExpr *>(extract->expr.get());
  if (or_expr->getWidth() != 32 || or_expr->left->getKind() != klee::Expr::Shl || or_expr->right->getKind() != klee::Expr::AShr) {
    return false;
  }

  klee::ShlExpr *shl_expr   = dynamic_cast<klee::ShlExpr *>(or_expr->left.get());
  klee::AShrExpr *ashr_expr = dynamic_cast<klee::AShrExpr *>(or_expr->right.get());

  if (shl_expr->left->getKind() != klee::Expr::And || ashr_expr->left->getKind() != klee::Expr::And) {
    return false;
  }

  u64 shl_right_const;
  if (!solver_toolbox.strict_value_from_expr(shl_expr->right, shl_right_const) || shl_right_const != 8) {
    return false;
  }

  u64 ashr_right_const;
  if (!solver_toolbox.strict_value_from_expr(ashr_expr->right, ashr_right_const) || ashr_right_const != 8) {
    return false;
  }

  klee::AndExpr *and_lhs = dynamic_cast<klee::AndExpr *>(shl_expr->left.get());
  klee::AndExpr *and_rhs = dynamic_cast<klee::AndExpr *>(ashr_expr->left.get());

  if (and_lhs->left->getKind() != klee::Expr::ZExt || and_rhs->left->getKind() != klee::Expr::ZExt) {
    return false;
  }

  u64 and_lhs_right_const;
  if (!solver_toolbox.strict_value_from_expr(and_lhs->right, and_lhs_right_const) || and_lhs_right_const != 0xff) {
    return false;
  }

  u64 and_rhs_right_const;
  if (!solver_toolbox.strict_value_from_expr(and_rhs->right, and_rhs_right_const) || and_rhs_right_const != 0xff00) {
    return false;
  }

  klee::ZExtExpr *zext_lhs = dynamic_cast<klee::ZExtExpr *>(and_lhs->left.get());
  klee::ZExtExpr *zext_rhs = dynamic_cast<klee::ZExtExpr *>(and_rhs->left.get());

  if (!solver_toolbox.are_exprs_always_equal(zext_lhs->src, zext_rhs->src)) {
    return false;
  }

  target = zext_lhs->src;

  return true;
}

bool match_endian_swap_pattern(klee::ref<klee::Expr> expr, klee::ref<klee::Expr> &target) {
  if (match_endian_swap_16_pattern(expr, target)) {
    return true;
  }

  // TODO: detect 32 bit endian swaps.

  return false;
}

klee::ref<klee::Expr> concat_exprs(const std::vector<klee::ref<klee::Expr>> &exprs, bool left_to_right) {
  klee::ref<klee::Expr> result;

  if (exprs.empty()) {
    return result;
  }

  std::vector<klee::ref<klee::Expr>> target_exprs;

  if (left_to_right) {
    for (auto it = exprs.rbegin(); it != exprs.rend(); it++) {
      klee::ref<klee::Expr> expr = *it;
      target_exprs.push_back(expr);
    }
  } else {
    target_exprs = exprs;
  }

  for (auto it = target_exprs.rbegin(); it != target_exprs.rend(); it++) {
    klee::ref<klee::Expr> expr = *it;
    if (result.isNull()) {
      result = expr;
    } else {
      result = solver_toolbox.exprBuilder->Concat(expr, result);
    }
  }

  return result;
}

bool is_smaller_expr_contained_in_expr(klee::ref<klee::Expr> smaller_expr, klee::ref<klee::Expr> expr, expr_pos_t &pos) {
  const bits_t smaller_size = smaller_expr->getWidth();
  const bits_t bigger_size  = expr->getWidth();

  if (smaller_size >= bigger_size) {
    return false;
  }

  for (bits_t i = 0; i < bigger_size - smaller_size; i += 8) {
    klee::ref<klee::Expr> sub_expr = solver_toolbox.exprBuilder->Extract(expr, i, smaller_size);
    if (solver_toolbox.are_exprs_always_equal(sub_expr, smaller_expr)) {
      pos.offset = i;
      pos.size   = smaller_size;
      return true;
    }
  }

  return false;
}

std::vector<klee::ref<klee::Expr>> split_expr(klee::ref<klee::Expr> expr, expr_pos_t pos) {
  assert(!expr.isNull() && "Invalid expr");

  const bits_t size = expr->getWidth();
  assert(pos.offset < size && "Offset greater than size");
  assert(pos.size > 0 && "Zero size");
  assert(pos.size <= size && "Size is too big");
  assert(pos.offset + pos.size <= size && "Out of bounds");

  std::vector<klee::ref<klee::Expr>> sub_exprs;

  if (pos.offset > 0) {
    klee::ref<klee::Expr> left = solver_toolbox.exprBuilder->Extract(expr, 0, pos.offset);
    sub_exprs.insert(sub_exprs.begin(), simplify(left));
  }

  klee::ref<klee::Expr> middle = solver_toolbox.exprBuilder->Extract(expr, pos.offset, pos.size);
  sub_exprs.insert(sub_exprs.begin(), simplify(middle));

  if (pos.offset + pos.size < size) {
    klee::ref<klee::Expr> right = solver_toolbox.exprBuilder->Extract(expr, pos.offset + pos.size, size - pos.offset - pos.size);
    sub_exprs.insert(sub_exprs.begin(), simplify(right));
  }

  return sub_exprs;
}

std::vector<klee::ref<klee::Expr>> break_expr_into_structs_aware_chunks(klee::ref<klee::Expr> target_expr,
                                                                        const std::vector<expr_struct_t> &expr_structs,
                                                                        std::optional<bytes_t> max_chunk_size) {
  std::vector<klee::ref<klee::Expr>> chunks;

  for (const expr_group_t &group : get_expr_groups(target_expr)) {
    std::vector<klee::ref<klee::Expr>> subgroups{group.expr};

    bool changes = true;
    while (changes) {
      changes = false;

      std::vector<klee::ref<klee::Expr>> subgroups_copy = subgroups;
      subgroups.clear();

      for (klee::ref<klee::Expr> expr : subgroups_copy) {

        bool split = false;
        for (const expr_struct_t &expr_struct : expr_structs) {
          for (klee::ref<klee::Expr> field : expr_struct.fields) {
            expr_pos_t pos;
            if (is_smaller_expr_contained_in_expr(field, expr, pos)) {
              const std::vector<klee::ref<klee::Expr>> slices = split_expr(expr, pos);
              subgroups.insert(subgroups.end(), slices.begin(), slices.end());

              split   = true;
              changes = true;

              break;
            }
          }

          if (split) {
            break;
          }
        }

        if (!split) {
          subgroups.push_back(expr);
        }
      }
    }

    for (klee::ref<klee::Expr> subgroup : subgroups) {
      if (!max_chunk_size.has_value()) {
        chunks.push_back(subgroup);
        continue;
      }

      const bytes_t total_size = subgroup->getWidth() / 8;

      bytes_t processed_bytes = 0;
      while (processed_bytes != total_size) {
        const bytes_t target_size = std::min(max_chunk_size.value(), total_size - processed_bytes);
        const bytes_t offset      = total_size - processed_bytes - target_size;

        klee::ref<klee::Expr> subsubgroup = solver_toolbox.exprBuilder->Extract(subgroup, offset * 8, target_size * 8);
        subsubgroup                       = simplify(subsubgroup);

        chunks.push_back(subsubgroup);
        processed_bytes += target_size;
      }
    }
  }

  std::reverse(chunks.begin(), chunks.end());

  klee::ref<klee::Expr> merged = concat_exprs(chunks);
  if (!solver_toolbox.are_exprs_always_equal(merged, target_expr)) {
    std::stringstream ss;
    ss << "***** Bug in break_expr_into_structs_aware_chunks *****\n";
    ss << "Target expr: " << expr_to_string(target_expr) << "\n";
    ss << "Chunks:\n";
    for (klee::ref<klee::Expr> chunk : chunks) {
      ss << "  " << expr_to_string(chunk, true) << "\n";
    }
    ss << "Merged expr: " << expr_to_string(merged) << "\n";
    panic("%s", ss.str().c_str());
  }

  return chunks;
}

} // namespace LibCore