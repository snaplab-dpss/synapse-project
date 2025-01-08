#include "map_register_write.h"

namespace synapse {
namespace ctrl {

using tofino::DS_ID;
using tofino::Table;

namespace {
DS_ID get_map_register(const EP *ep, addr_t obj) {
  const Context &ctx = ep->get_ctx();
  const tofino::TofinoContext *tofino_ctx = ctx.get_target_ctx<tofino::TofinoContext>();
  const std::unordered_set<tofino::DS *> &data_structures = tofino_ctx->get_ds(obj);
  assert(data_structures.size() == 1 && "Multiple data structures found");
  tofino::DS *ds = *data_structures.begin();
  assert(ds->type == tofino::DSType::MAP_REGISTER && "Invalid data structure");
  return ds->id;
}

void get_data(const Call *call_node, addr_t &obj, std::vector<klee::ref<klee::Expr>> &keys,
              klee::ref<klee::Expr> &value) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "map_put" && "Unexpected function");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key = call.args.at("key").in;
  value = call.args.at("value").expr;

  obj = expr_addr_to_obj_addr(map_addr_expr);
  keys = Table::build_keys(key);
  value = value;
}
} // namespace

std::optional<spec_impl_t> MapRegisterWriteFactory::speculate(const EP *ep, const Node *node,
                                                              const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "map_put") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  addr_t map_addr = expr_addr_to_obj_addr(map_addr_expr);

  if (!ctx.check_ds_impl(map_addr, DSImpl::Tofino_MapRegister)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> MapRegisterWriteFactory::process_node(const EP *ep, const Node *node,
                                                          SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "map_put") {
    return impls;
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  get_data(call_node, obj, keys, value);

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_MapRegister)) {
    return impls;
  }

  DS_ID id = get_map_register(ep, obj);

  Module *module = new MapRegisterWrite(node, id, obj, keys, value);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace ctrl
} // namespace synapse