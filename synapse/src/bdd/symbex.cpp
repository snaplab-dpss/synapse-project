#include "symbex.h"
#include "bdd.h"
#include "../exprs/exprs.h"
#include "../exprs/solver.h"
#include "../log.h"

std::optional<addr_t> get_obj_from_call(const Call *node_call) {
  std::optional<addr_t> addr;

  assert(node_call);
  const call_t &call = node_call->get_call();

  klee::ref<klee::Expr> obj;

  if (call.function_name == "vector_borrow" ||
      call.function_name == "vector_return") {
    obj = call.args.at("vector").expr;
    assert(!obj.isNull());
  }

  else if (call.function_name == "map_get" || call.function_name == "map_put") {
    obj = call.args.at("map").expr;
    assert(!obj.isNull());
  }

  else if (call.function_name == "dchain_allocate_new_index" ||
           call.function_name == "dchain_is_index_allocated" ||
           call.function_name == "dchain_rejuvenate_index") {
    obj = call.args.at("chain").expr;
    assert(!obj.isNull());
  }

  if (!obj.isNull())
    addr = expr_addr_to_obj_addr(obj);

  return addr;
}

dchain_config_t get_dchain_config_from_bdd(const BDD &bdd, addr_t dchain_addr) {
  const std::vector<call_t> &init = bdd.get_init();

  for (const call_t &call : init) {
    if (call.function_name != "dchain_allocate")
      continue;

    klee::ref<klee::Expr> chain_out = call.args.at("chain_out").out;
    klee::ref<klee::Expr> index_range = call.args.at("index_range").expr;

    assert(!chain_out.isNull());
    assert(!index_range.isNull());

    addr_t chain_out_addr = expr_addr_to_obj_addr(chain_out);

    if (chain_out_addr != dchain_addr)
      continue;

    u64 index_range_value = solver_toolbox.value_from_expr(index_range);
    return dchain_config_t{index_range_value};
  }

  PANIC("Should have found dchain configuration");
}

bits_t get_key_size(const BDD &bdd, addr_t addr) {
  const std::vector<call_t> &init = bdd.get_init();

  for (const call_t &call : init) {
    if (call.function_name == "map_allocate") {
      klee::ref<klee::Expr> _map = call.args.at("map_out").out;
      assert(!_map.isNull());

      addr_t _map_addr = expr_addr_to_obj_addr(_map);
      if (_map_addr != addr)
        continue;

      klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
      assert(!key_size.isNull());

      return solver_toolbox.value_from_expr(key_size);
    }

    if (call.function_name == "sketch_allocate") {
      klee::ref<klee::Expr> _sketch = call.args.at("sketch").expr;
      assert(!_sketch.isNull());

      addr_t _sketch_addr = expr_addr_to_obj_addr(_sketch);
      if (_sketch_addr != addr)
        continue;

      klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
      assert(!key_size.isNull());

      return solver_toolbox.value_from_expr(key_size);
    }
  }

  PANIC("Should have found at least one node with a key");
}

map_config_t get_map_config_from_bdd(const BDD &bdd, addr_t map_addr) {
  const std::vector<call_t> &init = bdd.get_init();

  for (const call_t &call : init) {
    if (call.function_name != "map_allocate")
      continue;

    klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
    klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
    klee::ref<klee::Expr> map_out = call.args.at("map_out").out;

    assert(!capacity.isNull());
    assert(!key_size.isNull());
    assert(!map_out.isNull());

    addr_t map_out_addr = expr_addr_to_obj_addr(map_out);
    if (map_out_addr != map_addr)
      continue;

    u64 capacity_value = solver_toolbox.value_from_expr(capacity);
    bits_t key_size_value = solver_toolbox.value_from_expr(key_size) * 8;

    return map_config_t{capacity_value, static_cast<bits_t>(key_size_value)};
  }

  PANIC("Should have found map configuration");
}

