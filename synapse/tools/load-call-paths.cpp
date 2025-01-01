#include <iostream>
#include <vector>
#include <CLI/CLI.hpp>

#include "../src/call_paths/call_paths.h"
#include "../src/exprs/exprs.h"

int main(int argc, char **argv, char **envp) {
  CLI::App app{"Load call paths"};

  std::vector<std::string> input_call_path_files;
  app.add_option("call-paths", input_call_path_files, "Call paths")->required();

  CLI11_PARSE(app, argc, argv);

  for (auto file : input_call_path_files) {
    std::cout << "Loading: " << file << "\n";
  }

  call_paths_t call_paths(input_call_path_files);

  for (size_t i = 0; i < call_paths.cps.size(); i++) {
    std::cout << "Call Path " << i << "\n";
    std::cout << "  Assuming: ";
    for (auto constraint : call_paths.cps[i]->constraints) {
      std::cout << expr_to_string(constraint) << "\n";
    }
    std::cout << "  Calls:" << "\n";
    for (auto call : call_paths.cps[i]->calls) {
      std::cout << "    Function: " << call.function_name << "\n";
      if (!call.args.empty()) {
        std::cout << "      With Args:" << "\n";
        for (auto arg : call.args) {
          std::cout << "        " << arg.first << "\n";
          std::cout << "            Expr: " << expr_to_string(arg.second.expr) << "\n";

          if (!arg.second.in.isNull()) {
            std::cout << "            Before: " << expr_to_string(arg.second.in) << "\n";
          }

          if (!arg.second.out.isNull()) {
            std::cout << "            After: " << expr_to_string(arg.second.out) << "\n";
          }

          if (arg.second.meta.size()) {
            std::cout << "            Meta: \n";
            for (auto meta : arg.second.meta) {
              std::cout << "                  " << meta.symbol << " (" << meta.size
                        << " bits)\n";
            }
          }

          if (arg.second.fn_ptr_name.first) {
            std::cout << "            Fn: " << arg.second.fn_ptr_name.second << "\n";
          }
        }
      }
      if (!call.extra_vars.empty()) {
        std::cout << "      With Extra Vars:" << "\n";
        for (auto extra_var : call.extra_vars) {
          std::cout << "        " << extra_var.first << "\n";
          if (!extra_var.second.first.isNull()) {
            std::cout << "            Before: " << expr_to_string(extra_var.second.first)
                      << "\n";
          }
          if (!extra_var.second.second.isNull()) {
            std::cout << "            After: " << expr_to_string(extra_var.second.second)
                      << "\n";
          }
        }
      }

      if (!call.ret.isNull()) {
        std::cout << "      With Ret: " << expr_to_string(call.ret) << "\n";
      }
    }
  }

  return 0;
}
