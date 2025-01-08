#pragma once

#include "../module.h"
#include "../module_factory.h"
#include "tofino_context.h"

namespace synapse {
namespace tofino {

class TofinoModule : public Module {
public:
  TofinoModule(ModuleType _type, const std::string &_name, const Node *node)
      : Module(_type, TargetType::Tofino, _name, node) {}

  TofinoModule(ModuleType _type, TargetType _next_type, const std::string &_name, const Node *node)
      : Module(_type, TargetType::Tofino, _next_type, _name, node) {}

  virtual std::unordered_set<DS_ID> get_generated_ds() const { return {}; }
};

struct table_data_t {
  addr_t obj;
  u32 num_entries;
  std::vector<klee::ref<klee::Expr>> keys;
  std::vector<klee::ref<klee::Expr>> values;
  std::optional<symbol_t> hit;
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
  TofinoModuleFactory(ModuleType _type, const std::string &_name)
      : ModuleFactory(_type, TargetType::Tofino, _name) {}

  static const TofinoContext *get_tofino_ctx(const EP *ep);
  static TofinoContext *get_mutable_tofino_ctx(EP *ep);

  static const TNA &get_tna(const EP *ep);
  static TNA &get_mutable_tna(EP *ep);

  static Symbols get_dataplane_state(const EP *ep, const Node *node);

  // ======================================================================
  //  Simple Tables
  // ======================================================================

  static Table *build_table(const EP *ep, const Node *node, const table_data_t &data);
  static bool can_build_table(const EP *ep, const Node *node, const table_data_t &data);

  // ======================================================================
  //  Vector Registers
  // ======================================================================

  static std::unordered_set<Register *>
  build_or_reuse_vector_registers(const EP *ep, const Node *node,
                                  const vector_register_data_t &data);
  static bool can_build_or_reuse_vector_registers(const EP *ep, const Node *node,
                                                  const vector_register_data_t &data);

  // ======================================================================
  //  FCFS Cached Table
  // ======================================================================

  static FCFSCachedTable *get_fcfs_cached_table(const EP *ep, const Node *node, addr_t obj);
  static FCFSCachedTable *build_or_reuse_fcfs_cached_table(const EP *ep, const Node *node,
                                                           addr_t obj, klee::ref<klee::Expr> key,
                                                           u32 num_entries, u32 cache_capacity);
  static bool can_get_or_build_fcfs_cached_table(const EP *ep, const Node *node, addr_t obj,
                                                 klee::ref<klee::Expr> key, u32 num_entries,
                                                 u32 cache_capacity);
  static std::vector<u32> enum_fcfs_cache_cap(u32 num_entries);
  static hit_rate_t get_fcfs_cache_success_rate(const Context &ctx, const Node *node,
                                                klee::ref<klee::Expr> key, u32 cache_capacity);

  // ======================================================================
  //  Heavy Hitter Table
  // ======================================================================

  static bool can_build_or_reuse_hh_table(const EP *ep, const Node *node, addr_t obj,
                                          const std::vector<klee::ref<klee::Expr>> &keys,
                                          u32 num_entries, u32 cms_width, u32 cms_height);
  static HHTable *build_or_reuse_hh_table(const EP *ep, const Node *node, addr_t obj,
                                          const std::vector<klee::ref<klee::Expr>> &keys,
                                          u32 num_entries, u32 cms_width, u32 cms_height);
  static hit_rate_t get_hh_table_hit_success_rate(const Context &ctx, const Node *node,
                                                  klee::ref<klee::Expr> key, u32 capacity);

  // ======================================================================
  //  Count Min Sketch
  // ======================================================================

  static bool can_build_or_reuse_cms(const EP *ep, const Node *node, addr_t obj,
                                     const std::vector<klee::ref<klee::Expr>> &keys, u32 width,
                                     u32 height);
  static CountMinSketch *build_or_reuse_cms(const EP *ep, const Node *node, addr_t obj,
                                            const std::vector<klee::ref<klee::Expr>> &keys,
                                            u32 width, u32 height);
};

} // namespace tofino
} // namespace synapse