#include <LibSynapse/Modules/Tofino/SendToController.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibBDD/Visitors/BDDVisualizer.h>
#include <LibSynapse/Visualizers/EPVisualizer.h>

#include <LibSynapse/Modules/Controller/ParseHeader.h>
#include <LibSynapse/Modules/Controller/If.h>
#include <LibSynapse/Modules/Controller/Then.h>
#include <LibSynapse/Modules/Controller/Else.h>
#include <LibSynapse/Modules/Controller/AbortTransaction.h>
#include <LibSynapse/Modules/Controller/DataplaneMapTableLookup.h>
#include <LibSynapse/Modules/Controller/DataplaneGuardedMapTableLookup.h>
#include <LibSynapse/Modules/Controller/DataplaneGuardedMapTableGuardCheck.h>
#include <LibSynapse/Modules/Controller/DataplaneVectorTableLookup.h>
#include <LibSynapse/Modules/Controller/DataplaneDchainTableIsIndexAllocated.h>
#include <LibSynapse/Modules/Controller/DataplaneFCFSCachedTableRead.h>
#include <LibSynapse/Modules/Controller/DataplaneHHTableRead.h>

#include <LibSynapse/Modules/Tofino/ParserExtraction.h>
#include <LibSynapse/Modules/Tofino/If.h>
#include <LibSynapse/Modules/Tofino/MapTableLookup.h>
#include <LibSynapse/Modules/Tofino/GuardedMapTableLookup.h>
#include <LibSynapse/Modules/Tofino/GuardedMapTableGuardCheck.h>
#include <LibSynapse/Modules/Tofino/VectorTableLookup.h>
#include <LibSynapse/Modules/Tofino/DchainTableLookup.h>
#include <LibSynapse/Modules/Tofino/FCFSCachedTableRead.h>
#include <LibSynapse/Modules/Tofino/HHTableRead.h>
#include <LibSynapse/Modules/Tofino/IntegerAllocatorIsAllocated.h>
#include <LibSynapse/Modules/Tofino/LPMLookup.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

namespace {
std::unique_ptr<BDD> replicate_hdr_parsing_ops(const EP *ep, const BDDNode *node, const BDDNode *&next) {
  std::vector<const Call *> prev_borrows = node->get_prev_functions({"packet_borrow_next_chunk"}, ep->get_target_roots(ep->get_active_target()));
  std::vector<const Call *> prev_returns = node->get_prev_functions({"packet_return_chunk"}, ep->get_target_roots(ep->get_active_target()));

  std::vector<const BDDNode *> hdr_parsing_ops;
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_borrows.begin(), prev_borrows.end());
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_returns.begin(), prev_returns.end());

  if (hdr_parsing_ops.empty()) {
    return nullptr;
  }

  const BDD *old_bdd = ep->get_bdd();

  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);
  new_bdd->add_cloned_non_branches(node->get_id(), hdr_parsing_ops);

  next = new_bdd->get_node_by_id(node->get_id());

  return new_bdd;
}
} // namespace

std::optional<spec_impl_t> SendToControllerFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  Context new_ctx = ctx;

  // Don't send to the controller if the node is already a route.
  if (node->get_type() == BDDNodeType::Route) {
    return {};
  }

  const hit_rate_t hr = new_ctx.get_profiler().get_hr(node);
  new_ctx.get_mutable_perf_oracle().add_controller_traffic(hr);

  const std::string &instance_id = ep->get_active_target().instance_id;

  spec_impl_t spec_impl(decide(ep, node), new_ctx);
  spec_impl.next_target = TargetType(TargetArchitecture::Controller, instance_id);

  return spec_impl;
}

struct initial_controller_logic_t {
  EPNode *head;
  EPNode *tail;
  Symbols extra_symbols;

  void update(EPNode *ep_node) {
    assert(ep_node);
    if (head == nullptr) {
      head = ep_node;
      tail = ep_node;
    } else {
      assert(tail);
      tail->set_children(ep_node);
      ep_node->set_prev(tail);
      tail = ep_node;
    }
  }

  void set_tail(EPNode *ep_node) {
    assert(ep_node);
    tail = ep_node;
  }
};

