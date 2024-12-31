#include "table_delete.h"

namespace tofino_cpu {

namespace {
void table_delete_data_from_map_op(const Call *call_node, addr_t &obj,
                                   std::vector<klee::ref<klee::Expr>> &keys) {
  const call_t &call = call_node->get_call();
  ASSERT(call.function_name == "map_erase", "Not a map_erase call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key = call.args.at("key").in;

  obj = expr_addr_to_obj_addr(map_addr_expr);
  keys = Table::build_keys(key);
}

void table_delete_data_from_dchain_op(
    const Call *call_node, addr_t &obj,
    std::vector<klee::ref<klee::Expr>> &keys) {
  const call_t &call = call_node->get_call();
  ASSERT(call.function_name == "dchain_free_index", "Not a dchain call");

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index = call.args.at("index").expr;

  addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

  obj = dchain_addr;
  keys.push_back(index);
}

bool get_table_delete_data(const Call *call_node, addr_t &obj,
                           std::vector<klee::ref<klee::Expr>> &keys) {
  const call_t &call = call_node->get_call();

  if (call.function_name == "map_erase") {
    table_delete_data_from_map_op(call_node, obj, keys);
  } else if (call.function_name == "dchain_free_index") {
    table_delete_data_from_dchain_op(call_node, obj, keys);
  } else {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t>
TableDeleteGenerator::speculate(const EP *ep, const Node *node,
                                const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *call_node = static_cast<const Call *>(node);

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  if (!get_table_delete_data(call_node, obj, keys)) {
    return std::nullopt;
  }

  if (!ctx.can_impl_ds(obj, DSImpl::Tofino_Table)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> TableDeleteGenerator::process_node(const EP *ep,
                                                       const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *call_node = static_cast<const Call *>(node);

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  if (!get_table_delete_data(call_node, obj, keys)) {
    return impls;
  }

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_Table)) {
    return impls;
  }

  Module *module = new TableDelete(node, obj, keys);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace tofino_cpu
