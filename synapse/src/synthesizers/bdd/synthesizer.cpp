#include "synthesizer.h"
#include "../../exprs/exprs.h"
#include "../../exprs/solver.h"
#include "../../log.h"

#define NF_TEMPLATE_FILENAME "nf.template.cpp"
#define PROFILER_TEMPLATE_FILENAME "profiler.template.cpp"

#define MARKER_NF_STATE "NF_STATE"
#define MARKER_NF_INIT "NF_INIT"
#define MARKER_NF_PROCESS "NF_PROCESS"

#define POPULATE_SYNTHESIZER(FNAME)                                            \
  {                                                                            \
    #FNAME, std::bind(&BDDSynthesizer::FNAME, this, std::placeholders::_1,     \
                      std::placeholders::_2)                                   \
  }

static std::string template_from_type(BDDSynthesizerTarget target) {
  std::string template_filename;

  switch (target) {
  case BDDSynthesizerTarget::NF:
    template_filename = NF_TEMPLATE_FILENAME;
    break;
  case BDDSynthesizerTarget::PROFILER:
    template_filename = PROFILER_TEMPLATE_FILENAME;
    break;
  }

  return template_filename;
}

BDDSynthesizer::BDDSynthesizer(BDDSynthesizerTarget _target, std::ostream &_out)
    : Synthesizer(template_from_type(_target),
                  {
                      {MARKER_NF_STATE, 0},
                      {MARKER_NF_INIT, 0},
                      {MARKER_NF_PROCESS, 0},
                  },
                  _out),
      target(_target), transpiler(this),
      init_synthesizers({
          POPULATE_SYNTHESIZER(map_allocate),
          POPULATE_SYNTHESIZER(vector_allocate),
          POPULATE_SYNTHESIZER(dchain_allocate),
          POPULATE_SYNTHESIZER(sketch_allocate),
      }),
      process_synthesizers({
          POPULATE_SYNTHESIZER(packet_borrow_next_chunk),
          POPULATE_SYNTHESIZER(packet_return_chunk),
          POPULATE_SYNTHESIZER(nf_set_rte_ipv4_udptcp_checksum),
          POPULATE_SYNTHESIZER(expire_items_single_map),
          POPULATE_SYNTHESIZER(expire_items_single_map_iteratively),
          POPULATE_SYNTHESIZER(map_get),
          POPULATE_SYNTHESIZER(map_put),
          POPULATE_SYNTHESIZER(map_erase),
          POPULATE_SYNTHESIZER(vector_borrow),
          POPULATE_SYNTHESIZER(vector_return),
          POPULATE_SYNTHESIZER(dchain_allocate_new_index),
          POPULATE_SYNTHESIZER(dchain_rejuvenate_index),
          POPULATE_SYNTHESIZER(dchain_expire_one),
          POPULATE_SYNTHESIZER(dchain_is_index_allocated),
          POPULATE_SYNTHESIZER(dchain_free_index),
          POPULATE_SYNTHESIZER(sketch_compute_hashes),
          POPULATE_SYNTHESIZER(sketch_refresh),
          POPULATE_SYNTHESIZER(sketch_fetch),
          POPULATE_SYNTHESIZER(sketch_touch_buckets),
          POPULATE_SYNTHESIZER(sketch_expire),
      }) {}

void BDDSynthesizer::synthesize(const BDD *bdd) {
  // Global state
  stack_push();

  init_pre_process(bdd);
  process(bdd);
  init_post_process();

  Synthesizer::dump();
}

void BDDSynthesizer::init_pre_process(const BDD *bdd) {
  coder_t &coder = get(MARKER_NF_INIT);

  coder << "bool nf_init() {\n";
  coder.inc();

  const calls_t &calls = bdd->get_init();

  for (const call_t &call : calls) {
    coder.indent();
    coder << "if (!";
    synthesize_init(coder, call);
    coder << ") {\n";

    coder.inc();
    coder.indent();
    coder << "return false;\n";

    coder.dec();
    coder.indent();
    coder << "}\n";
  }

  if (target == BDDSynthesizerTarget::PROFILER) {
    coder_t &coder_nf_state = get(MARKER_NF_STATE);

    coder_nf_state.indent();
    coder_nf_state << "uint64_t path_profiler_counter[";
    coder_nf_state << bdd->size();
    coder_nf_state << "]";
    coder_nf_state << ";\n";

    coder.indent();
    coder << "memset(";
    coder << "(void*)path_profiler_counter, ";
    coder << "0, ";
    coder << "sizeof(path_profiler_counter)";
    coder << ");\n";

    coder.indent();
    coder << "path_profiler_counter_ptr = path_profiler_counter;\n";

    coder.indent();
    coder << "path_profiler_counter_sz = ";
    coder << bdd->size();
    coder << ";\n";
  }
}

