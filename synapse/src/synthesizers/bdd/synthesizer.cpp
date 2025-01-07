#include "synthesizer.h"
#include "../../exprs/exprs.h"
#include "../../exprs/solver.h"
#include "../../exprs/retriever.h"
#include "../../log.h"

#define POPULATE_SYNTHESIZER(FNAME)                                                                \
  { #FNAME, std::bind(&BDDSynthesizer::FNAME, this, std::placeholders::_1, std::placeholders::_2) }

namespace synapse {
namespace {
constexpr const char *const NF_TEMPLATE_FILENAME = "nf.template.cpp";
constexpr const char *const PROFILER_TEMPLATE_FILENAME = "profiler.template.cpp";

constexpr const char *const MARKER_NF_STATE = "NF_STATE";
constexpr const char *const MARKER_NF_INIT = "NF_INIT";
constexpr const char *const MARKER_NF_PROCESS = "NF_PROCESS";

std::string template_from_type(BDDSynthesizerTarget target) {
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
} // namespace

BDDSynthesizer::BDDSynthesizer(BDDSynthesizerTarget _target, std::ostream &_out)
    : Synthesizer(template_from_type(_target),
                  {
                      {MARKER_NF_STATE, 0},
                      {MARKER_NF_INIT, 0},
                      {MARKER_NF_PROCESS, 0},
                  },
                  _out),
      target(_target), transpiler(this), init_synthesizers({
                                             POPULATE_SYNTHESIZER(map_allocate),
                                             POPULATE_SYNTHESIZER(vector_allocate),
                                             POPULATE_SYNTHESIZER(dchain_allocate),
                                             POPULATE_SYNTHESIZER(cms_allocate),
                                             POPULATE_SYNTHESIZER(tb_allocate),
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
          POPULATE_SYNTHESIZER(map_size),
          POPULATE_SYNTHESIZER(vector_borrow),
          POPULATE_SYNTHESIZER(vector_return),
          POPULATE_SYNTHESIZER(vector_clear),
          POPULATE_SYNTHESIZER(vector_sample_lt),
          POPULATE_SYNTHESIZER(dchain_allocate_new_index),
          POPULATE_SYNTHESIZER(dchain_rejuvenate_index),
          POPULATE_SYNTHESIZER(dchain_expire_one),
          POPULATE_SYNTHESIZER(dchain_is_index_allocated),
          POPULATE_SYNTHESIZER(dchain_free_index),
          POPULATE_SYNTHESIZER(cms_increment),
          POPULATE_SYNTHESIZER(cms_count_min),
          POPULATE_SYNTHESIZER(cms_periodic_cleanup),
          POPULATE_SYNTHESIZER(tb_is_tracing),
          POPULATE_SYNTHESIZER(tb_trace),
          POPULATE_SYNTHESIZER(tb_update_and_check),
          POPULATE_SYNTHESIZER(tb_expire),
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
    for (const auto &[node_id, map_addr] : nodes_to_map) {
      coder.indent();
      coder << "stats_per_map[" << transpiler.transpile(map_addr) << "]";
      coder << ".init(";
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

  symbol_t device = bdd->get_device();
  symbol_t len = bdd->get_packet_len();
  symbol_t now = bdd->get_time();

  var_t device_var = build_var("device", device.expr);
  var_t len_var = build_var("packet_length", len.expr);
  var_t now_var = build_var("now", now.expr);

  coder << "int nf_process(";
  coder << "uint16_t " << device_var.name << ", ";
  coder << "uint8_t *buffer, ";
  coder << "uint16_t " << len_var.name << ", ";
  coder << "time_ns_t " << now_var.name;
  coder << ") {\n";

  coder.inc();
  stack_push();

  stack_add(device_var);
  stack_add(len_var);
  stack_add(now_var);

  synthesize(bdd->get_root());

  coder.dec();
  coder << "}\n";
}

void BDDSynthesizer::synthesize(const Node *node) {
  coder_t &coder = get(MARKER_NF_PROCESS);

  node->visit_nodes([this, &coder](const Node *node) {
    NodeVisitAction action = NodeVisitAction::Continue;

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
    case NodeType::Branch: {
      const Branch *branch_node = dynamic_cast<const Branch *>(node);

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

      action = NodeVisitAction::Stop;
    } break;
    case NodeType::Call: {
      const Call *call_node = dynamic_cast<const Call *>(node);
      synthesize_process(coder, call_node);
    } break;
    case NodeType::Route: {
      const Route *route_node = dynamic_cast<const Route *>(node);

      RouteOp op = route_node->get_operation();
      int dst_device = route_node->get_dst_device();

      switch (op) {
      case RouteOp::Drop: {
        coder.indent();
        coder << "return DROP;\n";
      } break;
      case RouteOp::Broadcast: {
        coder.indent();
        coder << "return FLOOD;\n";
      } break;
      case RouteOp::Forward: {
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
  if (this->init_synthesizers.find(call.function_name) == this->init_synthesizers.end()) {
    SYNAPSE_PANIC("No init synthesizer found for function: %s\n", call.function_name.c_str());
  }

  (this->init_synthesizers[call.function_name])(coder, call);
}

void BDDSynthesizer::synthesize_process(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  if (this->process_synthesizers.find(call.function_name) == this->process_synthesizers.end()) {
    SYNAPSE_PANIC("No process synthesizer found for function: %s\n", call.function_name.c_str());
  }

  (this->process_synthesizers[call.function_name])(coder, call_node);
}

void BDDSynthesizer::packet_borrow_next_chunk(coder_t &coder, const Call *call_node) {
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

void BDDSynthesizer::packet_return_chunk(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> chunk_addr = call.args.at("the_chunk").expr;
  klee::ref<klee::Expr> chunk = call.args.at("the_chunk").in;

  var_t hdr = stack_get(chunk_addr);
  std::vector<mod_t> changes = build_expr_mods(hdr.expr, chunk);

  for (const mod_t &mod : changes) {
    std::vector<klee::ref<klee::Expr>> bytes = bytes_in_expr(mod.expr);
    for (size_t i = 0; i < bytes.size(); i++) {
      coder.indent();
      coder << hdr.name;
      coder << "[";
      coder << mod.offset + i;
      coder << "] = ";
      coder << transpiler.transpile(bytes[i]);
      coder << ";\n";
    }
  }

  coder.indent();
  coder << "packet_return_chunk(";
  coder << "buffer, ";
  coder << stack_get(chunk_addr).name;
  coder << ")";
  coder << ";\n";
}

void BDDSynthesizer::nf_set_rte_ipv4_udptcp_checksum(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> ip_header = call.args.at("ip_header").expr;
  klee::ref<klee::Expr> tcpudp_header = call.args.at("l4_header").expr;
  symbol_t checksum = call_node->get_local_symbol("checksum");

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

void BDDSynthesizer::expire_items_single_map(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> chain = call.args.at("chain").expr;
  klee::ref<klee::Expr> vector = call.args.at("vector").expr;
  klee::ref<klee::Expr> map = call.args.at("map").expr;
  klee::ref<klee::Expr> time = call.args.at("time").expr;

  symbol_t number_of_freed_flows = call_node->get_local_symbol("number_of_freed_flows");

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

void BDDSynthesizer::expire_items_single_map_iteratively(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> vector = call.args.at("vector").expr;
  klee::ref<klee::Expr> map = call.args.at("map").expr;
  klee::ref<klee::Expr> start = call.args.at("start").expr;
  klee::ref<klee::Expr> n_elems = call.args.at("n_elems").expr;

  symbol_t number_of_freed_flows = call_node->get_local_symbol("number_of_freed_flows");

  var_t nfreed = build_var("freed_flows", number_of_freed_flows.expr);

  coder.indent();
  coder << "int " << nfreed.name << " = ";
  coder << "expire_items_single_map_iteratively(";
  coder << stack_get(vector).name << ", ";
  coder << stack_get(map).name << ", ";
  coder << transpiler.transpile(start) << ", ";
  coder << transpiler.transpile(n_elems);
  coder << ")";
  coder << ";\n";

  stack_add(nfreed);
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

BDDSynthesizer::var_t BDDSynthesizer::build_var_ptr(const std::string &base_name,
                                                    klee::ref<klee::Expr> addr,
                                                    klee::ref<klee::Expr> value, coder_t &coder,
                                                    bool &found_in_stack) {
  bytes_t size = value->getWidth() / 8;

  var_t var;
  if (!(found_in_stack = stack_find(addr, var))) {
    var = build_var(base_name, value, addr);
    coder.indent();
    coder << "uint8_t " << var.name << "[" << size << "];\n";
  } else if (solver_toolbox.are_exprs_always_equal(var.expr, value)) {
    return var;
  }

  var_t stack_value;
  if (stack_find(value, stack_value)) {
    if (stack_value.addr.isNull()) {
      bits_t width = stack_value.expr->getWidth();
      SYNAPSE_ASSERT(width <= klee::Expr::Int64, "Invalid width");
      coder.indent();
      coder << "*(" << BDDTranspiler::type_from_size(width) << "*)";
      coder << var.name;
      coder << " = ";
      coder << stack_value.name;
      coder << ";\n";
    } else {
      coder.indent();
      coder << "memcpy(";
      coder << "(void*)" << var.name << ", ";
      coder << "(void*)" << stack_value.name << ", ";
      coder << size;
      coder << ");\n";
    }

    var.expr = stack_value.expr;
  } else {
    for (bytes_t b = 0; b < size; b++) {
      klee::ref<klee::Expr> byte = solver_toolbox.exprBuilder->Extract(var.expr, b * 8, 8);
      coder.indent();
      coder << var.name << "[" << b << "] = ";
      coder << transpiler.transpile(byte);
      coder << ";\n";
    }
  }

  return var;
}

void BDDSynthesizer::map_get(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> map_addr = call.args.at("map").expr;
  klee::ref<klee::Expr> key_addr = call.args.at("key").expr;
  klee::ref<klee::Expr> key = call.args.at("key").in;
  klee::ref<klee::Expr> value_out_addr = call.args.at("value_out").expr;
  klee::ref<klee::Expr> value_out = call.args.at("value_out").out;

  symbol_t map_has_this_key = call_node->get_local_symbol("map_has_this_key");

  var_t r = build_var("map_hit", map_has_this_key.expr);
  var_t v = build_var("value", value_out);

  bool key_in_stack;
  var_t k = build_var_ptr("key", key_addr, key, coder, key_in_stack);

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
    nodes_to_map.insert({call_node->get_id(), map_addr});
    coder.indent();
    coder << "stats_per_map[" << transpiler.transpile(map_addr) << "]";
    coder << ".update(";
    coder << call_node->get_id() << ", ";
    coder << k.name << ", ";
    coder << key->getWidth() / 8 << ", ";
    coder << "now"; // FIXME: we should get this from the stack
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
  var_t k = build_var_ptr("key", key_addr, key, coder, key_in_stack);

  coder.indent();
  coder << "map_put(";
  coder << stack_get(map_addr).name << ", ";
  coder << k.name << ", ";
  coder << transpiler.transpile(value);
  coder << ")";
  coder << ";\n";

  if (target == BDDSynthesizerTarget::PROFILER) {
    nodes_to_map.insert({call_node->get_id(), map_addr});
    coder.indent();
    coder << "stats_per_map[" << transpiler.transpile(map_addr) << "]";
    coder << ".update(";
    coder << call_node->get_id() << ", ";
    coder << k.name << ", ";
    coder << key->getWidth() / 8 << ", ";
    coder << "now"; // FIXME: we should get this from the stack
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
  var_t k = build_var_ptr("key", key_addr, key, coder, key_in_stack);

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
    nodes_to_map.insert({call_node->get_id(), map_addr});
    coder.indent();
    coder << "stats_per_map[" << transpiler.transpile(map_addr) << "]";
    coder << ".update(";
    coder << call_node->get_id() << ", ";
    coder << k.name << ", ";
    coder << key->getWidth() / 8 << ", ";
    coder << "now"; // FIXME: we should get this from the stack
    coder << ")";
    coder << ";\n";
  }

  if (!key_in_stack) {
    stack_add(k);
  } else {
    stack_replace(k, key);
  }
}

void BDDSynthesizer::map_size(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> map_addr = call.args.at("map").expr;
  klee::ref<klee::Expr> map_usage = call.ret;

  var_t s = build_var("map_usage", map_usage);

  coder.indent();
  coder << "uint32_t " << s.name << ";\n";

  coder.indent();
  coder << s.name << " = ";
  coder << "map_size(";
  coder << stack_get(map_addr).name;
  coder << ")";
  coder << ";\n";

  stack_add(s);
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

  if (solver_toolbox.are_exprs_always_equal(v.expr, value)) {
    return;
  }

  var_t new_v;
  if (value->getKind() != klee::Expr::Constant && stack_find(value, new_v)) {
    coder.indent();
    coder << "memcpy(";
    coder << "(void*)" << v.name << ", ";
    coder << "(void*)";
    if (new_v.addr.isNull()) {
      coder << "&";
    }
    coder << new_v.name << ", ";
    coder << value->getWidth() / 8;
    coder << ");\n";
    return;
  }

  std::vector<mod_t> changes = build_expr_mods(v.expr, value);
  for (const mod_t &mod : changes) {
    coder.indent();

    SYNAPSE_ASSERT(mod.width <= 64, "Vector element size is too large");

    if (mod.width == 8) {
      coder << v.name;
      coder << "[";
      coder << mod.offset;
      coder << "]";
    } else {
      coder << "*(" << BDDTranspiler::type_from_size(mod.width) << "*)";
      coder << v.name;
    }

    coder << " = ";
    coder << transpiler.transpile(mod.expr);
    coder << ";\n";
  }
}

void BDDSynthesizer::vector_clear(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> vector_addr = call.args.at("vector").expr;

  coder.indent();
  coder << "vector_borrow(";
  coder << stack_get(vector_addr).name;
  coder << ")";
  coder << ";\n";
}

void BDDSynthesizer::vector_sample_lt(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> vector_addr = call.args.at("vector").expr;
  klee::ref<klee::Expr> samples = call.args.at("samples").expr;
  klee::ref<klee::Expr> threshold_addr = call.args.at("threshold").expr;
  klee::ref<klee::Expr> threshold = call.args.at("threshold").in;
  klee::ref<klee::Expr> index_out_addr = call.args.at("index_out").expr;
  klee::ref<klee::Expr> index_out = call.args.at("index_out").out;

  bool threshold_in_stack;
  var_t t = build_var_ptr("threshold", threshold_addr, threshold, coder, threshold_in_stack);

  symbol_t found_sample = call_node->get_local_symbol("found_sample");

  var_t f = build_var("found_sample", found_sample.expr);
  var_t i = build_var("sample_index", index_out);

  coder.indent();
  coder << "int " << i.name << ";\n";

  coder.indent();
  coder << "int " << f.name << " = ";
  coder << "vector_sample_lt(";
  coder << stack_get(vector_addr).name << ", ";
  coder << transpiler.transpile(samples) << ", ";
  coder << t.name << ", ";
  coder << "&" << i.name;
  coder << ")";
  coder << ";\n";

  if (!threshold_in_stack) {
    stack_add(t);
  } else {
    stack_replace(t, threshold);
  }

  stack_add(f);
  stack_add(i);
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

void BDDSynthesizer::dchain_allocate_new_index(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> dchain_addr = call.args.at("chain").expr;
  klee::ref<klee::Expr> time = call.args.at("time").expr;
  klee::ref<klee::Expr> index_out = call.args.at("index_out").out;

  symbol_t out_of_space = call_node->get_local_symbol("out_of_space");

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

void BDDSynthesizer::dchain_rejuvenate_index(coder_t &coder, const Call *call_node) {
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
  // const call_t &call = call_node->get_call();

  // coder.indent();
  // coder << "// dchain_expire_one";
  // coder << ";\n";

  SYNAPSE_PANIC("TODO");
}

void BDDSynthesizer::dchain_is_index_allocated(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> dchain_addr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index = call.args.at("index").expr;

  symbol_t is_allocated = call_node->get_local_symbol("dchain_is_index_allocated");

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

void BDDSynthesizer::cms_allocate(coder_t &coder, const call_t &call) {
  klee::ref<klee::Expr> height = call.args.at("height").expr;
  klee::ref<klee::Expr> width = call.args.at("width").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> cleanup_interval = call.args.at("cleanup_interval").expr;
  klee::ref<klee::Expr> cms_out = call.args.at("cms_out").out;

  var_t cms_out_var = build_var("cms", cms_out);

  coder_t &coder_nf_state = get(MARKER_NF_STATE);
  coder_nf_state.indent();
  coder_nf_state << "struct CMS *";
  coder_nf_state << cms_out_var.name;
  coder_nf_state << ";\n";

  coder << "cms_allocate(";
  coder << transpiler.transpile(height) << ", ";
  coder << transpiler.transpile(width) << ", ";
  coder << transpiler.transpile(key_size) << ", ";
  coder << transpiler.transpile(cleanup_interval) << ", ";
  coder << "&" << cms_out_var.name;
  coder << ")";

  stack_add(cms_out_var);
}

void BDDSynthesizer::cms_increment(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> cms_addr = call.args.at("cms").expr;
  klee::ref<klee::Expr> key = call.args.at("key").in;
  klee::ref<klee::Expr> key_addr = call.args.at("key").expr;

  bool key_in_stack;
  var_t k = build_var_ptr("key", key_addr, key, coder, key_in_stack);

  coder.indent();
  coder << "cms_increment(";
  coder << stack_get(cms_addr).name << ", ";
  coder << k.name;
  coder << ")";
  coder << ";\n";

  if (!key_in_stack) {
    stack_add(k);
  } else {
    stack_replace(k, key);
  }
}

void BDDSynthesizer::cms_count_min(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> cms_addr = call.args.at("cms").expr;
  klee::ref<klee::Expr> key = call.args.at("key").in;
  klee::ref<klee::Expr> key_addr = call.args.at("key").expr;
  klee::ref<klee::Expr> min_estimate = call.ret;

  bool key_in_stack;
  var_t k = build_var_ptr("key", key_addr, key, coder, key_in_stack);

  var_t me = build_var("min_estimate", min_estimate);

  coder.indent();
  coder << "uint64_t " << me.name << " = ";
  coder << "cms_count_min(";
  coder << stack_get(cms_addr).name << ", ";
  coder << k.name;
  coder << ")";
  coder << ";\n";

  if (!key_in_stack) {
    stack_add(k);
  } else {
    stack_replace(k, key);
  }

  stack_add(me);
}

void BDDSynthesizer::cms_periodic_cleanup(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> cms_addr = call.args.at("cms").expr;
  klee::ref<klee::Expr> time = call.args.at("time").expr;

  symbol_t cleanup_success = call_node->get_local_symbol("cleanup_success");

  var_t cs = build_var("cleanup_success", cleanup_success.expr);

  coder.indent();
  coder << "int " << cs.name << " = ";
  coder << "cms_periodic_cleanup(";
  coder << stack_get(cms_addr).name << ", ";
  coder << transpiler.transpile(time);
  coder << ")";
  coder << ";\n";

  stack_add(cs);
}

void BDDSynthesizer::tb_allocate(coder_t &coder, const call_t &call) {
  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> rate = call.args.at("rate").expr;
  klee::ref<klee::Expr> burst = call.args.at("burst").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> tb_out = call.args.at("tb_out").out;

  var_t tb_out_var = build_var("tb", tb_out);

  coder_t &coder_nf_state = get(MARKER_NF_STATE);
  coder_nf_state.indent();
  coder_nf_state << "struct TokenBucket *";
  coder_nf_state << tb_out_var.name;
  coder_nf_state << ";\n";

  coder << "tb_allocate(";
  coder << transpiler.transpile(capacity) << ", ";
  coder << transpiler.transpile(rate) << "ull, ";
  coder << transpiler.transpile(burst) << "ull, ";
  coder << transpiler.transpile(key_size) << ", ";
  coder << "&" << tb_out_var.name;
  coder << ")";

  stack_add(tb_out_var);
}

void BDDSynthesizer::tb_is_tracing(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> tb_addr = call.args.at("tb").expr;
  klee::ref<klee::Expr> key_addr = call.args.at("key").expr;
  klee::ref<klee::Expr> key = call.args.at("key").in;
  klee::ref<klee::Expr> index_out = call.args.at("index_out").out;
  klee::ref<klee::Expr> is_tracing = call.ret;

  bool key_in_stack;
  var_t k = build_var_ptr("key", key_addr, key, coder, key_in_stack);

  var_t it = build_var("is_tracing", is_tracing);
  var_t index = build_var("index", index_out);

  coder.indent();
  coder << "int " << index.name << ";\n";

  coder.indent();
  coder << "int " << it.name << " = ";
  coder << "tb_is_tracing(";
  coder << stack_get(tb_addr).name << ", ";
  coder << k.name << ", ";
  coder << "&" << index.name;
  coder << ")";
  coder << ";\n";

  stack_add(it);
  stack_add(index);

  if (!key_in_stack) {
    stack_add(k);
  } else {
    stack_replace(k, key);
  }
}

void BDDSynthesizer::tb_trace(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> tb_addr = call.args.at("tb").expr;
  klee::ref<klee::Expr> key_addr = call.args.at("key").expr;
  klee::ref<klee::Expr> key = call.args.at("key").in;
  klee::ref<klee::Expr> pkt_len = call.args.at("pkt_len").expr;
  klee::ref<klee::Expr> time = call.args.at("time").expr;
  klee::ref<klee::Expr> index_out = call.args.at("index_out").out;
  klee::ref<klee::Expr> successfuly_tracing = call.ret;

  bool key_in_stack;
  var_t k = build_var_ptr("key", key_addr, key, coder, key_in_stack);

  var_t st = build_var("successfuly_tracing", successfuly_tracing);
  var_t i = build_var("index", index_out);

  coder.indent();
  coder << "int " << i.name << ";\n";

  coder.indent();
  coder << "int " << st.name << " = ";
  coder << "tb_trace(";
  coder << stack_get(tb_addr).name << ", ";
  coder << k.name << ", ";
  coder << transpiler.transpile(pkt_len) << ", ";
  coder << transpiler.transpile(time) << ", ";
  coder << "&" << i.name;
  coder << ")";
  coder << ";\n";

  stack_add(st);
  stack_add(i);

  if (!key_in_stack) {
    stack_add(k);
  } else {
    stack_replace(k, key);
  }
}

void BDDSynthesizer::tb_update_and_check(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> tb_addr = call.args.at("tb").expr;
  klee::ref<klee::Expr> index = call.args.at("index").expr;
  klee::ref<klee::Expr> pkt_len = call.args.at("pkt_len").expr;
  klee::ref<klee::Expr> time = call.args.at("time").expr;
  klee::ref<klee::Expr> pass = call.ret;

  var_t p = build_var("pass", pass);

  coder.indent();
  coder << "int " << p.name << " = ";
  coder << "tb_update_and_check(";
  coder << stack_get(tb_addr).name << ", ";
  coder << transpiler.transpile(index) << ", ";
  coder << transpiler.transpile(pkt_len) << ", ";
  coder << transpiler.transpile(time);
  coder << ")";
  coder << ";\n";

  stack_add(p);
}

void BDDSynthesizer::tb_expire(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> tb_addr = call.args.at("tb").expr;
  klee::ref<klee::Expr> time = call.args.at("time").expr;

  coder.indent();
  coder << "tb_expire(";
  coder << stack_get(tb_addr).name << ", ";
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
  SYNAPSE_PANIC("Variable not found in stack: %s\n", name.c_str());
}

BDDSynthesizer::var_t BDDSynthesizer::stack_get(klee::ref<klee::Expr> expr) {
  var_t var;
  if (stack_find(expr, var)) {
    return var;
  }

  stack_dbg();
  SYNAPSE_PANIC("Variable not found in stack: %s\n", expr_to_string(expr).c_str());
}

code_t BDDSynthesizer::slice_var(const var_t &var, bits_t offset, bits_t size) const {
  SYNAPSE_ASSERT(offset + size <= var.expr->getWidth(), "Out of bounds");

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
  if (var.expr.isNull()) {
    SYNAPSE_PANIC("Trying to add a variable with a null expression: %s\n", var.name.c_str());
  }

  stack_frame_t &frame = stack.back();
  frame.vars.push_back(var);
}

void BDDSynthesizer::stack_replace(const var_t &var, klee::ref<klee::Expr> new_expr) {
  for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
    stack_frame_t &frame = *it;
    for (var_t &v : frame.vars) {
      if (v.name == var.name) {
        klee::ref<klee::Expr> old_expr = v.expr;
        SYNAPSE_ASSERT(old_expr->getWidth() == new_expr->getWidth(), "Width mismatch");
        v.expr = new_expr;
        return;
      }
    }
  }

  stack_dbg();
  SYNAPSE_PANIC("Variable not found in stack: %s\nExpr: %s\n", var.name.c_str(),
                expr_to_string(new_expr).c_str());
}

BDDSynthesizer::var_t BDDSynthesizer::build_var(const std::string &name,
                                                klee::ref<klee::Expr> expr) {
  return build_var(name, expr, nullptr);
}

BDDSynthesizer::var_t BDDSynthesizer::build_var(const std::string &name, klee::ref<klee::Expr> expr,
                                                klee::ref<klee::Expr> addr) {
  if (reserved_var_names.find(name) == reserved_var_names.end()) {
    reserved_var_names[name] = 1;
    return var_t(name, expr, addr);
  }

  reserved_var_names[name] += 1;
  return var_t(name + std::to_string(reserved_var_names[name]), expr, addr);
}
} // namespace synapse