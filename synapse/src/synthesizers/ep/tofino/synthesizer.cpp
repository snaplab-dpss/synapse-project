#include "synthesizer.hpp"
#include "transpiler.hpp"
#include "../../../targets/module.hpp"

namespace synapse {
namespace tofino {
namespace {

constexpr const char *const MARKER_CPU_HEADER                   = "CPU_HEADER";
constexpr const char *const MARKER_RECIRC_HEADER                = "RECIRCULATION_HEADER";
constexpr const char *const MARKER_CUSTOM_HEADERS               = "CUSTOM_HEADERS";
constexpr const char *const MARKER_INGRESS_HEADERS              = "INGRESS_HEADERS";
constexpr const char *const MARKER_INGRESS_METADATA             = "INGRESS_METADATA";
constexpr const char *const MARKER_INGRESS_PARSER               = "INGRESS_PARSER";
constexpr const char *const MARKER_INGRESS_CONTROL              = "INGRESS_CONTROL";
constexpr const char *const MARKER_INGRESS_CONTROL_APPLY        = "INGRESS_CONTROL_APPLY";
constexpr const char *const MARKER_INGRESS_CONTROL_APPLY_RECIRC = "INGRESS_CONTROL_APPLY_RECIRC";
constexpr const char *const MARKER_INGRESS_DEPARSER             = "INGRESS_DEPARSER";
constexpr const char *const MARKER_EGRESS_HEADERS               = "EGRESS_HEADERS";
constexpr const char *const MARKER_EGRESS_METADATA              = "EGRESS_METADATA";
constexpr const char *const TEMPLATE_FILENAME                   = "tofino.template.p4";

template <class T> const T *get_tofino_ds(const EP *ep, DS_ID id) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  const DS *ds                    = tofino_ctx->get_ds_from_id(id);
  assert(ds && "DS not found");
  return dynamic_cast<const T *>(ds);
}

const Parser &get_tofino_parser(const EP *ep) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  const TNA &tna                  = tofino_ctx->get_tna();
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

code_t EPSynthesizer::var_t::get_type() const { return force_bool ? "bool" : Transpiler::type_from_expr(expr); }

bool EPSynthesizer::var_t::is_bool() const { return force_bool || is_conditional(expr); }

void EPSynthesizer::var_t::declare(coder_t &coder, std::optional<code_t> assignment) const {
  coder.indent();
  coder << get_type();
  coder << " ";
  coder << get_stem();
  if (assignment.has_value()) {
    coder << " = ";
    coder << *assignment;
  }
  coder << ";\n";
}

EPSynthesizer::var_t EPSynthesizer::var_t::get_slice(bits_t offset, bits_t size) const {
  assert(offset + size <= expr->getWidth() && "Invalid slice");

  const bits_t lo                  = offset;
  const bits_t hi                  = offset + size - 1;
  const code_t slice_name          = name + "[" + std::to_string(hi) + ":" + std::to_string(lo) + "]";
  klee::ref<klee::Expr> slice_expr = solver_toolbox.exprBuilder->Extract(expr, offset, size);

  return var_t(slice_name, slice_expr, force_bool);
}

code_t EPSynthesizer::var_t::get_stem() const {
  size_t pos = name.find_last_of('.');
  if (pos == std::string::npos) {
    return name;
  }
  return name.substr(pos + 1);
}

void EPSynthesizer::Stack::push(const var_t &var) {
  if (names.find(var.name) == names.end()) {
    vars.push_back(var);
    names.insert(var.name);
  }
}

void EPSynthesizer::Stack::push(const Stack &stack) {
  for (const var_t &var : stack.vars) {
    push(var);
  }
}

void EPSynthesizer::Stack::clear() {
  vars.clear();
  names.clear();
}

std::optional<EPSynthesizer::var_t> EPSynthesizer::Stack::get_exact(klee::ref<klee::Expr> expr) const {
  for (auto var_it = vars.rbegin(); var_it != vars.rend(); var_it++) {
    const var_t &var = *var_it;
    if (solver_toolbox.are_exprs_always_equal(var.expr, expr)) {
      return var;
    }
  }

  return std::nullopt;
}

std::optional<EPSynthesizer::var_t> EPSynthesizer::Stack::get(klee::ref<klee::Expr> expr) const {
  if (std::optional<var_t> var = get_exact(expr)) {
    return var;
  }

  for (auto var_it = vars.rbegin(); var_it != vars.rend(); var_it++) {
    const var_t &var = *var_it;

    bits_t expr_size = expr->getWidth();
    bits_t var_size  = var.expr->getWidth();

    if (expr_size > var_size) {
      continue;
    }

    for (bits_t offset = 0; offset + expr_size <= var_size; offset += 8) {
      klee::ref<klee::Expr> var_slice = solver_toolbox.exprBuilder->Extract(var.expr, offset, expr_size);

      if (solver_toolbox.are_exprs_always_equal(var_slice, expr)) {
        return var.get_slice(offset, expr_size);
      }
    }
  }
  return std::nullopt;
}

const std::vector<EPSynthesizer::var_t> &EPSynthesizer::Stack::get_all() const { return vars; }

void EPSynthesizer::Stacks::push() { stacks.emplace_back(); }

void EPSynthesizer::Stacks::pop() { stacks.pop_back(); }

void EPSynthesizer::Stacks::insert_front(const var_t &var) { stacks.front().push(var); }
void EPSynthesizer::Stacks::insert_front(const Stack &stack) { stacks.front().push(stack); }

void EPSynthesizer::Stacks::insert_back(const var_t &var) { stacks.back().push(var); }
void EPSynthesizer::Stacks::insert_back(const Stack &stack) { stacks.back().push(stack); }

EPSynthesizer::Stack EPSynthesizer::Stacks::squash() const {
  Stack squashed;
  for (const Stack &stack : stacks) {
    squashed.push(stack);
  }
  return squashed;
}

std::optional<EPSynthesizer::var_t> EPSynthesizer::Stacks::get(klee::ref<klee::Expr> expr) const {
  for (auto stack_it = stacks.rbegin(); stack_it != stacks.rend(); stack_it++) {
    if (std::optional<var_t> var = stack_it->get_exact(expr)) {
      return var;
    }
  }

  for (auto stack_it = stacks.rbegin(); stack_it != stacks.rend(); stack_it++) {
    if (std::optional<var_t> var = stack_it->get(expr)) {
      return var;
    }
  }

  return std::nullopt;
}

void EPSynthesizer::Stacks::clear() { stacks.clear(); }

const std::vector<EPSynthesizer::Stack> &EPSynthesizer::Stacks::get_all() const { return stacks; }

EPSynthesizer::EPSynthesizer(std::ostream &_out, const BDD *bdd)
    : Synthesizer(TEMPLATE_FILENAME,
                  {
                      {MARKER_CPU_HEADER, 1},
                      {MARKER_RECIRC_HEADER, 1},
                      {MARKER_CUSTOM_HEADERS, 0},
                      {MARKER_INGRESS_HEADERS, 1},
                      {MARKER_INGRESS_METADATA, 1},
                      {MARKER_INGRESS_PARSER, 1},
                      {MARKER_INGRESS_CONTROL, 1},
                      {MARKER_INGRESS_CONTROL_APPLY, 3},
                      {MARKER_INGRESS_CONTROL_APPLY_RECIRC, 3},
                      {MARKER_INGRESS_DEPARSER, 2},
                      {MARKER_EGRESS_HEADERS, 1},
                      {MARKER_EGRESS_METADATA, 1},
                  },
                  _out),
      transpiler(this) {
  symbol_t device = bdd->get_device();
  symbol_t time   = bdd->get_time();

  alloc_var("meta.port", solver_toolbox.exprBuilder->Extract(device.expr, 0, 16), GLOBAL | EXACT_NAME);
  alloc_var("meta.time", time.expr, GLOBAL | EXACT_NAME);
}

coder_t &EPSynthesizer::get(const std::string &marker) {
  if (marker == MARKER_INGRESS_CONTROL_APPLY && active_recirc_code_path) {
    return Synthesizer::get(MARKER_INGRESS_CONTROL_APPLY_RECIRC);
  }
  return Synthesizer::get(marker);
}

void EPSynthesizer::visit(const EP *ep) {
  EPVisitor::visit(ep);

  // Transpile the parser after the whole EP has been visited so we have all the
  // headers available.
  transpile_parser(get_tofino_parser(ep));

  coder_t &cpu_hdr = get(MARKER_CPU_HEADER);
  for (const var_t &var : cpu_hdr_vars.get_all()) {
    bits_t pad = var.is_bool() ? 7 : (8 - var.expr->getWidth()) % 8;

    if (pad > 0) {
      cpu_hdr.indent();
      cpu_hdr << "@padding bit<";
      cpu_hdr << pad;
      cpu_hdr << "> pad_";
      cpu_hdr << var.get_stem();
      cpu_hdr << ";\n";
    }

    var.declare(cpu_hdr);
  }

  coder_t &recirc_hdr = get(MARKER_RECIRC_HEADER);
  for (const var_t &var : recirc_hdr_vars.get_all()) {
    bits_t pad = var.is_bool() ? 7 : (8 - var.expr->getWidth()) % 8;

    if (pad > 0) {
      recirc_hdr.indent();
      recirc_hdr << "@padding bit<";
      recirc_hdr << pad;
      recirc_hdr << "> pad_";
      recirc_hdr << var.get_stem();
      recirc_hdr << ";\n";
    }

    var.declare(recirc_hdr);
  }

  Synthesizer::dump();
}

void EPSynthesizer::visit(const EP *ep, const EPNode *ep_node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);
  ingress_apply.indent();
  ingress_apply << "// EP node  " << ep_node->get_id() << "\n";
  ingress_apply.indent();
  ingress_apply << "// BDD node " << ep_node->get_module()->get_node()->dump(true) << "\n";
  EPVisitor::visit(ep, ep_node);
}

void EPSynthesizer::transpile_parser(const Parser &parser) {
  coder_t &ingress_parser = get(MARKER_INGRESS_PARSER);

  std::vector<const ParserState *> states{parser.get_initial_state()};
  bool state_init = true;

  while (!states.empty()) {
    const ParserState *state = states.front();
    states.erase(states.begin());

    ingress_vars.push();
    hdrs_stacks.push();

    for (node_id_t id : state->ids) {
      if (parser_vars.find(id) != parser_vars.end()) {
        const Stack &vars = parser_vars.at(id);
        ingress_vars.insert_back(vars);
      }

      if (parser_hdrs.find(id) != parser_hdrs.end()) {
        const Stack &vars = parser_hdrs.at(id);
        hdrs_stacks.insert_back(vars);
      }
    }

    switch (state->type) {
    case ParserStateType::EXTRACT: {
      const ParserStateExtract *extract = dynamic_cast<const ParserStateExtract *>(state);
      const code_t state_name           = get_parser_state_name(state, state_init);

      std::optional<var_t> hdr_var = parser_hdrs.at(*extract->ids.begin()).get(extract->hdr);
      assert(hdr_var && "Header not found");

      assert(extract->next && "Next state not found");
      const code_t next_state = get_parser_state_name(extract->next, false);

      ingress_parser.indent();
      ingress_parser << "state " << state_name << " {\n";

      ingress_parser.inc();
      ingress_parser.indent();
      ingress_parser << "pkt.extract(" << hdr_var->name << ");\n";

      ingress_parser.indent();
      ingress_parser << "transition " << next_state << ";\n";

      ingress_parser.dec();
      ingress_parser.indent();
      ingress_parser << "}\n";

      states.push_back(extract->next);
    } break;
    case ParserStateType::SELECT: {
      const ParserStateSelect *select = dynamic_cast<const ParserStateSelect *>(state);
      const code_t state_name         = get_parser_state_name(state, state_init);

      ingress_parser.indent();
      ingress_parser << "state " << state_name << " {\n";

      ingress_parser.inc();
      ingress_parser.indent();
      ingress_parser << "transition select (" << transpiler.transpile(select->field) << ") {\n";

      ingress_parser.inc();

      const code_t next_true  = get_parser_state_name(select->on_true, false);
      const code_t next_false = get_parser_state_name(select->on_false, false);

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
      const code_t state_name               = get_parser_state_name(state, state_init);

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

    ingress_vars.pop();
    hdrs_stacks.pop();

    state_init = false;
  }
}

EPSynthesizer::var_t EPSynthesizer::alloc_var(const code_t &proposed_name, klee::ref<klee::Expr> expr,
                                              alloc_opt_t option) {
  assert(option & (LOCAL | GLOBAL) && "Neither LOCAL nor GLOBAL specified");

  const code_t name = (option & EXACT_NAME) ? proposed_name : create_unique_name(proposed_name);
  const var_t var(name, expr, option & FORCE_BOOL, option & (HEADER | HEADER_FIELD));

  if (option & HEADER) {
    hdrs_stacks.insert_back(var);
  } else {
    if (option & LOCAL) {
      ingress_vars.insert_back(var);
    } else {
      ingress_vars.insert_front(var);
    }
  }

  return var;
}

code_path_t EPSynthesizer::alloc_recirc_coder() {
  size_t size = recirc_coders.size();
  recirc_coders.emplace_back();
  return size;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::SendToController *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);
  const Symbols &symbols = node->get_symbols();

  for (const symbol_t &symbol : symbols.get()) {
    std::optional<var_t> var = ingress_vars.get(symbol.expr);

    if (!var) {
      // This can happen when the symbol is not used and synapse optimized it away.
      continue;
    }

    var_t cpu_var = *var;
    cpu_var.name  = "hdr.cpu." + var->get_stem();
    cpu_hdr_vars.push(cpu_var);

    ingress_apply.indent();
    ingress_apply << cpu_var.name;
    ingress_apply << " = ";
    ingress_apply << var->name;
    ingress_apply << ";\n";
  }

  ingress_apply.indent();
  ingress_apply << "send_to_controller(";
  ingress_apply << ep_node->get_id();
  ingress_apply << ");\n";

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::Recirculate *node) {
  assert(ep_node->get_children().size() == 1);
  const EPNode *next = ep_node->get_children()[0];

  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);
  coder_t &recirc        = get(MARKER_INGRESS_CONTROL_APPLY_RECIRC);

  // 1. Allocate a new recirculation code path
  code_path_t code_path = alloc_recirc_coder();

  // 2. Build the recirculation header and populate it with the current stack
  ingress_apply.indent();
  ingress_apply << "hdr.recirc.setValid();\n";
  ingress_apply.indent();
  ingress_apply << "hdr.recirc.code_path = " << code_path << ";\n";

  Stacks stack_backup = ingress_vars;

  std::vector<var_t> recirc_vars;
  for (const symbol_t &symbol : node->get_symbols().get()) {
    std::optional<var_t> var = ingress_vars.get(symbol.expr);

    if (!var) {
      // This can happen when the symbol is not used and synapse optimized it away.
      continue;
    }

    var_t recirc_var = *var;
    recirc_var.name  = "hdr.recirc." + var->get_stem();

    recirc_hdr_vars.push(*var);
    recirc_vars.push_back(recirc_var);

    ingress_apply.indent();
    ingress_apply << recirc_var.name;
    ingress_apply << " = ";
    ingress_apply << var->name;
    ingress_apply << ";\n";
  }

  for (const var_t &var : ingress_vars.squash().get_all()) {
    if (var.is_header_field) {
      recirc_vars.push_back(var);
    }
  }

  // 3. Forward to recirculation port
  int port = node->get_recirc_port();
  ingress_apply.indent();
  ingress_apply << "fwd(" << port << ");\n";

  // 4. Replace the ingress apply coder with the recirc coder
  active_recirc_code_path = code_path;

  // 5. Clear the stack, rebuild it with hdr.recirc fields, and setup the recirculation code block
  // TODO: Both the port and time information should also be migrated, as they are only parsed during the normal control
  // operation (not recirc or cpu).

  ingress_vars.clear();
  ingress_vars.push();
  for (const var_t &var : recirc_vars) {
    ingress_vars.insert_back(var);
  }

  recirc.indent();
  recirc << "if (hdr.recirc.code_path == " << code_path << ") {\n";
  recirc.inc();

  ingress_vars.push();
  visit(ep, next);
  ingress_vars.pop();
  recirc.dec();

  recirc.indent();
  recirc << "}\n";

  // 6. Revert the state back to before the recirculation was made
  active_recirc_code_path.reset();
  ingress_vars = stack_backup;

  return EPVisitor::Action::skipChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::Ignore *node) {
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::If *node) {
  coder_t &ingress = get(MARKER_INGRESS_CONTROL_APPLY);

  const std::vector<klee::ref<klee::Expr>> &conditions = node->get_conditions();
  const std::vector<EPNode *> &children                = ep_node->get_children();
  assert(children.size() == 2 && "If node must have 2 children");

  const EPNode *then_node = children[0];
  const EPNode *else_node = children[1];

  if (conditions.size() == 1) {
    const klee::ref<klee::Expr> condition = conditions[0];

    ingress.indent();
    ingress << "if (";
    ingress << transpiler.transpile(condition);
    ingress << ") {\n";

    ingress.inc();
    ingress_vars.push();
    visit(ep, then_node);
    ingress_vars.pop();
    ingress.dec();

    ingress.indent();
    ingress.stream << "} else {\n";

    ingress.inc();
    ingress_vars.push();
    visit(ep, else_node);
    ingress_vars.pop();
    ingress.dec();

    ingress.indent();
    ingress << "}\n";

    return EPVisitor::Action::skipChildren;
  }

  const var_t cond_var = alloc_var("cond", solver_toolbox.exprBuilder->True(), LOCAL | FORCE_BOOL);
  cond_var.declare(ingress, "false");

  for (klee::ref<klee::Expr> condition : conditions) {
    ingress.indent();
    ingress << "if (";
    ingress << transpiler.transpile(condition);
    ingress << ") {\n";

    ingress.inc();
  }

  ingress.indent();
  ingress << cond_var.name << " = true;\n";

  for (size_t i = 0; i < conditions.size(); i++) {
    ingress.dec();
    ingress.indent();
    ingress << "}\n";
  }

  ingress.indent();
  ingress << "if (";
  ingress << cond_var.name;
  panic("TODO: condition");
  ingress << ") {\n";

  ingress.inc();
  ingress_vars.push();
  visit(ep, then_node);
  ingress_vars.pop();
  ingress.dec();

  ingress.indent();
  ingress << "} else {\n";

  ingress.inc();
  ingress_vars.push();
  visit(ep, else_node);
  ingress_vars.pop();
  ingress.dec();

  ingress.indent();
  ingress << "}\n";

  return EPVisitor::Action::skipChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::ParserCondition *node) {
  parser_vars[node->get_node()->get_id()] = ingress_vars.squash();
  parser_hdrs[node->get_node()->get_id()] = hdrs_stacks.squash();
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::Then *node) {
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::Else *node) {
  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::Forward *node) {
  int dst_device   = node->get_dst_device();
  coder_t &ingress = get(MARKER_INGRESS_CONTROL_APPLY);

  ingress.indent();
  ingress << "fwd(" << dst_device << ");\n";

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
  const klee::ref<klee::Expr> expr = node->get_hdr();

  // On header valid only.
  ingress_vars.push();
  hdrs_stacks.push();

  const code_t hdr_name = create_unique_name("hdr");
  const var_t hdr       = alloc_var("hdr." + hdr_name, expr, LOCAL | EXACT_NAME | HEADER);
  const var_t hdr_data  = alloc_var("hdr." + hdr_name + ".data", expr, LOCAL | EXACT_NAME | HEADER_FIELD);

  coder_t &custom_hdrs = get(MARKER_CUSTOM_HEADERS);
  custom_hdrs.indent();
  custom_hdrs << "header " << hdr_name << "_h {\n";

  custom_hdrs.inc();
  custom_hdrs.indent();
  custom_hdrs << Transpiler::type_from_expr(expr) << " " << hdr_data.get_stem() << ";\n";

  custom_hdrs.dec();
  custom_hdrs.indent();
  custom_hdrs << "}\n";

  coder_t &ingress_hdrs = get(MARKER_INGRESS_HEADERS);
  ingress_hdrs.indent();
  ingress_hdrs << hdr_name << "_h " << hdr_name << ";\n";

  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);
  ingress_apply.indent();
  ingress_apply << "if(" << hdr.name << ".isValid()) {\n";

