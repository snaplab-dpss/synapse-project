#include <LibBDD/Visitors/BDDSynthesizer.h>
#include <LibBDD/BDD.h>
#include <LibCore/Debug.h>
#include <LibCore/Expr.h>
#include <LibCore/Solver.h>

namespace LibBDD {

namespace {
constexpr const char *const NF_TEMPLATE_FILENAME       = "nf.template.cpp";
constexpr const char *const PROFILER_TEMPLATE_FILENAME = "profiler.template.cpp";

constexpr const char *const MARKER_NF_STATE   = "NF_STATE";
constexpr const char *const MARKER_NF_INIT    = "NF_INIT";
constexpr const char *const MARKER_NF_PROCESS = "NF_PROCESS";

std::filesystem::path template_from_type(BDDSynthesizerTarget target) {
  std::filesystem::path template_file = std::filesystem::path(__FILE__).parent_path() / "Templates";

  switch (target) {
  case BDDSynthesizerTarget::NF:
    template_file /= NF_TEMPLATE_FILENAME;
    break;
  case BDDSynthesizerTarget::Profiler:
    template_file /= PROFILER_TEMPLATE_FILENAME;
    break;
  }

  return template_file;
}
} // namespace

#define TODO(expr)                                                                                                                                   \
  synthesizer->stack_dbg();                                                                                                                          \
  panic("TODO: %s\n", LibCore::expr_to_string(expr).c_str());

BDDSynthesizer::Transpiler::Transpiler(BDDSynthesizer *_synthesizer) : synthesizer(_synthesizer) {}

BDDSynthesizer::code_t BDDSynthesizer::Transpiler::transpile(klee::ref<klee::Expr> expr) {
  coders.emplace();
  coder_t &coder = coders.top();

  expr = LibCore::simplify(expr);

  if (LibCore::is_constant(expr)) {
    assert(expr->getWidth() <= 64 && "Unsupported constant width");
    u64 value = LibCore::solver_toolbox.value_from_expr(expr);
    coder << value;
    if (value > (1ull << 31)) {
      if (!LibCore::is_constant_signed(expr)) {
        coder << "U";
      }
      coder << "LL";
    }
  } else {
    visit(expr);

    // HACK: clear the visited map so we force the transpiler to revisit all
    // expressions.
    visited.clear();
  }

  code_t code = coder.dump();
  coders.pop();

  assert(!code.empty() && "Empty code");
  return code;
}

BDDSynthesizer::code_t BDDSynthesizer::Transpiler::type_from_size(bits_t size) {
  code_t type;

  switch (size) {
  case 1:
    type = "bool";
    break;
  case 8:
    type = "uint8_t";
    break;
  case 16:
    type = "uint16_t";
    break;
  case 32:
    type = "uint32_t";
    break;
  case 64:
    type = "uint64_t";
    break;
  default:
    panic("Unknown type (size=%u)", size);
  }

  return type;
}

BDDSynthesizer::code_t BDDSynthesizer::Transpiler::type_from_expr(klee::ref<klee::Expr> expr) { return type_from_size(expr->getWidth()); }

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitRead(const klee::ReadExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ReadExpr *>(&e);

  coder_t &coder = coders.top();

  BDDSynthesizer::var_t var = synthesizer->stack_get(expr);

  if (!var.addr.isNull()) {
    coder << "*";
  }

  coder << var.name;

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitNotOptimized(const klee::NotOptimizedExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::NotOptimizedExpr *>(&e);
  TODO(expr);
  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitSelect(const klee::SelectExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::SelectExpr *>(&e);
  TODO(expr);
  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitConcat(const klee::ConcatExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ConcatExpr *>(&e);

  coder_t &coder = coders.top();

  BDDSynthesizer::var_t var = synthesizer->stack_get(expr);

  if (!var.addr.isNull() && expr->getWidth() > 8) {
    coder << "*";

    if (expr->getWidth() <= 64) {
      coder << "(";
      coder << type_from_size(expr->getWidth());
      coder << "*)";
    }
  }

  coder << var.name;
  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitExtract(const klee::ExtractExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ExtractExpr *>(&e);

  bits_t width              = e.width;
  bits_t offset             = e.offset;
  klee::ref<klee::Expr> arg = e.expr;

  coder_t &coder = coders.top();

  if (width != arg->getWidth()) {
    coder << "(";
    coder << type_from_expr(arg);
    coder << ")";
    coder << "(";
  }

  coder << transpile(arg);

  if (offset > 0) {
    coder << ">>";
    coder << offset;
  }

  if (width != arg->getWidth()) {
    coder << ")";
  }

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitZExt(const klee::ZExtExpr &e) {
  klee::ref<klee::Expr> arg = e.getKid(0);

  coder_t &coder = coders.top();

  coder << "(";
  coder << type_from_expr(arg);
  coder << ")";
  coder << "(";
  coder << transpile(arg);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitSExt(const klee::SExtExpr &e) {
  // FIXME: We should handle sign extension properly.

  klee::ref<klee::Expr> arg = e.getKid(0);

  coder_t &coder = coders.top();

  coder << "(";
  coder << type_from_expr(arg);
  coder << ")";
  coder << "(";
  coder << transpile(arg);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitAdd(const klee::AddExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " + ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitSub(const klee::SubExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " - ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitMul(const klee::MulExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " * ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitUDiv(const klee::UDivExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " / ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitSDiv(const klee::SDivExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " / ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitURem(const klee::URemExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " % ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitSRem(const klee::SRemExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " % ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitNot(const klee::NotExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> arg = e.getKid(0);

  coder << "!(";
  coder << transpile(arg);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitAnd(const klee::AndExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " & ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitOr(const klee::OrExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " | ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitXor(const klee::XorExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::XorExpr *>(&e);
  TODO(expr);
  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitShl(const klee::ShlExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::ShlExpr *>(&e);
  TODO(expr);
  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitLShr(const klee::LShrExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::LShrExpr *>(&e);
  TODO(expr);
  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitAShr(const klee::AShrExpr &e) {
  klee::ref<klee::Expr> expr = const_cast<klee::AShrExpr *>(&e);
  TODO(expr);
  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitEq(const klee::EqExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  klee::ref<klee::Expr> var_expr;
  klee::ref<klee::Expr> const_expr;

  if (LibCore::is_constant(lhs)) {
    const_expr = lhs;
    var_expr   = rhs;
  } else {
    const_expr = rhs;
    var_expr   = lhs;
  }

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " == ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitNe(const klee::NeExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  klee::ref<klee::Expr> var_expr;
  klee::ref<klee::Expr> const_expr;

  if (LibCore::is_constant(lhs)) {
    const_expr = lhs;
    var_expr   = rhs;
  } else {
    const_expr = rhs;
    var_expr   = lhs;
  }

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " != ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitUlt(const klee::UltExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " < ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitUle(const klee::UleExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " <= ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitUgt(const klee::UgtExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " > ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitUge(const klee::UgeExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " >= ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitSlt(const klee::SltExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " < ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitSle(const klee::SleExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " <= ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitSgt(const klee::SgtExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " > ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

klee::ExprVisitor::Action BDDSynthesizer::Transpiler::visitSge(const klee::SgeExpr &e) {
  coder_t &coder = coders.top();

  klee::ref<klee::Expr> lhs = e.getKid(0);
  klee::ref<klee::Expr> rhs = e.getKid(1);

  coder << "(";
  coder << transpile(lhs);
  coder << ")";
  coder << " >= ";
  coder << "(";
  coder << transpile(rhs);
  coder << ")";

  return Action::skipChildren();
}

#define POPULATE_SYNTHESIZER(FNAME)                                                                                                                  \
  { #FNAME, std::bind(&BDDSynthesizer::FNAME, this, std::placeholders::_1, std::placeholders::_2) }

BDDSynthesizer::BDDSynthesizer(const BDD *_bdd, BDDSynthesizerTarget _target, std::filesystem::path _out_file)
    : Synthesizer(template_from_type(_target),
                  {
                      {MARKER_NF_STATE, 0},
                      {MARKER_NF_INIT, 0},
                      {MARKER_NF_PROCESS, 0},
                  },
                  _out_file),
      bdd(_bdd), target(_target), transpiler(this), function_synthesizers({
                                                        POPULATE_SYNTHESIZER(map_allocate),
                                                        POPULATE_SYNTHESIZER(vector_allocate),
                                                        POPULATE_SYNTHESIZER(dchain_allocate),
                                                        POPULATE_SYNTHESIZER(cms_allocate),
                                                        POPULATE_SYNTHESIZER(tb_allocate),
                                                        POPULATE_SYNTHESIZER(lpm_allocate),
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
                                                        POPULATE_SYNTHESIZER(lpm_lookup),
                                                        POPULATE_SYNTHESIZER(lpm_update),
                                                        POPULATE_SYNTHESIZER(lpm_from_file),
                                                    }) {}

void BDDSynthesizer::synthesize() {
  // Global state
  stack_push();

  init_pre_process();
  process();
  init_post_process();

  Synthesizer::dump();
}

void BDDSynthesizer::init_pre_process() {
  coder_t &coder = get(MARKER_NF_INIT);

  coder << "bool nf_init() {\n";
  coder.inc();

  for (const Call *call_node : bdd->get_init()) {
    success_condition_t cond = synthesize_function(coder, call_node);

    if (cond) {
      coder.indent();
      coder << "if (!";
      coder << cond->name;
      coder << ") {\n";

      coder.inc();
      coder.indent();
      coder << "return false;\n";

      coder.dec();
      coder.indent();
      coder << "}\n";
    }
  }

  if (target == BDDSynthesizerTarget::Profiler) {
    for (u16 device : bdd->get_devices()) {
      coder.indent();
      coder << "ports.push_back(" << device << ");\n";
    }
  }
}

void BDDSynthesizer::init_post_process() {
  coder_t &coder = get(MARKER_NF_INIT);

  if (target == BDDSynthesizerTarget::Profiler) {
    for (const auto &[node_id, map_addr] : nodes_to_map) {
      coder.indent();
      coder << "stats_per_map[" << transpiler.transpile(map_addr) << "]";
      coder << ".init(";
      coder << node_id;
      coder << ")";
      coder << ";\n";
    }

    for (node_id_t node_id : route_nodes) {
      coder.indent();
      coder << "forwarding_stats_per_route_op.insert({" << node_id << ", PortStats{}});\n";
    }

    for (node_id_t node_id : process_nodes) {
      coder.indent();
      coder << "node_pkt_counter.insert({" << node_id << ", 0});\n";
    }
  }

  coder.indent();
  coder << "return true;\n";

  coder.dec();
  coder << "}\n";
}

void BDDSynthesizer::process() {
  coder_t &coder = get(MARKER_NF_PROCESS);

  LibCore::symbol_t device = bdd->get_device();
  LibCore::symbol_t len    = bdd->get_packet_len();
  LibCore::symbol_t now    = bdd->get_time();

  var_t device_var = build_var("device", device.expr);
  var_t len_var    = build_var("packet_length", len.expr);
  var_t now_var    = build_var("now", now.expr);

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

  node->visit_nodes([this, &coder](const Node *future_node) {
    NodeVisitAction action = NodeVisitAction::Continue;

    process_nodes.insert(future_node->get_id());

    coder.indent();
    coder << "// Node ";
    coder << future_node->get_id();
    coder << "\n";

    if (target == BDDSynthesizerTarget::Profiler) {
      coder.indent();
      coder << "inc_path_counter(";
      coder << future_node->get_id();
      coder << ");\n";
    }

    switch (future_node->get_type()) {
    case NodeType::Branch: {
      const Branch *branch_node = dynamic_cast<const Branch *>(future_node);

      const Node *on_true  = branch_node->get_on_true();
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
      const Call *call_node = dynamic_cast<const Call *>(future_node);
      synthesize_function(coder, call_node);
    } break;
    case NodeType::Route: {
      const Route *route_node          = dynamic_cast<const Route *>(future_node);
      RouteOp op                       = route_node->get_operation();
      klee::ref<klee::Expr> dst_device = route_node->get_dst_device();

      route_nodes.insert(route_node->get_id());

      switch (op) {
      case RouteOp::Drop: {
        if (target == BDDSynthesizerTarget::Profiler) {
          coder.indent();
          coder << "forwarding_stats_per_route_op[" << future_node->get_id() << "].inc_drop();\n";
        }

        coder.indent();
        coder << "return DROP;\n";
      } break;
      case RouteOp::Broadcast: {
        if (target == BDDSynthesizerTarget::Profiler) {
          coder.indent();
          coder << "forwarding_stats_per_route_op[" << future_node->get_id() << "].inc_flood();\n";
        }

        coder.indent();
        coder << "return FLOOD;\n";
      } break;
      case RouteOp::Forward: {
        if (target == BDDSynthesizerTarget::Profiler) {
          coder.indent();
          coder << "forwarding_stats_per_route_op[" << future_node->get_id() << "].inc_fwd(";
          coder << transpiler.transpile(dst_device);
          coder << ");\n";
        }

        coder.indent();
        coder << "return " << transpiler.transpile(dst_device) << ";\n";

      } break;
      }
    } break;
    }

    return action;
  });
}

BDDSynthesizer::success_condition_t BDDSynthesizer::synthesize_function(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  if (this->function_synthesizers.find(call.function_name) == this->function_synthesizers.end()) {
    panic("No synthesizer found for function: %s\n", call.function_name.c_str());
  }

  return (this->function_synthesizers[call.function_name])(coder, call_node);
}

BDDSynthesizer::success_condition_t BDDSynthesizer::packet_borrow_next_chunk(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> length    = call.args.at("length").expr;
  klee::ref<klee::Expr> chunk     = call.args.at("chunk").out;
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

  return {};
}

BDDSynthesizer::success_condition_t BDDSynthesizer::packet_return_chunk(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> chunk_addr = call.args.at("the_chunk").expr;
  klee::ref<klee::Expr> chunk      = call.args.at("the_chunk").in;

  var_t hdr                                = stack_get(chunk_addr);
  std::vector<LibCore::expr_mod_t> changes = LibCore::build_expr_mods(hdr.expr, chunk);

  for (const LibCore::expr_mod_t &mod : changes) {
    std::vector<klee::ref<klee::Expr>> bytes = LibCore::bytes_in_expr(mod.expr);
    for (size_t i = 0; i < bytes.size(); i++) {
      coder.indent();
      coder << hdr.name;
      coder << "[";
      coder << (mod.offset / 8) + i;
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

  return {};
}

BDDSynthesizer::success_condition_t BDDSynthesizer::nf_set_rte_ipv4_udptcp_checksum(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> ip_header     = call.args.at("ip_header").expr;
  klee::ref<klee::Expr> tcpudp_header = call.args.at("l4_header").expr;
  LibCore::symbol_t checksum          = call_node->get_local_symbol("checksum");

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

  return {};
}

BDDSynthesizer::success_condition_t BDDSynthesizer::expire_items_single_map(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> chain  = call.args.at("chain").expr;
  klee::ref<klee::Expr> vector = call.args.at("vector").expr;
  klee::ref<klee::Expr> map    = call.args.at("map").expr;
  klee::ref<klee::Expr> time   = call.args.at("time").expr;

  LibCore::symbol_t number_of_freed_flows = call_node->get_local_symbol("number_of_freed_flows");

  var_t nfreed = build_var("freed_flows", number_of_freed_flows.expr);

  coder.indent();
  coder << "int " << nfreed.name << " = ";
  if (target == BDDSynthesizerTarget::Profiler) {
    coder << "profiler_expire_items_single_map(";
    coder << stack_get(chain).name << ", ";
    coder << stack_get(vector).name << ", ";
    coder << stack_get(map).name << ", ";
    coder << "now"; // FIXME: we should get this from the stack
    coder << ")";
    coder << ";\n";
  } else {
    coder << "expire_items_single_map(";
    coder << stack_get(chain).name << ", ";
    coder << stack_get(vector).name << ", ";
    coder << stack_get(map).name << ", ";
    coder << transpiler.transpile(time);
    coder << ")";
    coder << ";\n";
  }

  stack_add(nfreed);

  return {};
}

BDDSynthesizer::success_condition_t BDDSynthesizer::expire_items_single_map_iteratively(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> vector  = call.args.at("vector").expr;
  klee::ref<klee::Expr> map     = call.args.at("map").expr;
  klee::ref<klee::Expr> start   = call.args.at("start").expr;
  klee::ref<klee::Expr> n_elems = call.args.at("n_elems").expr;

  LibCore::symbol_t number_of_freed_flows = call_node->get_local_symbol("number_of_freed_flows");

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

  return {};
}

BDDSynthesizer::success_condition_t BDDSynthesizer::map_allocate(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> map_out  = call.args.at("map_out").out;
  LibCore::symbol_t success      = call_node->get_local_symbol("map_allocation_succeeded");

  var_t map_out_var = build_var("map", map_out);
  var_t success_var = build_var("map_allocation_succeeded", success.expr);

  coder_t &coder_nf_state = get(MARKER_NF_STATE);
  coder_nf_state.indent();
  coder_nf_state << "struct Map *";
  coder_nf_state << map_out_var.name;
  coder_nf_state << ";\n";

  coder.indent();
  coder << "int " << success_var.name << " = ";
  coder << "map_allocate(";
  coder << transpiler.transpile(capacity) << ", ";
  coder << transpiler.transpile(key_size) << ", ";
  coder << "&" << map_out_var.name;
  coder << ");\n";

  stack_add(map_out_var);

  return success_var;
}

BDDSynthesizer::success_condition_t BDDSynthesizer::map_get(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> map_addr       = call.args.at("map").expr;
  klee::ref<klee::Expr> key_addr       = call.args.at("key").expr;
  klee::ref<klee::Expr> key            = call.args.at("key").in;
  klee::ref<klee::Expr> value_out_addr = call.args.at("value_out").expr;
  klee::ref<klee::Expr> value_out      = call.args.at("value_out").out;

  LibCore::symbol_t map_has_this_key = call_node->get_local_symbol("map_has_this_key");

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

  if (target == BDDSynthesizerTarget::Profiler) {
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

  return r;
}

BDDSynthesizer::success_condition_t BDDSynthesizer::map_put(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> map_addr = call.args.at("map").expr;
  klee::ref<klee::Expr> key_addr = call.args.at("key").expr;
  klee::ref<klee::Expr> key      = call.args.at("key").in;
  klee::ref<klee::Expr> value    = call.args.at("value").expr;

  bool key_in_stack;
  var_t k = build_var_ptr("key", key_addr, key, coder, key_in_stack);

  coder.indent();
  coder << "map_put(";
  coder << stack_get(map_addr).name << ", ";
  coder << k.name << ", ";
  coder << transpiler.transpile(value);
  coder << ")";
  coder << ";\n";

  if (target == BDDSynthesizerTarget::Profiler) {
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

  return {};
}

BDDSynthesizer::success_condition_t BDDSynthesizer::map_erase(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> map_addr = call.args.at("map").expr;
  klee::ref<klee::Expr> key_addr = call.args.at("key").expr;
  klee::ref<klee::Expr> key      = call.args.at("key").in;
  klee::ref<klee::Expr> trash    = call.args.at("trash").expr;

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

  if (target == BDDSynthesizerTarget::Profiler) {
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

  return {};
}

BDDSynthesizer::success_condition_t BDDSynthesizer::map_size(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> map_addr  = call.args.at("map").expr;
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

  return {};
}

BDDSynthesizer::success_condition_t BDDSynthesizer::vector_allocate(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> elem_size  = call.args.at("elem_size").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> vector_out = call.args.at("vector_out").out;
  LibCore::symbol_t success        = call_node->get_local_symbol("vector_alloc_success");

  var_t vector_out_var = build_var("vector", vector_out);
  var_t success_var    = build_var("vector_alloc_success", success.expr);

  coder_t &coder_nf_state = get(MARKER_NF_STATE);
  coder_nf_state.indent();
  coder_nf_state << "struct Vector *";
  coder_nf_state << vector_out_var.name;
  coder_nf_state << ";\n";

  coder.indent();
  coder << "int " << success_var.name << " = ";
  coder << "vector_allocate(";
  coder << transpiler.transpile(elem_size) << ", ";
  coder << transpiler.transpile(capacity) << ", ";
  coder << "&" << vector_out_var.name;
  coder << ");\n";

  stack_add(vector_out_var);

  return success_var;
}

BDDSynthesizer::success_condition_t BDDSynthesizer::vector_borrow(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> vector_addr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index       = call.args.at("index").expr;
  klee::ref<klee::Expr> value_addr  = call.args.at("val_out").out;
  klee::ref<klee::Expr> value       = call.extra_vars.at("borrowed_cell").second;

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

  return {};
}

BDDSynthesizer::success_condition_t BDDSynthesizer::vector_return(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> value      = call.args.at("value").in;
  klee::ref<klee::Expr> value_addr = call.args.at("value").expr;

  var_t v = stack_get(value_addr);

  if (LibCore::solver_toolbox.are_exprs_always_equal(v.expr, value)) {
    return {};
  }

  var_t new_v;
  if (value->getKind() != klee::Expr::Constant && stack_find_or_create_tmp_slice_var(value, coder, new_v)) {
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
    return {};
  }

  std::vector<LibCore::expr_mod_t> changes = LibCore::build_expr_mods(v.expr, value);
  for (const LibCore::expr_mod_t &mod : changes) {
    coder.indent();

    assert(mod.width <= 64 && "Vector element size is too large");

    if (mod.width == 8) {
      coder << v.name;
      coder << "[";
      coder << mod.offset / 8;
      coder << "]";
    } else {
      coder << "*(" << Transpiler::type_from_size(mod.width) << "*)";
      coder << v.name;
    }

    coder << " = ";
    coder << transpiler.transpile(mod.expr);
    coder << ";\n";
  }

  return {};
}

BDDSynthesizer::success_condition_t BDDSynthesizer::vector_clear(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> vector_addr = call.args.at("vector").expr;

  coder.indent();
  coder << "vector_borrow(";
  coder << stack_get(vector_addr).name;
  coder << ")";
  coder << ";\n";

  return {};
}

BDDSynthesizer::success_condition_t BDDSynthesizer::vector_sample_lt(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> vector_addr    = call.args.at("vector").expr;
  klee::ref<klee::Expr> samples        = call.args.at("samples").expr;
  klee::ref<klee::Expr> threshold_addr = call.args.at("threshold").expr;
  klee::ref<klee::Expr> threshold      = call.args.at("threshold").in;
  klee::ref<klee::Expr> index_out_addr = call.args.at("index_out").expr;
  klee::ref<klee::Expr> index_out      = call.args.at("index_out").out;

  bool threshold_in_stack;
  var_t t = build_var_ptr("threshold", threshold_addr, threshold, coder, threshold_in_stack);

  LibCore::symbol_t found_sample = call_node->get_local_symbol("found_sample");

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

  return f;
}

BDDSynthesizer::success_condition_t BDDSynthesizer::dchain_allocate(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> index_range = call.args.at("index_range").expr;
  klee::ref<klee::Expr> chain_out   = call.args.at("chain_out").out;
  LibCore::symbol_t success         = call_node->get_local_symbol("is_dchain_allocated");

  var_t chain_out_var = build_var("dchain", chain_out);
  var_t success_var   = build_var("is_dchain_allocated", success.expr);

  coder_t &coder_nf_state = get(MARKER_NF_STATE);
  coder_nf_state.indent();
  coder_nf_state << "struct DoubleChain *";
  coder_nf_state << chain_out_var.name;
  coder_nf_state << ";\n";

  coder.indent();
  coder << "int " << success_var.name << " = ";
  coder << "dchain_allocate(";
  coder << transpiler.transpile(index_range) << ", ";
  coder << "&" << chain_out_var.name;
  coder << ");\n";

  stack_add(chain_out_var);

  return success_var;
}

BDDSynthesizer::success_condition_t BDDSynthesizer::dchain_allocate_new_index(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> dchain_addr = call.args.at("chain").expr;
  klee::ref<klee::Expr> time        = call.args.at("time").expr;
  klee::ref<klee::Expr> index_out   = call.args.at("index_out").out;

  LibCore::symbol_t not_out_of_space = call_node->get_local_symbol("not_out_of_space");

  var_t noos = build_var("not_out_of_space", not_out_of_space.expr);
  var_t i    = build_var("index", index_out);

  coder.indent();
  coder << "int " << i.name << ";\n";

  coder.indent();
  coder << "int " << noos.name << " = ";
  coder << "dchain_allocate_new_index(";
  coder << stack_get(dchain_addr).name << ", ";
  coder << "&" << i.name << ", ";
  coder << transpiler.transpile(time);
  coder << ")";
  coder << ";\n";

  stack_add(noos);
  stack_add(i);

  return noos;
}

BDDSynthesizer::success_condition_t BDDSynthesizer::dchain_rejuvenate_index(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> dchain_addr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index       = call.args.at("index").expr;
  klee::ref<klee::Expr> time        = call.args.at("time").expr;

  coder.indent();
  coder << "dchain_rejuvenate_index(";
  coder << stack_get(dchain_addr).name << ", ";
  coder << transpiler.transpile(index) << ", ";
  coder << transpiler.transpile(time);
  coder << ")";
  coder << ";\n";

  return {};
}

BDDSynthesizer::success_condition_t BDDSynthesizer::dchain_expire_one(coder_t &coder, const Call *call_node) {
  // const call_t &call = call_node->get_call();

  // coder.indent();
  // coder << "// dchain_expire_one";
  // coder << ";\n";

  panic("TODO: dchain_expire_one");
  return {};
}

BDDSynthesizer::success_condition_t BDDSynthesizer::dchain_is_index_allocated(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> dchain_addr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index       = call.args.at("index").expr;

  LibCore::symbol_t is_allocated = call_node->get_local_symbol("is_index_allocated");

  var_t ia = build_var("is_allocated", is_allocated.expr);

  coder.indent();
  coder << "int " << ia.name << " = ";
  coder << "dchain_is_index_allocated(";
  coder << stack_get(dchain_addr).name << ", ";
  coder << transpiler.transpile(index);
  coder << ")";
  coder << ";\n";

  stack_add(ia);

  return ia;
}

BDDSynthesizer::success_condition_t BDDSynthesizer::dchain_free_index(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> dchain_addr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index       = call.args.at("index").expr;

  coder.indent();
  coder << "dchain_free_index(";
  coder << stack_get(dchain_addr).name << ", ";
  coder << transpiler.transpile(index);
  coder << ")";
  coder << ";\n";

  return {};
}

BDDSynthesizer::success_condition_t BDDSynthesizer::cms_allocate(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> height           = call.args.at("height").expr;
  klee::ref<klee::Expr> width            = call.args.at("width").expr;
  klee::ref<klee::Expr> key_size         = call.args.at("key_size").expr;
  klee::ref<klee::Expr> cleanup_interval = call.args.at("cleanup_interval").expr;
  klee::ref<klee::Expr> cms_out          = call.args.at("cms_out").out;
  LibCore::symbol_t success              = call_node->get_local_symbol("cms_allocation_succeeded");

  var_t cms_out_var = build_var("cms", cms_out);
  var_t success_var = build_var("cms_allocation_succeeded", success.expr);

  coder_t &coder_nf_state = get(MARKER_NF_STATE);
  coder_nf_state.indent();
  coder_nf_state << "struct CMS *";
  coder_nf_state << cms_out_var.name;
  coder_nf_state << ";\n";

  coder.indent();
  coder << "int " << success_var.name << " = ";
  coder << "cms_allocate(";
  coder << transpiler.transpile(height) << ", ";
  coder << transpiler.transpile(width) << ", ";
  coder << transpiler.transpile(key_size) << ", ";
  coder << transpiler.transpile(cleanup_interval) << ", ";
  coder << "&" << cms_out_var.name;
  coder << ");\n";

  stack_add(cms_out_var);

  return success_var;
}

BDDSynthesizer::success_condition_t BDDSynthesizer::cms_increment(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> cms_addr = call.args.at("cms").expr;
  klee::ref<klee::Expr> key      = call.args.at("key").in;
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

  return {};
}

BDDSynthesizer::success_condition_t BDDSynthesizer::cms_count_min(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> cms_addr     = call.args.at("cms").expr;
  klee::ref<klee::Expr> key          = call.args.at("key").in;
  klee::ref<klee::Expr> key_addr     = call.args.at("key").expr;
  klee::ref<klee::Expr> min_estimate = call.ret;

  bool key_in_stack;
  var_t k = build_var_ptr("key", key_addr, key, coder, key_in_stack);

  var_t me = build_var("min_estimate", min_estimate);

  coder.indent();
  coder << "int " << me.name << " = ";
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

  return {};
}

BDDSynthesizer::success_condition_t BDDSynthesizer::cms_periodic_cleanup(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> cms_addr = call.args.at("cms").expr;
  klee::ref<klee::Expr> time     = call.args.at("time").expr;

  LibCore::symbol_t cleanup_success = call_node->get_local_symbol("cleanup_success");

  var_t cs = build_var("cleanup_success", cleanup_success.expr);

  coder.indent();
  coder << "int " << cs.name << " = ";
  coder << "cms_periodic_cleanup(";
  coder << stack_get(cms_addr).name << ", ";
  coder << transpiler.transpile(time);
  coder << ")";
  coder << ";\n";

  stack_add(cs);

  return cs;
}

BDDSynthesizer::success_condition_t BDDSynthesizer::tb_allocate(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> rate     = call.args.at("rate").expr;
  klee::ref<klee::Expr> burst    = call.args.at("burst").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> tb_out   = call.args.at("tb_out").out;
  LibCore::symbol_t success      = call_node->get_local_symbol("tb_allocation_succeeded");

  var_t tb_out_var  = build_var("tb", tb_out);
  var_t success_var = build_var("tb_allocation_succeeded", success.expr);

  coder_t &coder_nf_state = get(MARKER_NF_STATE);
  coder_nf_state.indent();
  coder_nf_state << "struct TokenBucket *";
  coder_nf_state << tb_out_var.name;
  coder_nf_state << ";\n";

  coder.indent();
  coder << "tb_allocate(";
  coder << transpiler.transpile(capacity) << ", ";
  coder << transpiler.transpile(rate) << "ull, ";
  coder << transpiler.transpile(burst) << "ull, ";
  coder << transpiler.transpile(key_size) << ", ";
  coder << "&" << tb_out_var.name;
  coder << ");\n";

  stack_add(tb_out_var);

  return success_var;
}

BDDSynthesizer::success_condition_t BDDSynthesizer::lpm_allocate(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> lpm_out = call.args.at("lpm_out").out;
  LibCore::symbol_t success     = call_node->get_local_symbol("lpm_alloc_success");

  var_t lpm_out_var = build_var("lpm", lpm_out);
  var_t success_var = build_var("lpm_alloc_success", success.expr);

  coder_t &coder_nf_state = get(MARKER_NF_STATE);
  coder_nf_state.indent();
  coder_nf_state << "struct LPM *";
  coder_nf_state << lpm_out_var.name;
  coder_nf_state << ";\n";

  coder.indent();
  coder << "int " << success_var.name << " = ";
  coder << "lpm_allocate(";
  coder << "&" << lpm_out_var.name;
  coder << ");\n";

  stack_add(lpm_out_var);

  return success_var;
}

BDDSynthesizer::success_condition_t BDDSynthesizer::tb_is_tracing(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> tb_addr    = call.args.at("tb").expr;
  klee::ref<klee::Expr> key_addr   = call.args.at("key").expr;
  klee::ref<klee::Expr> key        = call.args.at("key").in;
  klee::ref<klee::Expr> index_out  = call.args.at("index_out").out;
  klee::ref<klee::Expr> is_tracing = call.ret;

  bool key_in_stack;
  var_t k = build_var_ptr("key", key_addr, key, coder, key_in_stack);

  var_t it    = build_var("is_tracing", is_tracing);
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

  return it;
}

BDDSynthesizer::success_condition_t BDDSynthesizer::tb_trace(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> tb_addr             = call.args.at("tb").expr;
  klee::ref<klee::Expr> key_addr            = call.args.at("key").expr;
  klee::ref<klee::Expr> key                 = call.args.at("key").in;
  klee::ref<klee::Expr> pkt_len             = call.args.at("pkt_len").expr;
  klee::ref<klee::Expr> time                = call.args.at("time").expr;
  klee::ref<klee::Expr> index_out           = call.args.at("index_out").out;
  klee::ref<klee::Expr> successfuly_tracing = call.ret;

  bool key_in_stack;
  var_t k = build_var_ptr("key", key_addr, key, coder, key_in_stack);

  var_t st = build_var("successfuly_tracing", successfuly_tracing);
  var_t i  = build_var("index", index_out);

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

  return st;
}

BDDSynthesizer::success_condition_t BDDSynthesizer::tb_update_and_check(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> tb_addr = call.args.at("tb").expr;
  klee::ref<klee::Expr> index   = call.args.at("index").expr;
  klee::ref<klee::Expr> pkt_len = call.args.at("pkt_len").expr;
  klee::ref<klee::Expr> time    = call.args.at("time").expr;
  klee::ref<klee::Expr> pass    = call.ret;

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

  return p;
}

BDDSynthesizer::success_condition_t BDDSynthesizer::tb_expire(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> tb_addr = call.args.at("tb").expr;
  klee::ref<klee::Expr> time    = call.args.at("time").expr;

  coder.indent();
  coder << "tb_expire(";
  coder << stack_get(tb_addr).name << ", ";
  coder << transpiler.transpile(time);
  coder << ")";
  coder << ";\n";

  return {};
}

BDDSynthesizer::success_condition_t BDDSynthesizer::lpm_lookup(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> lpm_addr  = call.args.at("lpm").expr;
  klee::ref<klee::Expr> prefix    = call.args.at("prefix").expr;
  klee::ref<klee::Expr> value_out = call.args.at("value_out").out;

  LibCore::symbol_t lpm_lookup_match = call_node->get_local_symbol("lpm_lookup_match");

  var_t lookup_match_var = build_var("lpm_lookup_match", lpm_lookup_match.expr);
  var_t lpm_matching_dev = build_var("lpm_matching_dev", value_out);

  coder.indent();
  coder << "uint16_t " << lpm_matching_dev.name << ";\n";

  coder.indent();
  coder << "int " << lookup_match_var.name << " = ";
  coder << "lpm_lookup(";
  coder << stack_get(lpm_addr).name << ", ";
  coder << transpiler.transpile(prefix) << ", ";
  coder << "&" << lpm_matching_dev.name;
  coder << ")";
  coder << ";\n";

  stack_add(lookup_match_var);
  stack_add(lpm_matching_dev);

  return lookup_match_var;
}

BDDSynthesizer::success_condition_t BDDSynthesizer::lpm_update(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> lpm_addr  = call.args.at("lpm").expr;
  klee::ref<klee::Expr> prefix    = call.args.at("prefix").expr;
  klee::ref<klee::Expr> prefixlen = call.args.at("prefixlen").expr;
  klee::ref<klee::Expr> value     = call.args.at("value").expr;

  LibCore::symbol_t lpm_update_elem_result = call_node->get_local_symbol("lpm_update_elem_result");

  var_t update_result_var = build_var("lpm_update_elem_success", lpm_update_elem_result.expr);

  coder.indent();
  coder << "int " << update_result_var.name << " = ";
  coder << "lpm_update(";
  coder << stack_get(lpm_addr).name << ", ";
  coder << transpiler.transpile(prefix) << ", ";
  coder << transpiler.transpile(prefixlen) << ", ";
  coder << transpiler.transpile(value);
  coder << ")";
  coder << ";\n";

  stack_add(update_result_var);

  return update_result_var;
}

BDDSynthesizer::success_condition_t BDDSynthesizer::lpm_from_file(coder_t &coder, const Call *call_node) {
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> lpm_addr  = call.args.at("lpm").expr;
  klee::ref<klee::Expr> cfg_fname = call.args.at("cfg_fname").in;

  std::string cfg_fname_str = LibCore::expr_to_ascii(cfg_fname);

  coder.indent();
  coder << "lpm_from_file(";
  coder << stack_get(lpm_addr).name << ", ";
  coder << "\"" << cfg_fname_str << "\"";
  coder << ");\n";

  return {};
}

void BDDSynthesizer::stack_dbg() const {
  std::cerr << "================= Vars ================= \n";
  for (const stack_frame_t &frame : stack) {
    std::cerr << "------------------------------------------\n";
    for (const var_t &var : frame.vars) {
      std::cerr << "[";
      std::cerr << var.name;
      if (!var.addr.isNull()) {
        std::cerr << "@" << LibCore::expr_to_string(var.addr, true);
      }
      std::cerr << "]: ";
      std::cerr << LibCore::expr_to_string(var.expr, false) << "\n";
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
  panic("Variable not found in stack: %s\n", name.c_str());
}

BDDSynthesizer::var_t BDDSynthesizer::stack_get(klee::ref<klee::Expr> expr) {
  var_t var;
  if (stack_find(expr, var)) {
    return var;
  }

  stack_dbg();
  panic("Variable not found in stack: %s\n", LibCore::expr_to_string(expr).c_str());
}

BDDSynthesizer::code_t BDDSynthesizer::slice_var(const var_t &var, bits_t offset, bits_t size) const {
  assert(offset + size <= var.expr->getWidth() && "Out of bounds");

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
      if (LibCore::solver_toolbox.are_exprs_always_equal(v.expr, expr) || LibCore::solver_toolbox.are_exprs_always_equal(v.addr, expr)) {
        out_var = v;
        return true;
      }

      klee::Expr::Width expr_bits = expr->getWidth();
      klee::Expr::Width var_bits  = v.expr->getWidth();

      if (expr_bits > var_bits) {
        continue;
      }

      for (bits_t offset = 0; offset <= var_bits - expr_bits; offset += 8) {
        klee::ref<klee::Expr> var_slice = LibCore::solver_toolbox.exprBuilder->Extract(v.expr, offset, expr_bits);

        if (LibCore::solver_toolbox.are_exprs_always_equal(var_slice, expr)) {
          out_var      = v;
          out_var.name = slice_var(v, offset, expr_bits);
          out_var.expr = var_slice;
          return true;
        }
      }
    }
  }

  return false;
}

bool BDDSynthesizer::stack_find_or_create_tmp_slice_var(klee::ref<klee::Expr> expr, coder_t &coder, var_t &out_var) {
  for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
    stack_frame_t &frame = *it;

    for (const var_t &v : frame.vars) {
      if (LibCore::solver_toolbox.are_exprs_always_equal(v.expr, expr) || LibCore::solver_toolbox.are_exprs_always_equal(v.addr, expr)) {
        out_var = v;
        return true;
      }

      klee::Expr::Width expr_bits = expr->getWidth();
      klee::Expr::Width var_bits  = v.expr->getWidth();

      if (expr_bits > var_bits) {
        continue;
      }

      for (bits_t offset = 0; offset <= var_bits - expr_bits; offset += 8) {
        klee::ref<klee::Expr> var_slice = LibCore::solver_toolbox.exprBuilder->Extract(v.expr, offset, expr_bits);

        if (LibCore::solver_toolbox.are_exprs_always_equal(var_slice, expr)) {
          out_var = build_var(v.name + "_slice", var_slice);

          if (expr_bits <= 64) {
            coder.indent();
            coder << transpiler.type_from_expr(out_var.expr) << " " << out_var.name;
            coder << " = ";
            if (!v.addr.isNull() && out_var.addr.isNull()) {
              coder << "*";
            }
            coder << slice_var(v, offset, expr_bits);
            coder << ";\n";
          } else {
            coder.indent();
            coder << "uint8_t " << out_var.name << "[" << expr_bits / 8 << "];\n";

            if (!v.addr.isNull()) {
              coder.indent();
              coder << "memcpy(";
              coder << "(void*)" << out_var.name << ", ";
              coder << "(void*)" << slice_var(v, offset, expr_bits);
              coder << ", ";
              coder << expr_bits / 8;
              coder << ");\n";
            } else {
              for (bytes_t b = 0; b < expr_bits / 8; b++) {
                klee::ref<klee::Expr> byte = LibCore::solver_toolbox.exprBuilder->Extract(v.expr, offset + b * 8, 8);
                coder.indent();
                coder << out_var.name << "[" << b << "] = ";
                coder << transpiler.transpile(byte);
                coder << ";\n";
              }
            }
          }

          stack_add(out_var);
          return true;
        }
      }
    }
  }

  return false;
}

void BDDSynthesizer::stack_add(const var_t &var) {
  if (var.expr.isNull()) {
    panic("Trying to add a variable with a null expression: %s\n", var.name.c_str());
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
        assert(old_expr->getWidth() == new_expr->getWidth() && "Width mismatch");
        v.expr = new_expr;
        return;
      }
    }
  }

  stack_dbg();
  panic("Variable not found in stack: %s\nExpr: %s\n", var.name.c_str(), LibCore::expr_to_string(new_expr).c_str());
}

BDDSynthesizer::var_t BDDSynthesizer::build_var(const std::string &name, klee::ref<klee::Expr> expr) { return build_var(name, expr, nullptr); }

BDDSynthesizer::var_t BDDSynthesizer::build_var(const std::string &name, klee::ref<klee::Expr> expr, klee::ref<klee::Expr> addr) {
  if (reserved_var_names.find(name) == reserved_var_names.end()) {
    reserved_var_names[name] = 1;
    return var_t(name, expr, addr);
  }

  reserved_var_names[name] += 1;
  return var_t(name + std::to_string(reserved_var_names[name]), expr, addr);
}

BDDSynthesizer::var_t BDDSynthesizer::build_var_ptr(const std::string &base_name, klee::ref<klee::Expr> addr, klee::ref<klee::Expr> value,
                                                    coder_t &coder, bool &found_in_stack) {
  bytes_t size = value->getWidth() / 8;

  var_t var;
  if (!(found_in_stack = stack_find(addr, var))) {
    var = build_var(base_name, value, addr);
    coder.indent();
    coder << "uint8_t " << var.name << "[" << size << "];\n";
  } else if (LibCore::solver_toolbox.are_exprs_always_equal(var.expr, value)) {
    return var;
  }

  var_t stack_value;
  if (stack_find_or_create_tmp_slice_var(value, coder, stack_value)) {
    const bits_t width = stack_value.expr->getWidth();
    if (width <= 64) {
      coder.indent();
      coder << "*(" << Transpiler::type_from_size(width) << "*)";
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
      klee::ref<klee::Expr> byte = LibCore::solver_toolbox.exprBuilder->Extract(var.expr, b * 8, 8);
      coder.indent();
      coder << var.name << "[" << b << "] = ";
      coder << transpiler.transpile(byte);
      coder << ";\n";
    }
  }

  return var;
}

} // namespace LibBDD