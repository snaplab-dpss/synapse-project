#include <LibClone/EmbeddingProfiler.h>

#include <LibClone/Profilers/x86EmbeddingProfiler.h>

#include <LibCore/Debug.h>
#include <LibCore/Solver.h>

#include <queue>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace LibClone {

using LibCore::expr_addr_to_obj_addr;
using LibCore::solver_toolbox;

using LibBDD::arg_t;
using LibBDD::BDDNodeType;
using LibBDD::BDDNodeVisitAction;
using LibBDD::Branch;
using LibBDD::Call;
using LibBDD::call_t;
using LibBDD::Route;

void EmbeddingProfiler::register_data_structure(addr_t address, std::unique_ptr<const DataStructure> ds) {
  assert_or_panic(data_structures.find(address) == data_structures.end(), "DS with addr: %lu already registered", address);
  data_structures.emplace(address, std::move(ds));
}

u32 EmbeddingProfiler::get_u32(const Call *call_node, const std::string &param) {
  const call_t &call = call_node->get_call();
  assert_or_panic(call.args.find(param) != call.args.end(), "Function should have the argument");
  const arg_t &arg = call.args.at(param);
  assert_or_panic(LibCore::is_constant(arg.expr), "Argument should be a Constant");
  return (u32)solver_toolbox.value_from_expr(arg.expr);
}

addr_t EmbeddingProfiler::get_allocation_address(const Call *call_node, const std::string &param) const {
  const call_t &call = call_node->get_call();
  assert_or_panic(call.args.find(param) != call.args.end(), "Function should have the argument");
  const arg_t &arg = call.args.at(param);
  assert_or_panic(LibCore::is_constant(arg.out), "Argument should be a Constant");
  return expr_addr_to_obj_addr(arg.out);
}

addr_t EmbeddingProfiler::get_address(const Call *call_node, const std::string &param) const {
  const call_t &call = call_node->get_call();
  assert_or_panic(call.args.find(param) != call.args.end(), "Function should have the argument");
  const arg_t &arg = call.args.at(param);
  assert_or_panic(LibCore::is_constant(arg.expr), "Argument should be a Constant");
  return expr_addr_to_obj_addr(arg.expr);
}

bool EmbeddingProfiler::has_data_structure(addr_t address) const { return data_structures.find(address) != data_structures.end(); }

const DataStructure *EmbeddingProfiler::lookup_data_structure(addr_t address) const {
  assert_or_panic(data_structures.find(address) != data_structures.end(), "DataStructure not stored");
  return data_structures.at(address).get();
}

const EmbeddingCost EmbeddingProfiler::visitGenericCall(const Call *node) {
  const std::string &func = node->get_call().function_name;

  if (func == "map_allocate")
    return this->visitMapAllocate(node);
  if (func == "map_get")
    return this->visitMapGet(node);
  if (func == "map_put")
    return this->visitMapPut(node);
  if (func == "map_erase")
    return this->visitMapErase(node);
  if (func == "expire_items_single_map")
    return this->visitExpireItemsSingleMap(node);
  if (func == "expire_items_single_map_iteratively")
    return this->visitExpireItemsSingleMapIteratively(node);

  if (func == "vector_allocate")
    return this->visitVectorAllocate(node);
  if (func == "vector_borrow")
    return this->visitVectorBorrow(node);
  if (func == "vector_return")
    return this->visitVectorReturn(node);
  if (func == "vector_clear")
    return this->visitVectorClear(node);
  if (func == "vector_sample_lt")
    return this->visitVectorSampleLt(node);

  if (func == "dchain_allocate")
    return this->visitDchainAllocate(node);
  if (func == "dchain_allocate_new_index")
    return this->visitDchainAllocateNewIndex(node);
  if (func == "dchain_rejuvenate_index")
    return this->visitDchainRejuvenateIndex(node);
  if (func == "dchain_expire_one_index")
    return this->visitDchainExpireOneIndex(node);
  if (func == "dchain_is_index_allocated")
    return this->visitDchainIsIndexAllocated(node);
  if (func == "dchain_free_index")
    return this->visitDchainFreeIndex(node);

  if (func == "cms_allocate")
    return this->visitCMSAllocate(node);
  if (func == "cms_increment")
    return this->visitCMSIncrement(node);
  if (func == "cms_count_min")
    return this->visitCMSCountMin(node);
  if (func == "cms_periodic_cleanup")
    return this->visitCMSPeriodicCleanup(node);

  if (func == "lpm_allocate")
    return this->visitLPMAllocate(node);
  if (func == "lpm_free")
    return this->visitLPMFree(node);
  if (func == "lpm_from_file")
    return this->visitLPMFromFile(node);
  if (func == "lpm_update")
    return this->visitLPMUpdate(node);
  if (func == "lpm_lookup")
    return this->visitLPMLookup(node);

  if (func == "tb_allocate")
    return this->visitTokenBucketAllocate(node);
  if (func == "tb_is_tracing")
    return this->visitTokenBucketIsTracing(node);
  if (func == "tb_trace")
    return this->visitTokenBucketTrace(node);
  if (func == "tb_update_and_check")
    return this->visitTokenBucketUpdateAndCheck(node);
  if (func == "tb_expire")
    return this->visitTokenBucketExpire(node);

  return EmbeddingCost(1, 0);
}

