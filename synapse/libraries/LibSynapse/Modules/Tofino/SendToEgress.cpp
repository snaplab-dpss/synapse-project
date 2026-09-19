#include <LibSynapse/Modules/Tofino/SendToEgress.h>
#include <LibSynapse/Modules/Tofino/ModifyHeader.h>
#include <LibSynapse/ExecutionPlan.h>
#include <iostream>
#include <cstdlib>
#include <LibCore/Solver.h>

namespace LibSynapse {
namespace Tofino {

namespace {
// Crossing into egress is only worth considering once the ingress has real work in it.
constexpr size_t MIN_MODULES_BEFORE_EGRESS_CROSSING = 8;

} // namespace

std::optional<spec_impl_t> SendToEgressFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  // Same shape as SendToController's speculation, which is also a transition that consumes no BDD
  // node: copy the context and charge what the transition costs. Here that charge is nothing,
  // which is the whole point of the crossing, and the target does not change either -- the egress
  // is still Tofino.
  //
  // Deliberately no speculation, and not for want of trying: returning one costs nothing and so
  // ties or wins every comparison, which drives the expensive phase-2 lookahead combinatorially.
  // Measured: with a candidate here the search made 0 steps in 150 s where it otherwise makes 161.
  // This is the same reason Recirculate returns nothing, stated more precisely than its comment.
  //
  // The crossing's value reaches the search through the compute-run lookahead in TofinoModule.cpp
  // instead: where the plan would cut an unrolled loop (loop_cut_due), and when the pipeline
  // fills, a free fresh context is tried before a recirculation is charged, so a plan that can
  // use the egress there is predicted to need fewer laps.
  return {};
}

std::vector<impl_t> SendToEgressFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  const EPLeaf active_leaf = ep->get_active_leaf();

  if (!active_leaf.node) {
    return decline("no active leaf");
  }

  // The egress cannot choose a port: ucast_egress_port is written in ingress. So the crossing is
  // legal exactly when the port the packet will leave by is a function of what is already known
  // here. Two things determine it: the device expression of every reachable forward, and, when
  // those disagree, the branch conditions that select between them. Drops contribute nothing, the
  // egress can drop on its own; broadcast needs multicast setup in ingress, so it is out.
  //
  // Deliberately pessimistic in one way: a route several recirculations downstream is judged
  // against what is known here, when an intervening recirculation would really have reset things.
  // Future pass boundaries are plan decisions we cannot see from here, so the conservative reading
  // is the sound one.
  const bdd_node_ids_t &roots = ep->get_target_roots(TargetType::Tofino);
  Symbols available           = node->get_prev_symbols(roots);
  available.add(ep->get_bdd()->get_device());
  available.add(ep->get_bdd()->get_time());

  std::unordered_set<std::string> known;
  for (const symbol_t &symbol : available.get()) {
    known.insert(symbol.name);
  }

  const auto computable_here = [&known](klee::ref<klee::Expr> expr) {
    if (expr.isNull()) {
      return true;
    }
    for (const std::string &symbol_name : symbol_t::get_symbols_names(expr)) {
      if (!known.contains(symbol_name)) {
        return false;
      }
    }
    return true;
  };

  bool legal              = true;
  bool work_remains       = false;
  const char *why_illegal = nullptr;
  std::vector<klee::ref<klee::Expr>> devices;
  std::vector<klee::ref<klee::Expr>> conditions;

  node->visit_nodes([&](const BDDNode *future) {
    switch (future->get_type()) {
    case BDDNodeType::Route: {
      const LibBDD::Route *route = static_cast<const LibBDD::Route *>(future);
      switch (route->get_operation()) {
      case LibBDD::RouteOp::Drop:
        break;
      case LibBDD::RouteOp::Broadcast:
        legal       = false;
        why_illegal = "broadcast ahead";
        break;
      case LibBDD::RouteOp::Forward:
        if (!computable_here(route->get_dst_device())) {
          legal       = false;
          why_illegal = "forward port not computable at the cut";
        } else {
          devices.push_back(route->get_dst_device());
        }
        break;
      }
      break;
    }
    case BDDNodeType::Branch:
      work_remains = true;
      conditions.push_back(static_cast<const LibBDD::Branch *>(future)->get_condition());
      break;
    case BDDNodeType::Call:
      // Handing a chunk back unchanged lowers to nothing: a crossing with only that and the
      // routes ahead buys no depth and costs the egress pass, its headers and, past a rewrite the
      // ingress already made, the checksum update the egress cannot do.
      if (!ModifyHeaderFactory::returns_chunk_unchanged(future)) {
        work_remains = true;
      }
      break;
    default:
      work_remains = true;
      break;
    }
    return legal ? BDDNodeVisitAction::Continue : BDDNodeVisitAction::Stop;
  });

