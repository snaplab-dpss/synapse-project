#include "table_lookup.h"

namespace tofino_cpu {

using tofino::Table;

namespace {
void table_data_from_map_op(const Call *call_node, addr_t &obj,
                            std::vector<klee::ref<klee::Expr>> &keys,
                            std::vector<klee::ref<klee::Expr>> &values,
                            std::optional<symbol_t> &hit) {
  const call_t &call = call_node->get_call();
  ASSERT(call.function_name == "map_get", "Not a map_get call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key = call.args.at("key").in;
  klee::ref<klee::Expr> value_out = call.args.at("value_out").out;

  symbols_t symbols = call_node->get_locally_generated_symbols();

  symbol_t map_has_this_key;
  bool found = get_symbol(symbols, "map_has_this_key", map_has_this_key);
  ASSERT(found, "Symbol map_has_this_key not found");

  obj = expr_addr_to_obj_addr(map_addr_expr);
  keys = Table::build_keys(key);
  values = {value_out};
  hit = map_has_this_key;
}

void table_data_from_vector_op(const Call *call_node, addr_t &obj,
                               std::vector<klee::ref<klee::Expr>> &keys,
                               std::vector<klee::ref<klee::Expr>> &values,
                               std::optional<symbol_t> &hit) {
  // We can implement even if we later update the vector's contents!

  const call_t &call = call_node->get_call();
  ASSERT(call.function_name == "vector_borrow", "Not a vector_borrow call");

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index = call.args.at("index").expr;
  klee::ref<klee::Expr> cell = call.extra_vars.at("borrowed_cell").second;

  obj = expr_addr_to_obj_addr(vector_addr_expr);
  keys = {index};
  values = {cell};
}

void table_data_from_dchain_op(const Call *call_node, addr_t &obj,
                               std::vector<klee::ref<klee::Expr>> &keys,
                               std::vector<klee::ref<klee::Expr>> &values,
                               std::optional<symbol_t> &hit) {
  const call_t &call = call_node->get_call();
  ASSERT(call.function_name == "dchain_is_index_allocated" ||
             call.function_name == "dchain_rejuvenate_index",
         "Not a dchain call");

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index = call.args.at("index").expr;

  addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

  obj = dchain_addr;
  keys = {index};

  if (call.function_name == "dchain_is_index_allocated") {
    symbols_t symbols = call_node->get_locally_generated_symbols();
    symbol_t is_allocated;
    bool found = get_symbol(symbols, "dchain_is_index_allocated", is_allocated);
    ASSERT(found, "Symbol dchain_is_index_allocated not found");

    hit = is_allocated;
  }
}

bool get_table_lookup_data(const Call *call_node, addr_t &obj,
                           std::vector<klee::ref<klee::Expr>> &keys,
                           std::vector<klee::ref<klee::Expr>> &values,
                           std::optional<symbol_t> &hit) {
  const call_t &call = call_node->get_call();

  if (call.function_name == "map_get") {
    table_data_from_map_op(call_node, obj, keys, values, hit);
  } else if (call.function_name == "vector_borrow") {
    table_data_from_vector_op(call_node, obj, keys, values, hit);
  } else if (call.function_name == "dchain_is_index_allocated" ||
             call.function_name == "dchain_rejuvenate_index") {
    table_data_from_dchain_op(call_node, obj, keys, values, hit);
  } else {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t>
TableLookupGenerator::speculate(const EP *ep, const Node *node,
                                const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *call_node = static_cast<const Call *>(node);

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  std::vector<klee::ref<klee::Expr>> values;
  std::optional<symbol_t> found;
  if (!get_table_lookup_data(call_node, obj, keys, values, found)) {
    return std::nullopt;
  }

  if (!ctx.can_impl_ds(obj, DSImpl::Tofino_Table)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> TableLookupGenerator::process_node(const EP *ep,
                                                       const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *call_node = static_cast<const Call *>(node);

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  std::vector<klee::ref<klee::Expr>> values;
  std::optional<symbol_t> found;
  if (!get_table_lookup_data(call_node, obj, keys, values, found)) {
    return impls;
  }

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_Table)) {
    return impls;
  }

  Module *module = new TableLookup(node, obj, keys, values, found);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace tofino_cpu