initial_controller_logic_t build_initial_controller_logic(const EPLeaf active_leaf) {
  initial_controller_logic_t initial_controller_logic{.head = nullptr, .tail = nullptr, .extra_symbols = {}};

  struct prev_module_t {
    const Module *module;
    size_t chosen_child;
  };

  bool branch_conditions_found{false};

  std::vector<prev_module_t> prev_modules;
  const EPNode *prev_node = active_leaf.node;
  const EPNode *curr_node = nullptr;
  while (prev_node) {
    const Module *module    = prev_node->get_module();
    std::string instance_id = module->get_type().instance_id;
    assert(module && "EPNode without module");

    prev_module_t prev_module{.module = module, .chosen_child = 0};

    const std::vector<EPNode *> &children = prev_node->get_children();
    for (size_t i = 0; i < children.size(); i++) {
      if (children[i] == curr_node) {
        prev_module.chosen_child = i;
        break;
      }
    }

    if (module->get_type().type == ModuleCategory::Tofino_If || module->get_type().type == ModuleCategory::Tofino_ParserExtraction) {
      prev_modules.insert(prev_modules.begin(), prev_module);
      branch_conditions_found = true;
    } else if (module->get_target() == TargetArchitecture::Tofino) {
      const TofinoModule *tofino_module  = dynamic_cast<const TofinoModule *>(module);
      const std::unordered_set<DS_ID> ds = tofino_module->get_generated_ds();
      if (!ds.empty()) {
        prev_modules.insert(prev_modules.begin(), prev_module);
      }
    }

    curr_node = prev_node;
    prev_node = prev_node->get_prev();
  }

  for (const prev_module_t &prev : prev_modules) {
    std::string instance_id = prev.module->get_type().instance_id;
    if (prev.module->get_type().type == ModuleCategory::Tofino_ParserExtraction) {
      const ParserExtraction *parser_extraction_module = dynamic_cast<const ParserExtraction *>(prev.module);

      klee::ref<klee::Expr> length = solver_toolbox.exprBuilder->Constant(parser_extraction_module->get_length(), 16);

      Controller::ParseHeader *ctrl_parse_header =
          new Controller::ParseHeader(ModuleType(ModuleCategory::Controller_ParseHeader, instance_id), active_leaf.next,
                                      parser_extraction_module->get_hdr_addr(), parser_extraction_module->get_hdr(), length);

      EPNode *parse_header_ep_node = new EPNode(ctrl_parse_header);
      initial_controller_logic.update(parse_header_ep_node);
    }
  }

  if (!branch_conditions_found) {
    return initial_controller_logic;
  }

  for (const prev_module_t &prev : prev_modules) {
    // Why the exhaustive switch case statement here?
    // Because later if we add new modules the compiler warns us to update this.
    // Otherwise we might forget to add a new module here, leading to a hard bug to catch.
    switch (prev.module->get_type().type) {
      // ========================================
      // Conditions that triggered the packet
      // being sent to the controller
      // ========================================

    case ModuleCategory::Tofino_If: {
      const If *if_module = dynamic_cast<const If *>(prev.module);

      const klee::ref<klee::Expr> condition = if_module->get_original_condition();
      const BDDNode *if_node                = if_module->get_node();
      assert(if_node);

      initial_controller_logic.extra_symbols.add(if_node->get_used_symbols());

      Controller::If *ctrl_if =
          new Controller::If(ModuleType(ModuleCategory::Controller_If, prev.module->get_type().instance_id), active_leaf.next, condition);
      Controller::Then *ctrl_then =
          new Controller::Then(ModuleType(ModuleCategory::Controller_Then, prev.module->get_type().instance_id), active_leaf.next);
      Controller::Else *ctrl_else =
          new Controller::Else(ModuleType(ModuleCategory::Controller_Else, prev.module->get_type().instance_id), active_leaf.next);
      Controller::AbortTransaction *ctrl_abort_transaction = new Controller::AbortTransaction(
          ModuleType(ModuleCategory::Controller_AbortTransaction, prev.module->get_type().instance_id), active_leaf.next);

      EPNode *if_ep_node                = new EPNode(ctrl_if);
      EPNode *then_ep_node              = new EPNode(ctrl_then);
      EPNode *else_ep_node              = new EPNode(ctrl_else);
      EPNode *abort_transaction_ep_node = new EPNode(ctrl_abort_transaction);

      // Don't pass a constraint to the EP if node, otherwise it will later break the profiler.
      // We don't really care about it also, this is just boilerplate for correctness.
      if_ep_node->set_children(nullptr, then_ep_node, else_ep_node);
      then_ep_node->set_prev(if_ep_node);
      else_ep_node->set_prev(if_ep_node);

      initial_controller_logic.update(if_ep_node);

      // If the condition holds no longer true, we abort the transaction.
      // Update the tail with the next ep node.
      assert(prev.chosen_child == 0 || prev.chosen_child == 1);
      if (prev.chosen_child == 0) {
        else_ep_node->set_children(abort_transaction_ep_node);
        abort_transaction_ep_node->set_prev(else_ep_node);
        initial_controller_logic.set_tail(then_ep_node);
      } else {
        then_ep_node->set_children(abort_transaction_ep_node);
        abort_transaction_ep_node->set_prev(then_ep_node);
        initial_controller_logic.set_tail(else_ep_node);
      }
    } break;

      // ========================================
      // Controller writable data structure logic
      // ========================================

    case ModuleCategory::Tofino_MapTableLookup: {
      const MapTableLookup *map_table_lookup = dynamic_cast<const MapTableLookup *>(prev.module);

      Controller::DataplaneMapTableLookup *ctrl_map_table_lookup = new Controller::DataplaneMapTableLookup(
          ModuleType(ModuleCategory::Controller_DataplaneMapTableLookup, prev.module->get_type().instance_id), active_leaf.next,
          map_table_lookup->get_obj(), map_table_lookup->get_original_key(), map_table_lookup->get_value(), map_table_lookup->get_hit());

      EPNode *map_table_lookup_ep_node = new EPNode(ctrl_map_table_lookup);
      initial_controller_logic.update(map_table_lookup_ep_node);
    } break;
    case ModuleCategory::Tofino_GuardedMapTableLookup: {
      const GuardedMapTableLookup *guarded_map_table_lookup = dynamic_cast<const GuardedMapTableLookup *>(prev.module);

      Controller::DataplaneGuardedMapTableLookup *ctrl_guarded_map_table_lookup = new Controller::DataplaneGuardedMapTableLookup(
          ModuleType(ModuleCategory::Controller_DataplaneGuardedMapTableLookup, prev.module->get_type().instance_id), active_leaf.next,
          guarded_map_table_lookup->get_obj(), guarded_map_table_lookup->get_original_key(), guarded_map_table_lookup->get_value(),
          guarded_map_table_lookup->get_hit());

      EPNode *guarded_map_table_lookup_ep_node = new EPNode(ctrl_guarded_map_table_lookup);
      initial_controller_logic.update(guarded_map_table_lookup_ep_node);
    } break;
    case ModuleCategory::Tofino_GuardedMapTableGuardCheck: {
      const GuardedMapTableGuardCheck *guarded_map_table_guard_check = dynamic_cast<const GuardedMapTableGuardCheck *>(prev.module);

      Controller::DataplaneGuardedMapTableGuardCheck *ctrl_guarded_map_table_guard_check = new Controller::DataplaneGuardedMapTableGuardCheck(
          ModuleType(ModuleCategory::Controller_DataplaneGuardedMapTableGuardCheck, prev.module->get_type().instance_id), active_leaf.next,
          guarded_map_table_guard_check->get_obj(), guarded_map_table_guard_check->get_guard_allow(),
          guarded_map_table_guard_check->get_guard_allow_condition());

      EPNode *guarded_map_table_guard_check_ep_node = new EPNode(ctrl_guarded_map_table_guard_check);
      initial_controller_logic.update(guarded_map_table_guard_check_ep_node);
    } break;
    case ModuleCategory::Tofino_VectorTableLookup: {
      const VectorTableLookup *vector_table_lookup = dynamic_cast<const VectorTableLookup *>(prev.module);

      Controller::DataplaneVectorTableLookup *ctrl_vector_table_lookup = new Controller::DataplaneVectorTableLookup(
          ModuleType(ModuleCategory::Controller_DataplaneVectorTableLookup, prev.module->get_type().instance_id), active_leaf.next,
          vector_table_lookup->get_obj(), vector_table_lookup->get_key(), vector_table_lookup->get_value());

      EPNode *vector_table_lookup_ep_node = new EPNode(ctrl_vector_table_lookup);
      initial_controller_logic.update(vector_table_lookup_ep_node);
    } break;
    case ModuleCategory::Tofino_DchainTableLookup: {
      const DchainTableLookup *dchain_table_lookup = dynamic_cast<const DchainTableLookup *>(prev.module);

      if (!dchain_table_lookup->get_hit().has_value()) {
        break;
      }

      Controller::DataplaneDchainTableIsIndexAllocated *ctrl_dchain_is_index_allocated = new Controller::DataplaneDchainTableIsIndexAllocated(
          ModuleType(ModuleCategory::Controller_DataplaneDchainTableIsIndexAllocated, prev.module->get_type().instance_id), active_leaf.next,
          dchain_table_lookup->get_obj(), dchain_table_lookup->get_key(), dchain_table_lookup->get_hit().value());

      EPNode *dchain_is_index_allocated_ep_node = new EPNode(ctrl_dchain_is_index_allocated);
      initial_controller_logic.update(dchain_is_index_allocated_ep_node);
    } break;
    case ModuleType::Tofino_FCFSCachedTableRead: {
      const FCFSCachedTableRead *fcfs_cached_table_read = dynamic_cast<const FCFSCachedTableRead *>(prev.module);

      Controller::DataplaneFCFSCachedTableRead *ctrl_fcfs_cached_table_read = new Controller::DataplaneFCFSCachedTableRead(
          active_leaf.next, fcfs_cached_table_read->get_fcfs_cached_table_id(), fcfs_cached_table_read->get_obj(),
          fcfs_cached_table_read->get_original_key(), fcfs_cached_table_read->get_map_has_this_key());

      EPNode *fcfs_cached_table_read_ep_node = new EPNode(ctrl_fcfs_cached_table_read);
      initial_controller_logic.update(fcfs_cached_table_read_ep_node);
    } break;
    case ModuleCategory::Tofino_HHTableRead: {
      const HHTableRead *hh_table_read = dynamic_cast<const HHTableRead *>(prev.module);

      Controller::DataplaneHHTableRead *ctrl_hh_table_read = new Controller::DataplaneHHTableRead(
          ModuleType(ModuleCategory::Controller_DataplaneHHTableRead, prev.module->get_type().instance_id), active_leaf.next,
          hh_table_read->get_obj(), hh_table_read->get_original_key(), hh_table_read->get_value(), hh_table_read->get_hit());

      EPNode *hh_table_read_ep_node = new EPNode(ctrl_hh_table_read);
      initial_controller_logic.update(hh_table_read_ep_node);
    } break;
    case ModuleCategory::Tofino_IntegerAllocatorIsAllocated: {
      panic("TODO: implement controller constraints checker logic for IntegerAllocatorIsAllocated");
    } break;
    case ModuleCategory::Tofino_LPMLookup: {
      panic("TODO: implement controller constraints checker logic for LPMLookup");
    } break;

      // ========================================
      // We don't care about hese
      // ========================================

    case ModuleType::InvalidModule:
    case ModuleType::Tofino_Ignore:
    case ModuleType::Tofino_SendToController:
    case ModuleType::Tofino_Forward:
    case ModuleType::Tofino_Drop:
    case ModuleType::Tofino_Broadcast:
    case ModuleType::Tofino_Recirculate:
    case ModuleType::Tofino_Then:
    case ModuleType::Tofino_Else:
    case ModuleType::Tofino_ParserCondition:
    case ModuleType::Tofino_ParserExtraction:
    case ModuleType::Tofino_ParserReject:
    case ModuleType::Tofino_ModifyHeader:
    case ModuleType::Tofino_VectorRegisterLookup:
    case ModuleType::Tofino_VectorRegisterUpdate:
    case ModuleType::Tofino_FCFSCachedTableReadWrite:
    case ModuleType::Tofino_FCFSCachedTableWrite:
    case ModuleType::Tofino_MeterUpdate:
    case ModuleType::Tofino_HHTableOutOfBandUpdate:
    case ModuleType::Tofino_IntegerAllocatorRejuvenate:
    case ModuleType::Tofino_IntegerAllocatorAllocate:
    case ModuleType::Tofino_CMSQuery:
    case ModuleType::Tofino_CMSIncrement:
    case ModuleType::Tofino_CMSIncAndQuery:
    case ModuleType::Tofino_CuckooHashTableReadWrite:
    case ModuleType::Controller_Ignore:
    case ModuleType::Controller_ParseHeader:
    case ModuleType::Controller_ModifyHeader:
    case ModuleType::Controller_ChecksumUpdate:
    case ModuleType::Controller_If:
    case ModuleType::Controller_Then:
    case ModuleType::Controller_Else:
    case ModuleType::Controller_Forward:
    case ModuleType::Controller_Broadcast:
    case ModuleType::Controller_Drop:
    case ModuleType::Controller_AbortTransaction:
    case ModuleType::Controller_DataplaneMapTableAllocate:
    case ModuleType::Controller_DataplaneMapTableLookup:
    case ModuleType::Controller_DataplaneMapTableUpdate:
    case ModuleType::Controller_DataplaneMapTableDelete:
    case ModuleType::Controller_DataplaneGuardedMapTableAllocate:
    case ModuleType::Controller_DataplaneGuardedMapTableLookup:
    case ModuleType::Controller_DataplaneGuardedMapTableGuardCheck:
    case ModuleType::Controller_DataplaneGuardedMapTableUpdate:
    case ModuleType::Controller_DataplaneGuardedMapTableDelete:
    case ModuleType::Controller_DataplaneVectorTableAllocate:
    case ModuleType::Controller_DataplaneVectorTableLookup:
    case ModuleType::Controller_DataplaneVectorTableUpdate:
    case ModuleType::Controller_DataplaneDchainTableAllocate:
    case ModuleType::Controller_DataplaneDchainTableIsIndexAllocated:
    case ModuleType::Controller_DataplaneDchainTableAllocateNewIndex:
    case ModuleType::Controller_DataplaneDchainTableFreeIndex:
    case ModuleType::Controller_DataplaneDchainTableRefreshIndex:
    case ModuleType::Controller_DataplaneVectorRegisterAllocate:
    case ModuleType::Controller_DataplaneVectorRegisterLookup:
    case ModuleType::Controller_DataplaneVectorRegisterUpdate:
    case ModuleType::Controller_DataplaneFCFSCachedTableAllocate:
    case ModuleType::Controller_DataplaneFCFSCachedTableRead:
    case ModuleType::Controller_DataplaneFCFSCachedTableWrite:
    case ModuleType::Controller_DataplaneHHTableAllocate:
    case ModuleType::Controller_DataplaneHHTableRead:
    case ModuleType::Controller_DataplaneHHTableUpdate:
    case ModuleType::Controller_DataplaneHHTableIsIndexAllocated:
    case ModuleType::Controller_DataplaneHHTableDelete:
    case ModuleType::Controller_DataplaneHHTableOutOfBandUpdate:
    case ModuleType::Controller_DataplaneIntegerAllocatorAllocate:
    case ModuleType::Controller_DataplaneIntegerAllocatorFreeIndex:
    case ModuleType::Controller_DataplaneMeterAllocate:
    case ModuleType::Controller_DataplaneMeterInsert:
    case ModuleType::Controller_DataplaneCMSAllocate:
    case ModuleType::Controller_DataplaneCuckooHashTableAllocate:
    case ModuleType::Controller_DataplaneCMSQuery:
    case ModuleType::Controller_DchainAllocate:
    case ModuleType::Controller_DchainAllocateNewIndex:
    case ModuleType::Controller_DchainRejuvenateIndex:
    case ModuleType::Controller_DchainIsIndexAllocated:
    case ModuleType::Controller_DchainFreeIndex:
    case ModuleType::Controller_VectorAllocate:
    case ModuleType::Controller_VectorRead:
    case ModuleType::Controller_VectorWrite:
    case ModuleType::Controller_MapAllocate:
    case ModuleType::Controller_MapGet:
    case ModuleType::Controller_MapPut:
    case ModuleType::Controller_MapErase:
    case ModuleType::Controller_ChtAllocate:
    case ModuleType::Controller_ChtFindBackend:
    case ModuleType::Controller_HashObj:
    case ModuleType::Controller_TokenBucketAllocate:
    case ModuleType::Controller_TokenBucketIsTracing:
    case ModuleType::Controller_TokenBucketTrace:
    case ModuleType::Controller_TokenBucketUpdateAndCheck:
    case ModuleType::Controller_TokenBucketExpire:
    case ModuleType::Controller_CMSAllocate:
    case ModuleType::Controller_CMSUpdate:
    case ModuleType::Controller_CMSQuery:
    case ModuleType::Controller_CMSIncrement:
    case ModuleType::Controller_CMSCountMin:
    case ModuleType::x86_Ignore:
    case ModuleType::x86_If:
    case ModuleType::x86_Then:
    case ModuleType::x86_Else:
    case ModuleType::x86_Forward:
    case ModuleType::x86_Drop:
    case ModuleType::x86_Broadcast:
    case ModuleType::x86_ParseHeader:
    case ModuleType::x86_ModifyHeader:
    case ModuleType::x86_ChecksumUpdate:
    case ModuleType::x86_MapGet:
    case ModuleType::x86_MapPut:
    case ModuleType::x86_MapErase:
    case ModuleType::x86_VectorRead:
    case ModuleType::x86_VectorWrite:
    case ModuleType::x86_DchainAllocateNewIndex:
    case ModuleType::x86_DchainRejuvenateIndex:
    case ModuleType::x86_DchainIsIndexAllocated:
    case ModuleType::x86_DchainFreeIndex:
    case ModuleType::x86_CMSIncrement:
    case ModuleType::x86_CMSCountMin:
    case ModuleType::x86_CMSPeriodicCleanup:
    case ModuleType::x86_ExpireItemsSingleMap:
    case ModuleType::x86_ExpireItemsSingleMapIteratively:
    case ModuleType::x86_ChtFindBackend:
    case ModuleType::x86_HashObj:
    case ModuleType::x86_TokenBucketIsTracing:
    case ModuleType::x86_TokenBucketTrace:
    case ModuleType::x86_TokenBucketUpdateAndCheck:
    case ModuleType::x86_TokenBucketExpire:
      break;
    }
  }

  return initial_controller_logic;
}

