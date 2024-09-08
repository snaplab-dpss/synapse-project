#include "synthesizer.h"

namespace tofino {

void EPSynthesizer::transpile_table(
    coder_t &coder, const Table *table,
    const std::vector<klee::ref<klee::Expr>> &keys,
    const std::vector<klee::ref<klee::Expr>> &values) {
  code_t action_name = table->id + "_get_value";

  std::vector<code_t> action_params;

  for (size_t i = 0; i < values.size(); i++) {
    klee::ref<klee::Expr> value = values[i];
    code_t param = table->id + "_value_" + std::to_string(i);
    action_params.push_back(param);
    var_stacks.back().emplace_back(param, value);

    coder.indent();
    coder << type_from_expr(value);
    coder << " ";
    coder << param;
    coder << ";\n";
  }

  if (!values.empty()) {
    coder.indent();
    coder << "action " << action_name << "(";

    for (size_t i = 0; i < values.size(); i++) {
      klee::ref<klee::Expr> value = values[i];

      if (i != 0) {
        coder << ", ";
      }

      coder << type_from_expr(value);
      coder << " ";
      coder << "_" << action_params[i];
    }

    coder << ") {\n";

    coder.inc();

    for (const code_t &param : action_params) {
      coder.indent();
      coder << param;
      coder << " = ";
      coder << "_" << param;
      coder << ";\n";
    }

    coder.dec();
    coder.indent();
    coder << "}\n";

    coder.indent();
    coder << "\n";
  }

  coder.indent();
  coder << "table " << table->id << " {\n";
  coder.inc();

  coder.indent();
  coder << "key = {\n";
  coder.inc();

  for (klee::ref<klee::Expr> key : keys) {
    coder.indent();
    coder << transpiler.transpile(key) << ": exact;\n";
  }

  coder.dec();
  coder.indent();
  coder << "}\n";

  coder.indent();
  coder << "actions = {";

  if (!values.empty()) {
    coder << "\n";
    coder.inc();

    coder.indent();
    coder << action_name << ";\n";

    coder.dec();
    coder.indent();
  }

  coder << "}\n";

  coder.indent();
  coder << "size = " << table->num_entries << ";\n";

  coder.dec();
  coder.indent();
  coder << "}\n";
  coder << "\n";
}

void EPSynthesizer::transpile_register(coder_t &coder, const Register *reg,
                                       klee::ref<klee::Expr> index,
                                       klee::ref<klee::Expr> value) {
  coder.indent();
  coder << "Register<";
  coder << type_from_size(reg->index);
  coder << ",_>";
  coder << "(" << reg->num_entries << ",0) ";
  coder << reg->id;
  coder << ";\n";

  for (RegisterAction action : reg->actions) {
    coder.indent();
    coder << "RegisterAction ";
    coder << reg->id;
    coder << "_";

    switch (action) {
    case RegisterAction::READ:
      coder << "read";
      break;
    case RegisterAction::WRITE:
      coder << "write";
      break;
    case RegisterAction::SWAP:
      coder << "update";
      break;
    }

    coder << type_from_size(reg->index);
    coder << " _index, ";
    coder << type_from_expr(value);
    coder << " _value) {\n";

    coder.inc();

    coder.indent();
    coder << "}\n";
  }
  coder.indent();
}

void EPSynthesizer::transpile_fcfs_cached_table(
    coder_t &coder, const FCFSCachedTable *fcfs_cached_table,
    klee::ref<klee::Expr> key, klee::ref<klee::Expr> value) {
  for (const Table &table : fcfs_cached_table->tables) {
  }
}

} // namespace tofino
