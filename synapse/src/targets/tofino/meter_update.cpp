#include "meter_update.h"

namespace synapse {
namespace tofino {
namespace {
bool get_tb_data(const EP *ep, const Call *tb_is_tracing, const Call *tb_update_and_check,
                 addr_t &obj, tb_config_t &cfg, std::vector<klee::ref<klee::Expr>> &keys,
                 klee::ref<klee::Expr> pkt_len, klee::ref<klee::Expr> &hit,
                 klee::ref<klee::Expr> pass, DS_ID &id) {
  const call_t &call_is_tracing = tb_is_tracing->get_call();
  assert(call_is_tracing.function_name == "tb_is_tracing" && "Unexpected function");

  const call_t &call_update = tb_update_and_check->get_call();
  assert(call_update.function_name == "tb_update_and_check" && "Unexpected function");

  klee::ref<klee::Expr> tb_addr_expr = call_is_tracing.args.at("tb").expr;
  klee::ref<klee::Expr> key = call_is_tracing.args.at("key").in;
  klee::ref<klee::Expr> index_out = call_is_tracing.args.at("index_out").out;
  klee::ref<klee::Expr> is_tracing = call_is_tracing.ret;

  obj = expr_addr_to_obj_addr(tb_addr_expr);
  keys = Table::build_keys(key);
  pkt_len = call_update.args.at("pkt_len").expr;
  hit = is_tracing;
  pass = call_update.ret;
  id = "tb_" + std::to_string(tb_is_tracing->get_id());

  const Context &ctx = ep->get_ctx();
  cfg = ctx.get_tb_config(obj);

  return true;
}

Meter *build_meter(const EP *ep, const Node *node, DS_ID id, const tb_config_t &cfg,
                   const std::vector<klee::ref<klee::Expr>> &keys) {
  std::vector<bits_t> keys_size;
  for (klee::ref<klee::Expr> key : keys) {
    keys_size.push_back(key->getWidth());
  }

  Meter *meter = new Meter(id, cfg.capacity, cfg.rate, cfg.burst, keys_size);

  const TofinoContext *tofino_ctx = TofinoModuleFactory::get_tofino_ctx(ep);
  if (!tofino_ctx->check_placement(ep, node, meter)) {
    delete meter;
    return nullptr;
  }

  return meter;
}

std::unique_ptr<BDD> delete_future_tb_update(EP *ep, const Node *node,
                                             const Call *tb_update_and_check,
                                             const Node *&new_next) {
  const BDD *old_bdd = ep->get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  const Node *next = node->get_next();

  if (next) {
    new_next = new_bdd->get_node_by_id(next->get_id());
  } else {
    new_next = nullptr;
  }

  bool replace_next = (tb_update_and_check == next);
  Node *replacement = new_bdd->delete_non_branch(tb_update_and_check->get_id());

  if (replace_next) {
    new_next = replacement;
  }

  return new_bdd;
}
} // namespace

std::optional<spec_impl_t> MeterUpdateFactory::speculate(const EP *ep, const Node *node,
                                                         const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *tb_is_tracing = dynamic_cast<const Call *>(node);

  const Call *tb_update_and_check;
  if (!is_tb_tracing_check_followed_by_update_on_true(tb_is_tracing, tb_update_and_check)) {
    return std::nullopt;
  }

  addr_t obj;
  tb_config_t cfg;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> pkt_len;
  klee::ref<klee::Expr> hit;
  klee::ref<klee::Expr> pass;
  DS_ID id;

  if (!get_tb_data(ep, tb_is_tracing, tb_update_and_check, obj, cfg, keys, pkt_len, hit, pass,
                   id)) {
    return std::nullopt;
  }

  if (!ctx.can_impl_ds(obj, DSImpl::Tofino_Table)) {
    return std::nullopt;
  }

  Meter *meter = build_meter(ep, node, id, cfg, keys);

  if (!meter) {
    return std::nullopt;
  }

  delete meter;

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(obj, DSImpl::Tofino_Meter);

  spec_impl_t spec_impl(decide(ep, node), new_ctx);
  spec_impl.skip.insert(tb_update_and_check->get_id());

  return spec_impl;
}

std::vector<impl_t> MeterUpdateFactory::process_node(const EP *ep, const Node *node,
                                                     SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *tb_is_tracing = dynamic_cast<const Call *>(node);

  const Call *tb_update_and_check;
  if (!is_tb_tracing_check_followed_by_update_on_true(tb_is_tracing, tb_update_and_check)) {
    return impls;
  }

  addr_t obj;
  tb_config_t cfg;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> pkt_len;
  klee::ref<klee::Expr> hit;
  klee::ref<klee::Expr> pass;
  DS_ID id;

  if (!get_tb_data(ep, tb_is_tracing, tb_update_and_check, obj, cfg, keys, pkt_len, hit, pass,
                   id)) {
    return impls;
  }

  if (!ep->get_ctx().can_impl_ds(obj, DSImpl::Tofino_Meter)) {
    return impls;
  }

  Meter *meter = build_meter(ep, node, id, cfg, keys);

  if (!meter) {
    return impls;
  }

  Module *module = new MeterUpdate(node, id, obj, keys, pkt_len, hit, pass);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  const Node *new_next;
  std::unique_ptr<BDD> new_bdd =
      delete_future_tb_update(new_ep, node, tb_update_and_check, new_next);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(obj, DSImpl::Tofino_Meter);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->place(new_ep, node, obj, meter);

  EPLeaf leaf(ep_node, new_next);
  new_ep->process_leaf(ep_node, {leaf});
  new_ep->replace_bdd(std::move(new_bdd));

  return impls;
}

} // namespace tofino
} // namespace synapse