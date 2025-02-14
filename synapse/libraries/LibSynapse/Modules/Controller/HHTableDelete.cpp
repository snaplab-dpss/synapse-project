#include <LibSynapse/Modules/Controller/HHTableDelete.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using Tofino::Table;

namespace {
void get_map_erase_data(const LibBDD::Call *call_node, addr_t &obj, std::vector<klee::ref<klee::Expr>> &keys) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "map_erase" && "Not a map_erase call");

  obj  = LibCore::expr_addr_to_obj_addr(call.args.at("map").expr);
  keys = Table::build_keys(call.args.at("key").in);
}
} // namespace

std::optional<spec_impl_t> HHTableDeleteFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
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

std::vector<impl_t> HHTableDeleteFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                       LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "map_erase") {
    return impls;
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  get_map_erase_data(call_node, obj, keys);

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    return impls;
  }

  Module *module  = new HHTableDelete(node, obj, keys);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> HHTableDeleteFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
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
  get_map_erase_data(call_node, obj, keys);

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  return std::make_unique<HHTableDelete>(node, obj, keys);
}

} // namespace Controller
} // namespace LibSynapse