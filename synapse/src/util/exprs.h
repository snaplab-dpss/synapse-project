#pragma once

#include <unordered_set>
#include <optional>

#include <klee/Expr.h>
#include <klee/Constraints.h>

#include "../types.h"

namespace synapse {
std::string expr_to_string(klee::ref<klee::Expr> expr, bool one_liner = false);
std::string pretty_print_expr(klee::ref<klee::Expr> expr, bool use_signed = true);

bool is_readLSB(klee::ref<klee::Expr> expr);
bool is_readLSB(klee::ref<klee::Expr> expr, std::string &symbol);
bool is_packet_readLSB(klee::ref<klee::Expr> expr);
bool is_packet_readLSB(klee::ref<klee::Expr> expr, bytes_t &offset, int &n_bytes);
bool is_bool(klee::ref<klee::Expr> expr);
bool is_constant(klee::ref<klee::Expr> expr);
bool is_constant_signed(klee::ref<klee::Expr> expr);

i64 get_constant_signed(klee::ref<klee::Expr> expr);
bool manager_contains(const klee::ConstraintManager &constraints, klee::ref<klee::Expr> expr);
klee::ConstraintManager join_managers(const klee::ConstraintManager &m1,
                                      const klee::ConstraintManager &m2);
addr_t expr_addr_to_obj_addr(klee::ref<klee::Expr> obj_addr);

klee::ref<klee::Expr> swap_packet_endianness(klee::ref<klee::Expr> expr);
klee::ref<klee::Expr> constraint_from_expr(klee::ref<klee::Expr> expr);

klee::ref<klee::Expr> filter(klee::ref<klee::Expr> expr,
                             const std::vector<std::string> &allowed_symbols);

struct mod_t {
  bytes_t offset;
  bits_t width;
  klee::ref<klee::Expr> expr;

  mod_t(u32 _offset, bits_t _width, klee::ref<klee::Expr> _expr)
      : offset(_offset), width(_width), expr(_expr) {}
  mod_t(const mod_t &mod) : offset(mod.offset), width(mod.width), expr(mod.expr) {}
};

std::vector<mod_t> build_expr_mods(klee::ref<klee::Expr> before, klee::ref<klee::Expr> after);
std::vector<klee::ref<klee::Expr>> bytes_in_expr(klee::ref<klee::Expr> expr);

struct expr_group_t {
  bool has_symbol;
  std::string symbol;
  bytes_t offset;
  bytes_t size;
  klee::ref<klee::Expr> expr;
};

std::vector<expr_group_t> get_expr_groups(klee::ref<klee::Expr> expr);

struct symbolic_read_t {
  bytes_t offset;
  std::string symbol;
};

struct symbolic_read_hash_t {
  std::size_t operator()(const symbolic_read_t &s) const noexcept;
};

struct symbolic_read_equal_t {
  bool operator()(const symbolic_read_t &a, const symbolic_read_t &b) const noexcept;
};

typedef std::unordered_set<symbolic_read_t, symbolic_read_hash_t, symbolic_read_equal_t>
    symbolic_reads_t;

symbolic_reads_t get_unique_symbolic_reads(klee::ref<klee::Expr> expr,
                                           std::optional<std::string> symbol_filter = std::nullopt);
} // namespace synapse