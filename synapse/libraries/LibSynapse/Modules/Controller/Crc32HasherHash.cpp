#include <LibSynapse/Modules/Controller/Crc32HasherHash.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

bool matches(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }
  return dynamic_cast<const Call *>(node)->get_call().function_name == "crc32_hasher_hash";
}

std::unique_ptr<Crc32HasherHash> build(const BDDNode *node) {
  const call_t &call = dynamic_cast<const Call *>(node)->get_call();
  return std::make_unique<Crc32HasherHash>(node, expr_addr_to_obj_addr(call.args.at("hasher").expr), call.args.at("data").in, call.ret);
}

} // namespace

std::optional<spec_impl_t> Crc32HasherHashFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (!matches(node)) {
    return {};
  }
  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> Crc32HasherHashFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!matches(node)) {
    return {};
  }

  EPNode *ep_node = new EPNode(build(node).release());

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> Crc32HasherHashFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!matches(node)) {
    return {};
  }
  return build(node);
}

} // namespace Controller
} // namespace LibSynapse
