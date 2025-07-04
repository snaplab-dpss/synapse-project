#include <LibSynapse/Modules/Controller/DataplaneVectorTableUpdate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

namespace {

struct vector_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
};

vector_table_data_t get_vector_table_data(const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "vector_return" && "Not a vector_return call");

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;
  klee::ref<klee::Expr> value            = call.args.at("value").in;

  vector_table_data_t data = {
      .obj   = LibCore::expr_addr_to_obj_addr(vector_addr_expr),
      .key   = index,
      .value = value,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneVectorTableUpdateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "vector_return") {
    return std::nullopt;
  }

  if (call_node->is_vector_return_without_modifications()) {
    return std::nullopt;
  }

  vector_table_data_t data = get_vector_table_data(call_node);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_VectorTable)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneVectorTableUpdateFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                    LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "vector_return") {
    if (ep->get_id() == 194 && node->get_id() == 175) {
      std::cerr << "Not a vector return\n";
      dbg_pause();
    }
    return impls;
  }

  if (call_node->is_vector_return_without_modifications()) {
    if (ep->get_id() == 194 && node->get_id() == 175) {
      std::cerr << "Not a vector return without modifications\n";
      dbg_pause();
    }
    return impls;
  }

  vector_table_data_t data = get_vector_table_data(call_node);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_VectorTable)) {
    if (ep->get_id() == 194 && node->get_id() == 175) {
      std::cerr << "Not a Vector Table\n";
      std::cerr << "Object: " << data.obj << "\n";
      ep->get_ctx().debug();
      dbg_pause();
    }
    return impls;
  }

  Module *module  = new DataplaneVectorTableUpdate(node, data.obj, data.key, data.value);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> DataplaneVectorTableUpdateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "vector_return") {
    return {};
  }

  if (call_node->is_vector_return_without_modifications()) {
    return {};
  }

  vector_table_data_t data = get_vector_table_data(call_node);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_VectorTable)) {
    return {};
  }

  return std::make_unique<DataplaneVectorTableUpdate>(node, data.obj, data.key, data.value);
}

} // namespace Controller
} // namespace LibSynapse