void BDDSynthesizer::init_post_process() {
  coder_t &coder = get(MARKER_NF_INIT);

  if (target == BDDSynthesizerTarget::PROFILER) {
    for (node_id_t node_id : map_stats_nodes) {
      coder.indent();
      coder << "map_stats.init(";
      coder << node_id;
      coder << ")";
      coder << ";\n";
    }
  }

  coder.indent();
  coder << "return true;\n";

  coder.dec();
  coder << "}\n";
}

void BDDSynthesizer::process(const BDD *bdd) {
  coder_t &coder = get(MARKER_NF_PROCESS);

  coder << "int nf_process(";
  coder << "uint16_t device, ";
  coder << "uint8_t *buffer, ";
  coder << "uint16_t packet_length, ";
  coder << "time_ns_t now) {\n";

  symbol_t device = bdd->get_device();
  symbol_t packet_len = bdd->get_packet_len();
  symbol_t now = bdd->get_time();

  coder.inc();
  stack_push();

  stack_add(build_var("device", device.expr));
  stack_add(build_var("packet_length", packet_len.expr));
  stack_add(build_var("now", now.expr));

  synthesize(bdd->get_root());

  coder.dec();
  coder << "}\n";
}

void BDDSynthesizer::synthesize(const Node *node) {
  coder_t &coder = get(MARKER_NF_PROCESS);

  node->visit_nodes([this, &coder](const Node *node) {
    NodeVisitAction action = NodeVisitAction::VISIT_CHILDREN;

    coder.indent();
    coder << "// Node ";
    coder << node->get_id();
    coder << "\n";

    if (target == BDDSynthesizerTarget::PROFILER) {
      coder.indent();
      coder << "inc_path_counter(";
      coder << node->get_id();
      coder << ");\n";
    }

    switch (node->get_type()) {
    case NodeType::BRANCH: {
      const Branch *branch_node = static_cast<const Branch *>(node);

      const Node *on_true = branch_node->get_on_true();
      const Node *on_false = branch_node->get_on_false();

      klee::ref<klee::Expr> condition = branch_node->get_condition();

      coder.indent();
      coder << "if (";
      coder << transpiler.transpile(condition);
      coder << ") {\n";

      coder.inc();
      this->stack_push();
      this->synthesize(on_true);
      this->stack_pop();
      coder.dec();

      coder.indent();
      coder << "} else {\n";

      coder.inc();
      this->stack_push();
      this->synthesize(on_false);
      this->stack_pop();
      coder.dec();

      coder.indent();
      coder << "}";

      coder << " ";
      coder << "// ";
      coder << transpiler.transpile(condition);

      coder << "\n";

      action = NodeVisitAction::STOP;
    } break;
    case NodeType::CALL: {
      const Call *call_node = static_cast<const Call *>(node);
      // const call_t &call = call_node->get_call();
      // symbols_t generated_symbols =
      // call_node->get_locally_generated_symbols();
      synthesize_process(coder, call_node);
    } break;
    case NodeType::ROUTE: {
      const Route *route_node = static_cast<const Route *>(node);

      RouteOperation op = route_node->get_operation();
      int dst_device = route_node->get_dst_device();

      switch (op) {
      case RouteOperation::DROP: {
        coder.indent();
        coder << "return DROP;\n";
      } break;
      case RouteOperation::BCAST: {
        coder.indent();
        coder << "return FLOOD;\n";
      } break;
      case RouteOperation::FWD: {
        coder.indent();
        coder << "return " << dst_device << ";\n";
      } break;
      }
    } break;
    }

    return action;
  });
}

