#include <LibSynapse/Modules/Tofino/ComputeTable.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;

bool compute_table_matches(const BDDNode *node, const std::string &fn) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }
  return dynamic_cast<const Call *>(node)->get_call().function_name == fn;
}

std::unique_ptr<EP> build_compute_table_ep(const EP *ep, const BDDNode *node, ComputeTable *module, klee::ref<klee::Expr> in, TableMatch match,
                                           const std::vector<table_entry_t> &entries) {
  const u32 capacity = static_cast<u32>(entries.size()) + 1;

  Table *table = new Table(module->get_table_id(), capacity, {in->getWidth()}, {module->get_out()->getWidth()}, TimeAware::No, match, entries);

  EPNode *ep_node            = new EPNode(module);
  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  TofinoContext *tofino_ctx = new_ep->get_mutable_ctx().get_mutable_target_ctx<TofinoContext>();
  tofino_ctx->place(new_ep.get(), node, node->get_id(), table);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return new_ep;
}

} // namespace Tofino
} // namespace LibSynapse
