#include <LibSynapse/Modules/Controller/TableLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using Tofino::Table;

namespace {
void table_data_from_map_op(const LibBDD::Call *call_node, addr_t &obj, std::vector<klee::ref<klee::Expr>> &keys,
                            std::vector<klee::ref<klee::Expr>> &values, std::optional<LibCore::symbol_t> &hit) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "map_get" && "Not a map_get call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  klee::ref<klee::Expr> value_out     = call.args.at("value_out").out;

  LibCore::symbol_t map_has_this_key = call_node->get_local_symbol("map_has_this_key");

  obj    = LibCore::expr_addr_to_obj_addr(map_addr_expr);
  keys   = Table::build_keys(key);
  values = {value_out};
  hit    = map_has_this_key;
}

void table_data_from_vector_op(const LibBDD::Call *call_node, addr_t &obj, std::vector<klee::ref<klee::Expr>> &keys,
                               std::vector<klee::ref<klee::Expr>> &values, std::optional<LibCore::symbol_t> &hit) {
  // We can implement even if we later update the vector's contents!

  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "vector_borrow" && "Not a vector_borrow call");

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;
  klee::ref<klee::Expr> cell             = call.extra_vars.at("borrowed_cell").second;

  obj    = LibCore::expr_addr_to_obj_addr(vector_addr_expr);
  keys   = {index};
  values = {cell};
}

void table_data_from_dchain_op(const LibBDD::Call *call_node, addr_t &obj, std::vector<klee::ref<klee::Expr>> &keys,
                               std::vector<klee::ref<klee::Expr>> &values, std::optional<LibCore::symbol_t> &hit) {
  const LibBDD::call_t &call = call_node->get_call();
  assert((call.function_name == "dchain_is_index_allocated" || call.function_name == "dchain_rejuvenate_index") && "Not a dchain call");

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;

  addr_t dchain_addr = LibCore::expr_addr_to_obj_addr(dchain_addr_expr);

  obj  = dchain_addr;
  keys = {index};

  if (call.function_name == "dchain_is_index_allocated") {
    hit = call_node->get_local_symbol("is_index_allocated");
  }
}

bool get_table_lookup_data(const LibBDD::Call *call_node, addr_t &obj, std::vector<klee::ref<klee::Expr>> &keys,
                           std::vector<klee::ref<klee::Expr>> &values, std::optional<LibCore::symbol_t> &hit) {
  const LibBDD::call_t &call = call_node->get_call();

  if (call.function_name == "map_get") {
    table_data_from_map_op(call_node, obj, keys, values, hit);
  } else if (call.function_name == "vector_borrow") {
    table_data_from_vector_op(call_node, obj, keys, values, hit);
  } else if (call.function_name == "dchain_is_index_allocated" || call.function_name == "dchain_rejuvenate_index") {
    table_data_from_dchain_op(call_node, obj, keys, values, hit);
  } else {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> TableLookupFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);

  if (call_node->is_vector_borrow_value_ignored()) {
    return std::nullopt;
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  std::vector<klee::ref<klee::Expr>> values;
  std::optional<LibCore::symbol_t> found;
  if (!get_table_lookup_data(call_node, obj, keys, values, found)) {
    return std::nullopt;
  }

  if (!ctx.can_impl_ds(obj, DSImpl::Tofino_Table)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> TableLookupFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);

  if (call_node->is_vector_borrow_value_ignored()) {
    return impls;
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  std::vector<klee::ref<klee::Expr>> values;
  std::optional<LibCore::symbol_t> found;
  if (!get_table_lookup_data(call_node, obj, keys, values, found)) {
    return impls;
  }

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_Table)) {
    return impls;
  }

  Module *module  = new TableLookup(node, obj, keys, values, found);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> TableLookupFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);

  if (call_node->is_vector_borrow_value_ignored()) {
    return {};
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  std::vector<klee::ref<klee::Expr>> values;
  std::optional<LibCore::symbol_t> found;
  if (!get_table_lookup_data(call_node, obj, keys, values, found)) {
    return {};
  }

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_Table)) {
    return {};
  }

  return std::make_unique<TableLookup>(node, obj, keys, values, found);
}

} // namespace Controller
} // namespace LibSynapse