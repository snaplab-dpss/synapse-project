#pragma once

#include <vector>
#include <string>
#include <unordered_set>
#include <optional>
#include <stdint.h>

#include "call_paths/call_paths.h"
#include "bdd/tree.h"
#include "util/exprs.h"

#define align_to_byte(B) ((B) % 8 == 0 ? (B) : (B) + 8 - (B) % 8)

namespace synapse {
enum class ModuleType;

class EP;
class EPNode;
class Module;
class Context;

struct port_ingress_t;

pps_t bps2pps(bps_t bps, bytes_t pkt_size);
bps_t pps2bps(pps_t pps, bytes_t pkt_size);

bool check_same_obj(const Call *call0, const Call *call1, const std::string &obj_name);

std::vector<mod_t> build_vector_modifications(const Call *vector_borrow, const Call *vector_return);
std::vector<mod_t> build_hdr_modifications(const Call *packet_borrow_next_chunk,
                                           const Call *packet_return_chunk);

std::vector<mod_t> ignore_checksum_modifications(const std::vector<mod_t> &modifications);

bool query_contains_map_has_key(const Branch *node);

const Call *packet_borrow_from_return(const EP *ep, const Call *packet_return_chunk);

std::vector<const Call *> get_prev_functions(const EP *ep, const Node *node,
                                             const std::vector<std::string> &functions_names,
                                             bool ignore_targets = false);

std::vector<const Call *> get_future_functions(const Node *root,
                                               const std::vector<std::string> &functions,
                                               bool stop_on_branches = false);

bool is_parser_drop(const Node *root);

std::unordered_set<std::string> get_symbols(const Node *node);

// A parser condition should be the single discriminating condition that
// decides whether a parsing state is performed or not. In BDD language, it
// decides if a specific packet_borrow_next_chunk is applied.
//
// One classic example would be condition that checks if the ethertype field
// on the ethernet header equals the IP protocol.
//
// A branch condition is considered a parsing condition if:
//   - Has pending chunks to be borrowed in the future
//   - Only looks at the packet
bool is_parser_condition(const Branch *node);

std::vector<const Module *> get_prev_modules(const EP *ep, const std::vector<ModuleType> &types);

bool is_expr_only_packet_dependent(klee::ref<klee::Expr> expr);

struct counter_data_t {
  bool valid;
  std::vector<const Node *> reads;
  std::vector<const Node *> writes;
  std::optional<u64> max_value;

  counter_data_t() : valid(false) {}
};

// Check if a given data structure is a counter. We expect counters to be
// implemented with vectors, such that (1) the value it stores is <= 64 bits,
// and (2) the only write operations performed on them increment the stored
// value.
counter_data_t is_counter(const EP *ep, addr_t obj);

// When we encounter a vector_return operation and want to retrieve its
// vector_borrow value counterpart. This is useful to compare changes to the
// value expression and retrieve the performed modifications (if any).
klee::ref<klee::Expr> get_original_vector_value(const EP *ep, const Node *node, addr_t target_addr);
klee::ref<klee::Expr> get_original_vector_value(const EP *ep, const Node *node, addr_t target_addr,
                                                const Node *&source);

bool is_vector_return_without_modifications(const EP *ep, const Node *node);
bool is_vector_read(const Call *vector_borrow);
bool is_vector_write(const Call *call_node);
bool is_vector_borrow_ignored(const Call *vector_borrow);

const Call *get_future_vector_return(const Call *vector_borrow);

// Get the data associated with this address.
klee::ref<klee::Expr> get_expr_from_addr(const EP *ep, addr_t addr);

struct map_coalescing_objs_t {
  addr_t map;
  addr_t dchain;
  objs_t vectors;
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
bool get_map_coalescing_objs_from_dchain_op(const EP *ep, const Call *dchain_op,
                                            map_coalescing_objs_t &map_objs);
bool get_map_coalescing_objs_from_map_op(const EP *ep, const Call *map_op,
                                         map_coalescing_objs_t &map_objs);

std::vector<const Call *> get_coalescing_nodes_from_key(const BDD *bdd, const Node *node,
                                                        klee::ref<klee::Expr> key,
                                                        const map_coalescing_objs_t &data);

klee::ref<klee::Expr> get_chunk_from_borrow(const Node *node);
bool borrow_has_var_len(const Node *node);

symbols_t get_prev_symbols(const Node *node, const nodes_t &stop_nodes = nodes_t());

struct rw_fractions_t {
  hit_rate_t read;
  hit_rate_t write_attempt;
  hit_rate_t write;
};

struct branch_direction_t {
  const Branch *branch;
  bool direction;
};

// Checks if the map_get and map_put provided nodes are counterparts of each
// other. That is, they have the same object and the same key.
bool are_map_read_write_counterparts(const Call *map_get, const Call *map_put);

branch_direction_t get_map_get_success_check(const Node *map_get);
rw_fractions_t get_cond_map_put_rw_profile_fractions(const EP *ep, const Node *map_get);

// Tries to find the pattern of a map_get followed by map_puts, but only when
// the map_get is not successful (i.e. the key is not found).
// Conditions to meet:
// (1) Has at least 1 future map_put
// (2) All map_put happen if the map_get was not successful
// (3) All map_puts with the target obj also have the same key as the map_get
bool is_map_get_followed_by_map_puts_on_miss(const BDD *bdd, const Call *map_get,
                                             std::vector<const Call *> &map_puts);

struct map_rw_pattern_t {
  const Call *map_get;
  branch_direction_t map_get_success_check;
  branch_direction_t map_put_extra_condition;
  const Call *dchain_allocate_new_index;
  branch_direction_t index_alloc_check;
  const Call *map_put;
};

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
bool is_map_get_followed_by_map_erases_on_hit(const BDD *bdd, const Call *map_get,
                                              std::vector<const Call *> &map_erases);

// Appends new non-branch nodes to the BDD in place of the provided current
// node.
// Clones all new_nodes and appends them to the BDD.
Node *add_non_branch_nodes_to_bdd(BDD *bdd, const Node *current,
                                  const std::vector<const Node *> &new_nodes);

// Appends a single new branch node to the BDD in place of the provided current
// node. This duplicates the BDD portion starting from the current node, and
// appends the cloned portion to one of the branches.
Branch *add_branch_to_bdd(BDD *bdd, const Node *current, klee::ref<klee::Expr> condition);

Node *delete_non_branch_node_from_bdd(BDD *bdd, node_id_t target_id);

// Returns de node kept from the branch deletion.
Node *delete_branch_node_from_bdd(BDD *bdd, node_id_t target_id, bool direction_to_keep);

// Deletes all vector operations solely responsible for map key management.
void delete_all_vector_key_operations_from_bdd(BDD *bdd);

branch_direction_t find_branch_checking_index_alloc(const EP *ep, const Node *node,
                                                    const symbol_t &out_of_space);
branch_direction_t find_branch_checking_index_alloc(const EP *ep,
                                                    const Node *dchain_allocate_new_index);

bool is_tb_tracing_check_followed_by_update_on_true(const Call *tb_is_tracing,
                                                    const Call *&tb_update_and_check);

std::string int2hr(u64 value);
std::string tput2str(u64 thpt, const std::string &units, bool human_readable = false);

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