  ingress_apply.inc();

  parser_vars[node->get_node()->get_id()] = ingress_vars.squash();
  parser_hdrs[node->get_node()->get_id()] = hdrs_stacks.squash();

  assert(ep_node->get_children().size() == 1 && "ParserExtraction must have 1 child");
  visit(ep, ep_node->get_children()[0]);

  ingress_apply.dec();

  ingress_apply.indent();
  ingress_apply << "}\n";

  ingress_vars.pop();
  hdrs_stacks.pop();

  return EPVisitor::Action::skipChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::ModifyHeader *node) {
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  klee::ref<klee::Expr> hdr    = node->get_hdr();
  std::optional<var_t> hdr_var = ingress_vars.get(hdr);
  assert(hdr_var && "Header not found");

  const std::vector<expr_mod_t> &changes     = node->get_changes();
  const std::vector<expr_byte_swap_t> &swaps = node->get_swaps();

  for (const expr_byte_swap_t &byte_swap : swaps) {
    bits_t hi_offset = std::max(byte_swap.byte0, byte_swap.byte1) * 8;
    bits_t lo_offset = std::min(byte_swap.byte0, byte_swap.byte1) * 8;

    ingress_apply.indent();
    ingress_apply << "swap(";
    ingress_apply << hdr_var->name;
    ingress_apply << "[";
    ingress_apply << hi_offset + 7;
    ingress_apply << ":";
    ingress_apply << hi_offset;
    ingress_apply << "]";
    ingress_apply << ", ";
    ingress_apply << hdr_var->name;
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
    ingress_apply << hdr_var->name;
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
  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  DS_ID table_id                          = node->get_table_id();
  std::vector<klee::ref<klee::Expr>> keys = node->get_keys();
  std::optional<symbol_t> hit             = node->get_hit();

  const Table *table = get_tofino_ds<Table>(ep, table_id);

  std::vector<code_t> transpiled_keys;
  for (klee::ref<klee::Expr> key : keys) {
    transpiled_keys.push_back(transpiler.transpile(key));
  }

  if (declared_ds.find(table_id) == declared_ds.end()) {
    transpile_table_decl(ingress, table, keys, node->get_values());
    ingress << "\n";
    declared_ds.insert(table_id);
  }

  for (size_t i = 0; i < transpiled_keys.size(); i++) {
    klee::ref<klee::Expr> key    = keys[i];
    const code_t &transpiled_key = transpiled_keys[i];

    std::optional<var_t> key_var = ingress_vars.get(key);
    assert(key_var && "Key is not a variable");

    ingress_apply.indent();
    ingress_apply << key_var->name << " = " << transpiled_key << ";\n";
  }

  if (hit) {
    var_t hit_var = alloc_var("hit", hit->expr, LOCAL | FORCE_BOOL);
    hit_var.declare(ingress_apply, table_id + ".apply().hit");
  } else {
    ingress_apply.indent();
    ingress_apply << table_id << ".apply();\n";
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::VectorRegisterLookup *node) {
  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const std::unordered_set<DS_ID> &rids = node->get_rids();
  const klee::ref<klee::Expr> index     = node->get_index();
  const klee::ref<klee::Expr> value     = node->get_value();

  std::vector<const Register *> regs;
  std::for_each(rids.begin(), rids.end(), [ep, &regs](DS_ID rid) { regs.push_back(get_tofino_ds<Register>(ep, rid)); });
  std::sort(regs.begin(), regs.end(),
            [](const Register *r0, const Register *r1) { return naturalCompare(r0->id, r1->id); });

  for (const Register *reg : regs) {
    if (declared_ds.find(reg->id) == declared_ds.end()) {
      transpile_register_decl(ingress, reg, index, value);
      declared_ds.insert(reg->id);
    }
  }

  if (!regs.empty()) {
    ingress << "\n";
  }

  const std::string value_prefix_name = "vector_reg_value";
  const var_t value_var               = alloc_var(value_prefix_name, value, LOCAL);
  value_var.declare(ingress_apply, Transpiler::transpile_literal(0, value->getWidth()));

  int i         = 0;
  bits_t offset = 0;
  for (const Register *reg : regs) {
    const code_t action_name = build_register_action_name(ep_node, reg, RegisterActionType::READ);
    transpile_register_read_action_decl(ingress, reg, action_name);
    ingress << "\n";

    const klee::ref<klee::Expr> entry_expr = solver_toolbox.exprBuilder->Extract(value, offset, reg->value_size);
    std::optional<var_t> entry_var         = ingress_vars.get(entry_expr);
    assert(entry_var && "Register entry is not a variable");

    ingress_apply.indent();
    ingress_apply << entry_var->name << " = " << action_name + ".execute(" + transpiler.transpile(index) + ");\n";

    offset += reg->value_size;
    i++;
  }

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node, const tofino::VectorRegisterUpdate *node) {
  coder_t &ingress       = get(MARKER_INGRESS_CONTROL);
  coder_t &ingress_apply = get(MARKER_INGRESS_CONTROL_APPLY);

  const std::unordered_set<DS_ID> &rids = node->get_rids();
  klee::ref<klee::Expr> index           = node->get_index();
  klee::ref<klee::Expr> value           = node->get_read_value();
  klee::ref<klee::Expr> write_value     = node->get_write_value();

  std::vector<const Register *> regs;
  std::for_each(rids.begin(), rids.end(), [ep, &regs](DS_ID rid) { regs.push_back(get_tofino_ds<Register>(ep, rid)); });
  std::sort(regs.begin(), regs.end(),
            [](const Register *r0, const Register *r1) { return naturalCompare(r0->id, r1->id); });

  for (const Register *reg : regs) {
    if (declared_ds.find(reg->id) == declared_ds.end()) {
      transpile_register_decl(ingress, reg, index, value);
      declared_ds.insert(reg->id);
    }
  }

  if (!regs.empty()) {
    ingress << "\n";
  }

  int i         = 0;
  bits_t offset = 0;
  for (const Register *reg : regs) {
    const klee::ref<klee::Expr> reg_write_expr =
        solver_toolbox.exprBuilder->Extract(write_value, offset, reg->value_size);
    std::optional<var_t> reg_write_var = ingress_vars.get(reg_write_expr);
    assert(reg_write_var && "Register write value is not a variable");

    const code_t action_name = build_register_action_name(ep_node, reg, RegisterActionType::WRITE);
    transpile_register_write_action_decl(ingress, reg, action_name, *reg_write_var);
    ingress << "\n";

    ingress_apply.indent();
    ingress_apply << action_name;
    ingress_apply << ".execute(" << transpiler.transpile(index) << ");\n";

    i++;
    offset += reg->value_size;
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

EPVisitor::Action EPSynthesizer::visit(const EP *ep, const EPNode *ep_node,
                                       const tofino::FCFSCachedTableReadOrWrite *node) {
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

code_t EPSynthesizer::create_unique_name(const code_t &prefix) {
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
  std::cerr << "================= Stack ================= \n";
  for (const Stack &stack : ingress_vars.get_all()) {
    std::cerr << "------------------------------------------\n";
    for (const var_t &var : stack.get_all()) {
      std::cerr << var.name;
      if (var.is_bool()) {
        std::cerr << " (bool)";
      }
      if (var.is_header_field) {
        std::cerr << " (header)";
      }
      std::cerr << ": ";
      std::cerr << expr_to_string(var.expr, true);
      std::cerr << "\n";
    }
  }
  std::cerr << "======================================== \n";
}

} // namespace tofino
} // namespace synapse