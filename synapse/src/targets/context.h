#pragma once

#include <unordered_map>

#include "../util.h"
#include "../log.h"
#include "../profiler.h"
#include "../bdd/bdd.h"
#include "../execution_plan/node.h"
#include "target.h"

class EP;

enum class DSImpl {
  Tofino_Table,
  Tofino_VectorRegister,
  Tofino_FCFSCachedTable,
  Tofino_Meter,
  TofinoCPU_Map,
  TofinoCPU_Vector,
  TofinoCPU_Dchain,
  TofinoCPU_Cht,
  TofinoCPU_Sketch,
  TofinoCPU_TB,
  x86_Map,
  x86_Vector,
  x86_Dchain,
  x86_Cht,
  x86_Sketch,
  x86_TB,
};

std::ostream &operator<<(std::ostream &os, DSImpl impl);

struct expiration_data_t {
  time_ns_t expiration_time;
  symbol_t number_of_freed_flows;
};

struct spec_impl_t;

class TargetContext {
public:
  TargetContext() {}

  virtual ~TargetContext() {}

  virtual TargetContext *clone() const = 0;
  virtual void debug() const {}
};

class Context {
private:
  Profiler profiler;

  std::unordered_map<addr_t, map_config_t> map_configs;
  std::unordered_map<addr_t, vector_config_t> vector_configs;
  std::unordered_map<addr_t, dchain_config_t> dchain_configs;
  std::unordered_map<addr_t, sketch_config_t> sketch_configs;
  std::unordered_map<addr_t, cht_config_t> cht_configs;
  std::unordered_map<addr_t, tb_config_t> tb_configs;

  std::vector<map_coalescing_objs_t> coalescing_candidates;
  std::optional<expiration_data_t> expiration_data;

  std::unordered_map<addr_t, DSImpl> ds_impls;
  std::unordered_map<TargetType, TargetContext *> target_ctxs;
  std::unordered_map<ep_node_id_t, constraints_t> constraints_per_node;

public:
  Context(const BDD *bdd, const targets_t &targets,
          const TargetType initial_target, const Profiler &profiler);
  Context(const Context &other);
  Context(Context &&other);

  Context &operator=(const Context &other);
  ~Context();

  const Profiler &get_profiler() const;
  Profiler &get_mutable_profiler();

  const map_config_t &get_map_config(addr_t addr) const;
  const vector_config_t &get_vector_config(addr_t addr) const;
  const dchain_config_t &get_dchain_config(addr_t addr) const;
  const sketch_config_t &get_sketch_config(addr_t addr) const;
  const cht_config_t &get_cht_config(addr_t addr) const;
  const tb_config_t &get_tb_config(addr_t addr) const;

  std::optional<map_coalescing_objs_t>
  get_map_coalescing_objs(addr_t obj) const;
  const std::optional<expiration_data_t> &get_expiration_data() const;

  template <class TCtx> const TCtx *get_target_ctx() const;
  template <class TCtx> TCtx *get_mutable_target_ctx();

  const std::unordered_map<addr_t, DSImpl> &get_ds_impls() const;
  void save_ds_impl(addr_t obj, DSImpl impl);
  bool has_ds_impl(addr_t obj) const;
  bool check_ds_impl(addr_t obj, DSImpl impl) const;
  bool can_impl_ds(addr_t obj, DSImpl impl) const;

  void update_constraints_per_node(ep_node_id_t node,
                                   const constraints_t &constraints);
  constraints_t get_node_constraints(const EPNode *node) const;

  void add_hit_rate_estimation(const constraints_t &constraints,
                               klee::ref<klee::Expr> new_constraint,
                               hit_rate_t estimation_rel);
  void remove_hit_rate_node(const constraints_t &constraints);
  void scale_profiler(const constraints_t &constraints, hit_rate_t factor);

  void debug() const;
};

#define EXPLICIT_TARGET_CONTEXT_INSTANTIATION(NS, TCTX)                        \
  namespace NS {                                                               \
  class TCTX;                                                                  \
  }                                                                            \
  template <> const NS::TCTX *Context::get_target_ctx<NS::TCTX>() const;       \
  template <> NS::TCTX *Context::get_mutable_target_ctx<NS::TCTX>();

EXPLICIT_TARGET_CONTEXT_INSTANTIATION(tofino, TofinoContext)
EXPLICIT_TARGET_CONTEXT_INSTANTIATION(tofino_cpu, TofinoCPUContext)
EXPLICIT_TARGET_CONTEXT_INSTANTIATION(x86, x86Context)