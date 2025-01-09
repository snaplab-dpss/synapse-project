#include "synthesizer.h"
#include "../../../targets/module.h"

namespace synapse {
namespace tofino {
namespace {

constexpr const char *const MARKER_CPU_HEADER = "CPU_HEADER";
constexpr const char *const MARKER_CUSTOM_HEADERS = "CUSTOM_HEADERS";

constexpr const char *const MARKER_INGRESS_HEADERS = "INGRESS_HEADERS";
constexpr const char *const MARKER_INGRESS_METADATA = "INGRESS_METADATA";
constexpr const char *const MARKER_INGRESS_PARSER = "INGRESS_PARSER";
constexpr const char *const MARKER_INGRESS_CONTROL = "INGRESS_CONTROL";
constexpr const char *const MARKER_INGRESS_CONTROL_APPLY = "INGRESS_CONTROL_APPLY";
constexpr const char *const MARKER_INGRESS_DEPARSER = "INGRESS_DEPARSER";

constexpr const char *const MARKER_EGRESS_HEADERS = "EGRESS_HEADERS";
constexpr const char *const MARKER_EGRESS_METADATA = "EGRESS_METADATA";

constexpr const char *const TEMPLATE_FILENAME = "tofino.template.p4";

template <class T> const T *get_tofino_ds(const EP *ep, DS_ID id) {
  const Context &ctx = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  const DS *ds = tofino_ctx->get_ds_from_id(id);
  assert(ds && "DS not found");
  return dynamic_cast<const T *>(ds);
}

const Parser &get_tofino_parser(const EP *ep) {
  const Context &ctx = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  const TNA &tna = tofino_ctx->get_tna();
  return tna.parser;
}

code_t get_parser_state_name(const ParserState *state, bool state_init) {
  coder_t coder;

  if (state_init) {
    coder << "parser_init";
    return coder.dump();
  }

  coder << "parser_";
  bool first = true;
  for (node_id_t id : state->ids) {
    if (!first) {
      coder << "_";
    } else {
      first = false;
    }
    coder << id;
  }

  return coder.dump();
}

bool naturalCompare(const std::string &a, const std::string &b) {
  size_t i = 0, j = 0;

  while (i < a.size() && j < b.size()) {
    if (std::isdigit(a[i]) && std::isdigit(b[j])) {
      size_t start_i = i, start_j = j;
      while (i < a.size() && std::isdigit(a[i]))
        ++i;
      while (j < b.size() && std::isdigit(b[j]))
        ++j;

      int num_a = std::stoi(a.substr(start_i, i - start_i));
      int num_b = std::stoi(b.substr(start_j, j - start_j));

      if (num_a != num_b)
        return num_a < num_b;
    } else {
      if (a[i] != b[j])
        return a[i] < b[j];
      ++i;
      ++j;
    }
  }

  return a.size() < b.size();
}
} // namespace

EPSynthesizer::EPSynthesizer(std::ostream &_out, const BDD *bdd)
    : Synthesizer(TEMPLATE_FILENAME,
                  {
                      {MARKER_CPU_HEADER, 1},
                      {MARKER_CUSTOM_HEADERS, 0},
                      {MARKER_INGRESS_HEADERS, 1},
                      {MARKER_INGRESS_METADATA, 1},
                      {MARKER_INGRESS_PARSER, 1},
                      {MARKER_INGRESS_CONTROL, 1},
                      {MARKER_INGRESS_CONTROL_APPLY, 2},
                      {MARKER_INGRESS_DEPARSER, 2},
                      {MARKER_EGRESS_HEADERS, 1},
                      {MARKER_EGRESS_METADATA, 1},
                  },
                  _out),
      var_stacks(1), transpiler(this) {
  symbol_t device = bdd->get_device();
  symbol_t time = bdd->get_time();

  // Hack
  var_stacks.back().emplace_back("ig_intr_md.ingress_port", solver_toolbox.exprBuilder->Extract(device.expr, 0, 16));

  var_stacks.back().emplace_back("ig_intr_md.ingress_mac_tstamp[47:16]", time);
}

void EPSynthesizer::visit(const EP *ep) {
  EPVisitor::visit(ep);

  // Transpile the parser after the whole EP has been visited so we have all the
  // headers available.
  transpile_parser(get_tofino_parser(ep));

  Synthesizer::dump();
}

void EPSynthesizer::visit(const EP *ep, const EPNode *ep_node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);
  ingress_apply.indent();
  ingress_apply << "// Node " << ep_node->get_id() << "\n";
  EPVisitor::visit(ep, ep_node);
}

void EPSynthesizer::transpile_parser(const Parser &parser) {
  coder_t &ingress_parser = get(MARKER_INGRESS_PARSER);

  std::vector<const ParserState *> states{parser.get_initial_state()};
  bool state_init = true;

  while (!states.empty()) {
    const ParserState *state = states.front();
    states.erase(states.begin());

    var_stacks.emplace_back();
    hdrs_stacks.emplace_back();

    for (node_id_t id : state->ids) {
      if (parser_vars.find(id) != parser_vars.end()) {
        const vars_t &vars = parser_vars.at(id);
        var_stacks.back().insert(var_stacks.back().end(), vars.begin(), vars.end());
      }

      if (parser_hdrs.find(id) != parser_hdrs.end()) {
        const vars_t &vars = parser_hdrs.at(id);
        hdrs_stacks.back().insert(hdrs_stacks.back().end(), vars.begin(), vars.end());
      }
    }

    switch (state->type) {
    case ParserStateType::EXTRACT: {
      const ParserStateExtract *extract = dynamic_cast<const ParserStateExtract *>(state);

      code_t state_name = get_parser_state_name(state, state_init);

      var_t hdr_var;
      bool hdr_found = get_hdr_var(*extract->ids.begin(), extract->hdr, hdr_var);
      assert(hdr_found && "Header not found");

      assert(extract->next && "Next state not found");
      code_t next_state = get_parser_state_name(extract->next, false);

      ingress_parser.indent();
      ingress_parser << "state " << state_name << " {\n";

      ingress_parser.inc();
      ingress_parser.indent();
      ingress_parser << "pkt.extract(" << hdr_var.name << ");\n";

      ingress_parser.indent();
      ingress_parser << "transition " << next_state << ";\n";

      ingress_parser.dec();
      ingress_parser.indent();
      ingress_parser << "}\n";

      states.push_back(extract->next);
    } break;
    case ParserStateType::SELECT: {
      const ParserStateSelect *select = dynamic_cast<const ParserStateSelect *>(state);

      code_t state_name = get_parser_state_name(state, state_init);

      ingress_parser.indent();
      ingress_parser << "state " << state_name << " {\n";

      ingress_parser.inc();
      ingress_parser.indent();
      ingress_parser << "transition select (" << transpiler.transpile(select->field) << ") {\n";

      ingress_parser.inc();

      code_t next_true = get_parser_state_name(select->on_true, false);
      code_t next_false = get_parser_state_name(select->on_false, false);

      for (int value : select->values) {
        ingress_parser.indent();
        ingress_parser << value << ": " << next_true << ";\n";
      }

      ingress_parser.indent();
      ingress_parser << "default: " << next_false << ";\n";

      ingress_parser.dec();
      ingress_parser.indent();
      ingress_parser << "}\n";

      ingress_parser.dec();
      ingress_parser.indent();
      ingress_parser << "}\n";

      states.push_back(select->on_true);
      states.push_back(select->on_false);
    } break;
    case ParserStateType::TERMINATE: {
      const ParserStateTerminate *terminate = dynamic_cast<const ParserStateTerminate *>(state);

      code_t state_name = get_parser_state_name(state, state_init);

      ingress_parser.indent();
      ingress_parser << "state " << state_name << " {\n";

      ingress_parser.inc();
      ingress_parser.indent();
      ingress_parser << "transition ";
      if (terminate->accept) {
        ingress_parser << "accept";
      } else {
        ingress_parser << "reject";
      }
      ingress_parser << ";\n";

      ingress_parser.dec();
      ingress_parser.indent();
      ingress_parser << "}\n";
    } break;
    }

    var_stacks.pop_back();
    hdrs_stacks.pop_back();

    state_init = false;
  }
}

code_t EPSynthesizer::slice_var(const var_t &var, unsigned offset, bits_t size) const {
  assert(offset + size <= var.expr->getWidth() && "Invalid slice");
  coder_t coder;
  coder << var.name << "[" << offset + size - 1 << ":" << offset << "]";
  return coder.dump();
}

code_t EPSynthesizer::type_from_var(const var_t &var) const {
  if (var.is_bool) {
    return "bool";
  }

  return transpiler.type_from_expr(var.expr);
}

bool EPSynthesizer::get_var(klee::ref<klee::Expr> expr, var_t &out_var) const {
  for (auto vars_it = var_stacks.rbegin(); vars_it != var_stacks.rend(); ++vars_it) {
    for (const var_t &var : *vars_it) {
      if (solver_toolbox.are_exprs_always_equal(var.expr, expr)) {
        out_var = var;
        return true;
      }

      bits_t expr_size = expr->getWidth();
      bits_t var_size = var.expr->getWidth();

      if (expr_size > var_size) {
        continue;
      }

      for (bits_t offset = 0; offset + expr_size <= var_size; offset += 8) {
        klee::ref<klee::Expr> var_slice = solver_toolbox.exprBuilder->Extract(var.expr, offset, expr_size);

        if (solver_toolbox.are_exprs_always_equal(var_slice, expr)) {
          out_var = var;
          out_var.name = slice_var(var, offset, expr_size);
          return true;
        }
      }
    }
  }

  return false;
}

EPSynthesizer::vars_t EPSynthesizer::get_squashed_vars() const {
  vars_t vars;

  for (const vars_t &stack : var_stacks) {
    vars.insert(vars.end(), stack.begin(), stack.end());
  }

  return vars;
}

EPSynthesizer::vars_t EPSynthesizer::get_squashed_hdrs() const {
  vars_t vars;

  for (const vars_t &stack : hdrs_stacks) {
    vars.insert(vars.end(), stack.begin(), stack.end());
  }

  return vars;
}

bool EPSynthesizer::get_hdr_var(node_id_t node_id, klee::ref<klee::Expr> expr, var_t &out_var) const {
  for (const var_t &var : parser_hdrs.at(node_id)) {
    if (solver_toolbox.are_exprs_always_equal(var.expr, expr)) {
      out_var = var;
      return true;
    }

    bits_t expr_size = expr->getWidth();
    bits_t var_size = var.expr->getWidth();

    if (expr_size > var_size) {
      continue;
    }

    for (bits_t offset = 0; offset <= var_size - expr_size; offset += 8) {
      klee::ref<klee::Expr> var_slice = solver_toolbox.exprBuilder->Extract(var.expr, offset, expr_size);

      if (solver_toolbox.are_exprs_always_equal(var_slice, expr)) {
        out_var = var;
        out_var.name = slice_var(var, offset, expr_size);
        return true;
      }
    }
  }

  return false;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::SendToController *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);
  coder_t &cpu_hdr = get(MARKER_CPU_HEADER);

