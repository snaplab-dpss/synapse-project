#pragma once

#include <LibSynapse/Modules/Module.h>
#include <LibSynapse/Modules/ModuleFactory.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/Modules/Tofino/DataStructures/DataStructures.h>

namespace LibSynapse {
namespace Tofino {

class TofinoModule : public Module {
public:
  TofinoModule(ModuleType _type, const std::string &_name, const BDDNode *_node) : Module(_type, TargetType::Tofino, _name, _node) {}

  TofinoModule(ModuleType _type, TargetType _next_type, const std::string &_name, const BDDNode *_node)
      : Module(_type, TargetType::Tofino, _next_type, _name, _node) {}

  virtual std::unordered_set<DS_ID> get_generated_ds() const { return {}; }
};

struct map_table_data_t {
  addr_t obj;
  u32 capacity;
  klee::ref<klee::Expr> original_key;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  std::optional<symbol_t> hit;
  TimeAware time_aware;
};

struct vector_table_data_t {
  addr_t obj;
  u32 capacity;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
};

struct dchain_table_data_t {
  addr_t obj;
  u32 capacity;
  klee::ref<klee::Expr> key;
  std::optional<symbol_t> hit;
};

struct vector_register_data_t {
  addr_t obj;
  u32 capacity;
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

  bool was_ds_already_used(const EPNode *leaf, DS_ID ds_id) const;

  static const TofinoContext *get_tofino_ctx(const EP *ep);
  static TofinoContext *get_mutable_tofino_ctx(EP *ep);

  static const TNA &get_tna(const EP *ep);
  static TNA &get_mutable_tna(EP *ep);

  static Symbols get_relevant_dataplane_state(const EP *ep, const BDDNode *node);
  static bool expr_fits_in_action(klee::ref<klee::Expr> expr);

  // ======================================================================
  //  Map Table
  // ======================================================================

  static MapTable *build_or_reuse_map_table(const EP *ep, const BDDNode *node, const map_table_data_t &data);
  static bool can_build_or_reuse_map_table(const EP *ep, const BDDNode *node, const map_table_data_t &data);

  // ======================================================================
  //  Guarded Map Table
  // ======================================================================

  static GuardedMapTable *build_or_reuse_guarded_map_table(const EP *ep, const BDDNode *node, const map_table_data_t &data);
  static bool can_build_or_reuse_guarded_map_table(const EP *ep, const BDDNode *node, const map_table_data_t &data);

  // ======================================================================
  //  Vector Table
  // ======================================================================

  static VectorTable *build_or_reuse_vector_table(const EP *ep, const BDDNode *node, const vector_table_data_t &data);
  static bool can_build_or_reuse_vector_table(const EP *ep, const BDDNode *node, const vector_table_data_t &data);

  // ======================================================================
  //  Dchain Table
  // ======================================================================

  static DchainTable *build_or_reuse_dchain_table(const EP *ep, const BDDNode *node, const dchain_table_data_t &data);
  static bool can_build_or_reuse_dchain_table(const EP *ep, const BDDNode *node, const dchain_table_data_t &data);

  // ======================================================================
  //  Vector Registers
  // ======================================================================

  static VectorRegister *build_or_reuse_vector_register(const EP *ep, const BDDNode *node, const vector_register_data_t &data);
  static bool can_build_or_reuse_vector_register(const EP *ep, const BDDNode *node, const vector_register_data_t &data);

  // ======================================================================
  //  FCFS Cached Table
  // ======================================================================

  static FCFSCachedTable *get_fcfs_cached_table(const EP *ep, const BDDNode *node, addr_t obj);
  static FCFSCachedTable *build_or_reuse_fcfs_cached_table(const EP *ep, const BDDNode *node, addr_t obj, klee::ref<klee::Expr> key, u32 capacity,
                                                           u32 cache_capacity);
  static bool can_get_or_build_fcfs_cached_table(const EP *ep, const BDDNode *node, addr_t obj, klee::ref<klee::Expr> key, u32 capacity,
                                                 u32 cache_capacity);
  static std::vector<u32> enum_fcfs_cache_cap(u32 capacity);
  static hit_rate_t get_fcfs_cache_success_rate(const Context &ctx, const BDDNode *node, klee::ref<klee::Expr> key, u32 cache_capacity);

  // ======================================================================
  //  Heavy Hitter Table
  // ======================================================================

  static bool can_build_or_reuse_hh_table(const EP *ep, const BDDNode *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys, u32 capacity,
                                          u32 cms_width, u32 cms_height);
  static HHTable *build_or_reuse_hh_table(const EP *ep, const BDDNode *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys, u32 capacity,
                                          u32 cms_width, u32 cms_height);
  static hit_rate_t get_hh_table_hit_success_rate(const EP *ep, const Context &ctx, const BDDNode *node, addr_t map, klee::ref<klee::Expr> key,
                                                  u32 capacity, u32 cms_width);

  // ======================================================================
  //  Count Min Sketch
  // ======================================================================

  static bool can_build_or_reuse_cms(const EP *ep, const BDDNode *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys, u32 width,
                                     u32 height);
  static CountMinSketch *build_or_reuse_cms(const EP *ep, const BDDNode *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys, u32 width,
                                            u32 height);

  // ======================================================================
  //  LPM
  // ======================================================================

  static LPM *build_lpm(const EP *ep, const BDDNode *node, addr_t obj);
  static bool can_build_lpm(const EP *ep, const BDDNode *node, addr_t obj);

  // ======================================================================
  //  Cuckoo Hash Table
  // ======================================================================

  static bool can_build_or_reuse_cuckoo_hash_table(const EP *ep, const BDDNode *node, addr_t obj, klee::ref<klee::Expr> key, u32 capacity);
  static CuckooHashTable *build_or_reuse_cuckoo_hash_table(const EP *ep, const BDDNode *node, addr_t obj, klee::ref<klee::Expr> key, u32 capacity);
  static hit_rate_t get_cuckoo_hash_table_hit_success_rate(const EP *ep, const Context &ctx, const BDDNode *node, addr_t map,
                                                           klee::ref<klee::Expr> key, u32 capacity);
};

} // namespace Tofino
} // namespace LibSynapse