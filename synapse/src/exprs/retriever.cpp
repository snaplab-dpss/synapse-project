#include <iostream>

#include <klee/util/ExprVisitor.h>

#include "retriever.h"
#include "simplifier.h"
#include "solver.h"
#include "exprs.h"
#include "../log.h"

namespace synapse {
class SymbolRetriever : public klee::ExprVisitor::ExprVisitor {
private:
  std::unordered_map<std::string, klee::UpdateList> roots_updates;
  std::vector<klee::ref<klee::ReadExpr>> retrieved_reads;
  std::vector<klee::ref<klee::ReadExpr>> retrieved_reads_packet_chunks;
  std::unordered_set<std::string> retrieved_strings;

public:
  SymbolRetriever() : ExprVisitor(true) {}

  klee::ExprVisitor::Action visitRead(const klee::ReadExpr &e) {
    klee::ref<klee::ReadExpr> expr = const_cast<klee::ReadExpr *>(&e);
    const klee::UpdateList &ul = e.updates;
    const klee::Array *root = ul.root;

    retrieved_strings.insert(root->name);
    retrieved_reads.emplace_back(expr);
    roots_updates.insert({root->name, ul});

    if (root->name == "packet_chunks") {
      retrieved_reads_packet_chunks.emplace_back(expr);
    }

    return klee::ExprVisitor::Action::doChildren();
  }

  const std::vector<klee::ref<klee::ReadExpr>> &get_retrieved() {
    return retrieved_reads;
  }

  const std::vector<klee::ref<klee::ReadExpr>> &get_retrieved_packet_chunks() {
    return retrieved_reads_packet_chunks;
  }

  const std::unordered_set<std::string> &get_retrieved_strings() {
    return retrieved_strings;
  }

  const std::unordered_map<std::string, klee::UpdateList> &get_retrieved_roots_updates() {
    return roots_updates;
  }

