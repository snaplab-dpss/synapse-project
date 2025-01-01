#include "map_register_delete.h"

namespace tofino {

namespace {
struct map_register_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  u32 num_entries;

  map_register_data_t(const EP *ep, const Call *map_erase) {

    const call_t &call = map_erase->get_call();
    ASSERT(call.function_name == "map_erase", "Not a map_erase call");

    obj = expr_addr_to_obj_addr(call.args.at("map").expr);
    key = call.args.at("key").in;
    num_entries = ep->get_ctx().get_map_config(obj).capacity;
  }
};

std::vector<const Node *>
get_future_related_nodes(const EP *ep, const Node *node,
                         const map_coalescing_objs_t &map_coalescing_objs) {
  std::vector<const Call *> ops =
      get_future_functions(node, {"dchain_free_index", "map_erase"});

  std::vector<const Node *> related_ops;
  for (const Call *op : ops) {
    const call_t &call = op->get_call();

    if (call.function_name == "dchain_free_index") {
      klee::ref<klee::Expr> obj_expr = call.args.at("chain").expr;
      addr_t obj = expr_addr_to_obj_addr(obj_expr);

      if (obj != map_coalescing_objs.dchain) {
        continue;
      }
    } else if (call.function_name == "map_erase") {
      klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;
      addr_t obj = expr_addr_to_obj_addr(obj_expr);

      if (obj != map_coalescing_objs.map) {
        continue;
      }
    }

    related_ops.push_back(op);
  }

  return related_ops;
}

std::unique_ptr<BDD>
delete_future_related_nodes(const EP *ep, const Node *map_erase,
                            const map_coalescing_objs_t &map_coalescing_objs,
                            const Node *&new_next) {
  const BDD *old_bdd = ep->get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  const Node *next = map_erase->get_next();

  if (next) {
    new_next = new_bdd->get_node_by_id(next->get_id());
  } else {
    new_next = nullptr;
  }

  std::vector<const Node *> future_nodes =
      get_future_related_nodes(ep, next, map_coalescing_objs);

  for (const Node *node : future_nodes) {
    bool replace = (node == next);
    Node *replacement =
        delete_non_branch_node_from_bdd(new_bdd.get(), node->get_id());
    if (replace) {
      next = replacement;
    }
  }

  new_next = next;

  return new_bdd;
}
} // namespace

std::optional<spec_impl_t>
MapRegisterDeleteFactory::speculate(const EP *ep, const Node *node,
                                    const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *map_erase = static_cast<const Call *>(node);
  const call_t &call = map_erase->get_call();

  if (call.function_name != "map_erase") {
    return std::nullopt;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_map_op(ep, map_erase, map_objs)) {
    return std::nullopt;
  }

  if (!ctx.can_impl_ds(map_objs.map, DSImpl::Tofino_MapRegister) ||
      !ctx.can_impl_ds(map_objs.dchain, DSImpl::Tofino_MapRegister)) {
    return std::nullopt;
  }

  map_register_data_t map_register_data(ep, map_erase);

  if (!can_build_or_reuse_map_register(ep, node, map_register_data.obj,
                                       map_register_data.key,
                                       map_register_data.num_entries)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_MapRegister);
  new_ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_MapRegister);

  spec_impl_t spec_impl(decide(ep, node), new_ctx);

  std::vector<const Node *> ignore_nodes =
      get_future_related_nodes(ep, node, map_objs);

  for (const Node *op : ignore_nodes) {
    spec_impl.skip.insert(op->get_id());
  }

  return spec_impl;
}

std::vector<impl_t>
MapRegisterDeleteFactory::process_node(const EP *ep, const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *map_erase = static_cast<const Call *>(node);
  const call_t &call = map_erase->get_call();

  if (call.function_name != "map_erase") {
    return impls;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_map_op(ep, map_erase, map_objs)) {
    return impls;
  }

  if (!ep->get_ctx().can_impl_ds(map_objs.map, DSImpl::Tofino_MapRegister) ||
      !ep->get_ctx().can_impl_ds(map_objs.dchain, DSImpl::Tofino_MapRegister)) {
    return impls;
  }

  map_register_data_t map_register_data(ep, map_erase);

  MapRegister *map_register = build_or_reuse_map_register(
      ep, map_erase, map_register_data.obj, map_register_data.key,
      map_register_data.num_entries);

  if (!map_register) {
    return impls;
  }

  Module *module =
      new MapRegisterDelete(map_erase, map_register->id, map_register_data.obj,
                            map_register_data.key);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  const Node *new_next;
  std::unique_ptr<BDD> new_bdd =
      delete_future_related_nodes(new_ep, map_erase, map_objs, new_next);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_MapRegister);
  ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_MapRegister);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->place(new_ep, map_erase, map_objs.map, map_register);

  EPLeaf leaf(ep_node, new_next);

  new_ep->process_leaf(ep_node, {leaf});
  new_ep->replace_bdd(std::move(new_bdd));
  new_ep->assert_integrity();

  return impls;
}

} // namespace tofino
