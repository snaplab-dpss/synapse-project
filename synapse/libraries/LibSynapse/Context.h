#pragma once

#include <LibSynapse/PerfOracle.h>
#include <LibCore/Cow.h>
#include <LibSynapse/Profiler.h>
#include <LibSynapse/Target.h>

#include <LibBDD/BDD.h>
#include <LibBDD/Config.h>
#include <LibBDD/Profile.h>

#include <LibCore/Symbol.h>
#include <LibCore/Types.h>

#include <filesystem>
#include <unordered_map>

namespace LibSynapse {

class EP;

using LibBDD::BDD;
using LibBDD::bf_config_t;
using LibBDD::cht_config_t;
using LibBDD::cms_config_t;
using LibBDD::dchain_config_t;
using LibBDD::map_coalescing_objs_t;
using LibBDD::map_config_t;
using LibBDD::symbol_translation_t;
using LibBDD::tb_config_t;
using LibBDD::vector_config_t;

using LibCore::expr_struct_t;
using LibCore::symbol_t;
using LibCore::SymbolManager;

enum class DSImpl {
  // ========================================
  // Tofino
  // ========================================

  Tofino_MapTable,
  Tofino_MapSetTable,
  Tofino_GuardedMapTable,
  Tofino_VectorTable,
  Tofino_DchainTable,
  Tofino_VectorRegister,
  Tofino_FCFSCachedTable,
  Tofino_FCFSCachedSet,
  Tofino_Meter,
  Tofino_HeavyHitterTable,
  Tofino_IntegerAllocator,
  Tofino_CountMinSketch,
  Tofino_BloomFilter,
  Tofino_CuckooHashTable,
  Tofino_LPM,

  // ========================================
  // Tofino CPU
  // ========================================

  Controller_Map,
  Controller_Vector,
  Controller_DoubleChain,
  Controller_ConsistentHashTable,
  Controller_CountMinSketch,
  Controller_BloomFilter,
  Controller_TokenBucket,

  // ========================================
  // x86
  // ========================================

  x86_Map,
  x86_Vector,
  x86_DoubleChain,
  x86_ConsistentHashTable,
  x86_CountMinSketch,
  x86_TokenBucket,
};

std::ostream &operator<<(std::ostream &os, DSImpl impl);
std::string ds_impl_to_string(DSImpl impl);

struct expiration_data_t {
  time_ns_t expiration_time;
  symbol_t number_of_freed_flows;
};

// Where a BDD node sits in the loops symbolic execution unrolled (LibBDD::BDD::detect_loops).
enum class LoopNodeRole { Iteration, Step, Prefix };

struct loop_node_t {
  size_t loop; // Index into loops_t::loops.
  LoopNodeRole role;
  size_t iteration; // Iteration: its iteration; Step: the iteration it follows; Prefix: 0.
  size_t body_op;   // Iteration: its body op; Step: the body op whose value it changes; Prefix: 0.
};

struct loops_t {
  std::vector<LibBDD::loop_t> loops;
  std::vector<size_t> body_of;                                       // By loop: the first loop with the same body (itself when none before it has).
  std::unordered_map<bdd_node_id_t, std::vector<loop_node_t>> nodes; // A node may carry two body ops (a rotate and its inline operation).
  std::unordered_map<std::string, bdd_node_id_t> producers;          // The symbol each node above produces -> that node.
  // A value an iteration's op node reads by name -> the body op computing it, by the operand's
  // place among the call's arguments: what a prefix value stands for.
  std::unordered_map<std::string, size_t> roles;
  // The value each op node above computes, as an expression string -> that node: a rotate holding
  // the same operation inline computes that node's body op.
  std::unordered_map<std::string, bdd_node_id_t> values;
};

class TargetContext {
public:
  TargetContext() {}

  virtual ~TargetContext() {}

  virtual TargetContext *clone() const = 0;
  virtual void debug() const {}
  // The search is about to process `ep`'s active leaf, which may sit on another path than the
  // module placed last: state that follows the leaf's position on its path is brought in line.
  virtual void sync_active_leaf(const EP *ep) {}
  // The target's placement state, for a walk's stop dump.
  virtual void dump(std::ostream &os) const {}
};

class Context {
private:
  Profiler profiler;
  // Everything below changes at a handful of search steps but is copied at every speculated
  // node: copies share it until one of them writes (see S()).
  struct State {
    std::unordered_map<addr_t, map_config_t> map_configs;
    std::unordered_map<addr_t, vector_config_t> vector_configs;
    std::unordered_map<addr_t, dchain_config_t> dchain_configs;
    std::unordered_map<addr_t, cms_config_t> cms_configs;
    std::unordered_map<addr_t, bf_config_t> bf_configs;
    std::unordered_map<addr_t, cht_config_t> cht_configs;
    std::unordered_map<addr_t, tb_config_t> tb_configs;

