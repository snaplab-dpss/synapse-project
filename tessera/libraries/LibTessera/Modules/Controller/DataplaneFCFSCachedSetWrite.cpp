#include <LibTessera/Modules/Controller/DataplaneFCFSCachedSetWrite.h>
#include <LibTessera/Modules/Tofino/FCFSCachedSetReadInsert.h>
#include <LibTessera/ExecutionPlan.h>

namespace LibTessera {
namespace Controller {

using LibTessera::Tofino::FCFSCachedSetReadInsert;

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

using Tofino::DS_ID;
using Tofino::Table;

namespace {

struct map_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
};

map_table_data_t get_map_table_update_data(const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "map_put" && "Not a map_put call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;

  const map_table_data_t data = {
      .obj = expr_addr_to_obj_addr(map_addr_expr),
      .key = key,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneFCFSCachedSetWriteFactory::speculate(const EP *ep, const BDDNode *node,
                                                                         const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_put") {
    return {};
  }

  const map_table_data_t data = get_map_table_update_data(call_node);

  if (!speculations.ctx.check_ds_impl(data.obj, DSImpl::Tofino_FCFSCachedSet)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> DataplaneFCFSCachedSetWriteFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_put") {
    return {};
  }

  const map_table_data_t data = get_map_table_update_data(call_node);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_FCFSCachedSet)) {
    return {};
  }

  Module *module  = new DataplaneFCFSCachedSetWrite(node, data.obj, data.key);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneFCFSCachedSetWriteFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  // We don't need this for now.
  return {};
}

} // namespace Controller
} // namespace LibTessera