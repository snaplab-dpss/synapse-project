#pragma once

#include <LibCore/Types.h>

#include <optional>
#include <unordered_set>

namespace LibBDD {

class BDD;
class Call;

std::optional<addr_t> get_obj_from_call(const Call *call);

struct dchain_config_t {
  u64 index_range;
};

struct map_config_t {
  u64 capacity;
  bits_t key_size;
};

struct vector_config_t {
  u64 capacity;
  bits_t elem_size;
};

struct cms_config_t {
  u64 height;
  u64 width;
  bits_t key_size;
  time_ns_t cleanup_interval;
};

struct cht_config_t {
  u64 capacity;
  u64 height;
};

struct tb_config_t {
  u64 capacity;
  Bps_t rate;
  bytes_t burst;
  bits_t key_size;
};

dchain_config_t get_dchain_config_from_bdd(const BDD &bdd, addr_t dchain_addr);
map_config_t get_map_config_from_bdd(const BDD &bdd, addr_t map_addr);
vector_config_t get_vector_config_from_bdd(const BDD &bdd, addr_t vector_addr);
cms_config_t get_cms_config_from_bdd(const BDD &bdd, addr_t cms_addr);
cht_config_t get_cht_config_from_bdd(const BDD &bdd, addr_t cht_addr);
tb_config_t get_tb_config_from_bdd(const BDD &bdd, addr_t tb_addr);

struct map_coalescing_objs_t {
  addr_t map;
  addr_t dchain;
  std::unordered_set<addr_t> vectors;
};

} // namespace LibBDD