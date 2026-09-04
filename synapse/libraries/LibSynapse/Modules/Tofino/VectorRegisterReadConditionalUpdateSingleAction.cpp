#include <LibSynapse/Modules/Tofino/VectorRegisterReadConditionalUpdateSingleAction.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Branch;
using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;
using LibCore::is_condition_from_symbol;
using LibCore::is_increment_by_one;
using LibCore::simplify_conditional;

namespace {

vector_register_data_t get_vector_register_data(const Context &ctx, const Call *vector_borrow, const Call *vector_return) {
  const call_t &vb = vector_borrow->get_call();
  const call_t &vr = vector_return->get_call();

  const addr_t obj           = expr_addr_to_obj_addr(vb.args.at("vector").expr);
  const vector_config_t &cfg = ctx.get_vector_config(obj);

  const vector_register_data_t vector_register_data = {
      .obj         = obj,
      .capacity    = static_cast<u32>(cfg.capacity),
      .index       = vb.args.at("index").expr,
      .value       = vb.extra_vars.at("borrowed_cell").second,
      .write_value = vr.args.at("value").in,
      .actions     = {RegisterActionType::Read, RegisterActionType::Swap},
  };

  return vector_register_data;
}

std::unique_ptr<BDD> rebuild_bdd(const EP *ep, const BDDNode *node, const std::vector<const Call *> &vector_returns, const BDDNode *&new_next_node) {
  const BDD *old_bdd           = ep->get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  const BDDNode *old_next_node = node->get_next();
  new_next_node                = new_bdd->get_node_by_id(old_next_node->get_id());

  for (const Call *to_remove : vector_returns) {
    BDDNode *new_node = new_bdd->delete_non_branch(to_remove->get_id());
    if (to_remove->get_id() == old_next_node->get_id()) {
      new_next_node = new_node;
    }
  }

  return new_bdd;
}

std::vector<const Call *> get_future_vector_returns(Call::vector_conditional_write_result_t &vector_conditional_write_result) {
  std::vector<const Call *> vector_returns;
  vector_returns.push_back(vector_conditional_write_result.vector_return_write);
  for (const Call *vector_return_read : vector_conditional_write_result.vector_return_reads) {
    vector_returns.push_back(vector_return_read);
  }
  return vector_returns;
}

bool exprs_structurally_equal(klee::ref<klee::Expr> a, klee::ref<klee::Expr> b) {
  if (a.isNull() || b.isNull()) {
    return a.isNull() && b.isNull();
  }
  return a->compare(*b) == 0;
}

void collect_read_names(klee::ref<klee::Expr> e, std::unordered_set<std::string> &out) {
  if (e.isNull()) {
    return;
  }
  if (e->getKind() == klee::Expr::Read) {
    out.insert(dynamic_cast<klee::ReadExpr *>(e.get())->updates.root->name);
  }
  for (unsigned i = 0; i < e->getNumKids(); i++) {
    collect_read_names(e->getKid(i), out);
  }
}

void collect_subtree_read_names(const BDDNode *root, std::unordered_set<std::string> &out) {
  root->visit_nodes([&](const BDDNode *n) {
    if (n->get_type() == BDDNodeType::Call) {
      const call_t &c = dynamic_cast<const Call *>(n)->get_call();
      for (const auto &[name, arg] : c.args) {
        collect_read_names(arg.expr, out);
        collect_read_names(arg.in, out);
        collect_read_names(arg.out, out);
      }
      collect_read_names(c.ret, out);
      for (const auto &[name, ev] : c.extra_vars) {
        collect_read_names(ev.first, out);
        collect_read_names(ev.second, out);
      }
    } else if (n->get_type() == BDDNodeType::Branch) {
      collect_read_names(dynamic_cast<const Branch *>(n)->get_condition(), out);
    }
    return BDDNodeVisitAction::Continue;
  });
}

// Structural expr equality that compares everything (kinds, widths, constants,
// non-ignored symbols) exactly, but treats reads of `ignore`d symbols as wildcards
// (any ignored symbol matches any other). Used to compare fork clones where the
// only difference is freshly-renamed symbols.
bool exprs_equal_modulo(klee::ref<klee::Expr> a, klee::ref<klee::Expr> b, const std::unordered_set<std::string> &ignore) {
  if (a.isNull() || b.isNull()) {
    return a.isNull() && b.isNull();
  }
  if (a->getKind() != b->getKind() || a->getWidth() != b->getWidth()) {
    return false;
  }
  if (a->getKind() == klee::Expr::Read) {
    const std::string &na = dynamic_cast<klee::ReadExpr *>(a.get())->updates.root->name;
    const std::string &nb = dynamic_cast<klee::ReadExpr *>(b.get())->updates.root->name;
    if (!exprs_equal_modulo(a->getKid(0), b->getKid(0), ignore)) { // index
      return false;
    }
    if (ignore.count(na) && ignore.count(nb)) {
      return true; // both are fork-renamed: match regardless of the specific name
    }
    return na == nb;
  }
  if (a->getKind() == klee::Expr::Constant) {
    return a->compare(*b) == 0;
  }
  if (a->getNumKids() != b->getNumKids()) {
    return false;
  }
  for (unsigned i = 0; i < a->getNumKids(); i++) {
    if (!exprs_equal_modulo(a->getKid(i), b->getKid(i), ignore)) {
      return false;
    }
  }
  return true;
}

// Alpha-equivalence: the two sides of a conditional-write branch are clones KLEE
// produced by forking the same continuation, so they are identical except for
// freshly-renamed symbols (why are_subtrees_equal, symbol-sensitive, rejects them).
// Compare structure + all exprs exactly, wildcarding only the given renamed symbols.
bool subtrees_alpha_equal(const BDDNode *a, const BDDNode *b, const std::unordered_set<std::string> &ignore) {
  if (a == nullptr || b == nullptr) {
    return a == nullptr && b == nullptr;
  }
  if (a->get_type() != b->get_type()) {
    return false;
  }
  if (a->get_type() == BDDNodeType::Call) {
    const call_t &ca = dynamic_cast<const Call *>(a)->get_call();
    const call_t &cb = dynamic_cast<const Call *>(b)->get_call();
    if (ca.function_name != cb.function_name || ca.args.size() != cb.args.size()) {
      return false;
    }
    for (const auto &[name, arg] : ca.args) {
      auto it = cb.args.find(name);
      if (it == cb.args.end() || !exprs_equal_modulo(arg.expr, it->second.expr, ignore) || !exprs_equal_modulo(arg.in, it->second.in, ignore) ||
          !exprs_equal_modulo(arg.out, it->second.out, ignore)) {
        return false;
      }
    }
    if (!exprs_equal_modulo(ca.ret, cb.ret, ignore)) {
      return false;
    }
    return subtrees_alpha_equal(a->get_next(), b->get_next(), ignore);
  }
  if (a->get_type() == BDDNodeType::Branch) {
    const Branch *ba = dynamic_cast<const Branch *>(a);
    const Branch *bb = dynamic_cast<const Branch *>(b);
    if (!exprs_equal_modulo(ba->get_condition(), bb->get_condition(), ignore)) {
      return false;
    }
    return subtrees_alpha_equal(ba->get_on_true(), bb->get_on_true(), ignore) && subtrees_alpha_equal(ba->get_on_false(), bb->get_on_false(), ignore);
  }
  // Route / leaf: forwarding decisions aren't fork-renamed, compare exactly.
  return a->equals(b);
}

// The two branch sides are alpha-equivalent iff they match structurally once the
// symbols that appear in only one side (the fork-renamed ones) are wildcarded.
bool branch_sides_alpha_equal(const BDDNode *on_true, const BDDNode *on_false) {
  std::unordered_set<std::string> true_syms, false_syms, renamed;
  collect_subtree_read_names(on_true, true_syms);
  collect_subtree_read_names(on_false, false_syms);
  for (const std::string &s : true_syms) {
    if (!false_syms.count(s)) {
      renamed.insert(s);
    }
  }
  for (const std::string &s : false_syms) {
    if (!true_syms.count(s)) {
      renamed.insert(s);
    }
  }
  return subtrees_alpha_equal(on_true, on_false, renamed);
}

// A max/min swap displaces min(read, write); the NF computes it via a downstream
// min(a, b) call. Walk the straight-line calls at the head of `start` and return
// the first min() whose two arguments are {x, y} (in either order) -- i.e. the
// shadow of this register's conditional max-write. Returns nullptr if none.
const Call *find_shadow_min(const BDDNode *start, klee::ref<klee::Expr> x, klee::ref<klee::Expr> y) {
  for (const BDDNode *n = start; n != nullptr && n->get_type() == BDDNodeType::Call; n = n->get_next()) {
    const call_t &c = dynamic_cast<const Call *>(n)->get_call();
    if (c.function_name != "min") {
      continue;
    }
    auto ita = c.args.find("a");
    auto itb = c.args.find("b");
    if (ita == c.args.end() || itb == c.args.end()) {
      continue;
    }
    klee::ref<klee::Expr> a = ita->second.expr;
    klee::ref<klee::Expr> b = itb->second.expr;
    if ((exprs_structurally_equal(a, x) && exprs_structurally_equal(b, y)) ||
        (exprs_structurally_equal(a, y) && exprs_structurally_equal(b, x))) {
      return dynamic_cast<const Call *>(n);
    }
  }
  return nullptr;
}

} // namespace

