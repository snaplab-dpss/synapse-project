#pragma once

#include <unordered_set>
#include <optional>

#include <klee/Expr.h>
#include <klee/Constraints.h>

#include "../types.h"

std::string expr_to_string(klee::ref<klee::Expr> expr, bool one_liner = false);
std::string pretty_print_expr(klee::ref<klee::Expr> expr,
                              bool use_signed = true);

bool is_readLSB(klee::ref<klee::Expr> expr, std::string &symbol);
bool is_packet_readLSB(klee::ref<klee::Expr> expr);
bool is_packet_readLSB(klee::ref<klee::Expr> expr, bytes_t &offset,
                       int &n_bytes);
bool is_bool(klee::ref<klee::Expr> expr);
bool is_constant(klee::ref<klee::Expr> expr);
bool is_constant_signed(klee::ref<klee::Expr> expr);

int64_t get_constant_signed(klee::ref<klee::Expr> expr);
bool manager_contains(const klee::ConstraintManager &constraints,
                      klee::ref<klee::Expr> expr);
klee::ConstraintManager join_managers(const klee::ConstraintManager &m1,
                                      const klee::ConstraintManager &m2);
addr_t expr_addr_to_obj_addr(klee::ref<klee::Expr> obj_addr);

klee::ref<klee::Expr> swap_packet_endianness(klee::ref<klee::Expr> expr);
klee::ref<klee::Expr> constraint_from_expr(klee::ref<klee::Expr> expr);

klee::ref<klee::Expr> filter(klee::ref<klee::Expr> expr,
                             const std::vector<std::string> &allowed_symbols);

struct modification_t {
  int byte;
  klee::ref<klee::Expr> expr;

  modification_t(int _byte, klee::ref<klee::Expr> _expr)
      : byte(_byte), expr(_expr) {}

  modification_t(const modification_t &modification)
      : byte(modification.byte), expr(modification.expr) {}
};

std::vector<modification_t> build_modifications(klee::ref<klee::Expr> before,
                                                klee::ref<klee::Expr> after);