  static bool contains(klee::ref<klee::Expr> expr, const std::string &symbol) {
    SymbolRetriever retriever;
    retriever.visit(expr);
    auto symbols = retriever.get_retrieved_strings();
    auto found_it = std::find(symbols.begin(), symbols.end(), symbol);
    return found_it != symbols.end();
  }
};

std::vector<klee::ref<klee::ReadExpr>> get_reads(klee::ref<klee::Expr> expr) {
  SymbolRetriever retriever;
  retriever.visit(expr);
  return retriever.get_retrieved();
}

std::vector<klee::ref<klee::ReadExpr>> get_packet_reads(klee::ref<klee::Expr> expr) {
  SymbolRetriever retriever;
  retriever.visit(expr);
  return retriever.get_retrieved_packet_chunks();
}

bool has_symbols(klee::ref<klee::Expr> expr) {
  if (expr.isNull())
    return false;

  return !get_symbols(expr).empty();
}

std::unordered_set<std::string> get_symbols(klee::ref<klee::Expr> expr) {
  SymbolRetriever retriever;
  retriever.visit(expr);
  return retriever.get_retrieved_strings();
}

std::vector<byte_read_t> get_bytes_read(klee::ref<klee::Expr> expr) {
  static std::unordered_map<unsigned,
                            std::pair<klee::ref<klee::Expr>, std::vector<byte_read_t>>>
      cache;

  auto cache_found_it = cache.find(expr->hash());
  if (cache_found_it != cache.end()) {
    ASSERT(expr == cache_found_it->second.first, "Hash collision");
    return cache_found_it->second.second;
  }

  std::vector<byte_read_t> bytes;

  switch (expr->getKind()) {
  case klee::Expr::Kind::Read: {
    klee::ReadExpr *read = dyn_cast<klee::ReadExpr>(expr);
    klee::ref<klee::Expr> index = read->index;

    if (index->getKind() == klee::Expr::Kind::Constant) {
      klee::ConstantExpr *index_const = dynamic_cast<klee::ConstantExpr *>(index.get());

      unsigned byte = index_const->getZExtValue();
      std::string symbol = read->updates.root->name;

      bytes.push_back({byte, symbol});
    }
  } break;
  case klee::Expr::Kind::Concat: {
    klee::ConcatExpr *concat = dyn_cast<klee::ConcatExpr>(expr);

    klee::ref<klee::Expr> left = concat->getLeft();
    klee::ref<klee::Expr> right = concat->getRight();

    std::vector<byte_read_t> lbytes = get_bytes_read(left);
    std::vector<byte_read_t> rbytes = get_bytes_read(right);

    bytes.insert(bytes.end(), lbytes.begin(), lbytes.end());
    bytes.insert(bytes.end(), rbytes.begin(), rbytes.end());
  } break;
  default:
    // Nothing to do here.
    break;
  }

  cache[expr->hash()] = {expr, bytes};

  return bytes;
}

klee::ref<klee::Expr> concat_lsb(klee::ref<klee::Expr> expr, klee::ref<klee::Expr> byte) {
  if (expr->getKind() != klee::Expr::Concat) {
    return solver_toolbox.exprBuilder->Concat(expr, byte);
  }

  auto lhs = expr->getKid(0);
  auto rhs = expr->getKid(1);

  klee::ref<klee::Expr> new_kids[] = {lhs, concat_lsb(rhs, byte)};
  return expr->rebuild(new_kids);
}

std::vector<expr_group_t> get_expr_groups(klee::ref<klee::Expr> expr) {
  std::vector<expr_group_t> groups;

  auto old = expr;

  auto process_read = [&](klee::ref<klee::Expr> read_expr) {
    ASSERT(read_expr->getKind() == klee::Expr::Read, "Not a read");
    klee::ReadExpr *read = dyn_cast<klee::ReadExpr>(read_expr);

    klee::ref<klee::Expr> index = read->index;
    const std::string symbol = read->updates.root->name;

    ASSERT(index->getKind() == klee::Expr::Kind::Constant, "Non-constant index");

    klee::ConstantExpr *index_const = dynamic_cast<klee::ConstantExpr *>(index.get());

    unsigned byte = index_const->getZExtValue();

    if (groups.size() && groups.back().has_symbol && groups.back().symbol == symbol &&
        groups.back().offset - 1 == byte) {
      groups.back().n_bytes++;
      groups.back().offset = byte;
      groups.back().expr = concat_lsb(groups.back().expr, read_expr);
    } else {
      groups.emplace_back(
          expr_group_t{true, symbol, byte, read_expr->getWidth() / 8, read_expr});
    }
  };

  auto process_not_read = [&](klee::ref<klee::Expr> not_read_expr) {
    ASSERT(not_read_expr->getKind() != klee::Expr::Read, "Non read is actually a read");
    unsigned size = not_read_expr->getWidth();
    ASSERT(size % 8 == 0, "Size not multiple of 8");
    groups.emplace_back(expr_group_t{false, "", 0, size / 8, not_read_expr});
  };

  if (expr->getKind() == klee::Expr::Extract) {
    klee::ref<klee::Expr> extract_expr = expr;
    klee::ref<klee::Expr> out;
    if (simplify_extract(extract_expr, out)) {
      expr = out;
    }
  }

  while (expr->getKind() == klee::Expr::Concat) {
    klee::ref<klee::Expr> lhs = expr->getKid(0);
    klee::ref<klee::Expr> rhs = expr->getKid(1);

    ASSERT(lhs->getKind() != klee::Expr::Concat, "Nested concats");

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

void print_groups(const std::vector<expr_group_t> &groups) {
  std::cerr << "Groups: " << groups.size() << "\n";
  for (const auto &group : groups) {
    if (group.has_symbol) {
      std::cerr << "Group:" << " symbol=" << group.symbol << " offset=" << group.offset
                << " n_bytes=" << group.n_bytes
                << " expr=" << expr_to_string(group.expr, true) << "\n";
    } else {
      std::cerr << "Group: offset=" << group.offset << " n_bytes=" << group.n_bytes
                << " expr=" << expr_to_string(group.expr, true) << "\n";
    }
  }
}
} // namespace synapse