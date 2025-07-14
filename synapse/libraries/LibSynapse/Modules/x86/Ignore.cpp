#include <LibSynapse/Modules/x86/Ignore.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace x86 {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {
bool should_ignore(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name == "vector_return") {
    return call_node->is_vector_return_without_modifications();
  }

  return false;
}
} // namespace

std::optional<spec_impl_t> IgnoreFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const { return {}; }

std::vector<impl_t> IgnoreFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!should_ignore(node)) {
    return {};
  }

  Module *module  = new Ignore(node);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> IgnoreFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!should_ignore(node)) {
    return {};
  }

  return std::make_unique<Ignore>(node);
}

} // namespace x86
} // namespace LibSynapse