void BDDSynthesizer::synthesize_init(coder_t &coder, const call_t &call) {
  if (this->init_synthesizers.find(call.function_name) ==
      this->init_synthesizers.end()) {
    std::cerr << "No init synthesizer found for function: "
              << call.function_name << "\n";
    exit(1);
  }

  (this->init_synthesizers[call.function_name])(coder, call);
}

void BDDSynthesizer::synthesize_process(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  if (this->process_synthesizers.find(call.function_name) ==
      this->process_synthesizers.end()) {
    std::cerr << "No process synthesizer found for function: "
              << call.function_name << "\n";
    exit(1);
  }

  (this->process_synthesizers[call.function_name])(coder, call_node);
}

void BDDSynthesizer::packet_borrow_next_chunk(coder_t &coder,
                                              const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> length = call.args.at("length").expr;
  klee::ref<klee::Expr> chunk = call.args.at("chunk").out;
  klee::ref<klee::Expr> out_chunk = call.extra_vars.at("the_chunk").second;

  var_t hdr = build_var("hdr", out_chunk, chunk);

  coder.indent();
  coder << "uint8_t* " << hdr.name << ";\n";

  coder.indent();
  coder << "packet_borrow_next_chunk(";
  coder << "buffer, ";
  coder << transpiler.transpile(length) << ", ";
  coder << "(void**)&" << hdr.name;
  coder << ")";
  coder << ";\n";

  stack_add(hdr);
}

void BDDSynthesizer::packet_return_chunk(coder_t &coder,
                                         const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> chunk_addr = call.args.at("the_chunk").expr;
  klee::ref<klee::Expr> chunk = call.args.at("the_chunk").in;

  var_t hdr = stack_get(chunk_addr);

  std::vector<modification_t> changes = build_modifications(hdr.expr, chunk);

  for (const modification_t &modification : changes) {
    coder.indent();
    coder << hdr.name;
    coder << "[";
    coder << modification.byte;
    coder << "] = ";
    coder << transpiler.transpile(modification.expr);
    coder << ";\n";
  }

  coder.indent();
  coder << "packet_return_chunk(";
  coder << "buffer, ";
  coder << stack_get(chunk_addr).name;
  coder << ")";
  coder << ";\n";
}

void BDDSynthesizer::nf_set_rte_ipv4_udptcp_checksum(coder_t &coder,
                                                     const Call *call_node) {
  const call_t &call = call_node->get_call();
  symbols_t generated_symbols = call_node->get_locally_generated_symbols();

  klee::ref<klee::Expr> ip_header = call.args.at("ip_header").expr;
  klee::ref<klee::Expr> tcpudp_header = call.args.at("l4_header").expr;

  symbol_t checksum;
  bool found = get_symbol(generated_symbols, "checksum", checksum);
  assert(found && "Symbol checksum not found");

  var_t c = build_var("checksum", checksum.expr);

  coder.indent();
  coder << "int " << c.name << " = ";
  coder << "rte_ipv4_udptcp_cksum(";
  coder << "(struct rte_ipv4_hdr*)";
  coder << stack_get(ip_header).name << ", ";
  coder << "(void*)";
  coder << stack_get(tcpudp_header).name;
  coder << ")";
  coder << ";\n";

  stack_add(c);
}

void BDDSynthesizer::expire_items_single_map(coder_t &coder,
                                             const Call *call_node) {
  const call_t &call = call_node->get_call();
  symbols_t generated_symbols = call_node->get_locally_generated_symbols();

  klee::ref<klee::Expr> chain = call.args.at("chain").expr;
  klee::ref<klee::Expr> vector = call.args.at("vector").expr;
  klee::ref<klee::Expr> map = call.args.at("map").expr;
  klee::ref<klee::Expr> time = call.args.at("time").expr;

  symbol_t number_of_freed_flows;
  bool found = get_symbol(generated_symbols, "number_of_freed_flows",
                          number_of_freed_flows);
  assert(found && "Symbol number_of_freed_flows not found");

  var_t nfreed = build_var("freed_flows", number_of_freed_flows.expr);

  coder.indent();
  coder << "int " << nfreed.name << " = ";
  coder << "expire_items_single_map(";
  coder << stack_get(chain).name << ", ";
  coder << stack_get(vector).name << ", ";
  coder << stack_get(map).name << ", ";
  coder << transpiler.transpile(time);
  coder << ")";
  coder << ";\n";

  stack_add(nfreed);
}