const EmbeddingCosts EmbeddingProfiler::compute_all_costs(const BDD &bdd) {

  EmbeddingCosts costs;

  auto costs_computer = [this, &costs](const BDDNode *node) {
    switch (node->get_type()) {
    case BDDNodeType::Call: {
      const Call *call = dynamic_cast<const Call *>(node);
      assert_or_panic(costs.find(call->get_id()) == costs.end(), "Call Node %lu already visited", call->get_id());
      costs.emplace(call->get_id(), visitGenericCall(call));
    } break;
    case BDDNodeType::Branch: {
      const Branch *branch = dynamic_cast<const Branch *>(node);
      assert_or_panic(costs.find(branch->get_id()) == costs.end(), "Branch Node %lu already visited", branch->get_id());
      costs.emplace(branch->get_id(), visitBranch(branch));
    } break;
    case BDDNodeType::Route: {
      const Route *route = dynamic_cast<const Route *>(node);
      assert_or_panic(costs.find(route->get_id()) == costs.end(), "Route Node %lu already visited", route->get_id());
      costs.emplace(route->get_id(), visitRoute(route));
    } break;
    }

    return BDDNodeVisitAction::Continue;
  };

  const std::vector<Call *> &bdd_init = bdd.get_init();
  assert_or_panic(!bdd_init.empty(), "Empty Init");

  const Call *init    = bdd_init.front();
  const BDDNode *root = bdd.get_root();

  init->visit_nodes(costs_computer);
  root->visit_nodes(costs_computer);

  debug_embedding_costs(costs);

  return costs;
}

void EmbeddingProfiler::debug_embedding_costs(const EmbeddingCosts &costs) const {
  std::vector<std::pair<bdd_node_id_t, EmbeddingCost>> sorted(costs.begin(), costs.end());

  // Sort by node id for stable output
  std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) { return a.first < b.first; });

  u32 total_processing = 0;
  u32 total_memory     = 0;

  std::cerr << "==== Embedding Costs Debug ====\n";

  for (const auto &[node_id, cost] : sorted) {
    std::cerr << "Node " << node_id << " -> processing: " << cost.processing << ", memory: " << cost.memory << "\n";

    total_processing += cost.processing;
    total_memory += cost.memory;
  }

  std::cerr << "--------------------------------\n";
  std::cerr << "TOTAL processing: " << total_processing << "\n";
  std::cerr << "TOTAL memory    : " << total_memory << "\n";
}

EmbeddingProfilers::EmbeddingProfilers() { elements.push_back(std::move(std::make_unique<x86EmbeddingProfiler>())); }

std::vector<EmbeddingProfiler *> EmbeddingProfilers::get_profilers() const {
  std::vector<EmbeddingProfiler *> profilers;

  for (const std::unique_ptr<EmbeddingProfiler> &profiler : elements) {
    profilers.push_back(profiler.get());
  }
  return profilers;
}

} // namespace LibClone