  const Symbols &symbols = node->get_symbols();

  for (const symbol_t &symbol : symbols.get()) {
    var_t var;
    bool found = get_var(symbol.expr, var);
    assert(found && "Symbol not found");

    ingress_apply.indent();
    ingress_apply << "hdr.cpu.";
    ingress_apply << var.name;
    ingress_apply << " = ";
    ingress_apply << var.name;
    ingress_apply << ";\n";

    if (var.is_bool) {
      cpu_hdr.indent();
      cpu_hdr << "@padding bit<7> pad_";
      cpu_hdr << var.name;
      cpu_hdr << ";\n";
    }

    cpu_hdr.indent();
    cpu_hdr << type_from_var(var);
    cpu_hdr << " ";
    cpu_hdr << var.name << ";\n";
  }

  ingress_apply.indent();
  ingress_apply << "send_to_controller(";
  ingress_apply << ep_node->get_id();
  ingress_apply << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::Recirculate *node) {
  panic("TODO: Recirculate");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::Ignore *node) {
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::If *node) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL_APPLY);

  const std::vector<klee::ref<klee::Expr>> &conditions = node->get_conditions();

  const std::vector<EPNode *> &children = ep_node->get_children();
  assert(children.size() == 2 && "If node must have 2 children");

  const EPNode *then_node = children[0];
  const EPNode *else_node = children[1];

  if (conditions.size() == 1) {
    klee::ref<klee::Expr> condition = conditions[0];

    ingress.indent();
    ingress << "if (";
    ingress << transpiler.transpile(condition);
    ingress << ") {\n";

    ingress.inc();
    var_stacks.emplace_back();
    visit(ep, then_node);
    var_stacks.pop_back();
    ingress.dec();

    ingress.indent();
    ingress.stream << "} else {\n";

    ingress.inc();
    var_stacks.emplace_back();
    visit(ep, else_node);
    var_stacks.pop_back();
    ingress.dec();

    ingress.indent();
    ingress << "}\n";

    return EPVisitor::Action::skipChildren;
  }

  code_t cond_val = get_unique_name("cond");

  ingress.indent();
  ingress << "bool " << cond_val << " = false;\n";

  for (klee::ref<klee::Expr> condition : conditions) {
    ingress.indent();
    ingress << "if (";
    ingress << transpiler.transpile(condition);
    ingress << ") {\n";

    ingress.inc();
  }

  ingress.indent();
  ingress << cond_val << " = true;\n";

  for (size_t i = 0; i < conditions.size(); i++) {
    ingress.dec();
    ingress.indent();
    ingress << "}\n";
  }

  ingress.indent();
  ingress << "if (";
  ingress << cond_val;
  panic("TODO: condition");
  ingress << ") {\n";

  ingress.inc();
  var_stacks.emplace_back();
  visit(ep, then_node);
  var_stacks.pop_back();
  ingress.dec();

  ingress.indent();
  ingress << "} else {\n";

  ingress.inc();
  var_stacks.emplace_back();
  visit(ep, else_node);
  var_stacks.pop_back();
  ingress.dec();

  ingress.indent();
  ingress << "}\n";

  return EPVisitor::Action::skipChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::ParserCondition *node) {
  parser_vars[node->get_node()->get_id()] = get_squashed_vars();
  parser_hdrs[node->get_node()->get_id()] = get_squashed_hdrs();
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::Then *node) {
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::Else *node) {
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::Forward *node) {
  int dst_device = node->get_dst_device();

  coder_t &ingress = get(MARKER_INGRESS_CONTROL_APPLY);

  ingress.indent();
  ingress << "fwd(";
  ingress << dst_device;
  ingress << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::Drop *node) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL_APPLY);
  ingress.indent();
  ingress << "drop();\n";
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::ParserReject *node) {
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::Broadcast *node) {
  panic("TODO: Broadcast");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::ParserExtraction *node) {
  klee::ref<klee::Expr> hdr = node->get_hdr();
  code_t hdr_name = get_unique_name("hdr");

  coder_t &custom_hdrs = get(MARKER_CUSTOM_HEADERS);
  custom_hdrs.indent();
  custom_hdrs << "header " << hdr_name << "_h {\n";

  custom_hdrs.inc();
  custom_hdrs.indent();
  custom_hdrs << transpiler.type_from_expr(hdr) << " data;\n";

  custom_hdrs.dec();
  custom_hdrs.indent();
  custom_hdrs << "}\n";

  coder_t &ingress_hdrs = get(MARKER_INGRESS_HEADERS);
  ingress_hdrs.indent();
  ingress_hdrs << hdr_name << "_h " << hdr_name << ";\n";

  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);
  ingress_apply.indent();
  ingress_apply << "if(hdr." << hdr_name << ".isValid()) {\n";

  ingress_apply.inc();

  // On header valid only.
  var_stacks.emplace_back();
  hdrs_stacks.emplace_back();

  var_stacks.back().emplace_back("hdr." + hdr_name + ".data", hdr);
  hdrs_stacks.back().emplace_back("hdr." + hdr_name, hdr);

  parser_vars[node->get_node()->get_id()] = get_squashed_vars();
  parser_hdrs[node->get_node()->get_id()] = get_squashed_hdrs();

  assert(ep_node->get_children().size() == 1 && "ParserExtraction must have 1 child");
  visit(ep, ep_node->get_children()[0]);

  var_stacks.pop_back();
  ingress_apply.dec();

  ingress_apply.indent();
  ingress_apply << "}\n";

  return EPVisitor::Action::skipChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::ModifyHeader *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  klee::ref<klee::Expr> hdr = node->get_hdr();

  var_t hdr_var;
  bool found = get_var(hdr, hdr_var);
  assert(found && "Header not found");

  const std::vector<expr_mod_t> &changes = node->get_changes();
  const std::vector<expr_byte_swap_t> &swaps = node->get_swaps();

  for (const expr_byte_swap_t &byte_swap : swaps) {
    bits_t hi_offset = std::max(byte_swap.byte0, byte_swap.byte1) * 8;
    bits_t lo_offset = std::min(byte_swap.byte0, byte_swap.byte1) * 8;

    ingress_apply.indent();
    ingress_apply << "swap(";
    ingress_apply << hdr_var.name;
    ingress_apply << "[";
    ingress_apply << hi_offset + 7;
    ingress_apply << ":";
    ingress_apply << hi_offset;
    ingress_apply << "]";
    ingress_apply << ", ";
    ingress_apply << hdr_var.name;
    ingress_apply << "[";
    ingress_apply << lo_offset + 7;
    ingress_apply << ":";
    ingress_apply << lo_offset;
    ingress_apply << "]";
    ingress_apply << ");\n";
  }

  for (const expr_mod_t &mod : changes) {
    auto swapped = [&mod](const expr_byte_swap_t &byte_swap) -> bool {
      assert(mod.width == 8 && "TODO: deal with non-byte modifications with swaps");
      return mod.offset == byte_swap.byte0 * 8 || mod.offset == byte_swap.byte1 * 8;
    };

    if (std::any_of(swaps.begin(), swaps.end(), swapped)) {
      continue;
    }

    klee::ref<klee::Expr> expr = mod.expr;
    ingress_apply.indent();
    ingress_apply << hdr_var.name;
    ingress_apply << "[";
    ingress_apply << mod.offset + mod.width - 1;
    ingress_apply << ":";
    ingress_apply << mod.offset;
    ingress_apply << "]";
    ingress_apply << " = ";
    ingress_apply << transpiler.transpile(expr);
    ingress_apply << ";\n";
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::TableLookup *node) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  DS_ID table_id = node->get_table_id();
  std::optional<symbol_t> hit = node->get_hit();

  const Table *table = get_tofino_ds<Table>(ep, table_id);

  if (declared_ds.find(table_id) == declared_ds.end()) {
    ingress << transpile_table_decl(ingress.lvl, table, node->get_keys(), node->get_values());
    ingress << "\n";
    declared_ds.insert(table_id);
  }

  ingress_apply.indent();

  if (hit) {
    code_t hit_var_name = get_unique_name("hit_" + table_id);
    var_stacks.back().emplace_back(hit_var_name, hit.value(), true);
    ingress_apply << "bool " << hit_var_name << " = ";
  }

  ingress_apply << table_id << ".apply()";
  if (hit) {
    ingress_apply << ".hit";
  }
  ingress_apply << ";";
  ingress_apply << "\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::VectorRegisterLookup *node) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const std::unordered_set<DS_ID> &rids = node->get_rids();
  klee::ref<klee::Expr> index = node->get_index();
  klee::ref<klee::Expr> value = node->get_value();

  std::vector<const Register *> regs;
  std::for_each(rids.begin(), rids.end(), [ep, &regs](DS_ID rid) { regs.push_back(get_tofino_ds<Register>(ep, rid)); });
  std::sort(regs.begin(), regs.end(), [](const Register *r0, const Register *r1) { return naturalCompare(r0->id, r1->id); });

  for (const Register *reg : regs) {
    if (declared_ds.find(reg->id) == declared_ds.end()) {
      ingress << transpile_register_decl(ingress.lvl, reg, index, value);
      declared_ds.insert(reg->id);
    }
  }

  if (!regs.empty()) {
    ingress << "\n";
  }

  int i = 0;
  bits_t offset = 0;
  std::vector<var_t> read_vars;
  for (const Register *reg : regs) {
    code_t entry_name = get_unique_name("vector_reg_entry_" + std::to_string(i));
    klee::ref<klee::Expr> entry_expr = solver_toolbox.exprBuilder->Extract(value, offset, reg->value_size);
    var_t entry_var(entry_name, entry_expr);
    read_vars.push_back(entry_var);

    code_t action_name = build_register_action_name(ep_node, reg, RegisterActionType::READ);
    ingress << transpile_register_read_action_decl(ingress.lvl, reg, action_name);
    ingress << "\n";

    ingress_apply.indent();
    ingress_apply << transpiler.type_from_size(reg->value_size) << " " << entry_name;
    ingress_apply << " = ";
    ingress_apply << action_name;
    ingress_apply << ".execute(" << transpiler.transpile(index) << ");\n";

    offset += reg->value_size;
    i++;
  }

  for (const var_t &read_var : read_vars) {
    var_stacks.back().push_back(read_var);
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::VectorRegisterUpdate *node) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const std::unordered_set<DS_ID> &rids = node->get_rids();
  klee::ref<klee::Expr> index = node->get_index();
  klee::ref<klee::Expr> value = node->get_read_value();
  klee::ref<klee::Expr> write_value = node->get_write_value();

  std::vector<const Register *> regs;
  std::for_each(rids.begin(), rids.end(), [ep, &regs](DS_ID rid) { regs.push_back(get_tofino_ds<Register>(ep, rid)); });
  std::sort(regs.begin(), regs.end(), [](const Register *r0, const Register *r1) { return naturalCompare(r0->id, r1->id); });

  for (const Register *reg : regs) {
    if (declared_ds.find(reg->id) == declared_ds.end()) {
      ingress << transpile_register_decl(ingress.lvl, reg, index, value);
      declared_ds.insert(reg->id);
    }
  }

  if (!regs.empty()) {
    ingress << "\n";
  }

  int i = 0;
  bits_t offset = 0;
  std::vector<var_t> write_vars;
  for (const Register *reg : regs) {
    code_t reg_write_name = get_unique_name("vector_reg_entry_" + std::to_string(i));
    klee::ref<klee::Expr> reg_write_expr = solver_toolbox.exprBuilder->Extract(write_value, offset, reg->value_size);
    var_t reg_write_var(reg_write_name, reg_write_expr);
    write_vars.push_back(reg_write_var);

    code_t action_name = build_register_action_name(ep_node, reg, RegisterActionType::WRITE);
    ingress << transpile_register_write_action_decl(ingress.lvl, reg, action_name, reg_write_var);
    ingress << "\n";

    ingress_apply.indent();
    ingress_apply << reg_write_var.name << " = " << transpiler.transpile(reg_write_expr) << ";\n";

    ingress_apply.indent();
    ingress_apply << action_name;
    ingress_apply << ".execute(" << transpiler.transpile(index) << ");\n";

    i++;
    offset += reg->value_size;
  }

  for (const var_t &write_var : write_vars) {
    var_stacks.back().push_back(write_var);
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::FCFSCachedTableRead *node) {
  // coder_t &ingress = get(MARKER_INGRESS_CONTROL);
  // coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  // DS_ID cached_table_id = node->get_cached_table_id();
  // klee::ref<klee::Expr> key = node->get_key();
  // klee::ref<klee::Expr> value = node->get_value();
  // const symbol_t &map_has_this_key = node->get_map_has_this_key();

  // const DS *ds = get_tofino_ds(ep, cached_table_id);
  // const FCFSCachedTable *fcfs_cached_table = dynamic_cast<const FCFSCachedTable *>(ds);

  // transpile_fcfs_cached_table(ingress, fcfs_cached_table, key, value);
  // dbg();

  panic("TODO: FCFSCachedTableRead");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::FCFSCachedTableReadOrWrite *node) {
  panic("TODO: FCFSCachedTableReadOrWrite");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::FCFSCachedTableWrite *node) {
  panic("TODO: FCFSCachedTableWrite");
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::FCFSCachedTableDelete *node) {
  panic("TODO: FCFSCachedTableDelete");
  return EPVisitor::Action::doChildren;
}

code_t EPSynthesizer::get_unique_name(const code_t &prefix) {
  if (var_prefix_usage.find(prefix) == var_prefix_usage.end()) {
    var_prefix_usage[prefix] = 0;
  }

  int &counter = var_prefix_usage[prefix];

  coder_t coder;
  coder << prefix << "_" << counter;

  counter++;

  return coder.dump();
}

void EPSynthesizer::dbg_vars() const {
  std::cerr << "================= Vars ================= \n";
  for (const vars_t &vars : var_stacks) {
    std::cerr << "------------------------------------------\n";
    for (const var_t &var : vars) {
      std::cerr << var.name << ": ";
      std::cerr << expr_to_string(var.expr, false) << "\n";
    }
  }
  std::cerr << "======================================== \n";
}

} // namespace tofino
} // namespace synapse