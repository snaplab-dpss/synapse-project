#pragma once

#include <vector>
#include <string>
#include <unordered_set>
#include <optional>
#include <stdint.h>

#include "call_paths/call_paths.h"
#include "bdd/tree.h"
#include "exprs/exprs.h"

enum class ModuleType;
class EP;
class Module;
class Context;

bool check_same_obj(const Call *call0, const Call *call1,
                    const std::string &obj_name);

std::vector<modification_t>
build_vector_modifications(const Call *vector_borrow,
                           const Call *vector_return);
std::vector<modification_t>
build_hdr_modifications(const Call *packet_borrow_next_chunk,
                        const Call *packet_return_chunk);

std::vector<modification_t>
ignore_checksum_modifications(const std::vector<modification_t> &modifications);

symbol_t create_symbol(const std::string &label, bits_t size);

bool query_contains_map_has_key(const Branch *node);

const Call *packet_borrow_from_return(const EP *ep,
                                      const Call *packet_return_chunk);

std::vector<const Call *>
get_prev_functions(const EP *ep, const Node *node,
                   const std::vector<std::string> &functions_names,
                   bool ignore_targets = false);

std::vector<const Call *>
get_future_functions(const Node *root,
                     const std::vector<std::string> &functions,
                     bool stop_on_branches = false);

bool is_parser_drop(const Node *root);

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

std::vector<const Module *>
get_prev_modules(const EP *ep, const std::vector<ModuleType> &types);

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
klee::ref<klee::Expr> get_original_vector_value(const EP *ep, const Node *node,
                                                addr_t target_addr);
klee::ref<klee::Expr> get_original_vector_value(const EP *ep, const Node *node,
                                                addr_t target_addr,
                                                const Node *&source);

bool is_vector_return_without_modifications(const EP *ep, const Node *node);
bool is_vector_read(const Call *vector_borrow);

std::vector<const Call *> get_future_vector_return(const Call *vector_borrow);

// Get the data associated with this address.
klee::ref<klee::Expr> get_expr_from_addr(const EP *ep, addr_t addr);

struct map_coalescing_objs_t {
  addr_t map;
  addr_t dchain;
  addr_t vector_key;
  objs_t vectors_values;
};

bool get_map_coalescing_objs_from_bdd(const BDD *bdd, addr_t obj,
                                      map_coalescing_objs_t &data);
bool get_map_coalescing_objs_from_dchain_op(const EP *ep, const Call *dchain_op,
                                            map_coalescing_objs_t &map_objs);
bool get_map_coalescing_objs_from_map_op(const EP *ep, const Call *map_op,
                                         map_coalescing_objs_t &map_objs);

std::vector<const Call *>
get_coalescing_nodes_from_key(const BDD *bdd, const Node *node,
                              klee::ref<klee::Expr> key,
                              const map_coalescing_objs_t &data);

klee::ref<klee::Expr> get_chunk_from_borrow(const Node *node);
bool borrow_has_var_len(const Node *node);

symbols_t get_prev_symbols(const Node *node,
                           const nodes_t &stop_nodes = nodes_t());

struct rw_fractions_t {
  hit_rate_t read;
  hit_rate_t write_attempt;
  hit_rate_t write;
};

rw_fractions_t get_cond_map_put_rw_profile_fractions(const EP *ep,
                                                     const Node *map_get);

// Tries to find the pattern of a map_get followed by map_puts, but only when
// the map_get is not successful (i.e. the key is not found).
// Conditions to meet:
// (1) Has at least 1 future map_put
// (2) All map_put happen if the map_get was not successful
// (3) All map_puts with the target obj also have the same key as the map_get
// (4) All map_puts with the target obj update with the same value
bool is_map_get_followed_by_map_puts_on_miss(
    const BDD *bdd, const Call *map_get, std::vector<const Call *> &map_puts);

// (1) Has at least 1 future map_put
// (2) All map_put happen if the dchain_allocate_new_index was successful
// (3) All map_puts with the target obj also have the same key as the map_get
// (4) All map_puts with the target obj update with the same value
bool is_map_update_with_dchain(const EP *ep,
                               const Call *dchain_allocate_new_index,
                               std::vector<const Call *> &map_puts);

// Tries to find the pattern of a map_get followed by map_erases, but only when
// the map_get is successful (i.e. the key is found).
// Conditions to meet:
// (1) Has at least 1 future map_erase
// (2) All map_erase happen if the map_get was successful
// (3) All map_erases with the target obj also have the same key as the map_get
bool is_map_get_followed_by_map_erases_on_hit(
    const BDD *bdd, const Call *map_get, std::vector<const Call *> &map_erases);

// Appends new non-branch nodes to the BDD in place of the provided current
// node.
// Clones all new_nodes and appends them to the BDD.
void add_non_branch_nodes_to_bdd(const EP *ep, BDD *bdd, const Node *current,
                                 const std::vector<const Node *> &new_nodes,
                                 Node *&new_current);

// Appends a single new branch node to the BDD in place of the provided current
// node. This duplicates the BDD portion starting from the current node, and
// appends the cloned portion to one of the branches.
void add_branch_to_bdd(const EP *ep, BDD *bdd, const Node *current,
                       klee::ref<klee::Expr> condition, Branch *&new_branch);

void delete_non_branch_node_from_bdd(const EP *ep, BDD *bdd,
                                     node_id_t target_id, Node *&new_current);

void delete_branch_node_from_bdd(const EP *ep, BDD *bdd, node_id_t target_id,
                                 bool direction_to_keep, Node *&new_current);

const Branch *find_branch_checking_index_alloc(const EP *ep, const Node *node,
                                               const symbol_t &out_of_space);

bool is_tb_tracing_check_followed_by_update_on_true(
    const Call *tb_is_tracing, const Call *&tb_update_and_check);

std::string int2hr(u64 value);
std::string tput2str(u64 thpt, const std::string &units,
                     bool human_readable = false);

void update_fwd_tput_calcs(Context &ctx, const EP *ep, const Node *node,
                           int dst_device);
void update_s2c_tput_calc(Context &ctx, const EP *ep, const Node *node);