vector_config_t get_vector_config_from_bdd(const BDD &bdd, addr_t vector_addr) {
  const std::vector<call_t> &init = bdd.get_init();

  for (const call_t &call : init) {
    if (call.function_name != "vector_allocate")
      continue;

    klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
    klee::ref<klee::Expr> elem_size = call.args.at("elem_size").expr;
    klee::ref<klee::Expr> vector_out = call.args.at("vector_out").out;

    assert(!capacity.isNull());
    assert(!elem_size.isNull());
    assert(!vector_out.isNull());

    addr_t vector_out_addr = expr_addr_to_obj_addr(vector_out);
    if (vector_out_addr != vector_addr)
      continue;

    u64 capacity_value = solver_toolbox.value_from_expr(capacity);
    bits_t elem_size_value = solver_toolbox.value_from_expr(elem_size) * 8;

    return vector_config_t{capacity_value, elem_size_value};
  }

  PANIC("Should have found vector configuration");
}

sketch_config_t get_sketch_config_from_bdd(const BDD &bdd, addr_t sketch_addr) {
  const std::vector<call_t> &init = bdd.get_init();

  for (const call_t &call : init) {
    if (call.function_name != "sketch_allocate")
      continue;

    klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
    klee::ref<klee::Expr> threshold = call.args.at("threshold").expr;
    klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
    klee::ref<klee::Expr> sketch_out = call.args.at("sketch_out").out;

    assert(!capacity.isNull());
    assert(!threshold.isNull());
    assert(!key_size.isNull());
    assert(!sketch_out.isNull());

    addr_t sketch_out_addr = expr_addr_to_obj_addr(sketch_out);
    if (sketch_out_addr != sketch_addr)
      continue;

    u64 capacity_value = solver_toolbox.value_from_expr(capacity);
    u64 threshold_value = solver_toolbox.value_from_expr(threshold);
    bits_t key_size_value = solver_toolbox.value_from_expr(key_size) * 8;

    return sketch_config_t{capacity_value, threshold_value, key_size_value};
  }

  PANIC("Should have found sketch configuration");
}

cht_config_t get_cht_config_from_bdd(const BDD &bdd, addr_t cht_addr) {
  const std::vector<call_t> &init = bdd.get_init();

  for (const call_t &call : init) {
    if (call.function_name != "cht_fill_cht")
      continue;

    klee::ref<klee::Expr> capacity = call.args.at("backend_capacity").expr;
    klee::ref<klee::Expr> height = call.args.at("cht_height").expr;
    klee::ref<klee::Expr> cht = call.args.at("cht").expr;

    assert(!capacity.isNull());
    assert(!height.isNull());
    assert(!cht.isNull());

    addr_t _cht_addr = expr_addr_to_obj_addr(cht);
    if (_cht_addr != cht_addr)
      continue;

    u64 capacity_value = solver_toolbox.value_from_expr(capacity);
    u64 height_value = solver_toolbox.value_from_expr(height);

    return cht_config_t{capacity_value, height_value};
  }

  PANIC("Should have found cht configuration");
}

tb_config_t get_tb_config_from_bdd(const BDD &bdd, addr_t tb_addr) {
  const std::vector<call_t> &init = bdd.get_init();

  for (const call_t &call : init) {
    if (call.function_name != "tb_allocate")
      continue;

    klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
    klee::ref<klee::Expr> rate = call.args.at("rate").expr;
    klee::ref<klee::Expr> burst = call.args.at("burst").expr;
    klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
    klee::ref<klee::Expr> tb_out = call.args.at("tb_out").out;

    assert(!capacity.isNull());
    assert(!rate.isNull());
    assert(!burst.isNull());
    assert(!key_size.isNull());
    assert(!tb_out.isNull());

    addr_t tb_out_addr = expr_addr_to_obj_addr(tb_out);
    if (tb_out_addr != tb_addr)
      continue;

    u64 capacity_value = solver_toolbox.value_from_expr(capacity);
    Bps_t rate_value = solver_toolbox.value_from_expr(rate);
    bytes_t burst_value = solver_toolbox.value_from_expr(burst);
    bits_t key_size_value = solver_toolbox.value_from_expr(key_size) * 8;

    return tb_config_t{capacity_value, rate_value, burst_value, key_size_value};
  }

  PANIC("Should have found tb configuration");
}