void BDDSynthesizer::expire_items_single_map_iteratively(
    coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  coder.indent();
  coder << "// expire_items_single_map_iteratively";
  coder << ";\n";

  assert(false && "Not implemented");
}

void BDDSynthesizer::map_allocate(coder_t &coder, const call_t &call) {
  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> map_out = call.args.at("map_out").out;

  var_t map_out_var = build_var("map", map_out);

  coder_t &coder_nf_state = get(MARKER_NF_STATE);
  coder_nf_state.indent();
  coder_nf_state << "struct Map *";
  coder_nf_state << map_out_var.name;
  coder_nf_state << ";\n";

  coder << "map_allocate(";
  coder << transpiler.transpile(capacity) << ", ";
  coder << transpiler.transpile(key_size) << ", ";
  coder << "&" << map_out_var.name;
  coder << ")";

  stack_add(map_out_var);
}

BDDSynthesizer::var_t BDDSynthesizer::build_key(klee::ref<klee::Expr> key_addr,
                                                klee::ref<klee::Expr> key,
                                                coder_t &coder,
                                                bool &key_in_stack) {
  bytes_t key_size = key->getWidth() / 8;

  var_t k;
  key_in_stack = stack_find(key_addr, k);

  if (!key_in_stack) {
    k = build_var("key", key, key_addr);

    coder.indent();
    coder << "uint8_t " << k.name << "[" << key_size << "];\n";
  }

  var_t key_value;
  if (stack_find(key, key_value)) {
    coder.indent();
    coder << "memcpy(";
    coder << "(void*)" << k.name << ", ";
    coder << "(void*)" << key_value.name << ", ";
    coder << key_size;
    coder << ");\n";

    k.expr = key_value.expr;
  } else {
    for (bytes_t b = 0; b < key_size; b++) {
      klee::ref<klee::Expr> key_byte =
          solver_toolbox.exprBuilder->Extract(k.expr, b * 8, 8);
      coder.indent();
      coder << k.name << "[" << b << "] = ";
      coder << transpiler.transpile(key_byte);
      coder << ";\n";
    }
  }

  return k;
}

void BDDSynthesizer::map_get(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();
  symbols_t generated_symbols = call_node->get_locally_generated_symbols();

  klee::ref<klee::Expr> map_addr = call.args.at("map").expr;
  klee::ref<klee::Expr> key_addr = call.args.at("key").expr;
  klee::ref<klee::Expr> key = call.args.at("key").in;
  klee::ref<klee::Expr> value_out_addr = call.args.at("value_out").expr;
  klee::ref<klee::Expr> value_out = call.args.at("value_out").out;

  symbol_t map_has_this_key;
  bool found =
      get_symbol(generated_symbols, "map_has_this_key", map_has_this_key);
  assert(found && "Symbol map_has_this_key not found");

  var_t r = build_var("map_hit", map_has_this_key.expr);
  var_t v = build_var("value", value_out);

  bool key_in_stack;
  var_t k = build_key(key_addr, key, coder, key_in_stack);

  coder.indent();
  coder << "int " << v.name << ";\n";

  coder.indent();
  coder << "int " << r.name << " = ";
  coder << "map_get(";
  coder << stack_get(map_addr).name << ", ";
  coder << k.name << ", ";
  coder << "&" << v.name;
  coder << ")";
  coder << ";\n";

  if (target == BDDSynthesizerTarget::PROFILER) {
    map_stats_nodes.insert(call_node->get_id());
    coder.indent();
    coder << "map_stats.update(";
    coder << call_node->get_id() << ", ";
    coder << k.name << ", ";
    coder << key->getWidth() / 8;
    coder << ")";
    coder << ";\n";
  }

  stack_add(r);
  stack_add(v);

  if (!key_in_stack) {
    stack_add(k);
  } else {
    stack_replace(k, key);
  }
}

