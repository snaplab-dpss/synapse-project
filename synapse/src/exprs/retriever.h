#pragma once

#include <vector>
#include <unordered_set>
#include <optional>

#include <klee/Expr.h>

std::vector<klee::ref<klee::ReadExpr>> get_reads(klee::ref<klee::Expr> expr);
std::vector<klee::ref<klee::ReadExpr>> get_packet_reads(klee::ref<klee::Expr> expr);

bool has_symbols(klee::ref<klee::Expr> expr);
std::unordered_set<std::string> get_symbols(klee::ref<klee::Expr> expr);
std::optional<std::string> get_symbol(klee::ref<klee::Expr> expr);

struct byte_read_t {
  unsigned offset;
  std::string symbol;
};

struct expr_group_t {
  bool has_symbol;
  std::string symbol;
  unsigned offset;
  unsigned n_bytes;
  klee::ref<klee::Expr> expr;
};

std::vector<byte_read_t> get_bytes_read(klee::ref<klee::Expr> expr);
std::vector<expr_group_t> get_expr_groups(klee::ref<klee::Expr> expr);
void print_groups(const std::vector<expr_group_t> &groups);