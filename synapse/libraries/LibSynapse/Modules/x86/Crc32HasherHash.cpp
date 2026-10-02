#include <LibSynapse/Modules/x86/Crc32HasherHash.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace x86 {

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

addr_t get_hasher(const BDDNode *node) { return expr_addr_to_obj_addr(dynamic_cast<const Call *>(node)->get_call().args.at("hasher").expr); }

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

  EPNode *ep_node = new EPNode(new Crc32HasherHash(node, get_hasher(node)));

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
  return std::make_unique<Crc32HasherHash>(node, get_hasher(node));
}

} // namespace x86
} // namespace LibSynapse