void BDDSynthesizer::map_put(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> map_addr = call.args.at("map").expr;
  klee::ref<klee::Expr> key_addr = call.args.at("key").expr;
  klee::ref<klee::Expr> key = call.args.at("key").in;
  klee::ref<klee::Expr> value = call.args.at("value").expr;

  bool key_in_stack;
  var_t k = build_key(key_addr, key, coder, key_in_stack);

  coder.indent();
  coder << "map_put(";
  coder << stack_get(map_addr).name << ", ";
  coder << k.name << ", ";
  coder << transpiler.transpile(value);
  coder << ")";
  coder << ";\n";

  if (target == BDDSynthesizerTarget::PROFILER) {
    map_stats_nodes.insert(call_node->get_id());
    coder.indent();
    coder << "map_stats.update(";
    coder << call_node->get_id() << ", ";
    coder << k.name << ", ";
    coder << key->getWidth() / 8;
    coder << ")";
    coder << ";\n";
  }

  if (!key_in_stack) {
    stack_add(k);
  } else {
    stack_replace(k, key);
  }
}

void BDDSynthesizer::map_erase(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> map_addr = call.args.at("map").expr;
  klee::ref<klee::Expr> key_addr = call.args.at("key").expr;
  klee::ref<klee::Expr> key = call.args.at("key").in;
  klee::ref<klee::Expr> trash = call.args.at("trash").expr;

  bool key_in_stack;
  var_t k = build_key(key_addr, key, coder, key_in_stack);

  coder.indent();
  coder << "void* trash;\n";

  coder.indent();
  coder << "map_erase(";
  coder << stack_get(map_addr).name << ", ";
  coder << k.name << ", ";
  coder << "&trash";
  coder << ")";
  coder << ";\n";

  if (target == BDDSynthesizerTarget::PROFILER) {
    map_stats_nodes.insert(call_node->get_id());
    coder.indent();
    coder << "map_stats.update(";
    coder << call_node->get_id() << ", ";
    coder << k.name << ", ";
    coder << key->getWidth() / 8;
    coder << ")";
    coder << ";\n";
  }

  if (!key_in_stack) {
    stack_add(k);
  } else {
    stack_replace(k, key);
  }
}

void BDDSynthesizer::vector_allocate(coder_t &coder, const call_t &call) {
  klee::ref<klee::Expr> elem_size = call.args.at("elem_size").expr;
  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> vector_out = call.args.at("vector_out").out;

  var_t vector_out_var = build_var("vector", vector_out);

  coder_t &coder_nf_state = get(MARKER_NF_STATE);
  coder_nf_state.indent();
  coder_nf_state << "struct Vector *";
  coder_nf_state << vector_out_var.name;
  coder_nf_state << ";\n";

  coder << "vector_allocate(";
  coder << transpiler.transpile(elem_size) << ", ";
  coder << transpiler.transpile(capacity) << ", ";
  coder << "&" << vector_out_var.name;
  coder << ")";

  stack_add(vector_out_var);
}

void BDDSynthesizer::vector_borrow(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> vector_addr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index = call.args.at("index").expr;
  klee::ref<klee::Expr> value_addr = call.args.at("val_out").out;
  klee::ref<klee::Expr> value = call.extra_vars.at("borrowed_cell").second;

  var_t v = build_var("vector_value_out", value, value_addr);
  bytes_t value_size = value->getWidth() / 8;

  coder.indent();
  coder << "uint8_t* " << v.name << " = 0;\n";

  coder.indent();
  coder << "vector_borrow(";
  coder << stack_get(vector_addr).name << ", ";
  coder << transpiler.transpile(index) << ", ";
  coder << "(void**)&" << v.name;
  coder << ")";
  coder << ";\n";

  stack_add(v);
}

void BDDSynthesizer::vector_return(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> value = call.args.at("value").in;
  klee::ref<klee::Expr> value_addr = call.args.at("value").expr;

  var_t v = stack_get(value_addr);
  std::vector<modification_t> changes = build_modifications(v.expr, value);

  if (changes.empty()) {
    return;
  }

  var_t new_v;
  if (value->getKind() != klee::Expr::Constant && stack_find(value, new_v)) {
    coder.indent();
    coder << "memcpy(";
    coder << "(void*)" << v.name << ", ";
    coder << "(void*)" << new_v.name << ", ";
    coder << value->getWidth() / 8;
    coder << ");\n";
    return;
  }

  for (const modification_t &modification : changes) {
    coder.indent();
    coder << v.name;
    coder << "[";
    coder << modification.byte;
    coder << "] = ";
    coder << transpiler.transpile(modification.expr);
    coder << ";\n";
  }
}

