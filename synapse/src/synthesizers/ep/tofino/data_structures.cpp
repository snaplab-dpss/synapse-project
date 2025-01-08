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

void EPSynthesizer::transpile_table_decl(coder_t &coder, const Table *table,
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

void EPSynthesizer::transpile_register_decl(coder_t &coder, const Register *reg,
                                            klee::ref<klee::Expr> index,
                                            klee::ref<klee::Expr> value) {
  // * Template:
  // Register<{VALUE_WIDTH}, _>({CAPACITY}, {INIT_VALUE}) {NAME};
  // * Example:
  // Register<bit<32>, _>(1024, 0) my_register;

  const u64 init_value = 0;

  coder.indent();
  coder << "Register<";
  coder << transpiler.type_from_size(reg->index);
  coder << ",_>";

  coder << "(";
  coder << reg->num_entries;
  coder << ",";
  coder << init_value;
  coder << ")";

  coder << " ";

  coder << reg->id;
  coder << ";\n";
}

void EPSynthesizer::transpile_register_action_decl(coder_t &coder, const Register *reg,
                                                   tofino::RegisterActionType type,
                                                   const code_t &name,
                                                   klee::ref<klee::Expr> write_value) {
  // * Example:
  // RegisterAction<time_t, hash_t, bool>(expirator_t0) expirator_read_t0 = {
  // 		void apply(inout time_t alarm, out bool alive) {
  // 			if (meta.now < alarm) {
  // 				alive = true;
  // 			} else {
  // 				alive = false;
  // 			}
  // 		}
  // 	};

  code_t value_type = transpiler.type_from_size(reg->value);
  code_t index_type = transpiler.type_from_size(reg->index);

  coder.indent();
  coder << "RegisterAction<";
  coder << value_type;
  coder << ",";
  coder << index_type;
  coder << ",";
  coder << value_type;
  coder << ">";

  coder << "(";
  coder << reg->id;
  coder << ")";

  coder << " ";
  coder << name;
  coder << " = {\n";

  coder.inc();

  switch (type) {
  case RegisterActionType::READ: {
    panic("TODO: RegisterActionType::READ");
  } break;
  case RegisterActionType::WRITE: {
    coder.indent();
    coder << "void apply(inout ";
    coder << value_type;
    coder << " value, out ";
    coder << value_type;
    coder << " out_value) {\n";

    coder.inc();
    coder.indent();
    coder << "value = ";
    coder << transpiler.transpile(write_value);
    coder << ";\n";

    coder.dec();
    coder.indent();
    coder << "}\n";
  } break;
  case RegisterActionType::SWAP: {
    panic("TODO: RegisterActionType::SWAP");
  } break;
  }

  coder.indent();
  coder << "};\n";
}

void EPSynthesizer::transpile_fcfs_cached_table_decl(coder_t &coder,
                                                     const FCFSCachedTable *fcfs_cached_table,
                                                     const klee::ref<klee::Expr> key,
                                                     const klee::ref<klee::Expr> value) {
  // for (const Table &table : fcfs_cached_table->tables) {
  // }
  panic("TODO: transpile_fcfs_cached_table_decl")
}

} // namespace tofino
} // namespace synapse