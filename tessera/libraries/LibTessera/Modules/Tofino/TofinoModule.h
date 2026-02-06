#pragma once

#include <LibTessera/Modules/Module.h>
#include <LibTessera/Modules/ModuleFactory.h>
#include <LibTessera/Modules/Tofino/TofinoContext.h>
#include <LibTessera/Modules/Tofino/DataStructures/DataStructures.h>

namespace LibTessera {
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

struct map_set_table_data_t {
  addr_t obj;
  u32 capacity;
  klee::ref<klee::Expr> original_key;
  std::vector<klee::ref<klee::Expr>> keys;
  symbol_t index;
  std::optional<symbol_t> hit;
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

  static bool was_ds_already_used(const EPNode *leaf, DS_ID ds_id);
  static bool was_ds_already_used(const EPNode *leaf, const speculations_t &speculations, const BDDNode *node, addr_t obj, DSImpl ds_impl,
                                  DS_ID ds_id);

  static const TofinoContext *get_tofino_ctx(const EP *ep);
  static TofinoContext *get_mutable_tofino_ctx(EP *ep);

  static const TNA &get_tna(const EP *ep);
  static TNA &get_mutable_tna(EP *ep);

  static Symbols get_relevant_dataplane_state(const EP *ep, const BDDNode *node);
  static bool expr_fits_in_action(klee::ref<klee::Expr> expr);

  static std::vector<klee::ref<klee::Expr>> partition_expr_for_registers(const Context &ctx, klee::ref<klee::Expr> expr) {
    return Register::partition_value(ctx.get_target_ctx<TofinoContext>()->get_tna().tna_config.properties, expr, ctx.get_expr_structs());
  }

  static void speculate_sending_to_controller(const EP *ep, const BDDNode *node, Context &ctx, const speculations_t &speculations,
                                              hit_rate_t relative_hr_sent_to_controller, bool local_recirculation_decision);

  // ======================================================================
  //  Map Table
  // ======================================================================

  static MapTable *build_or_reuse_map_table(const EP *ep, const BDDNode *node, const map_table_data_t &data);
  static bool can_build_or_reuse_map_table(const EP *ep, const BDDNode *node, const map_table_data_t &data);

  // ======================================================================
  //  Map Set Table
  // ======================================================================

  static MapSetTable *build_or_reuse_map_set_table(const EP *ep, const BDDNode *node, const map_set_table_data_t &data);
  static bool can_build_or_reuse_map_set_table(const EP *ep, const BDDNode *node, const map_set_table_data_t &data);

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

  static DS_ID build_vector_register_id(addr_t obj);
  static VectorRegister *build_or_reuse_vector_register(const EP *ep, const BDDNode *node, const vector_register_data_t &data);
  static bool can_build_or_reuse_vector_register(const EP *ep, const BDDNode *node, const vector_register_data_t &data);

  // ======================================================================
  //  FCFS Cached Table
  // ======================================================================

  static DS_ID build_fcfs_ct_id(addr_t obj);
  static FCFSCachedTable *get_fcfs_ct(const EP *ep, const BDDNode *node, addr_t obj);
  static FCFSCachedTable *build_or_reuse_fcfs_ct(const EP *ep, const BDDNode *node, addr_t obj, klee::ref<klee::Expr> key, u32 capacity,
                                                 u32 cache_capacity, bool required_additional_table = true);
  static bool can_build_or_reuse_fcfs_ct(const EP *ep, const BDDNode *node, addr_t obj, klee::ref<klee::Expr> key, u32 capacity, u32 cache_capacity,
                                         bool required_additional_table = true);
  static bool can_reuse_fcfs_ct(const EP *ep, const BDDNode *node, addr_t obj, u32 cache_capacity, bool required_additional_table = true);
  static std::vector<u32> enum_fcfs_ct_cache_capacities(u32 capacity);
  static hit_rate_t get_fcfs_ct_cache_hit_rate(const Context &ctx, const BDDNode *map_put, klee::ref<klee::Expr> key, u32 cache_capacity);

  // ======================================================================
  //  FCFS Cached Set
  // ======================================================================

  static DS_ID build_fcfs_cs_id(addr_t obj);
  static FCFSCachedSet *get_fcfs_cs(const EP *ep, const BDDNode *node, addr_t obj);
  static FCFSCachedSet *build_or_reuse_fcfs_cs(const EP *ep, const BDDNode *node, addr_t obj, klee::ref<klee::Expr> key, u32 capacity,
                                               u32 cache_capacity, bool required_additional_table = true);
  static bool can_build_or_reuse_fcfs_cs(const EP *ep, const BDDNode *node, addr_t obj, klee::ref<klee::Expr> key, u32 capacity, u32 cache_capacity,
                                         bool required_additional_table = true);
  static bool can_reuse_fcfs_cs(const EP *ep, const BDDNode *node, addr_t obj, u32 cache_capacity, bool required_additional_table = true);
  static std::vector<u32> enum_fcfs_cs_cache_capacities(u32 capacity);
  static hit_rate_t get_fcfs_cs_cache_hit_rate(const Context &ctx, const BDDNode *map_put, klee::ref<klee::Expr> key, u32 cache_capacity);

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

  static DS_ID build_cms_id(addr_t obj);
  static bool can_build_or_reuse_cms(const EP *ep, const BDDNode *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys, u32 width,
                                     u32 height);
  static CountMinSketch *build_or_reuse_cms(const EP *ep, const BDDNode *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys, u32 width,
                                            u32 height);

  // ======================================================================
  //  Bloom Filter
  // ======================================================================

  static DS_ID build_bf_id(addr_t obj);
  static bool can_build_or_reuse_bf(const EP *ep, const BDDNode *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys, u32 width,
                                    u32 height);
  static BloomFilter *build_or_reuse_bf(const EP *ep, const BDDNode *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys, u32 width,
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
} // namespace LibTessera