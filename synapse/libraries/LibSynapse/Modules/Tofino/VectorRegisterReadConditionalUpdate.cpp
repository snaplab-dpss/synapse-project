#include <LibSynapse/Modules/Tofino/VectorRegisterReadConditionalUpdate.h>
#include <LibSynapse/Modules/Tofino/If.h>
#include <LibSynapse/Modules/Tofino/Then.h>
#include <LibSynapse/Modules/Tofino/Else.h>
#include <LibSynapse/Modules/Tofino/VectorRegisterLookup.h>
#include <LibSynapse/Modules/Tofino/VectorRegisterUpdate.h>
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

struct pattern_t {
  vector_register_data_t data;
  const Call *vector_borrow;
  const Branch *branch;
  const Call *read_vector_return;
  const Call *write_vector_return;
  bool read_on_true;
};

// Does `symbol` appear (in any call arg/ret or branch condition) anywhere in the
// subtree rooted at `root`?
bool symbol_used_in_subtree(const BDDNode *root, const std::string &symbol) {
  if (root == nullptr) {
    return false;
  }
  bool found = false;
  root->visit_nodes([&](const BDDNode *n) {
    std::unordered_set<std::string> names;
    auto collect = [&names](klee::ref<klee::Expr> e) {
      if (e.isNull()) {
        return;
      }
      std::vector<klee::ref<klee::Expr>> stack{e};
      while (!stack.empty()) {
        klee::ref<klee::Expr> cur = stack.back();
        stack.pop_back();
        if (cur.isNull()) {
          continue;
        }
        if (cur->getKind() == klee::Expr::Read) {
          names.insert(dynamic_cast<klee::ReadExpr *>(cur.get())->updates.root->name);
        }
        for (unsigned i = 0; i < cur->getNumKids(); i++) {
          stack.push_back(cur->getKid(i));
        }
      }
    };
    if (n->get_type() == BDDNodeType::Call) {
      const call_t &c = dynamic_cast<const Call *>(n)->get_call();
      for (const auto &[name, arg] : c.args) {
        collect(arg.expr);
        collect(arg.in);
        collect(arg.out);
      }
      collect(c.ret);
    } else if (n->get_type() == BDDNodeType::Branch) {
      collect(dynamic_cast<const Branch *>(n)->get_condition());
    }
    if (names.count(symbol)) {
      found = true;
      return BDDNodeVisitAction::Stop;
    }
    return BDDNodeVisitAction::Continue;
  });
  return found;
}

bool is_pattern(const Context &ctx, const BDD *bdd, const BDDNode *node, pattern_t &pattern) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }

  const Call *vector_borrow = dynamic_cast<const Call *>(node);
  const call_t &vb_call     = vector_borrow->get_call();

  if (vb_call.function_name != "vector_borrow") {
    return false;
  }

  pattern.data.obj      = expr_addr_to_obj_addr(vb_call.args.at("vector").expr);
  pattern.data.capacity = ctx.get_vector_config(pattern.data.obj).capacity;
  pattern.data.index    = vb_call.args.at("index").expr;
  pattern.data.value    = vb_call.extra_vars.at("borrowed_cell").second;
  pattern.data.actions  = {RegisterActionType::Read, RegisterActionType::Write};
  pattern.vector_borrow = vector_borrow;

  if (vector_borrow->get_next()->get_type() != BDDNodeType::Branch) {
    return false;
  }
  pattern.branch = dynamic_cast<const Branch *>(vector_borrow->get_next());

  std::optional<Call::vector_conditional_write_result_t> vector_conditional_write_result = vector_borrow->get_vector_conditional_write_data();
  if (!vector_conditional_write_result.has_value()) {
    return false;
  }

  if (vector_conditional_write_result->conditions.size() != 1) {
    return false;
  }

  if (vector_conditional_write_result->vector_return_reads.size() != 1) {
    return false;
  }

  pattern.read_vector_return  = vector_conditional_write_result->vector_return_reads.at(0);
  pattern.write_vector_return = vector_conditional_write_result->vector_return_write;

  if ((pattern.branch->get_on_true() != pattern.read_vector_return && pattern.branch->get_on_false() != pattern.read_vector_return) ||
      (pattern.branch->get_on_true() != pattern.write_vector_return && pattern.branch->get_on_false() != pattern.write_vector_return)) {
    return false;
  }

  pattern.data.write_value = pattern.write_vector_return->get_call().args.at("value").in;

  if (pattern.branch->get_on_true() == pattern.read_vector_return) {
    pattern.read_on_true = true;
  } else {
    pattern.read_on_true = false;
  }

  // This module splits the read onto one branch and the write onto the other, so it's
  // only valid when the read value isn't needed on the write branch's continuation. If
  // it is (e.g. a counter read once, incremented on one side, then compared on both),
  // the read value would be unbound there -- a single read-conditional-increment
  // register action must handle it instead. Decline so that module claims the node.
  const std::string read_symbol = vector_borrow->get_local_symbol("vector_data").name;
  if (symbol_used_in_subtree(pattern.write_vector_return->get_next(), read_symbol)) {
    return false;
  }

  return true;
}

struct rebuilt_bdd_result_t {
  std::unique_ptr<BDD> bdd;
  BDDNode *after_read;
  BDDNode *after_write;
};

