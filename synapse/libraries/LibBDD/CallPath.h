#pragma once

#include <LibCore/Types.h>
#include <LibCore/SymbolManager.h>

#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <vector>

#include <klee/Constraints.h>

namespace LibBDD {

using LibCore::SymbolManager;
using LibCore::Symbols;

struct meta_t {
  std::string symbol;
  bits_t offset;
  bits_t size;
};

struct arg_t {
  klee::ref<klee::Expr> expr;
  std::pair<bool, std::string> fn_ptr_name;
  klee::ref<klee::Expr> in;
  klee::ref<klee::Expr> out;
  std::vector<meta_t> meta;
};

using extra_var_t = std::pair<klee::ref<klee::Expr>, klee::ref<klee::Expr>>;

struct call_t {
  std::string function_name;
  std::unordered_map<std::string, extra_var_t> extra_vars;
  std::unordered_map<std::string, arg_t> args;
  klee::ref<klee::Expr> ret;
};

using calls_t = std::vector<call_t>;
bool are_calls_equal(call_t c1, call_t c2);

struct call_path_t {
  std::string file_name;
  klee::ConstraintManager constraints;
  calls_t calls;
  Symbols symbols;
};

std::unique_ptr<call_path_t> load_call_path(const std::filesystem::path &fpath, SymbolManager *manager);

struct call_paths_view_t {
  std::vector<call_path_t *> data;
  SymbolManager *manager;

  call_paths_view_t() : manager(nullptr) {}
  call_paths_view_t(const call_paths_view_t &)            = default;
  call_paths_view_t(call_paths_view_t &&)                 = default;
  call_paths_view_t &operator=(const call_paths_view_t &) = default;

  Symbols get_symbols() const;
};

struct call_paths_t {
  std::vector<std::unique_ptr<call_path_t>> data;
  SymbolManager *manager;

  call_paths_t() : manager(nullptr) {}
  call_paths_t(const call_paths_t &)            = default;
  call_paths_t(call_paths_t &&)                 = default;
  call_paths_t &operator=(const call_paths_t &) = default;

  call_paths_t(const std::vector<std::filesystem::path> &call_path_files, SymbolManager *manager);

  call_paths_view_t get_view() const;
};

std::ostream &operator<<(std::ostream &os, klee::ref<klee::Expr> expr);
std::ostream &operator<<(std::ostream &os, const arg_t &arg);
std::ostream &operator<<(std::ostream &os, const call_t &call);
std::ostream &operator<<(std::ostream &str, const call_path_t &cp);

} // namespace LibBDD