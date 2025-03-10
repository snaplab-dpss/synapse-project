#pragma once

#include <LibSynapse/Modules/Module.h>
#include <LibSynapse/Modules/ModuleFactory.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/Modules/Tofino/DataStructures/DataStructures.h>

namespace LibSynapse {
namespace Tofino {

class TofinoModule : public Module {
public:
  TofinoModule(ModuleType _type, const std::string &_name, const LibBDD::Node *node) : Module(_type, TargetType::Tofino, _name, node) {}

  TofinoModule(ModuleType _type, TargetType _next_type, const std::string &_name, const LibBDD::Node *node)
      : Module(_type, TargetType::Tofino, _next_type, _name, node) {}

  virtual std::unordered_set<DS_ID> get_generated_ds() const { return {}; }
};

struct map_table_data_t {
  addr_t obj;
  u32 num_entries;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  std::optional<LibCore::symbol_t> hit;
  TimeAware time_aware;
};

struct vector_table_data_t {
  addr_t obj;
  u32 num_entries;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
};

struct dchain_table_data_t {
  addr_t obj;
  u32 num_entries;
  klee::ref<klee::Expr> key;
  std::optional<LibCore::symbol_t> hit;
};

struct vector_register_data_t {
  addr_t obj;
  u32 num_entries;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> value;
  klee::ref<klee::Expr> write_value;
  std::unordered_set<RegisterActionType> actions;
};

class TofinoModuleFactory : public ModuleFactory {
protected:
  ModuleType type;
  TargetType target;

public:
  TofinoModuleFactory(ModuleType _type, const std::string &_name) : ModuleFactory(_type, TargetType::Tofino, _name) {}

  static const TofinoContext *get_tofino_ctx(const EP *ep);
  static TofinoContext *get_mutable_tofino_ctx(EP *ep);

  static const TNA &get_tna(const EP *ep);
  static TNA &get_mutable_tna(EP *ep);

  static LibCore::Symbols get_dataplane_state(const EP *ep, const LibBDD::Node *node);
  static bool expr_fits_in_action(klee::ref<klee::Expr> expr);

  // ======================================================================
  //  Map Table
  // ======================================================================

  static MapTable *build_or_reuse_map_table(const EP *ep, const LibBDD::Node *node, const map_table_data_t &data);
  static bool can_build_or_reuse_map_table(const EP *ep, const LibBDD::Node *node, const map_table_data_t &data);

  // ======================================================================
  //  Vector Table
  // ======================================================================

  static VectorTable *build_or_reuse_vector_table(const EP *ep, const LibBDD::Node *node, const vector_table_data_t &data);
  static bool can_build_or_reuse_vector_table(const EP *ep, const LibBDD::Node *node, const vector_table_data_t &data);

  // ======================================================================
  //  Dchain Table
  // ======================================================================

  static DchainTable *build_or_reuse_dchain_table(const EP *ep, const LibBDD::Node *node, const dchain_table_data_t &data);
  static bool can_build_or_reuse_dchain_table(const EP *ep, const LibBDD::Node *node, const dchain_table_data_t &data);

  // ======================================================================
  //  Vector Registers
  // ======================================================================

  static VectorRegister *build_or_reuse_vector_register(const EP *ep, const LibBDD::Node *node, const vector_register_data_t &data);
  static bool can_build_or_reuse_vector_register(const EP *ep, const LibBDD::Node *node, const vector_register_data_t &data);

  // ======================================================================
  //  FCFS Cached Table
  // ======================================================================

  static FCFSCachedTable *get_fcfs_cached_table(const EP *ep, const LibBDD::Node *node, addr_t obj);
  static FCFSCachedTable *build_or_reuse_fcfs_cached_table(const EP *ep, const LibBDD::Node *node, addr_t obj, klee::ref<klee::Expr> key,
                                                           u32 num_entries, u32 cache_capacity);
  static bool can_get_or_build_fcfs_cached_table(const EP *ep, const LibBDD::Node *node, addr_t obj, klee::ref<klee::Expr> key,
                                                 u32 num_entries, u32 cache_capacity);
  static std::vector<u32> enum_fcfs_cache_cap(u32 num_entries);
  static hit_rate_t get_fcfs_cache_success_rate(const Context &ctx, const LibBDD::Node *node, klee::ref<klee::Expr> key,
                                                u32 cache_capacity);

  // ======================================================================
  //  Heavy Hitter Table
  // ======================================================================

  static bool can_build_or_reuse_hh_table(const EP *ep, const LibBDD::Node *node, addr_t obj,
                                          const std::vector<klee::ref<klee::Expr>> &keys, u32 num_entries, u32 cms_width, u32 cms_height);
  static HHTable *build_or_reuse_hh_table(const EP *ep, const LibBDD::Node *node, addr_t obj,
                                          const std::vector<klee::ref<klee::Expr>> &keys, u32 num_entries, u32 cms_width, u32 cms_height);
  static hit_rate_t get_hh_table_hit_success_rate(const Context &ctx, const LibBDD::Node *node, klee::ref<klee::Expr> key, u32 capacity);

  // ======================================================================
  //  Count Min Sketch
  // ======================================================================

  static bool can_build_or_reuse_cms(const EP *ep, const LibBDD::Node *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys,
                                     u32 width, u32 height);
  static CountMinSketch *build_or_reuse_cms(const EP *ep, const LibBDD::Node *node, addr_t obj,
                                            const std::vector<klee::ref<klee::Expr>> &keys, u32 width, u32 height);

  // ======================================================================
  //  LPM
  // ======================================================================

  static LPM *build_lpm(const EP *ep, const LibBDD::Node *node, addr_t obj);
  static bool can_build_lpm(const EP *ep, const LibBDD::Node *node, addr_t obj);
};

} // namespace Tofino
} // namespace LibSynapse