#pragma once

#include <vector>
#include <string>
#include <unordered_set>
#include <optional>
#include <stdint.h>

#include "../types.hpp"
#include "symbol.hpp"
#include "exprs.hpp"

#define align_to_byte(B) ((B) % 8 == 0 ? (B) : (B) + 8 - (B) % 8)

namespace synapse {
class Node;
class Call;
class Branch;
class BDD;

enum class ModuleType;
class EP;
class EPNode;
class Module;

struct port_ingress_t;

struct branch_direction_t;
struct map_rw_pattern_t;

pps_t bps2pps(bps_t bps, bytes_t pkt_size);
bps_t pps2bps(pps_t pps, bytes_t pkt_size);

std::string int2hr(u64 value);
std::string tput2str(u64 thpt, const std::string &units, bool human_readable = false);

std::vector<expr_mod_t> ignore_checksum_modifications(const std::vector<expr_mod_t> &modifications);

const Call *packet_borrow_from_return(const EP *ep, const Call *packet_return_chunk);

struct map_coalescing_objs_t {
  addr_t map;
  addr_t dchain;
  addrs_t vectors;
};

/*
  We use this method to check if we can coalesce the Map + Vector + Dchain
  paradigm into a single data structure mimicking a common map (which
  maps arbitrary data to arbitrary values).

  These data structures should coalesce if the index allocated by the dchain
  is used as a map value and a vector index.

  Multiple vectors can be coalesced into the same data structure, but typically
  a single map and a single dchain are used.

  The pattern we are looking is the following:

  1. Allocating the index
  -> dchain_allocate_new_index(dchain, &index)

  2. Storing the key
  -> vector_borrow(vector_1, index, value_1)

  3. Updating the map
  -> map_put(map_1, key, index)

  4. Returning the key
  -> vector_return(vector_1, index, value_1)

  [ Loop updating all the other vectors ]
  -> vector_borrow(vector_n, index, value_n)
  -> vector_return(vector_n, index, value_n)
*/
bool get_map_coalescing_objs_from_bdd(const BDD *bdd, addr_t obj, map_coalescing_objs_t &data);
bool get_map_coalescing_objs_from_dchain_op(const EP *ep, const Call *dchain_op, map_coalescing_objs_t &map_objs);
bool get_map_coalescing_objs_from_map_op(const EP *ep, const Call *map_op, map_coalescing_objs_t &map_objs);

std::vector<const Call *> get_coalescing_nodes_from_key(const Node *node, klee::ref<klee::Expr> key,
                                                        const map_coalescing_objs_t &data);

struct rw_fractions_t {
  hit_rate_t read;
  hit_rate_t write_attempt;
  hit_rate_t write;
};

rw_fractions_t get_cond_map_put_rw_profile_fractions(const EP *ep, const Node *map_get);

// Similar to is_map_get_followed_by_map_puts_on_miss, but all required nodes
// are as close together as possible, with no other nodes in between.
// For this reason, there will be only a single map_put at most.
// Conditions to meet:
// (1) 1 future map_put
// (2) map_put happens if the map_get was not successful
// (3) map_put with the target obj also has the same key as the map_get
// (4) There is at most a single extra branch condition between the map_get and
// map_put operations
// (5) If there is an extra branch condition for map writes, then the node paths
// on both the failed extra branch condition and the failed index allocation are
// the same.
bool is_compact_map_get_followed_by_map_put_on_miss(const EP *ep, const Call *map_get,
                                                    map_rw_pattern_t &map_rw_pattern);

// (1) Has at least 1 future map_put
// (2) All map_put happen if the dchain_allocate_new_index was successful
// (3) All map_puts with the target obj also have the same key as the map_get
// (4) All map_puts with the target obj update with the same value
bool is_map_update_with_dchain(const EP *ep, const Call *dchain_allocate_new_index,
                               std::vector<const Call *> &map_puts);

bool is_index_alloc_on_unsuccessful_map_get(const EP *ep, const Call *dchain_allocate_new_index);

// Tries to find the pattern of a map_get followed by map_erases, but only when
// the map_get is successful (i.e. the key is found).
// Conditions to meet:
// (1) Has at least 1 future map_erase
// (2) All map_erase happen if the map_get was successful
// (3) All map_erases with the target obj also have the same key as the map_get
bool is_map_get_followed_by_map_erases_on_hit(const Call *map_get, std::vector<const Call *> &map_erases);

// Deletes all vector operations solely responsible for map key management.
void delete_all_vector_key_operations_from_bdd(BDD *bdd);

bool is_tb_tracing_check_followed_by_update_on_true(const Call *tb_is_tracing, const Call *&tb_update_and_check);

// Vector of past recirculations, from the most recent to the oldest.
// Elements of the return vector are recirculation ports, and indexes are the
// order between recirculations.
std::vector<int> get_past_recirculations(const EPNode *node);

// Check if a forwarding decision was already made in the dataplane. This is
// useful for preventing new forwarding decisions (be it forwarding to a
// specific port or dropping altogether) from being made.
bool forwarding_decision_already_made(const EPNode *node);

port_ingress_t get_node_egress(const EP *ep, const EPNode *node);

// Checks if the nodes are equivalent. That is, they perform the same operation
// to the same objects (if any).
bool are_nodes_equivalent(const Node *node0, const Node *node1);

// Checks if two nodes on the BDD are actually equivalent. That is, they perform
// the exact same operations in the same order.
bool are_node_paths_equivalent(const Node *node0, const Node *node1);
} // namespace synapse