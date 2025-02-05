#include <LibSynapse/Modules/Tofino/LPMForward.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

namespace {
std::unique_ptr<LibBDD::BDD> delete_future_fwd_logic(EP *ep, const LibBDD::Node *node, std::vector<const LibBDD::Node *> fwd_logic) {
  const LibBDD::BDD *old_bdd           = ep->get_bdd();
  std::unique_ptr<LibBDD::BDD> new_bdd = std::make_unique<LibBDD::BDD>(*old_bdd);

  for (const LibBDD::Node *node : fwd_logic) {
    if (node->get_type() == LibBDD::NodeType::Branch) {
      bool arbitrary_direction_to_keep = false;
      new_bdd->delete_branch(node->get_id(), arbitrary_direction_to_keep);
    } else {
      new_bdd->delete_non_branch(node->get_id());
    }
  }

  return new_bdd;
}
} // namespace

std::optional<spec_impl_t> LPMForwardFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  std::vector<const LibBDD::Node *> fwd_logic;
  if (!ep->get_bdd()->is_fwd_pattern_depending_on_lpm(node, fwd_logic)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  spec_impl_t spec_impl(decide(ep, node), new_ctx);

  for (const LibBDD::Node *fwd_logic_node : fwd_logic) {
    if (fwd_logic_node->get_type() == LibBDD::NodeType::Route) {
      const LibBDD::Route *route_node = dynamic_cast<const LibBDD::Route *>(fwd_logic_node);
      LibBDD::RouteOp op              = route_node->get_operation();

      switch (op) {
      case LibBDD::RouteOp::Forward: {
        int dst_device = route_node->get_dst_device();
        new_ctx.get_mutable_perf_oracle().add_fwd_traffic(dst_device, new_ctx.get_profiler().get_hr(fwd_logic_node));
      } break;
      case LibBDD::RouteOp::Drop: {
        new_ctx.get_mutable_perf_oracle().add_dropped_traffic(new_ctx.get_profiler().get_hr(fwd_logic_node));
      } break;
      case LibBDD::RouteOp::Broadcast: {
        assert(false && "TODO: broadcast logic");
      } break;
      }
    }

    spec_impl.skip.insert(fwd_logic_node->get_id());
  }

  return spec_impl;
}

std::vector<impl_t> LPMForwardFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  std::vector<const LibBDD::Node *> fwd_logic;
  if (!ep->get_bdd()->is_fwd_pattern_depending_on_lpm(node, fwd_logic)) {
    return impls;
  }

  Module *module  = new LPMForward(node);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  std::unique_ptr<LibBDD::BDD> new_bdd = delete_future_fwd_logic(new_ep, node, fwd_logic);

  EPLeaf leaf(ep_node, nullptr);
  new_ep->process_leaf(ep_node, {leaf});
  new_ep->replace_bdd(std::move(new_bdd));

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->parser_accept(ep, node);

  std::vector<int> past_recirculations = ep_node->get_past_recirculations();

  for (const LibBDD::Node *fwd_logic_node : fwd_logic) {
    if (fwd_logic_node->get_type() == LibBDD::NodeType::Route) {
      const LibBDD::Route *route_node = dynamic_cast<const LibBDD::Route *>(fwd_logic_node);
      LibBDD::RouteOp op              = route_node->get_operation();

      // Careful, grab from the old BDD that actually has this node.
      hit_rate_t fwd_logic_hr = ep->get_ctx().get_profiler().get_hr(fwd_logic_node);

      switch (op) {
      case LibBDD::RouteOp::Forward: {
        int dst_device = route_node->get_dst_device();
        new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_fwd_traffic(dst_device,
                                                                            new_ep->get_node_egress(fwd_logic_hr, past_recirculations));
      } break;
      case LibBDD::RouteOp::Drop: {

        new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_dropped_traffic(fwd_logic_hr);
      } break;
      case LibBDD::RouteOp::Broadcast: {
        assert(false && "TODO: broadcast logic");
      } break;
      }
    }
  }

  return impls;
}

} // namespace Tofino
} // namespace LibSynapse