void BDDSynthesizer::dchain_allocate(coder_t &coder, const call_t &call) {
  klee::ref<klee::Expr> index_range = call.args.at("index_range").expr;
  klee::ref<klee::Expr> chain_out = call.args.at("chain_out").out;

  var_t chain_out_var = build_var("dchain", chain_out);

  coder_t &coder_nf_state = get(MARKER_NF_STATE);
  coder_nf_state.indent();
  coder_nf_state << "struct DoubleChain *";
  coder_nf_state << chain_out_var.name;
  coder_nf_state << ";\n";

  coder << "dchain_allocate(";
  coder << transpiler.transpile(index_range) << ", ";
  coder << "&" << chain_out_var.name;
  coder << ")";

  stack_add(chain_out_var);
}

void BDDSynthesizer::dchain_allocate_new_index(coder_t &coder,
                                               const Call *call_node) {
  const call_t &call = call_node->get_call();
  symbols_t generated_symbols = call_node->get_locally_generated_symbols();

  klee::ref<klee::Expr> dchain_addr = call.args.at("chain").expr;
  klee::ref<klee::Expr> time = call.args.at("time").expr;
  klee::ref<klee::Expr> index_out = call.args.at("index_out").out;

  symbol_t out_of_space;
  bool found = get_symbol(generated_symbols, "out_of_space", out_of_space);

  var_t oos = build_var("out_of_space", out_of_space.expr);
  var_t i = build_var("index", index_out);

  coder.indent();
  coder << "int " << i.name << ";\n";

  coder.indent();
  coder << "int " << oos.name << " = ";
  // Negate the result because we have an out-of-space symbol, which is true
  // when there is no space (i.e., the result is false).
  coder << "!";
  coder << "dchain_allocate_new_index(";
  coder << stack_get(dchain_addr).name << ", ";
  coder << "&" << i.name << ", ";
  coder << transpiler.transpile(time);
  coder << ")";
  coder << ";\n";

  stack_add(oos);
  stack_add(i);
}

void BDDSynthesizer::dchain_rejuvenate_index(coder_t &coder,
                                             const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> dchain_addr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index = call.args.at("index").expr;
  klee::ref<klee::Expr> time = call.args.at("time").expr;

  coder.indent();
  coder << "dchain_rejuvenate_index(";
  coder << stack_get(dchain_addr).name << ", ";
  coder << transpiler.transpile(index) << ", ";
  coder << transpiler.transpile(time);
  coder << ")";
  coder << ";\n";
}

void BDDSynthesizer::dchain_expire_one(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  coder.indent();
  coder << "// dchain_expire_one";
  coder << ";\n";

  assert(false && "TODO");
}

void BDDSynthesizer::dchain_is_index_allocated(coder_t &coder,
                                               const Call *call_node) {
  const call_t &call = call_node->get_call();
  symbols_t generated_symbols = call_node->get_locally_generated_symbols();

  klee::ref<klee::Expr> dchain_addr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index = call.args.at("index").expr;

  symbol_t is_allocated;
  bool found =
      get_symbol(generated_symbols, "dchain_is_index_allocated", is_allocated);
  assert(found && "Symbol dchain_is_index_allocated not found");

  var_t ia = build_var("is_allocated", is_allocated.expr);

  coder.indent();
  coder << "int " << ia.name << " = ";
  coder << "dchain_is_index_allocated(";
  coder << stack_get(dchain_addr).name << ", ";
  coder << transpiler.transpile(index);
  coder << ")";
  coder << ";\n";

  stack_add(ia);
}

void BDDSynthesizer::dchain_free_index(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> dchain_addr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index = call.args.at("index").expr;

  coder.indent();
  coder << "dchain_free_index(";
  coder << stack_get(dchain_addr).name << ", ";
  coder << transpiler.transpile(index);
  coder << ")";
  coder << ";\n";
}

