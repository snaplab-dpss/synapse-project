#include <iostream>
#include <vector>
#include <CLI/CLI.hpp>

#include "../src/call_paths/call_paths.h"

int main(int argc, char **argv, char **envp) {
  CLI::App app{"Load call paths"};

  std::vector<std::string> input_call_path_files;
  app.add_option("call-paths", input_call_path_files, "Call paths")->required();

  CLI11_PARSE(app, argc, argv);

  call_paths_t call_paths;

  for (auto file : input_call_path_files) {
    std::cout << "Loading: " << file << std::endl;
    call_paths.cps.push_back(load_call_path(file));
  }

  for (unsigned i = 0; i < call_paths.cps.size(); i++) {
    std::cout << "Call Path " << i << std::endl;
    std::cout << "  Assuming: ";
    for (auto constraint : call_paths.cps[i]->constraints) {
      constraint->dump();
      std::cout << std::endl;
    }
    std::cout << "  Calls:" << std::endl;
    for (auto call : call_paths.cps[i]->calls) {
      std::cout << "    Function: " << call.function_name << std::endl;
      if (!call.args.empty()) {
        std::cout << "      With Args:" << std::endl;
        for (auto arg : call.args) {
          std::cout << "        " << arg.first << std::endl;

          std::cout << "            Expr: ";
          arg.second.expr->dump();

          if (!arg.second.in.isNull()) {
            std::cout << "            Before: ";
            arg.second.in->dump();
          }

          if (!arg.second.out.isNull()) {
            std::cout << "            After: ";
            arg.second.out->dump();
          }

          if (arg.second.meta.size()) {
            std::cout << "            Meta: \n";
            for (auto meta : arg.second.meta) {
              std::cout << "                  " << meta.symbol;
              std::cout << " (" << meta.size << " bits)\n";
            }

            {
              char c;
              std::cin >> c;
            }
          }

          if (arg.second.fn_ptr_name.first) {
            std::cout << "            Fn: " << arg.second.fn_ptr_name.second;
            std::cout << std::endl;
          }
        }
      }
      if (!call.extra_vars.empty()) {
        std::cout << "      With Extra Vars:" << std::endl;
        for (auto extra_var : call.extra_vars) {
          std::cout << "        " << extra_var.first << std::endl;
          if (!extra_var.second.first.isNull()) {
            std::cout << "            Before: ";
            extra_var.second.first->dump();
          }
          if (!extra_var.second.second.isNull()) {
            std::cout << "            After: ";
            extra_var.second.second->dump();
          }
        }
      }

      if (!call.ret.isNull()) {
        std::cout << "      With Ret: ";
        call.ret->dump();
      }
    }
  }

  call_paths.free_call_paths();

  return 0;
}