rebuilt_bdd_result_t rebuild_bdd(const EP *ep, const BDDNode *node, const pattern_t &pattern) {
  rebuilt_bdd_result_t result;

  result.bdd         = std::make_unique<BDD>(*ep->get_bdd());
  result.after_read  = result.bdd->delete_non_branch(pattern.read_vector_return->get_id());
  result.after_write = result.bdd->delete_non_branch(pattern.write_vector_return->get_id());

  return result;
}

} // namespace

std::optional<spec_impl_t> VectorRegisterReadConditionalUpdateFactory::speculate(const EP *ep, const BDDNode *node,
                                                                                 const speculations_t &speculations) const {
  pattern_t pattern;
  if (!is_pattern(speculations.ctx, ep->get_bdd(), node, pattern)) {
    return {};
  }

  klee::ref<klee::Expr> condition               = simplify_conditional(pattern.branch->get_condition());
  const std::vector<If::condition_t> conditions = IfFactory::get_compatible_conditions(get_tna(ep), condition);

  if (conditions.empty()) {
    return {};
  }

  if (!speculations.ctx.can_impl_ds(pattern.data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  if (!expr_is_materializable(pattern.data.write_value)) {
    return {};
  }

  if (!can_build_or_reuse_vector_register(ep, pattern.vector_borrow, pattern.data)) {
    return {};
  }

  if (was_ds_already_used(ep->get_leaf_ep_node_from_bdd_node(node), speculations, node, pattern.data.obj, DSImpl::Tofino_VectorRegister,
                          build_vector_register_id(pattern.data.obj))) {
    return {};
  }

  Context new_ctx = speculations.ctx;
  new_ctx.save_ds_impl(node->get_id(), pattern.data.obj, DSImpl::Tofino_VectorRegister);

  spec_impl_t spec_impl(decide(ep, node), new_ctx);

  spec_impl.skip.insert(pattern.read_vector_return->get_id());
  spec_impl.skip.insert(pattern.write_vector_return->get_id());

  return spec_impl;
}

std::vector<impl_t> VectorRegisterReadConditionalUpdateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  pattern_t pattern;
  if (!is_pattern(ep->get_ctx(), ep->get_bdd(), node, pattern)) {
    return {};
  }

  klee::ref<klee::Expr> condition               = simplify_conditional(pattern.branch->get_condition());
  const std::vector<If::condition_t> conditions = IfFactory::get_compatible_conditions(get_tna(ep), condition);

  if (conditions.empty()) {
    return {};
  }

  if (!ep->get_ctx().can_impl_ds(pattern.data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  // The write value (e.g. rank = tz + 1) is materialized into a metadata field
  // before the register action, so it only needs to be MAU-computable.
  if (!expr_is_materializable(pattern.data.write_value)) {
    return {};
  }

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  VectorRegister *vector_register = build_or_reuse_vector_register(new_ep.get(), pattern.vector_borrow, pattern.data);
  if (!vector_register) {
    return {};
  }

  const EPNode *ep_node_leaf = ep->get_active_leaf().node;
  if (ep_node_leaf && was_ds_already_used(ep_node_leaf, vector_register->id)) {
    return {};
  }

  rebuilt_bdd_result_t rebuilt_bdd_result = rebuild_bdd(new_ep.get(), node, pattern);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(node->get_id(), pattern.data.obj, DSImpl::Tofino_VectorRegister);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, pattern.data.obj, vector_register);

  Module *if_module       = new If(pattern.branch, pattern.branch->get_condition(), conditions);
  Module *then_module     = new Then(pattern.branch);
  Module *else_module     = new Else(pattern.branch);
  Module *vector_reg_read = new VectorRegisterLookup(node, vector_register->id, pattern.data.obj, pattern.data.index, pattern.data.value);
  Module *vector_reg_update =
      new VectorRegisterUpdate(node, vector_register->id, pattern.data.obj, pattern.data.index, pattern.data.value, pattern.data.write_value);

  EPNode *if_node     = new EPNode(if_module);
  EPNode *then_node   = new EPNode(then_module);
  EPNode *else_node   = new EPNode(else_module);
  EPNode *read_node   = new EPNode(vector_reg_read);
  EPNode *update_node = new EPNode(vector_reg_update);

  if_node->set_children(condition, then_node, else_node);
  then_node->set_prev(if_node);
  else_node->set_prev(if_node);

  if (pattern.read_on_true) {
    then_node->set_children(read_node);
    else_node->set_children(update_node);

    read_node->set_prev(then_node);
    update_node->set_prev(else_node);
  } else {
    then_node->set_children(update_node);
    else_node->set_children(read_node);

    update_node->set_prev(then_node);
    read_node->set_prev(else_node);
  }

  const EPLeaf read_leaf(read_node, rebuilt_bdd_result.after_read);
  const EPLeaf write_leaf(update_node, rebuilt_bdd_result.after_write);

  new_ep->process_leaf(if_node, {read_leaf, write_leaf});
  new_ep->replace_bdd(std::move(rebuilt_bdd_result.bdd));

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> VectorRegisterReadConditionalUpdateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  return {};
}

} // namespace Tofino
} // namespace LibSynapse