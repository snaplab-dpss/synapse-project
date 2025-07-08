#include <LibSynapse/Modules/Controller/DataplaneHHTableDelete.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using Tofino::Table;

namespace {

void get_map_erase_data(const Context &ctx, const LibBDD::Call *call_node, addr_t &obj, std::vector<klee::ref<klee::Expr>> &keys) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "map_erase" && "Not a map_erase call");

  obj  = LibCore::expr_addr_to_obj_addr(call.args.at("map").expr);
  keys = Table::build_keys(call.args.at("key").in, ctx.get_expr_structs());
}

} // namespace

std::optional<spec_impl_t> DataplaneHHTableDeleteFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "map_erase") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  addr_t map_addr                     = LibCore::expr_addr_to_obj_addr(map_addr_expr);

  if (!ctx.can_impl_ds(map_addr, DSImpl::Tofino_HeavyHitterTable)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneHHTableDeleteFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                LibCore::SymbolManager *symbol_manager) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "map_erase") {
    return {};
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  get_map_erase_data(ep->get_ctx(), call_node, obj, keys);

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  Module *module  = new DataplaneHHTableDelete(node, obj, keys);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneHHTableDeleteFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "map_erase") {
    return {};
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  get_map_erase_data(ctx, call_node, obj, keys);

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  return std::make_unique<DataplaneHHTableDelete>(node, obj, keys);
}

} // namespace Controller
} // namespace LibSynapse