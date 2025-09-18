#include <LibSynapse/Modules/Tofino/MeterUpdate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {
const EPNode *get_ep_node_from_bdd_node(const EP *ep, const BDDNode *node) {
  std::vector<const EPNode *> ep_nodes;
  for (const EPLeaf &leaf : ep->get_active_leaves()) {
    if (leaf.node) {
      ep_nodes.push_back(leaf.node);
    }
  }

  while (!ep_nodes.empty()) {
    const EPNode *ep_node = ep_nodes[0];
    ep_nodes.erase(ep_nodes.begin());

    const Module *module = ep_node->get_module();
    assert(module && "Module not found");

    if (module->get_node() == node) {
      return ep_node;
    }

    const EPNode *prev = ep_node->get_prev();
    if (prev) {
      ep_nodes.push_back(prev);
    }
  }

  return nullptr;
}

const EPNode *get_ep_node_leaf_from_future_bdd_node(const EP *ep, const BDDNode *node) {
  const std::list<EPLeaf> &active_leaves = ep->get_active_leaves();

  while (node) {
    for (const EPLeaf &leaf : active_leaves) {
      if (leaf.next == node) {
        return leaf.node;
      }
    }

    node = node->get_prev();
  }

  return nullptr;
}

struct tb_data_t {
  addr_t obj;
  tb_config_t cfg;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> pkt_len;
  klee::ref<klee::Expr> hit;
  klee::ref<klee::Expr> pass;
  DS_ID id;
};

tb_data_t get_tb_data(const Context &ctx, const Call *tb_is_tracing, const Call *tb_update_and_check) {
  const call_t &call_is_tracing = tb_is_tracing->get_call();
  assert(call_is_tracing.function_name == "tb_is_tracing" && "Unexpected function");

  const call_t &call_update = tb_update_and_check->get_call();
  assert(call_update.function_name == "tb_update_and_check" && "Unexpected function");

  klee::ref<klee::Expr> tb_addr_expr = call_is_tracing.args.at("tb").expr;
  klee::ref<klee::Expr> key          = call_is_tracing.args.at("key").in;
  klee::ref<klee::Expr> index_out    = call_is_tracing.args.at("index_out").out;
  klee::ref<klee::Expr> is_tracing   = call_is_tracing.ret;

  const tb_data_t data = {
      .obj     = expr_addr_to_obj_addr(tb_addr_expr),
      .cfg     = ctx.get_tb_config(expr_addr_to_obj_addr(tb_addr_expr)),
      .keys    = Table::build_keys(key, ctx.get_expr_structs()),
      .pkt_len = call_update.args.at("pkt_len").expr,
      .hit     = is_tracing,
      .pass    = call_update.ret,
      .id      = "tb_" + std::to_string(tb_is_tracing->get_id()),
  };

  return data;
}

Meter *build_meter(const EP *ep, const BDDNode *node, DS_ID id, const tb_config_t &cfg, const std::vector<klee::ref<klee::Expr>> &keys) {
  std::vector<bits_t> keys_size;
  for (klee::ref<klee::Expr> key : keys) {
    keys_size.push_back(key->getWidth());
  }

  Meter *meter = new Meter(id, cfg.capacity, cfg.rate, cfg.burst, keys_size);

  const EPNode *module_ep_node = get_ep_node_from_bdd_node(ep, node);
  if (!module_ep_node) {
    module_ep_node = get_ep_node_leaf_from_future_bdd_node(ep, node);

    if (!module_ep_node) {
      panic("Could not find EPNode corresponding to BDDNode");
    }
  }

  const TargetType target = module_ep_node->get_module()->get_target();

  const TofinoContext *tofino_ctx = TofinoModuleFactory::get_tofino_ctx(ep, target);
  if (!tofino_ctx->can_place(ep, node, meter)) {
    delete meter;
    return nullptr;
  }

  return meter;
}

std::unique_ptr<BDD> delete_future_tb_update(EP *ep, const BDDNode *node, const Call *tb_update_and_check, const BDDNode *&new_next) {
  const BDD *old_bdd           = ep->get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  const BDDNode *next = node->get_next();

  if (next) {
    new_next = new_bdd->get_node_by_id(next->get_id());
  } else {
    new_next = nullptr;
  }

  bool replace_next    = (tb_update_and_check == next);
  BDDNode *replacement = new_bdd->delete_non_branch(tb_update_and_check->get_id());

  if (replace_next) {
    new_next = replacement;
  }

  return new_bdd;
}
} // namespace

std::optional<spec_impl_t> MeterUpdateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *tb_is_tracing = dynamic_cast<const Call *>(node);

  const Call *tb_update_and_check;
  if (!ep->get_bdd()->is_tb_tracing_check_followed_by_update_on_true(tb_is_tracing, tb_update_and_check)) {
    return {};
  }

  const tb_data_t data = get_tb_data(ep->get_ctx(), tb_is_tracing, tb_update_and_check);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_Meter)) {
    return {};
  }

  Meter *meter = build_meter(ep, node, data.id, data.cfg, data.keys);

  if (!meter) {
    return {};
  }

  delete meter;

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(data.obj, DSImpl::Tofino_Meter);

  spec_impl_t spec_impl(decide(ep, node), new_ctx);
  spec_impl.skip.insert(tb_update_and_check->get_id());

  return spec_impl;
}

std::vector<impl_t> MeterUpdateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *tb_is_tracing = dynamic_cast<const Call *>(node);

  const Call *tb_update_and_check;
  if (!ep->get_bdd()->is_tb_tracing_check_followed_by_update_on_true(tb_is_tracing, tb_update_and_check)) {
    return {};
  }

  const tb_data_t data = get_tb_data(ep->get_ctx(), tb_is_tracing, tb_update_and_check);

  if (!ep->get_ctx().can_impl_ds(data.obj, DSImpl::Tofino_Meter)) {
    return {};
  }

  Meter *meter = build_meter(ep, node, data.id, data.cfg, data.keys);

  if (!meter) {
    return {};
  }

  Module *module  = new MeterUpdate(get_type().instance_id, node, data.id, data.obj, data.keys, data.pkt_len, data.hit, data.pass);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const BDDNode *new_next;
  std::unique_ptr<BDD> new_bdd = delete_future_tb_update(new_ep.get(), node, tb_update_and_check, new_next);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(data.obj, DSImpl::Tofino_Meter);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get(), target);
  tofino_ctx->place(new_ep.get(), node, data.obj, meter);

  const EPLeaf leaf(ep_node, new_next);
  new_ep->process_leaf(ep_node, {leaf});
  new_ep->replace_bdd(std::move(new_bdd));

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> MeterUpdateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *tb_is_tracing = dynamic_cast<const Call *>(node);

  const Call *tb_update_and_check;
  if (!bdd->is_tb_tracing_check_followed_by_update_on_true(tb_is_tracing, tb_update_and_check)) {
    return {};
  }

  const tb_data_t data = get_tb_data(ctx, tb_is_tracing, tb_update_and_check);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_Meter)) {
    return {};
  }

  const std::unordered_set<Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>(target)->get_data_structures().get_ds(data.obj);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const Meter *meter = dynamic_cast<const Meter *>(*ds.begin());

  return std::make_unique<MeterUpdate>(get_type().instance_id, node, meter->id, data.obj, data.keys, data.pkt_len, data.hit, data.pass);
}

} // namespace Tofino
} // namespace LibSynapse