#include "synthesizer.h"

namespace synapse {
namespace tofino {

code_t EPSynthesizer::build_register_action_name(const EPNode *node, const Register *reg,
                                                 RegisterActionType action) const {
  coder_t coder;
  coder << reg->id;
  coder << "_";
  switch (action) {
  case RegisterActionType::READ:
    coder << "read";
    break;
  case RegisterActionType::WRITE:
    coder << "write";
    break;
  case RegisterActionType::SWAP:
    coder << "update";
    break;
  }
  coder << "_";
  coder << node->get_id();
  return coder.dump();
}

code_t EPSynthesizer::transpile_table_decl(indent_t lvl, const Table *table,
                                           const std::vector<klee::ref<klee::Expr>> &keys,
                                           const std::vector<klee::ref<klee::Expr>> &values) {
  coder_t coder(lvl);
  std::vector<code_t> action_params;

  const code_t action_name = table->id + "_get_value";

  for (size_t i = 0; i < values.size(); i++) {
    klee::ref<klee::Expr> value = values[i];
    code_t param = table->id + "_value_" + std::to_string(i);
    action_params.push_back(param);
    var_stacks.back().emplace_back(param, value);

    coder.indent();
    coder << transpiler.type_from_expr(value);
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

      coder << transpiler.type_from_expr(value);
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

  return coder.dump();
}

code_t EPSynthesizer::transpile_register_decl(indent_t lvl, const Register *reg,
                                              klee::ref<klee::Expr> index,
                                              klee::ref<klee::Expr> value) {
  // * Template:
  // Register<{VALUE_WIDTH}, _>({CAPACITY}, {INIT_VALUE}) {NAME};
  // * Example:
  // Register<bit<32>, _>(1024, 0) my_register;

  coder_t coder(lvl);
  const u64 init_value = 0;

  coder.indent();
  coder << "Register<";
  coder << transpiler.type_from_size(reg->index_size);
  coder << ",_>";

  coder << "(";
  coder << reg->num_entries;
  coder << ", ";
  coder << init_value;
  coder << ")";

  coder << " ";

  coder << reg->id;
  coder << ";\n";

  return coder.dump();
}

code_t EPSynthesizer::transpile_register_read_action_decl(indent_t lvl, const Register *reg,
                                                          const code_t &name) {
  // * Example:
  // RegisterAction<value_t, hash_t, bool>(reg) reg_read = {
  // 		void apply(inout value_t value, out value_t out_value) {
  // 			out_value = value;
  // 		}
  // 	};

  coder_t coder(lvl);

  code_t value_type = transpiler.type_from_size(reg->value_size);
  code_t index_type = transpiler.type_from_size(reg->index_size);

  coder.indent();
  coder << "RegisterAction<";
  coder << value_type;
  coder << ", ";
  coder << index_type;
  coder << ", ";
  coder << value_type;
  coder << ">";

  coder << "(";
  coder << reg->id;
  coder << ")";

  coder << " ";
  coder << name;
  coder << " = {\n";

  coder.inc();

  coder.indent();
  coder << "void apply(";
  coder << "inout " << value_type << " value";
  coder << ", ";
  coder << "out " << value_type << " out_value) {\n";

  coder.inc();
  coder.indent();
  coder << "out_value = value;\n";

  coder.dec();
  coder.indent();
  coder << "}\n";

  coder.dec();
  coder.indent();
  coder << "};\n";

  return coder.dump();
}

code_t EPSynthesizer::transpile_register_write_action_decl(indent_t lvl, const Register *reg,
                                                           const code_t &name,
                                                           const var_t &write_value) {
  // * Example:
  // RegisterAction<value_t, hash_t, void>(reg) reg_write = {
  // 		void apply(inout value_t value) {
  // 			value = some_external_var;
  // 		}
  // 	};

  coder_t coder(lvl);

  code_t value_type = transpiler.type_from_size(reg->value_size);
  code_t index_type = transpiler.type_from_size(reg->index_size);

  coder.indent();
  coder << type_from_var(write_value) << " " << write_value.name << ";\n";

  coder.indent();
  coder << "RegisterAction<";
  coder << value_type;
  coder << ", ";
  coder << index_type;
  coder << ", ";
  coder << "void";
  coder << ">";

  coder << "(";
  coder << reg->id;
  coder << ")";

  coder << " ";
  coder << name;
  coder << " = {\n";

  coder.inc();

  coder.indent();
  coder << "void apply(";
  coder << "inout " << value_type << " value";
  coder << ") {\n";

  coder.inc();
  coder.indent();
  coder << "value = " << write_value.name << ";\n";

  coder.dec();
  coder.indent();
  coder << "}\n";

  coder.dec();
  coder.indent();
  coder << "};\n";

  return coder.dump();
}

code_t EPSynthesizer::transpile_fcfs_cached_table_decl(indent_t lvl,
                                                       const FCFSCachedTable *fcfs_cached_table,
                                                       const klee::ref<klee::Expr> key,
                                                       const klee::ref<klee::Expr> value) {
  // for (const Table &table : fcfs_cached_table->tables) {
  // }
  panic("TODO: transpile_fcfs_cached_table_decl");
  coder_t coder;
  return coder.dump();
}

} // namespace tofino
} // namespace synapse