#include "synthesizer.h"

namespace synapse {
namespace tofino {

code_t EPSynthesizer::build_register_action_name(const EPNode *node, const Register *reg, RegisterActionType action) const {
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

void EPSynthesizer::transpile_table_decl(coder_t &coder, const Table *table, const std::vector<klee::ref<klee::Expr>> &keys,
                                         const std::vector<klee::ref<klee::Expr>> &values) {
  std::vector<var_t> keys_vars;
  std::vector<var_t> params_vars;

  for (klee::ref<klee::Expr> key : keys) {
    const std::string key_name = table->id + "_key";
    const var_t key_var        = alloc_var(key_name, key, GLOBAL);
    keys_vars.push_back(key_var);
  }

  for (klee::ref<klee::Expr> value : values) {
    const std::string param_name = table->id + "_value";
    const var_t param_value_var  = alloc_var(param_name, value, GLOBAL);

    params_vars.push_back(param_value_var);
    param_value_var.declare(coder, Transpiler::transpile_literal(0, value->getWidth()));
  }

  const code_t action_name = table->id + "_get_value";

  if (!values.empty()) {
    coder.indent();
    coder << "action " << action_name << "(";

    for (size_t i = 0; i < values.size(); i++) {
      const klee::ref<klee::Expr> value = values[i];

      if (i != 0) {
        coder << ", ";
      }

      coder << Transpiler::type_from_expr(value);
      coder << " ";
      coder << "_" << params_vars[i].name;
    }

    coder << ") {\n";

    coder.inc();

    for (const var_t &param : params_vars) {
      coder.indent();
      coder << param.name;
      coder << " = ";
      coder << "_" << param.name;
      coder << ";\n";
    }

    coder.dec();
    coder.indent();
    coder << "}\n";

    coder << "\n";
  }

  for (const var_t &key : keys_vars) {
    key.declare(coder, Transpiler::transpile_literal(0, key.expr->getWidth()));
  }

  coder.indent();
  coder << "table " << table->id << " {\n";
  coder.inc();

  coder.indent();
  coder << "key = {\n";
  coder.inc();

  for (const var_t &key : keys_vars) {
    coder.indent();
    coder << key.name << ": exact;\n";
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
}

void EPSynthesizer::transpile_register_decl(coder_t &coder, const Register *reg, klee::ref<klee::Expr> index,
                                            klee::ref<klee::Expr> value) {
  // * Template:
  // Register<{VALUE_WIDTH}, _>({CAPACITY}, {INIT_VALUE}) {NAME};
  // * Example:
  // Register<bit<32>, _>(1024, 0) my_register;

  const u64 init_value = 0;

  coder.indent();
  coder << "Register<";
  coder << Transpiler::type_from_size(reg->index_size);
  coder << ",_>";

  coder << "(";
  coder << reg->num_entries;
  coder << ", ";
  coder << init_value;
  coder << ")";

  coder << " ";

  coder << reg->id;
  coder << ";\n";
}

void EPSynthesizer::transpile_register_read_action_decl(coder_t &coder, const Register *reg, const code_t &name) {
  // * Example:
  // RegisterAction<value_t, hash_t, bool>(reg) reg_read = {
  // 		void apply(inout value_t value, out value_t out_value) {
  // 			out_value = value;
  // 		}
  // 	};

  code_t value_type = Transpiler::type_from_size(reg->value_size);
  code_t index_type = Transpiler::type_from_size(reg->index_size);

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
}

void EPSynthesizer::transpile_register_write_action_decl(coder_t &coder, const Register *reg, const code_t &name,
                                                         const var_t &write_value) {
  // * Example:
  // RegisterAction<value_t, hash_t, void>(reg) reg_write = {
  // 		void apply(inout value_t value) {
  // 			value = some_external_var;
  // 		}
  // 	};

  code_t value_type = Transpiler::type_from_size(reg->value_size);
  code_t index_type = Transpiler::type_from_size(reg->index_size);

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
}

void EPSynthesizer::transpile_fcfs_cached_table_decl(coder_t &coder, const FCFSCachedTable *fcfs_cached_table,
                                                     const klee::ref<klee::Expr> key, const klee::ref<klee::Expr> value) {
  // for (const Table &table : fcfs_cached_table->tables) {
  // }
  panic("TODO: transpile_fcfs_cached_table_decl");
}

} // namespace tofino
} // namespace synapse