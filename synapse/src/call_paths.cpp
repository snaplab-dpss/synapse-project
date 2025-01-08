#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

#include <klee/ExprBuilder.h>
#include <klee/perf-contracts.h>
#include <klee/Constraints.h>
#include <klee/Solver.h>
#include <expr/Parser.h>

#include <llvm/Support/MemoryBuffer.h>

#include "call_paths.h"
#include "util/exprs.h"
#include "util/solver.h"
#include "util/kQuery.h"
#include "system.h"

namespace synapse {

std::unique_ptr<call_path_t> load_call_path(const std::filesystem::path &fpath,
                                            SymbolManager *manager) {
  kQueryParser parser(manager);

  std::ifstream call_path_file(fpath.string());
  assert(call_path_file.is_open() && "Unable to open call path file.");

  std::unique_ptr<call_path_t> call_path = std::make_unique<call_path_t>();
  call_path->file_name = fpath.filename();

  enum class state_t {
    STATE_INIT,
    STATE_KQUERY,
    STATE_CALLS,
    STATE_CALLS_MULTILINE,
    STATE_DONE
  } state = state_t::STATE_INIT;

  std::vector<klee::ref<klee::Expr>> values;
  std::set<std::string> declared_arrays;

  int parenthesis_level = 0;

  std::string query_str;

  std::string current_extra_var;
  std::string current_arg;
  std::string current_arg_name;

  std::string current_expr_str;
  std::vector<std::string> current_exprs_str;

  while (!call_path_file.eof()) {
    std::string line;
    std::getline(call_path_file, line);

    switch (state) {
    case state_t::STATE_INIT: {
      if (line == ";;-- kQuery --") {
        state = state_t::STATE_KQUERY;
      }
    } break;

    case state_t::STATE_KQUERY: {
      if (line == ";;-- Calls --") {
        if (query_str.substr(query_str.length() - 2) == "])") {
          query_str = query_str.substr(0, query_str.length() - 2) + "\n";
          query_str += "])";
        } else if (query_str.substr(query_str.length() - 6) == "false)") {
          query_str = query_str.substr(0, query_str.length() - 1) + " [\n";
          query_str += "])";
        }

        kQuery_t kQuery = parser.parse(query_str);

        call_path->symbols = kQuery.symbols;

        for (klee::ref<klee::Expr> constraint : kQuery.constraints) {
          call_path->constraints.addConstraint(constraint);
        }

        for (klee::ref<klee::Expr> value : kQuery.values) {
          values.push_back(value);
        }

        state = state_t::STATE_CALLS;
      } else {
        query_str += "\n" + line;

        if (line.substr(0, sizeof("array ") - 1) == "array ") {
          declared_arrays.insert(line);
        }
      }
    } break;

    case state_t::STATE_CALLS: {
      if (line == ";;-- Constraints --") {
        assert(values.empty() && "Too many expressions in kQuery.");
        state = state_t::STATE_DONE;
      } else {
        size_t delim = line.find(":");
        assert(delim != std::string::npos && "Invalid call");
        std::string preamble = line.substr(0, delim);
        line = line.substr(delim + 1);

        current_extra_var.clear();
        current_exprs_str.clear();

        if (preamble == "extra") {
          while (line[0] == ' ') {
            line = line.substr(1);
          }

          delim = line.find("&");
          assert(delim != std::string::npos && "Invalid call");
          current_extra_var = line.substr(0, delim);
          line = line.substr(delim + 1);

          delim = line.find("[");
          assert(delim != std::string::npos && "Invalid call");
          line = line.substr(delim + 1);
        } else {
          call_path->calls.emplace_back();

          delim = line.find("(");
          assert(delim != std::string::npos && "Invalid call");
          call_path->calls.back().function_name = line.substr(0, delim);
        }

        for (char c : line) {
          current_expr_str += c;
          if (c == '(') {
            if (parenthesis_level == 0) {
              current_expr_str = "(";
            }
            parenthesis_level++;
          } else if (c == ')') {
            parenthesis_level--;
            assert(parenthesis_level >= 0 && "Invalid call");

            if (parenthesis_level == 0) {
              current_exprs_str.push_back(current_expr_str);
            }
          }
        }

        if (current_exprs_str.size() && parenthesis_level == 0) {
          auto last_store = current_exprs_str.back();
          if (line.size() < last_store.size())
            last_store = last_store.substr(last_store.size() - line.size());
          auto remainder_delim = line.find(last_store);
          auto remainder = line.substr(remainder_delim + last_store.size());
          auto ret_symbol = std::string("-> ");
          auto ret_delim = remainder.find(ret_symbol);
          if (ret_delim != std::string::npos && remainder.substr(ret_symbol.size() + 1) != "[]") {
            auto ret = remainder.substr(ret_symbol.size() + 1);
            current_exprs_str.push_back(ret);
          }
        }

        if (parenthesis_level > 0) {
          state = state_t::STATE_CALLS_MULTILINE;
        } else {
          if (!current_extra_var.empty()) {
            assert(current_exprs_str.size() == 2 && "Too many expression in extra variable.");
            if (current_exprs_str[0] != "(...)") {
              assert(values.size() >= 1 && "Not enough expression in kQuery.");
              call_path->calls.back().extra_vars[current_extra_var].first = values[0];
              values.erase(values.begin());
              current_exprs_str.erase(current_exprs_str.begin(), current_exprs_str.begin() + 2);
            }
            if (current_exprs_str[1] != "(...)") {
              assert(values.size() >= 1 && "Not enough expression in kQuery.");
              call_path->calls.back().extra_vars[current_extra_var].second = values[0];
              values.erase(values.begin());
              current_exprs_str.erase(current_exprs_str.begin(), current_exprs_str.begin() + 2);
            }
          } else {
            bool parsed_last_arg = false;
            while (!parsed_last_arg) {
              if (current_exprs_str[0] == "()")
                break;
              delim = current_exprs_str[0].find(",");
              if (delim == std::string::npos) {
                delim = current_exprs_str[0].size() - 1;
                parsed_last_arg = true;
              }
              current_arg = current_exprs_str[0].substr(0, delim);
              if (current_arg[0] == '(')
                current_arg = current_arg.substr(1);
              current_exprs_str[0] = current_exprs_str[0].substr(delim + 1);
              delim = current_arg.find(":");
              assert(delim != std::string::npos && "Invalid call");
              current_arg_name = current_arg.substr(0, delim);
              current_arg = current_arg.substr(delim + 1);

              delim = current_arg.find("&");
              if (delim == std::string::npos) {
                assert(values.size() >= 1 && "Not enough expression in kQuery.");
                call_path->calls.back().args[current_arg_name].expr = values[0];
                values.erase(values.begin(), values.begin() + 1);
              } else {
                call_path->calls.back().args[current_arg_name].expr = values[0];
                values.erase(values.begin(), values.begin() + 1);

                if (current_arg.substr(delim + 1) == "[...]") {
                  continue;
                }

                if (current_arg.substr(delim + 1)[0] != '[') {
                  call_path->calls.back().args[current_arg_name].fn_ptr_name =
                      std::make_pair(true, current_arg.substr(delim + 1));
                  continue;
                }

                current_arg = current_arg.substr(delim + 2);

                delim = current_arg.find("]");
                assert(delim != std::string::npos && "Invalid call");

                auto current_arg_meta = current_arg.substr(delim + 1);
                current_arg = current_arg.substr(0, delim);

                delim = current_arg.find("->");
                assert(delim != std::string::npos && "Invalid call");

                if (current_arg.substr(0, delim).size()) {
                  assert(values.size() >= 1 && "Not enough expression in kQuery.");
                  call_path->calls.back().args[current_arg_name].in = values[0];
                  values.erase(values.begin(), values.begin() + 1);
                }

                if (current_arg.substr(delim + 2).size()) {
                  assert(values.size() >= 1 && "Not enough expression in kQuery.");
                  call_path->calls.back().args[current_arg_name].out = values[0];
                  values.erase(values.begin(), values.begin() + 1);
                }

                if (current_arg_meta.size()) {
                  std::vector<std::string> expr_parts;

                  while (current_arg_meta.size()) {
                    auto start_delim = current_arg_meta.find("[");
                    auto end_delim = current_arg_meta.find("]");

                    assert(start_delim != std::string::npos && "Invalid call");
                    assert(end_delim != std::string::npos && "Invalid call");

                    auto size = end_delim - start_delim - 1;
                    assert(size > 0 && "Invalid call");

                    auto part = current_arg_meta.substr(start_delim + 1, end_delim - 1);
                    current_arg_meta = current_arg_meta.substr(end_delim + 1);

                    expr_parts.push_back(part);
                  }

                  bits_t offset = 0;

                  for (auto part : expr_parts) {
                    delim = part.find("->");
                    assert(delim != std::string::npos && "Invalid call");

                    part = part.substr(0, delim);

                    auto open_delim = part.find("(");
                    auto close_delim = part.find(")");

                    assert(open_delim != std::string::npos && "Invalid call");
                    assert(close_delim != std::string::npos && "Invalid call");

                    auto symbol = part.substr(0, open_delim);
                    auto meta_expr_str = part.substr(open_delim + 1);
                    meta_expr_str = meta_expr_str.substr(0, meta_expr_str.size() - 1);

                    auto meta_expr = parser.parse_expr(meta_expr_str);
                    auto meta_size = meta_expr->getWidth();
                    auto meta = meta_t{symbol, offset, meta_size};

                    offset += meta_size;

                    call_path->calls.back().args[current_arg_name].meta.push_back(meta);
                  }
                }
              }
            }
            current_exprs_str.erase(current_exprs_str.begin());
          }

          if (current_exprs_str.size()) {
            call_path->calls.back().ret = values[0];
            values.erase(values.begin());
          }
        }
      }
    } break;

    case state_t::STATE_CALLS_MULTILINE: {
      current_expr_str += " ";
      for (char c : line) {
        current_expr_str += c;
        if (c == '(') {
          if (parenthesis_level == 0) {
            current_expr_str = "(";
          }
          parenthesis_level++;
        } else if (c == ')') {
          parenthesis_level--;
          assert(parenthesis_level >= 0 && "Invalid call");

          if (parenthesis_level == 0) {
            current_exprs_str.push_back(current_expr_str);
          }
        }
      }

      if (current_exprs_str.size() && parenthesis_level == 0) {
        auto last_store = current_exprs_str.back();
        if (line.size() < last_store.size())
          last_store = last_store.substr(last_store.size() - line.size());
        auto remainder_delim = line.find(last_store);
        auto remainder = line.substr(remainder_delim + last_store.size());
        auto ret_symbol = std::string("-> ");
        auto ret_delim = remainder.find(ret_symbol);
        if (ret_delim != std::string::npos && remainder.substr(ret_symbol.size() + 1) != "[]") {
          auto ret = remainder.substr(ret_symbol.size() + 1);
          current_exprs_str.push_back(ret);
        }
      }

      if (parenthesis_level == 0) {
        if (!current_extra_var.empty()) {
          assert(current_exprs_str.size() == 2 && "Too many expression in extra variable.");
          if (current_exprs_str[0] != "(...)") {
            assert(values.size() >= 1 && "Not enough expression in kQuery.");
            call_path->calls.back().extra_vars[current_extra_var].first = values[0];
            values.erase(values.begin());
            current_exprs_str.erase(current_exprs_str.begin(), current_exprs_str.begin() + 2);
          }
          if (current_exprs_str[1] != "(...)") {
            assert(values.size() >= 1 && "Not enough expression in kQuery.");
            call_path->calls.back().extra_vars[current_extra_var].second = values[0];
            values.erase(values.begin());
            current_exprs_str.erase(current_exprs_str.begin(), current_exprs_str.begin() + 2);
          }
        } else {
          bool parsed_last_arg = false;
          size_t delim;

          while (!parsed_last_arg) {
            if (current_exprs_str[0] == "()")
              break;
            delim = current_exprs_str[0].find(",");
            if (delim == std::string::npos) {
              delim = current_exprs_str[0].size() - 1;
              parsed_last_arg = true;
            }
            current_arg = current_exprs_str[0].substr(0, delim);
            if (current_arg[0] == '(')
              current_arg = current_arg.substr(1);
            current_exprs_str[0] = current_exprs_str[0].substr(delim + 1);
            delim = current_arg.find(":");
            assert(delim != std::string::npos && "Invalid call");
            current_arg_name = current_arg.substr(0, delim);
            current_arg = current_arg.substr(delim + 1);

            delim = current_arg.find("&");
            if (delim == std::string::npos) {
              assert(values.size() >= 1 && "Not enough expression in kQuery.");
              call_path->calls.back().args[current_arg_name].expr = values[0];
              values.erase(values.begin(), values.begin() + 1);
            } else {
              call_path->calls.back().args[current_arg_name].expr = values[0];
              values.erase(values.begin(), values.begin() + 1);

              if (current_arg.substr(delim + 1) == "[...]") {
                continue;
              }

              if (current_arg.substr(delim + 1)[0] != '[') {
                call_path->calls.back().args[current_arg_name].fn_ptr_name =
                    std::make_pair(true, current_arg.substr(delim + 1));
                continue;
              }

              current_arg = current_arg.substr(delim + 2);

              delim = current_arg.find("]");
              assert(delim != std::string::npos && "Invalid call");

              auto current_arg_meta = current_arg.substr(delim + 1);
              current_arg = current_arg.substr(0, delim);

              delim = current_arg.find("->");
              assert(delim != std::string::npos && "Invalid call");

              if (current_arg.substr(0, delim).size()) {
                assert(values.size() >= 1 && "Not enough expression in kQuery.");
                call_path->calls.back().args[current_arg_name].in = values[0];
                values.erase(values.begin(), values.begin() + 1);
              }

              if (current_arg.substr(delim + 2).size()) {
                assert(values.size() >= 1 && "Not enough expression in kQuery.");
                call_path->calls.back().args[current_arg_name].out = values[0];
                values.erase(values.begin(), values.begin() + 1);
              }

              if (current_arg_meta.size()) {
                std::vector<std::string> expr_parts;

                while (current_arg_meta.size()) {
                  auto start_delim = current_arg_meta.find("[");
                  auto end_delim = current_arg_meta.find("]");

                  assert(start_delim != std::string::npos && "Invalid call");
                  assert(end_delim != std::string::npos && "Invalid call");

                  auto size = end_delim - start_delim - 1;
                  assert(size > 0 && "Invalid call");

                  auto part = current_arg_meta.substr(start_delim + 1, end_delim - 1);
                  current_arg_meta = current_arg_meta.substr(end_delim + 1);

                  expr_parts.push_back(part);
                }

                bits_t offset = 0;

                for (auto part : expr_parts) {
                  delim = part.find("->");
                  assert(delim != std::string::npos && "Invalid call");

                  part = part.substr(0, delim);

                  auto open_delim = part.find("(");
                  auto close_delim = part.find(")");

                  assert(open_delim != std::string::npos && "Invalid call");
                  assert(close_delim != std::string::npos && "Invalid call");

                  auto symbol = part.substr(0, open_delim);
                  auto meta_expr_str = part.substr(open_delim + 1);
                  meta_expr_str = meta_expr_str.substr(0, meta_expr_str.size() - 1);

                  auto meta_expr = parser.parse_expr(meta_expr_str);
                  auto meta_size = meta_expr->getWidth();
                  auto meta = meta_t{symbol, offset, meta_size};

                  offset += meta_size;

                  call_path->calls.back().args[current_arg_name].meta.push_back(meta);
                }
              }
            }
          }

          current_exprs_str.erase(current_exprs_str.begin());
        }

        if (current_exprs_str.size()) {
          call_path->calls.back().ret = values[0];
          values.erase(values.begin());
        }

        state = state_t::STATE_CALLS;
      }

      continue;
    } break;

    case state_t::STATE_DONE: {
      continue;
    } break;

    default: {
      panic("Invalid call path file.");
    } break;
    }
  }

  return call_path;
}

Symbols call_paths_view_t::get_symbols() const {
  Symbols symbols;
  for (const call_path_t *cp : data) {
    symbols.add(cp->symbols);
  }
  return symbols;
}

call_paths_t::call_paths_t(const std::vector<std::filesystem::path> &call_path_files,
                           SymbolManager *_manager)
    : manager(_manager) {
  for (const std::filesystem::path &fpath : call_path_files) {
    data.push_back(load_call_path(fpath, manager));
  }

  for (const auto &call_path : data) {
    for (const auto &expr : call_path->constraints) {
      assert(manager->manages(expr));
    }
    for (const auto &call : call_path->calls) {
      for (const auto &arg : call.args) {
        assert(manager->manages(arg.second.expr));
        assert(manager->manages(arg.second.in));
        assert(manager->manages(arg.second.out));
      }
      for (const auto &extra_var : call.extra_vars) {
        assert(manager->manages(extra_var.second.first));
        assert(manager->manages(extra_var.second.second));
      }
      assert(manager->manages(call.ret));
    }
  }
}

call_paths_view_t call_paths_t::get_view() const {
  call_paths_view_t view;
  for (const std::unique_ptr<call_path_t> &cp : data) {
    view.data.push_back(cp.get());
  }
  view.manager = manager;
  return view;
}

std::ostream &operator<<(std::ostream &os, const arg_t &arg) {
  if (arg.fn_ptr_name.first) {
    os << arg.fn_ptr_name.second;
    return os;
  }

  os << expr_to_string(arg.expr, true);

  if (!arg.in.isNull() || !arg.out.isNull()) {
    os << "[";

    if (!arg.in.isNull()) {
      os << expr_to_string(arg.in, true);
    }

    os << " -> ";

    if (!arg.out.isNull()) {
      os << expr_to_string(arg.out, true);
    }

    os << "]";
  }

  return os;
}

std::ostream &operator<<(std::ostream &os, const call_t &call) {
  os << call.function_name;
  os << "(";

  bool first = true;
  for (auto arg_pair : call.args) {
    auto label = arg_pair.first;
    auto arg = arg_pair.second;

    if (!first) {
      os << ",";
    }

    os << label << ":";
    os << arg;

    first = false;
  }

  os << ")";

  if (!call.ret.isNull()) {
    os << " => ";
    os << expr_to_string(call.ret, true);
  }

  return os;
}

std::ostream &operator<<(std::ostream &str, const call_path_t &cp) {
  str << "Callpath:\n";

  for (auto call : cp.calls) {
    str << "  " << call << "\n";
  }

  return str;
}

std::ostream &operator<<(std::ostream &os, klee::ref<klee::Expr> expr) {
  os << expr_to_string(expr, true);
  return os;
}
} // namespace synapse