  // When every forward writes the same port the branches between here and them cannot change it,
  // so their conditions need not be known. When they disagree, the choice itself has to be made
  // in ingress, which means every condition that can lead to a different port must be known here.
  if (legal && devices.size() > 1) {
    bool all_agree = true;
    for (size_t i = 1; i < devices.size() && all_agree; i++) {
      all_agree = solver_toolbox.are_exprs_always_equal(devices[0], devices[i]);
    }
    if (!all_agree) {
      for (const klee::ref<klee::Expr> &condition : conditions) {
        if (!computable_here(condition)) {
          legal       = false;
          why_illegal = "branch selecting between different ports not computable at the cut";
          break;
        }
      }
    }
  }

  if (!legal) {
    return decline(why_illegal);
  }

  // The egress holds no tables or registers, so crossing while data-structure work is still to
  // come costs the packet a lap to get back. Only refuse when *every* way forward runs into one:
  // if some path avoids them, that path is served fine and the others can recirculate.
  //
  // Judging it on the whole reachable subtree instead -- as this first did -- refuses far too
  // much: measured, it was turning down 15 crossings against 8 accepted, including the ones
  // partway down the SipHash chain that the hand-written solution takes, whose own path has no
  // data structure ahead at all.
  if (TofinoModuleFactory::data_structure_call_ahead(node)) {
    return decline("every way forward runs into a data-structure call before a route");
  }
  if (!work_remains) {
    return decline("nothing left to do past the cut");
  }

  const klee::ref<klee::Expr> dst_device = devices.empty() ? klee::ref<klee::Expr>() : devices[0];

  // One crossing per pass: there is no way back to ingress without recirculating, and a
  // recirculation needs a port, which egress cannot set. The same walk counts what this pass has
  // already placed: crossing buys depth, so offering it before the ingress holds any real work
  // only multiplies the search space, which it does violently (2e61 estimated states against the
  // 2e11 of a plain ingress search).
  size_t modules_this_pass = 0;
  for (const EPNode *prev = active_leaf.node; prev; prev = prev->get_prev()) {
    const Module *prev_module = prev->get_module();
    if (!prev_module || prev_module->get_target() != TargetType::Tofino) {
      break;
    }
    if (prev_module->get_type() == ModuleType::Tofino_SendToEgress) {
      return decline("this pass already crossed");
    }
    if (prev_module->get_type() == ModuleType::Tofino_Recirculate) {
      break;
    }
    modules_this_pass++;
  }

  // The floor keeps the search from multiplying states by offering a crossing before the ingress
  // holds any real work. It also deadlocks a plan whose gress has just filled: right after a
  // recirculation there are no modules in the pass yet, so crossing is refused, and the only way
  // to earn the modules is to place compute the exhausted gress cannot take. Overridable while
  // that interaction is under investigation.
  if (modules_this_pass < MIN_MODULES_BEFORE_EGRESS_CROSSING) {
    return decline("fewer modules in this pass than the floor (" + std::to_string(modules_this_pass) + " < " +
                   std::to_string(MIN_MODULES_BEFORE_EGRESS_CROSSING) + ")");
  }

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  // Deliberately no perf oracle update: unlike a recirculation this costs no extra pass through
  // the traffic manager.

  const Symbols symbols = get_relevant_dataplane_state(ep, node);

  // From here the computation is charged to the egress, which has a budget of its own. That is
  // what the crossing buys, and why it is worth a header's width of state: a recirculation does
  // not do it, since one gress is laid out as a whole.
  new_ep->get_mutable_ctx().get_mutable_target_ctx<TofinoContext>()->get_mutable_tna().pipeline.cross_to_egress();

  Module *module  = new SendToEgress(node, symbols, dst_device);
  EPNode *ep_node = new EPNode(module);

  // The node itself is not implemented here, only moved to the other pipeline.
  const EPLeaf leaf(ep_node, node);
  new_ep->process_leaf(ep_node, {leaf}, EP::BDDNodeMarking::Unprocessed);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));

  return impls;
}

std::unique_ptr<Module> SendToEgressFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  // Like recirculation, there is no standalone module to create.
  return {};
}

} // namespace Tofino
} // namespace LibSynapse