std::optional<spec_impl_t> VectorRegisterReadConditionalUpdateSingleActionFactory::speculate(const EP *ep, const BDDNode *node,
                                                                                             const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *vector_borrow = dynamic_cast<const Call *>(node);
  const call_t &call        = vector_borrow->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  std::optional<Call::vector_conditional_write_result_t> vector_conditional_write_result = vector_borrow->get_vector_conditional_write_data();
  if (!vector_conditional_write_result.has_value()) {
    return {};
  }

  if (vector_conditional_write_result->conditions.size() != 1) {
    return {};
  }

  klee::ref<klee::Expr> condition = simplify_conditional(vector_conditional_write_result->conditions[0]);

  if (!is_condition_from_symbol(condition, vector_borrow->get_local_symbol("vector_data").name)) {
    return {};
  }

  const vector_register_data_t vector_register_data =
      get_vector_register_data(ep->get_ctx(), vector_borrow, vector_conditional_write_result->vector_return_write);

  const symbol_t &read_symbol = vector_borrow->get_local_symbol("vector_data");

  if (!get_tna(ep).is_simple_register_conditional_expr(vector_conditional_write_result->conditions, read_symbol)) {
    return {};
  }

  if (!speculations.ctx.can_impl_ds(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  if (!expr_is_materializable(vector_register_data.write_value)) {
    return {};
  }

  if (!can_build_or_reuse_vector_register(ep, vector_borrow, vector_register_data)) {
    return {};
  }

  if (was_ds_already_used(ep->get_leaf_ep_node_from_bdd_node(node), speculations, node, vector_register_data.obj, DSImpl::Tofino_VectorRegister,
                          build_vector_register_id(vector_register_data.obj))) {
    return {};
  }

  Context new_ctx = speculations.ctx;
  new_ctx.save_ds_impl(node->get_id(), vector_register_data.obj, DSImpl::Tofino_VectorRegister);

  spec_impl_t spec_impl(decide(ep, node), new_ctx);

  for (const Call *vector_return : get_future_vector_returns(*vector_conditional_write_result)) {
    spec_impl.skip.insert(vector_return->get_id());
  }

  return spec_impl;
}

std::vector<impl_t> VectorRegisterReadConditionalUpdateSingleActionFactory::process_node(const EP *ep, const BDDNode *node,
                                                                                         SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *vector_borrow = dynamic_cast<const Call *>(node);
  const call_t &call        = vector_borrow->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  std::optional<Call::vector_conditional_write_result_t> vector_conditional_write_result = vector_borrow->get_vector_conditional_write_data();
  if (!vector_conditional_write_result.has_value()) {
    return {};
  }

  if (vector_conditional_write_result->conditions.size() != 1) {
    return {};
  }

  klee::ref<klee::Expr> condition = simplify_conditional(vector_conditional_write_result->conditions[0]);

  if (!is_condition_from_symbol(condition, vector_borrow->get_local_symbol("vector_data").name)) {
    return {};
  }

  const vector_register_data_t vector_register_data =
      get_vector_register_data(ep->get_ctx(), vector_borrow, vector_conditional_write_result->vector_return_write);

  if (!get_tna(ep).is_simple_register_conditional_expr({condition}, vector_register_data.value)) {
    return {};
  }

  if (!ep->get_ctx().can_impl_ds(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  if (!expr_is_materializable(vector_register_data.write_value)) {
    return {};
  }

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  VectorRegister *vector_register = build_or_reuse_vector_register(new_ep.get(), vector_borrow, vector_register_data);
  if (!vector_register) {
    return {};
  }

  const EPNode *ep_node_leaf = ep->get_active_leaf().node;
  if (ep_node_leaf && was_ds_already_used(ep_node_leaf, vector_register->id)) {
    return {};
  }

  const BDDNode *new_next_node;
  std::unique_ptr<BDD> new_bdd = rebuild_bdd(new_ep.get(), node, get_future_vector_returns(*vector_conditional_write_result), new_next_node);

  // Max/min swap: the displaced value of the conditional max-write is
  // min(read, write), which the NF computes with a downstream min() call. When both
  // sides of the (now write-absorbed) branch are alpha-equivalent continuations that
  // each recompute that same shadow, collapse the branch, have the register action
  // return the shadow (ReadConditionalWriteReturnOther, matching the expert's
  // swap_if_larger), consume the min() node, and bind its symbol to the register out.
  bool returns_shadow = false;
  klee::ref<klee::Expr> shadow_symbol;
  if (new_next_node && new_next_node->get_type() == BDDNodeType::Branch) {
    const Branch *branch    = dynamic_cast<const Branch *>(new_next_node);
    const Call *min_on_true = find_shadow_min(branch->get_on_true(), vector_register_data.value, vector_register_data.write_value);
    const Call *min_on_false = find_shadow_min(branch->get_on_false(), vector_register_data.value, vector_register_data.write_value);
    if (min_on_true && min_on_false && branch_sides_alpha_equal(branch->get_on_true(), branch->get_on_false())) {
      const bdd_node_id_t min_id = min_on_true->get_id();
      // Merge the branch's two (equivalent) sides in the profiler before it vanishes
      // from the BDD, so downstream throughput still maps and counts all its traffic.
      new_ep->get_mutable_ctx().get_mutable_profiler().collapse_branch(branch);
      new_next_node              = new_bdd->delete_branch(new_next_node->get_id(), BDD::BranchDeletionAction::KeepOnTrue);
      const Call *kept_min       = dynamic_cast<const Call *>(new_bdd->get_node_by_id(min_id));
      shadow_symbol              = kept_min->get_call().ret;
      // The register action now produces the shadow, but the min() node that used to
      // generate its symbol is about to be consumed. Move that symbol's generation to
      // the register-read (vector_borrow) node so downstream users stay satisfied.
      Call *reg_read = dynamic_cast<Call *>(new_bdd->get_mutable_node_by_id(node->get_id()));
      for (const symbol_t &sym : kept_min->get_local_symbols().get()) {
        reg_read->add_local_symbol(sym);
      }
      if (new_next_node && new_next_node->get_id() == min_id) {
        new_next_node = new_bdd->delete_non_branch(min_id);
      } else {
        new_bdd->delete_non_branch(min_id);
      }
      returns_shadow = true;
    }
  }

  vector_register->add_register_action(returns_shadow ? RegisterActionType::ReadConditionalWriteReturnOther : RegisterActionType::ReadConditionalWrite);

  Module *module =
      new VectorRegisterReadConditionalUpdateSingleAction(node, vector_register->id, vector_register_data.obj, vector_register_data.index,
                                                          vector_register_data.value, vector_register_data.write_value, condition, returns_shadow,
                                                          shadow_symbol);
  EPNode *ep_node = new EPNode(module);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(node->get_id(), vector_register_data.obj, DSImpl::Tofino_VectorRegister);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, vector_register_data.obj, vector_register);

  const EPLeaf leaf(ep_node, new_next_node);
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->replace_bdd(std::move(new_bdd));

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> VectorRegisterReadConditionalUpdateSingleActionFactory::create(const BDD *bdd, const Context &ctx,
                                                                                       const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *vector_borrow = dynamic_cast<const Call *>(node);
  const call_t &call        = vector_borrow->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  std::optional<Call::vector_conditional_write_result_t> vector_conditional_write_result = vector_borrow->get_vector_conditional_write_data();
  if (!vector_conditional_write_result.has_value()) {
    return {};
  }

  if (vector_conditional_write_result->conditions.size() != 1) {
    return {};
  }

  klee::ref<klee::Expr> condition = simplify_conditional(vector_conditional_write_result->conditions[0]);

  if (!is_condition_from_symbol(condition, vector_borrow->get_local_symbol("vector_data").name)) {
    return {};
  }

  const vector_register_data_t vector_register_data =
      get_vector_register_data(ctx, vector_borrow, vector_conditional_write_result->vector_return_write);

  if (!ctx.get_target_ctx<TofinoContext>()->get_tna().is_simple_register_conditional_expr({condition}, vector_register_data.value)) {
    return {};
  }

  if (!ctx.check_ds_impl(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  if (!expr_is_materializable(vector_register_data.write_value)) {
    return {};
  }

  const std::unordered_set<DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_ds(vector_register_data.obj);
  assert(ds.size() == 1);
  assert((*ds.begin())->type == DSType::VectorRegister);
  VectorRegister *vector_register = dynamic_cast<VectorRegister *>(*ds.begin());

  return std::make_unique<VectorRegisterReadConditionalUpdateSingleAction>(node, vector_register->id, vector_register_data.obj,
                                                                           vector_register_data.index, vector_register_data.value,
                                                                           vector_register_data.write_value, condition);
}

} // namespace Tofino
} // namespace LibSynapse