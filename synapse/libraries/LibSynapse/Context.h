#pragma once

#include <unordered_map>
#include <toml++/toml.hpp>

#include <LibSynapse/PerfOracle.h>
#include <LibSynapse/Profiler.h>
#include <LibSynapse/Target.h>
#include <LibBDD/BDD.h>
#include <LibBDD/Config.h>
#include <LibBDD/Profile.h>
#include <LibCore/Symbol.h>
#include <LibCore/Types.h>

namespace LibSynapse {

enum class DSImpl {
  // ========================================
  // Tofino
  // ========================================
  Tofino_Table,
  Tofino_VectorRegister,
  Tofino_FCFSCachedTable,
  Tofino_Meter,
  Tofino_HeavyHitterTable,
  Tofino_IntegerAllocator,
  Tofino_CountMinSketch,
  Tofino_CuckooHashTable,

  // ========================================
  // Tofino CPU
  // ========================================
  Controller_Map,
  Controller_Vector,
  Controller_DoubleChain,
  Controller_ConsistentHashTable,
  Controller_CountMinSketch,
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

struct expiration_data_t {
  time_ns_t expiration_time;
  LibCore::symbol_t number_of_freed_flows;
};

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
  PerfOracle perf_oracle;

  std::unordered_map<addr_t, LibBDD::map_config_t> map_configs;
  std::unordered_map<addr_t, LibBDD::vector_config_t> vector_configs;
  std::unordered_map<addr_t, LibBDD::dchain_config_t> dchain_configs;
  std::unordered_map<addr_t, LibBDD::cms_config_t> cms_configs;
  std::unordered_map<addr_t, LibBDD::cht_config_t> cht_configs;
  std::unordered_map<addr_t, LibBDD::tb_config_t> tb_configs;

  std::vector<LibBDD::map_coalescing_objs_t> coalescing_candidates;
  std::optional<expiration_data_t> expiration_data;

  std::unordered_map<addr_t, DSImpl> ds_impls;
  std::unordered_map<TargetType, TargetContext *> target_ctxs;

public:
  Context(const LibBDD::BDD *bdd, const TargetsView &targets, const toml::table &config, const Profiler &profiler);
  Context(const Context &other);
  Context(Context &&other);

  Context &operator=(const Context &other);
  ~Context();

  const Profiler &get_profiler() const;
  Profiler &get_mutable_profiler();

  const PerfOracle &get_perf_oracle() const;
  PerfOracle &get_mutable_perf_oracle();

  const LibBDD::map_config_t &get_map_config(addr_t addr) const;
  const LibBDD::vector_config_t &get_vector_config(addr_t addr) const;
  const LibBDD::dchain_config_t &get_dchain_config(addr_t addr) const;
  const LibBDD::cms_config_t &get_cms_config(addr_t addr) const;
  const LibBDD::cht_config_t &get_cht_config(addr_t addr) const;
  const LibBDD::tb_config_t &get_tb_config(addr_t addr) const;

  std::optional<LibBDD::map_coalescing_objs_t> get_map_coalescing_objs(addr_t obj) const;
  const std::optional<expiration_data_t> &get_expiration_data() const;

  template <class TCtx> const TCtx *get_target_ctx() const;
  template <class TCtx> TCtx *get_mutable_target_ctx();

  const std::unordered_map<addr_t, DSImpl> &get_ds_impls() const;
  void save_ds_impl(addr_t obj, DSImpl impl);
  bool has_ds_impl(addr_t obj) const;
  bool check_ds_impl(addr_t obj, DSImpl impl) const;
  bool can_impl_ds(addr_t obj, DSImpl impl) const;

  void debug() const;
};

} // namespace LibSynapse

#define EXPLICIT_TARGET_CONTEXT_INSTANTIATION(NAMESPACE, TARGET_CTX)                                                                       \
  namespace NAMESPACE {                                                                                                                    \
  class TARGET_CTX;                                                                                                                        \
  }                                                                                                                                        \
  template <> const NAMESPACE::TARGET_CTX *Context::get_target_ctx<NAMESPACE::TARGET_CTX>() const;                                         \
  template <> NAMESPACE::TARGET_CTX *Context::get_mutable_target_ctx<NAMESPACE::TARGET_CTX>();
