#include <LibSynapse/Modules/x86/Ignore.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace x86 {

namespace {
bool should_ignore(const LibBDD::Node *node) {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return false;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name == "vector_return") {
    return call_node->is_vector_return_without_modifications();
  }

  return false;
}
} // namespace

std::optional<spec_impl_t> IgnoreFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  return std::nullopt;
}

std::vector<impl_t> IgnoreFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (!should_ignore(node)) {
    return impls;
  }

  Module *module  = new Ignore(node);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> IgnoreFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (!should_ignore(node)) {
    return {};
  }

  return std::make_unique<Ignore>(node);
}

} // namespace x86
} // namespace LibSynapse