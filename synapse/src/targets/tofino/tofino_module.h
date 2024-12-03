#pragma once

#include "../module.h"
#include "../module_generator.h"
#include "tofino_context.h"

namespace tofino {

class TofinoModule : public Module {
public:
  TofinoModule(ModuleType _type, const std::string &_name, const Node *node)
      : Module(_type, TargetType::Tofino, _name, node) {}

  TofinoModule(ModuleType _type, TargetType _next_type,
               const std::string &_name, const Node *node)
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
  std::unordered_set<RegisterAction> actions;
};

class TofinoModuleGenerator : public ModuleGenerator {
protected:
  ModuleType type;
  TargetType target;

public:
  TofinoModuleGenerator(ModuleType _type, const std::string &_name)
      : ModuleGenerator(_type, TargetType::Tofino, _name) {}

protected:
  TofinoContext *get_mutable_tofino_ctx(EP *ep) const {
    Context &ctx = ep->get_mutable_ctx();
    return ctx.get_mutable_target_ctx<TofinoContext>();
  }

  const TofinoContext *get_tofino_ctx(const EP *ep) const {
    const Context &ctx = ep->get_ctx();
    return ctx.get_target_ctx<TofinoContext>();
  }

  TNA &get_mutable_tna(EP *ep) const {
    TofinoContext *ctx = get_mutable_tofino_ctx(ep);
    return ctx->get_mutable_tna();
  }

  const TNA &get_tna(const EP *ep) const {
    const TofinoContext *ctx = get_tofino_ctx(ep);
    return ctx->get_tna();
  }

  symbols_t get_dataplane_state(const EP *ep, const Node *node) const;

  // ======================================================================
  //  Simple Tables
  // ======================================================================

  Table *build_table(const EP *ep, const Node *node,
                     const table_data_t &data) const;
  bool can_build_table(const EP *ep, const Node *node,
                       const table_data_t &data) const;

  // ======================================================================
  //  Vector Registers
  // ======================================================================

  std::unordered_set<Register *>
  build_or_reuse_vector_registers(const EP *ep, const Node *node,
                                  const vector_register_data_t &data) const;
  bool
  can_build_or_reuse_vector_registers(const EP *ep, const Node *node,
                                      const vector_register_data_t &data) const;

  // ======================================================================
  //  FCFS Cached Table
  // ======================================================================

  FCFSCachedTable *get_fcfs_cached_table(const EP *ep, const Node *node,
                                         addr_t obj) const;
  FCFSCachedTable *
  build_or_reuse_fcfs_cached_table(const EP *ep, const Node *node, addr_t obj,
                                   klee::ref<klee::Expr> key, u32 num_entries,
                                   u32 cache_capacity) const;
  bool can_get_or_build_fcfs_cached_table(const EP *ep, const Node *node,
                                          addr_t obj, klee::ref<klee::Expr> key,
                                          u32 num_entries,
                                          u32 cache_capacity) const;
  std::vector<u32> enum_fcfs_cache_cap(u32 num_entries) const;
  hit_rate_t get_fcfs_cache_success_rate(const Context &ctx, const Node *node,
                                         klee::ref<klee::Expr> key,
                                         u32 cache_capacity) const;

  // ======================================================================
  //  Map Register
  // ======================================================================

  bool can_build_or_reuse_map_register(const EP *ep, const Node *node,
                                       addr_t obj, klee::ref<klee::Expr> key,
                                       u32 num_entries) const;
  MapRegister *build_or_reuse_map_register(const EP *ep, const Node *node,
                                           addr_t obj,
                                           klee::ref<klee::Expr> key,
                                           u32 num_entries) const;

  // ======================================================================
  //  Heavy Hitter Table
  // ======================================================================

  bool
  can_build_or_reuse_hh_table(const EP *ep, const Node *node, addr_t obj,
                              const std::vector<klee::ref<klee::Expr>> &keys,
                              u32 num_entries, u32 cms_width,
                              u32 cms_height) const;
  HHTable *
  build_or_reuse_hh_table(const EP *ep, const Node *node, addr_t obj,
                          const std::vector<klee::ref<klee::Expr>> &keys,
                          u32 num_entries, u32 cms_width, u32 cms_height) const;
  hit_rate_t get_hh_table_hit_success_rate(const Context &ctx, const Node *node,
                                           klee::ref<klee::Expr> key,
                                           u32 capacity) const;

  // ======================================================================
  //  Count Min Sketch
  // ======================================================================

  bool can_build_or_reuse_cms(const EP *ep, const Node *node, addr_t obj,
                              const std::vector<klee::ref<klee::Expr>> &keys,
                              u32 width, u32 height) const;
  CountMinSketch *
  build_or_reuse_cms(const EP *ep, const Node *node, addr_t obj,
                     const std::vector<klee::ref<klee::Expr>> &keys, u32 width,
                     u32 height) const;
};

} // namespace tofino