    std::vector<map_coalescing_objs_t> coalescing_candidates;
    std::unordered_set<addr_t> dchains_used_exclusively_for_linking_maps_with_vectors;
    std::unordered_map<addr_t, std::vector<hit_rate_t>> dchains_failing_to_allocate_new_index_hit_rates;
    std::optional<expiration_data_t> expiration_data;
    std::vector<expr_struct_t> expr_structs;

    std::unordered_map<addr_t, DSImpl> ds_impls;
    std::map<std::pair<bdd_node_id_t, addr_t>, DSImpl> ds_impls_decisions_per_bdd_node_and_obj;
    std::map<std::pair<addr_t, DSImpl>, u32> ds_usage_counts;
  };

  LibCore::Cow<PerfOracle> perf_oracle;
  LibCore::Cow<State> state;
  // The loops of the plan's BDD, empty once a reordering replaced it (invalidate_loops) until
  // something asks for them again. Apart from State so marking them stale copies nothing else.
  mutable LibCore::Cow<std::optional<loops_t>> loops;

  // The shared state: read-only from const members, detached on first write otherwise.
  const State &S() const { return *state; }
  State &S() { return state.mutate(); }

  std::unordered_map<TargetType, TargetContext *> target_ctxs;

public:
  Context(const BDD *bdd, const TargetsView &targets, const targets_config_t &targets_config, const Profiler &profiler);
  Context(const Context &other);
  Context(Context &&other);
  Context() = delete;

  Context &operator=(const Context &other);
  Context &operator=(Context &&other);

  ~Context();

  const Profiler &get_profiler() const;
  Profiler &get_mutable_profiler();

  const PerfOracle &get_perf_oracle() const;
  PerfOracle &get_mutable_perf_oracle();

  const map_config_t &get_map_config(addr_t addr) const;
  const vector_config_t &get_vector_config(addr_t addr) const;
  const dchain_config_t &get_dchain_config(addr_t addr) const;
  const cms_config_t &get_cms_config(addr_t addr) const;
  const bf_config_t &get_bf_config(addr_t addr) const;
  const cht_config_t &get_cht_config(addr_t addr) const;
  const tb_config_t &get_tb_config(addr_t addr) const;

  std::optional<map_coalescing_objs_t> get_map_coalescing_objs(addr_t obj) const;
  bool is_dchain_used_exclusively_for_linking_maps_with_vectors(addr_t dchain) const;
  const std::vector<hit_rate_t> &get_failing_to_allocate_new_index_hit_rates(addr_t dchain) const;
  const std::optional<expiration_data_t> &get_expiration_data() const;
  const std::vector<expr_struct_t> &get_expr_structs() const;

  // The loops of `bdd`, the BDD of the plan this context belongs to: detected when the context is
  // built, and again the first time they are asked for after the plan's BDD was replaced.
  const loops_t &get_loops(const BDD *bdd) const;
  // The plan's BDD was replaced: nodes moved, some renamed, so the loops found are no longer known.
  void invalidate_loops();

  template <class TCtx> const TCtx *get_target_ctx() const;
  template <class TCtx> const TCtx *get_target_ctx_if_available() const;
  template <class TCtx> TCtx *get_mutable_target_ctx();
  void sync_active_leaf(const EP *ep);
  void dump_targets(const std::filesystem::path &path) const;

  const std::unordered_map<addr_t, DSImpl> &get_ds_impls() const;
  const std::map<std::pair<bdd_node_id_t, addr_t>, DSImpl> &get_ds_impls_decisions_per_bdd_node_and_obj() const;
  const std::map<std::pair<addr_t, DSImpl>, u32> &get_ds_usage_counts() const;

  void save_ds_impl(bdd_node_id_t node_id, addr_t obj, DSImpl impl);
  bool has_ds_impl(addr_t obj) const;
  DSImpl get_ds_impl(addr_t obj) const;
  bool check_ds_impl(addr_t obj, DSImpl impl) const;
  bool can_impl_ds(addr_t obj, DSImpl impl) const;

  void translate(SymbolManager *symbol_manager, const std::vector<symbol_translation_t> &translated_symbols);

  void debug() const;

private:
  void bdd_pre_processing_get_coalescing_candidates(const BDD *bdd);
  void bdd_pre_processing_get_dchains_failing_to_allocate_new_index_hit_rates(const BDD *bdd);
  void bdd_pre_processing_get_ds_configs(const BDD *bdd);
  void bdd_pre_processing_get_structural_fields(const BDD *bdd);
  void bdd_pre_processing_build_tofino_parser(const BDD *bdd);
  void bdd_pre_processing_log();
};

} // namespace LibSynapse

#define EXPLICIT_TARGET_CONTEXT_INSTANTIATION(NAMESPACE, TARGET_CTX)                                                                                 \
  namespace NAMESPACE {                                                                                                                              \
  class TARGET_CTX;                                                                                                                                  \
  }                                                                                                                                                  \
  template <> const NAMESPACE::TARGET_CTX *Context::get_target_ctx<NAMESPACE::TARGET_CTX>() const;                                                   \
  template <> NAMESPACE::TARGET_CTX *Context::get_mutable_target_ctx<NAMESPACE::TARGET_CTX>();
