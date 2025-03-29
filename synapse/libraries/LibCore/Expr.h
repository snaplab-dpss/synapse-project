#pragma once

#include <LibCore/Types.h>

#include <unordered_set>
#include <optional>

#include <klee/Expr.h>
#include <klee/Constraints.h>

namespace LibCore {

std::string expr_to_string(klee::ref<klee::Expr> expr, bool one_liner = false);
std::string pretty_print_expr(klee::ref<klee::Expr> expr, bool use_signed = true);

bool is_readLSB(klee::ref<klee::Expr> expr);
bool is_readLSB(klee::ref<klee::Expr> expr, std::string &symbol);
bool is_packet_readLSB(klee::ref<klee::Expr> expr);
bool is_packet_readLSB(klee::ref<klee::Expr> expr, bytes_t &offset, int &n_bytes);
bool is_bool(klee::ref<klee::Expr> expr);
bool is_constant(klee::ref<klee::Expr> expr);
bool is_constant_signed(klee::ref<klee::Expr> expr);
bool is_conditional(klee::ref<klee::Expr> expr);
bool match_endian_swap_pattern(klee::ref<klee::Expr> expr, klee::ref<klee::Expr> &target);

i64 get_constant_signed(klee::ref<klee::Expr> expr);
bool manager_contains(const klee::ConstraintManager &constraints, klee::ref<klee::Expr> expr);
klee::ConstraintManager join_managers(const klee::ConstraintManager &m1, const klee::ConstraintManager &m2);
addr_t expr_addr_to_obj_addr(klee::ref<klee::Expr> obj_addr);
klee::ref<klee::Expr> swap_packet_endianness(klee::ref<klee::Expr> expr);
klee::ref<klee::Expr> constraint_from_expr(klee::ref<klee::Expr> expr);
klee::ref<klee::Expr> filter(klee::ref<klee::Expr> expr, const std::vector<std::string> &allowed_symbols);
klee::ref<klee::Expr> simplify(klee::ref<klee::Expr> expr);
klee::ref<klee::Expr> simplify_conditional(klee::ref<klee::Expr> condition);
klee::ref<klee::Expr> concat_exprs(const std::vector<klee::ref<klee::Expr>> &exprs, bool left_to_right = true);
std::string expr_to_ascii(klee::ref<klee::Expr> expr);

struct expr_pos_t {
  bits_t offset;
  bits_t size;
};

bool is_smaller_expr_contained_in_expr(klee::ref<klee::Expr> smaller_expr, klee::ref<klee::Expr> expr, expr_pos_t &pos);
std::vector<klee::ref<klee::Expr>> split_expr(klee::ref<klee::Expr> expr, expr_pos_t pos);

struct expr_mod_t {
  bits_t offset;
  bits_t width;
  klee::ref<klee::Expr> expr;

  expr_mod_t(bits_t _offset, bits_t _width, klee::ref<klee::Expr> _expr) : offset(_offset), width(_width), expr(_expr) {}
  expr_mod_t(const expr_mod_t &mod)            = default;
  expr_mod_t(expr_mod_t &&mod)                 = default;
  expr_mod_t &operator=(const expr_mod_t &mod) = default;
};

std::vector<expr_mod_t> build_expr_mods(klee::ref<klee::Expr> before, klee::ref<klee::Expr> after);

struct expr_struct_t {
  klee::ref<klee::Expr> expr;
  std::vector<klee::ref<klee::Expr>> fields;
};

std::vector<klee::ref<klee::Expr>> break_expr_into_structs_aware_chunks(klee::ref<klee::Expr> expr,
                                                                        const std::vector<expr_struct_t> &expr_struct,
                                                                        const bytes_t max_chunk_size);

std::vector<klee::ref<klee::Expr>> bytes_in_expr(klee::ref<klee::Expr> expr, bool force_big_endian_for_constants = false);

struct expr_byte_swap_t {
  bytes_t byte0;
  bytes_t byte1;
};

std::vector<expr_byte_swap_t> get_expr_byte_swaps(klee::ref<klee::Expr> before, klee::ref<klee::Expr> after);

struct expr_group_t {
  bool has_symbol;
  std::string symbol;
  bytes_t offset;
  bytes_t size;
  klee::ref<klee::Expr> expr;
};

using expr_groups_t = std::vector<expr_group_t>;

std::vector<expr_group_t> get_expr_groups(klee::ref<klee::Expr> expr);
std::vector<expr_group_t> get_expr_groups_from_condition(klee::ref<klee::Expr> expr);

struct symbolic_read_t {
  bytes_t byte;
  std::string symbol;
};

struct symbolic_read_hash_t {
  std::size_t operator()(const symbolic_read_t &s) const noexcept;
};

struct symbolic_read_equal_t {
  bool operator()(const symbolic_read_t &a, const symbolic_read_t &b) const noexcept;
};

using symbolic_reads_t = std::unordered_set<symbolic_read_t, symbolic_read_hash_t, symbolic_read_equal_t>;

symbolic_reads_t get_unique_symbolic_reads(klee::ref<klee::Expr> expr, std::optional<std::string> symbol_filter = std::nullopt);

} // namespace LibCore