std::vector<impl_t> SendToControllerFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  const EPLeaf active_leaf = ep->get_active_leaf();

  // We can't send to the controller if a forwarding decision was already made.
  if (active_leaf.node && active_leaf.node->forwarding_decision_already_made()) {
    return {};
  }

  // Don't send to the controller if the node is already a route.
  if (node->get_type() == BDDNodeType::Route) {
    return {};
  }

  // Otherwise we can always send to the controller, at any point in time.
  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const initial_controller_logic_t initial_controller_logic = build_initial_controller_logic(active_leaf);

  Symbols symbols = get_relevant_dataplane_state(ep, node, target);
  symbols.add(initial_controller_logic.extra_symbols);
  symbols.remove("packet_chunks");
  symbols.remove("next_time");

  Module *module   = new SendToController(type, node, symbols);
  EPNode *s2c_node = new EPNode(module);

  EPNode *ep_node_leaf = s2c_node;

  if (initial_controller_logic.head) {
    s2c_node->set_children(initial_controller_logic.head);
    initial_controller_logic.head->set_prev(s2c_node);
    ep_node_leaf = initial_controller_logic.tail;
    assert(ep_node_leaf && "Leaf is not supposed to be null");
  }

  // Now we need to replicate the parsing operations that were done before.
  const BDDNode *next          = node;
  std::unique_ptr<BDD> new_bdd = replicate_hdr_parsing_ops(ep, node, next);

  // Note that we don't point to the next BDD node, as it was not actually implemented.
  // We are delegating the implementation to other platform.
  EPLeaf leaf(ep_node_leaf, next);
  new_ep->process_leaf(s2c_node, {leaf}, false);

  if (new_bdd) {
    new_ep->replace_bdd(std::move(new_bdd));
  }

  const hit_rate_t hr = new_ep->get_ctx().get_profiler().get_hr(s2c_node);
  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_controller_traffic(new_ep->get_node_egress(hr, s2c_node));

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> SendToControllerFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  // We don't actually create a module for recirculation.
  return {};
}

} // namespace Tofino
} // namespace LibSynapse