void BDDSynthesizer::sketch_allocate(coder_t &coder, const call_t &call) {
  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> threshold = call.args.at("threshold").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> sketch_out = call.args.at("sketch_out").out;

  var_t sketch_out_var = build_var("sketch", sketch_out);

  coder_t &coder_nf_state = get(MARKER_NF_STATE);
  coder_nf_state.indent();
  coder_nf_state << "struct Sketch *";
  coder_nf_state << sketch_out_var.name;
  coder_nf_state << ";\n";

  coder << "sketch_allocate(";
  coder << transpiler.transpile(capacity) << ", ";
  coder << transpiler.transpile(threshold) << ", ";
  coder << transpiler.transpile(key_size) << ", ";
  coder << "&" << sketch_out_var.name;
  coder << ")";

  stack_add(sketch_out_var);
}

void BDDSynthesizer::sketch_compute_hashes(coder_t &coder,
                                           const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> sketch_addr = call.args.at("sketch").expr;
  klee::ref<klee::Expr> key = call.args.at("key").in;
  klee::ref<klee::Expr> key_addr = call.args.at("key").expr;

  bool key_in_stack;
  var_t k = build_key(key_addr, key, coder, key_in_stack);

  coder.indent();
  coder << "sketch_compute_hashes(";
  coder << stack_get(sketch_addr).name << ", ";
  coder << k.name;
  coder << ")";
  coder << ";\n";

  if (!key_in_stack) {
    stack_add(k);
  } else {
    stack_replace(k, key);
  }
}

void BDDSynthesizer::sketch_refresh(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> sketch_addr = call.args.at("sketch").expr;
  klee::ref<klee::Expr> time = call.args.at("time").expr;

  coder.indent();
  coder << "sketch_refresh(";
  coder << stack_get(sketch_addr).name << ", ";
  coder << transpiler.transpile(time);
  coder << ")";
  coder << ";\n";
}

void BDDSynthesizer::sketch_fetch(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();
  symbols_t generated_symbols = call_node->get_locally_generated_symbols();

  klee::ref<klee::Expr> sketch_addr = call.args.at("sketch").expr;

  symbol_t overflow;
  bool found = get_symbol(generated_symbols, "overflow", overflow);
  assert(found && "Symbol overflow not found");

  var_t o = build_var("overflow", overflow.expr);

  coder.indent();
  coder << "int " << o.name << " = ";
  coder << "sketch_fetch(";
  coder << stack_get(sketch_addr).name;
  coder << ")";
  coder << ";\n";

  stack_add(o);
}

void BDDSynthesizer::sketch_touch_buckets(coder_t &coder,
                                          const Call *call_node) {
  const call_t &call = call_node->get_call();
  symbols_t generated_symbols = call_node->get_locally_generated_symbols();

  klee::ref<klee::Expr> sketch_addr = call.args.at("sketch").expr;
  klee::ref<klee::Expr> time = call.args.at("time").expr;

  symbol_t success;
  bool found = get_symbol(generated_symbols, "success", success);
  assert(found && "Symbol success not found");

  var_t s = build_var("success", success.expr);

  coder.indent();
  coder << "int " << s.name << " = ";
  coder << "sketch_touch_buckets(";
  coder << stack_get(sketch_addr).name << ", ";
  coder << transpiler.transpile(time);
  coder << ")";
  coder << ";\n";

  stack_add(s);
}

void BDDSynthesizer::sketch_expire(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> sketch_addr = call.args.at("sketch").expr;
  klee::ref<klee::Expr> time = call.args.at("time").expr;

  coder.indent();
  coder << "sketch_expire(";
  coder << stack_get(sketch_addr).name << ", ";
  coder << transpiler.transpile(time);
  coder << ")";
  coder << ";\n";
}

void BDDSynthesizer::stack_dbg() const {
  std::cerr << "================= Vars ================= \n";
  for (const stack_frame_t &frame : stack) {
    std::cerr << "------------------------------------------\n";
    for (const var_t &var : frame.vars) {
      std::cerr << "[";
      std::cerr << var.name;
      if (!var.addr.isNull()) {
        std::cerr << "@" << expr_to_string(var.addr, true);
      }
      std::cerr << "]: ";
      std::cerr << expr_to_string(var.expr, false) << "\n";
    }
  }
  std::cerr << "======================================== \n";
}

