#include "table_update.h"

namespace synapse {
namespace ctrl {

using tofino::Table;

namespace {
void table_update_data_from_map_op(const Call *call_node, addr_t &obj, std::vector<klee::ref<klee::Expr>> &keys,
                                   std::vector<klee::ref<klee::Expr>> &values) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "map_put" && "Not a map_put call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  klee::ref<klee::Expr> value         = call.args.at("value").expr;

  obj    = expr_addr_to_obj_addr(map_addr_expr);
  keys   = Table::build_keys(key);
  values = {value};
}

void table_update_data_from_vector_op(const Call *call_node, addr_t &obj, std::vector<klee::ref<klee::Expr>> &keys,
                                      std::vector<klee::ref<klee::Expr>> &values) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "vector_return" && "Not a vector_return call");

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;
  klee::ref<klee::Expr> value            = call.args.at("value").in;

  obj    = expr_addr_to_obj_addr(vector_addr_expr);
  keys   = {index};
  values = {value};
}

void table_data_from_dchain_op(const Call *call_node, addr_t &obj, std::vector<klee::ref<klee::Expr>> &keys,
                               std::vector<klee::ref<klee::Expr>> &values) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "dchain_allocate_new_index" && "Not a dchain call");

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index_out        = call.args.at("index_out").out;

  addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

  obj  = dchain_addr;
  keys = {index_out};
  // No value, the index is actually the table key
}

bool get_table_update_data(const Call *call_node, addr_t &obj, std::vector<klee::ref<klee::Expr>> &keys,
                           std::vector<klee::ref<klee::Expr>> &values) {
  const call_t &call = call_node->get_call();

  if (call.function_name == "map_put") {
    table_update_data_from_map_op(call_node, obj, keys, values);
  } else if (call.function_name == "vector_return") {
    table_update_data_from_vector_op(call_node, obj, keys, values);
  } else if (call.function_name == "dchain_allocate_new_index") {
    table_data_from_dchain_op(call_node, obj, keys, values);
  } else {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> TableUpdateFactory::speculate(const EP *ep, const Node *node, const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);

  if (call_node->is_vector_return_without_modifications()) {
    return std::nullopt;
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  std::vector<klee::ref<klee::Expr>> values;
  if (!get_table_update_data(call_node, obj, keys, values)) {
    return std::nullopt;
  }

  if (!ctx.can_impl_ds(obj, DSImpl::Tofino_Table)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> TableUpdateFactory::process_node(const EP *ep, const Node *node,
                                                     SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);

  if (call_node->is_vector_return_without_modifications()) {
    return impls;
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  std::vector<klee::ref<klee::Expr>> values;
  if (!get_table_update_data(call_node, obj, keys, values)) {
    return impls;
  }

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_Table)) {
    return impls;
  }

  Module *module  = new TableUpdate(node, obj, keys, values);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace ctrl
} // namespace synapse