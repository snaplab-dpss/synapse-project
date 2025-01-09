#include "symbex.h"
#include "bdd.h"
#include "../util/exprs.h"
#include "../util/solver.h"
#include "../system.h"

namespace synapse {
std::optional<addr_t> get_obj_from_call(const Call *node_call) {
  std::optional<addr_t> addr;

  assert(node_call && "Invalid call node");
  const call_t &call = node_call->get_call();

  klee::ref<klee::Expr> obj;

  if (call.function_name == "vector_borrow" || call.function_name == "vector_return") {
    obj = call.args.at("vector").expr;
    assert(!obj.isNull() && "Invalid vector object");
  }

  else if (call.function_name == "map_get" || call.function_name == "map_put") {
    obj = call.args.at("map").expr;
    assert(!obj.isNull() && "Invalid map object");
  }

  else if (call.function_name == "dchain_allocate_new_index" ||
           call.function_name == "dchain_is_index_allocated" ||
           call.function_name == "dchain_rejuvenate_index") {
    obj = call.args.at("chain").expr;
    assert(!obj.isNull() && "Invalid dchain object");
  }

  if (!obj.isNull()) {
    addr = expr_addr_to_obj_addr(obj);
  }

  return addr;
}

dchain_config_t get_dchain_config_from_bdd(const BDD &bdd, addr_t dchain_addr) {
  const std::vector<call_t> &init = bdd.get_init();

  for (const call_t &call : init) {
    if (call.function_name != "dchain_allocate")
      continue;

    klee::ref<klee::Expr> chain_out = call.args.at("chain_out").out;
    klee::ref<klee::Expr> index_range = call.args.at("index_range").expr;

    assert(!chain_out.isNull() && "Invalid chain_out");
    assert(!index_range.isNull() && "Invalid index_range");

    addr_t chain_out_addr = expr_addr_to_obj_addr(chain_out);

    if (chain_out_addr != dchain_addr)
      continue;

    u64 index_range_value = solver_toolbox.value_from_expr(index_range);
    return dchain_config_t{index_range_value};
  }

  panic("Should have found dchain configuration");
}

bits_t get_key_size(const BDD &bdd, addr_t addr) {
  const std::vector<call_t> &init = bdd.get_init();

  for (const call_t &call : init) {
    if (call.function_name == "map_allocate") {
      klee::ref<klee::Expr> _map = call.args.at("map_out").out;
      assert(!_map.isNull() && "Invalid map_out");

      addr_t _map_addr = expr_addr_to_obj_addr(_map);
      if (_map_addr != addr)
        continue;

      klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
      assert(!key_size.isNull() && "Invalid key_size");

      return solver_toolbox.value_from_expr(key_size);
    }

    if (call.function_name == "cms_allocate") {
      klee::ref<klee::Expr> _cms = call.args.at("cms").expr;
      assert(!_cms.isNull() && "Invalid cms");

      addr_t _cms_addr = expr_addr_to_obj_addr(_cms);
      if (_cms_addr != addr)
        continue;

      klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
      assert(!key_size.isNull() && "Invalid key_size");

      return solver_toolbox.value_from_expr(key_size);
    }
  }

  panic("Should have found at least one node with a key");
}

map_config_t get_map_config_from_bdd(const BDD &bdd, addr_t map_addr) {
  const std::vector<call_t> &init = bdd.get_init();

  for (const call_t &call : init) {
    if (call.function_name != "map_allocate")
      continue;

    klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
    klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
    klee::ref<klee::Expr> map_out = call.args.at("map_out").out;

    assert(!capacity.isNull() && "Invalid capacity");
    assert(!key_size.isNull() && "Invalid key_size");
    assert(!map_out.isNull() && "Invalid map_out");

    addr_t map_out_addr = expr_addr_to_obj_addr(map_out);
    if (map_out_addr != map_addr)
      continue;

    u64 capacity_value = solver_toolbox.value_from_expr(capacity);
    bits_t key_size_value = solver_toolbox.value_from_expr(key_size) * 8;

    return map_config_t{capacity_value, static_cast<bits_t>(key_size_value)};
  }

  panic("Should have found map configuration");
}

vector_config_t get_vector_config_from_bdd(const BDD &bdd, addr_t vector_addr) {
  const std::vector<call_t> &init = bdd.get_init();

  for (const call_t &call : init) {
    if (call.function_name != "vector_allocate")
      continue;

    klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
    klee::ref<klee::Expr> elem_size = call.args.at("elem_size").expr;
    klee::ref<klee::Expr> vector_out = call.args.at("vector_out").out;

    assert(!capacity.isNull() && "Invalid capacity");
    assert(!elem_size.isNull() && "Invalid elem_size");
    assert(!vector_out.isNull() && "Invalid vector_out");

    addr_t vector_out_addr = expr_addr_to_obj_addr(vector_out);
    if (vector_out_addr != vector_addr)
      continue;

    u64 capacity_value = solver_toolbox.value_from_expr(capacity);
    bits_t elem_size_value = solver_toolbox.value_from_expr(elem_size) * 8;

    return vector_config_t{capacity_value, elem_size_value};
  }

  panic("Should have found vector configuration");
}

cms_config_t get_cms_config_from_bdd(const BDD &bdd, addr_t cms_addr) {
  const std::vector<call_t> &init = bdd.get_init();

  for (const call_t &call : init) {
    if (call.function_name != "cms_allocate")
      continue;

    klee::ref<klee::Expr> height = call.args.at("height").expr;
    klee::ref<klee::Expr> width = call.args.at("width").expr;
    klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
    klee::ref<klee::Expr> cleanup_interval = call.args.at("cleanup_interval").expr;
    klee::ref<klee::Expr> cms_out = call.args.at("cms_out").out;

    assert(!height.isNull() && "Invalid height");
    assert(!width.isNull() && "Invalid width");
    assert(!key_size.isNull() && "Invalid key_size");
    assert(!cleanup_interval.isNull() && "Invalid cleanup_interval");
    assert(!cms_out.isNull() && "Invalid cms_out");

    addr_t cms_out_addr = expr_addr_to_obj_addr(cms_out);
    if (cms_out_addr != cms_addr)
      continue;

    u64 height_value = solver_toolbox.value_from_expr(height);
    u64 width_value = solver_toolbox.value_from_expr(width);
    bits_t key_size_value = solver_toolbox.value_from_expr(key_size) * 8;
    time_ns_t cleanup_interval_value = solver_toolbox.value_from_expr(cleanup_interval);

    return cms_config_t{height_value, width_value, key_size_value, cleanup_interval_value};
  }

  panic("Should have found cms configuration");
}

cht_config_t get_cht_config_from_bdd(const BDD &bdd, addr_t cht_addr) {
  const std::vector<call_t> &init = bdd.get_init();

  for (const call_t &call : init) {
    if (call.function_name != "cht_fill_cht")
      continue;

    klee::ref<klee::Expr> capacity = call.args.at("backend_capacity").expr;
    klee::ref<klee::Expr> height = call.args.at("cht_height").expr;
    klee::ref<klee::Expr> cht = call.args.at("cht").expr;

    assert(!capacity.isNull() && "Invalid capacity");
    assert(!height.isNull() && "Invalid height");
    assert(!cht.isNull() && "Invalid cht");

    addr_t _cht_addr = expr_addr_to_obj_addr(cht);
    if (_cht_addr != cht_addr)
      continue;

    u64 capacity_value = solver_toolbox.value_from_expr(capacity);
    u64 height_value = solver_toolbox.value_from_expr(height);

    return cht_config_t{capacity_value, height_value};
  }

  panic("Should have found cht configuration");
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

    assert(!capacity.isNull() && "Invalid capacity");
    assert(!rate.isNull() && "Invalid rate");
    assert(!burst.isNull() && "Invalid burst");
    assert(!key_size.isNull() && "Invalid key_size");
    assert(!tb_out.isNull() && "Invalid tb_out");

    addr_t tb_out_addr = expr_addr_to_obj_addr(tb_out);
    if (tb_out_addr != tb_addr)
      continue;

    u64 capacity_value = solver_toolbox.value_from_expr(capacity);
    Bps_t rate_value = solver_toolbox.value_from_expr(rate);
    bytes_t burst_value = solver_toolbox.value_from_expr(burst);
    bits_t key_size_value = solver_toolbox.value_from_expr(key_size) * 8;

    return tb_config_t{capacity_value, rate_value, burst_value, key_size_value};
  }

  panic("Should have found tb configuration");
}
} // namespace synapse