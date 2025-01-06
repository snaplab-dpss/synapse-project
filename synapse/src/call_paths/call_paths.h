#pragma once

#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <vector>

#include <klee/Constraints.h>

#include "../types.h"
#include "../exprs/symbol.h"
#include "../exprs/array_manager.h"

namespace synapse {
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

typedef std::pair<klee::ref<klee::Expr>, klee::ref<klee::Expr>> extra_var_t;

struct call_t {
  std::string function_name;
  std::unordered_map<std::string, extra_var_t> extra_vars;
  std::unordered_map<std::string, arg_t> args;
  klee::ref<klee::Expr> ret;
};

typedef std::vector<call_t> calls_t;

struct call_path_t {
  std::string file_name;
  klee::ConstraintManager constraints;
  symbols_t symbols; // TODO: remove, arrays are stored in ArrayManager
  calls_t calls;
};

std::unique_ptr<call_path_t> load_call_path(const std::filesystem::path &fpath,
                                            ArrayManager *manager);

typedef std::vector<call_path_t *> call_paths_view_t;

struct call_paths_t {
  std::vector<std::unique_ptr<call_path_t>> cps;

  call_paths_t(const std::vector<std::filesystem::path> &call_path_files,
               ArrayManager *manager);

  call_paths_view_t get_view() const;
};

std::ostream &operator<<(std::ostream &os, klee::ref<klee::Expr> expr);
std::ostream &operator<<(std::ostream &os, const arg_t &arg);
std::ostream &operator<<(std::ostream &os, const call_t &call);
std::ostream &operator<<(std::ostream &str, const call_path_t &cp);
} // namespace synapse