void BDDSynthesizer::stack_push() { stack.emplace_back(); }

void BDDSynthesizer::stack_pop() { stack.pop_back(); }

BDDSynthesizer::var_t BDDSynthesizer::stack_get(const std::string &name) const {
  for (const stack_frame_t &frame : stack) {
    for (const var_t &var : frame.vars) {
      if (var.name == name) {
        return var;
      }
    }
  }

  stack_dbg();
  std::cerr << "Variable not found in stack: " << name << "\n";

  exit(1);
}

BDDSynthesizer::var_t BDDSynthesizer::stack_get(klee::ref<klee::Expr> expr) {
  var_t var;
  if (stack_find(expr, var)) {
    return var;
  }

  stack_dbg();
  std::cerr << "Variable not found in stack: " << expr_to_string(expr) << "\n";

  exit(1);
}

code_t BDDSynthesizer::slice_var(const var_t &var, bits_t offset,
                                 bits_t size) const {
  assert(offset + size <= var.expr->getWidth());

  coder_t coder;

  // If the expression has an address, then it's an array of bytes.
  // We need to check if the expression is within the range of the array.
  if (!var.addr.isNull()) {
    if (size > 8 && size <= 64) {
      coder << "(";
      coder << transpiler.type_from_size(size);
      coder << "*)";
    }

    coder << "(" + var.name << "+" << offset / 8 << ")";
    return coder.dump();
  }

  if (offset > 0) {
    coder << "(";
    coder << var.name;
    coder << ">>";
    coder << offset;
    coder << ") & ";
    coder << ((1 << size) - 1);
  } else {
    coder << var.name;
    coder << " & ";
    coder << ((1ull << size) - 1);
  }

  return coder.dump();
}

bool BDDSynthesizer::stack_find(klee::ref<klee::Expr> expr, var_t &out_var) {
  for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
    stack_frame_t &frame = *it;

    for (const var_t &v : frame.vars) {
      if (solver_toolbox.are_exprs_always_equal(v.expr, expr) ||
          solver_toolbox.are_exprs_always_equal(v.addr, expr)) {
        out_var = v;
        return true;
      }

      klee::Expr::Width expr_bits = expr->getWidth();
      klee::Expr::Width var_bits = v.expr->getWidth();

      if (expr_bits > var_bits) {
        continue;
      }

      for (bits_t offset = 0; offset <= var_bits - expr_bits; offset += 8) {
        klee::ref<klee::Expr> var_slice =
            solver_toolbox.exprBuilder->Extract(v.expr, offset, expr_bits);

        if (solver_toolbox.are_exprs_always_equal(var_slice, expr)) {
          out_var = v;
          out_var.name = slice_var(v, offset, expr_bits);
          out_var.expr = var_slice;
          return true;
        }
      }
    }
  }

  return false;
}

void BDDSynthesizer::stack_add(const var_t &var) {
  stack_frame_t &frame = stack.back();
  frame.vars.push_back(var);
}

void BDDSynthesizer::stack_replace(const var_t &var,
                                   klee::ref<klee::Expr> new_expr) {
  for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
    stack_frame_t &frame = *it;
    for (var_t &v : frame.vars) {
      if (v.name == var.name) {
        klee::ref<klee::Expr> old_expr = v.expr;
        assert(old_expr->getWidth() == new_expr->getWidth());
        v.expr = new_expr;
        return;
      }
    }
  }

  assert(false && "Variable not found in stack");
}

BDDSynthesizer::var_t BDDSynthesizer::build_var(const std::string &name,
                                                klee::ref<klee::Expr> expr) {
  return build_var(name, expr, nullptr);
}

BDDSynthesizer::var_t BDDSynthesizer::build_var(const std::string &name,
                                                klee::ref<klee::Expr> expr,
                                                klee::ref<klee::Expr> addr) {
  if (reserved_var_names.find(name) == reserved_var_names.end()) {
    reserved_var_names[name] = 1;
    return var_t(name, expr, addr);
  }

  reserved_var_names[name] += 1;
  return var_t(name + std::to_string(reserved_var_names[name]), expr, addr);
}