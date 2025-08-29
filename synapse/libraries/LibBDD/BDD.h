#pragma once

#include <LibCore/Debug.h>

#include <LibBDD/CallPath.h>
#include <LibBDD/Profile.h>
#include <LibBDD/Nodes/Nodes.h>
#include <LibBDD/Nodes/NodeManager.h>
#include <LibBDD/Config.h>
#include <LibBDD/Visitors/BDDVisualizer.h>

namespace LibBDD {

using LibCore::symbol_t;
using LibCore::SymbolManager;
using LibCore::Symbols;

class BDDVisitor;

class BDD {
private:
  bdd_node_id_t id;

  symbol_t device;
  symbol_t packet_len;
  symbol_t time;

  klee::ConstraintManager base_constraints;
  std::vector<Call *> init;
  BDDNode *root;

  BDDNodeManager manager;
  SymbolManager *symbol_manager;

public:
  BDD(SymbolManager *symbol_manager);

  BDD(const call_paths_view_t &call_paths);
  BDD(const std::filesystem::path &bdd_file, SymbolManager *symbol_manager);

  BDD(const BDD &other);
  BDD(BDD &&other);

  BDD &operator=(const BDD &other);

  bdd_node_id_t get_id() const { return id; }
  bdd_node_id_t &get_mutable_id() { return id; }

  symbol_t get_device() const { return device; }
  symbol_t get_packet_len() const { return packet_len; }
  symbol_t get_time() const { return time; }

  void set_device(const symbol_t &new_device) { device = new_device; }
  void set_packet_len(const symbol_t &new_packet_len) { packet_len = new_packet_len; }
  void set_time(const symbol_t &new_time) { time = new_time; }

  const BDDNode *get_root() const { return root; }
  const std::vector<Call *> &get_init() const { return init; }

  void set_root(BDDNode *_root) {
    root = _root;
    assert_or_panic(manager.has_node(_root), "Root node is not managed by the BDDNodeManager");
  }

  void set_init(const std::vector<Call *> &new_init) {
    init = new_init;
    for (Call *call : init) {
      assert_or_panic(manager.has_node(call), "Init node is not managed by the BDDNodeManager");
    }
  }

  std::string hash() const { return root->hash(true); }
  size_t size() const { return root->count_children(true) + 1; }
  std::unordered_set<u16> get_devices() const;

  Symbols get_generated_symbols(const BDDNode *node) const;
  void visit(BDDVisitor &visitor) const;
  void serialize(const std::filesystem::path &fpath) const;
  void deserialize(const std::filesystem::path &fpath);
  int get_node_depth(bdd_node_id_t id) const;

  enum class InspectionStatus {
    Ok,
    MissingRootNode,
    HasNullNode,
    BranchWithoutChildren,
    BrokenLink,
    MissingSymbol,
    DanglingInitNode,
    HasCycle,
    UnmanagedNode,
  };

  struct inspection_report_t {
    InspectionStatus status;
    std::string message;
  };

  // Debug operation that acts as an inspector, checking the BDD integrity and symbol availability.
  // Returns true if the BDD is valid, false otherwise.
  // A valid BDD should always pass this check.
  [[nodiscard]] inspection_report_t inspect() const;

  klee::ConstraintManager get_constraints(const BDDNode *node) const;
  bool get_map_coalescing_objs(addr_t obj, map_coalescing_objs_t &data) const;
  bool get_map_coalescing_objs_from_map_op(const Call *map_op, map_coalescing_objs_t &map_objs) const;
  bool get_map_coalescing_objs_from_dchain_op(const Call *dchain_op, map_coalescing_objs_t &map_objs) const;
  bool is_index_alloc_on_unsuccessful_map_get(const Call *dchain_allocate_new_index) const;
  bool is_map_update_with_dchain(const Call *dchain_allocate_new_index, std::vector<const Call *> &map_puts) const;
  branch_direction_t find_branch_checking_index_alloc(const Call *dchain_allocate_new_index) const;
  bool is_fwd_pattern_depending_on_lpm(const BDDNode *node, std::vector<const BDDNode *> &fwd_logic) const;
  bool is_tb_tracing_check_followed_by_update_on_true(const Call *tb_is_tracing, const Call *&tb_update_and_check) const;
  void delete_vector_key_operations(addr_t map);
  bool is_dchain_used_for_index_allocation_queries(addr_t dchain) const;
  bool is_dchain_used_exclusively_for_linking_maps_with_vectors(addr_t dchain) const;
  bool is_map_get_and_branch_checking_success(const Call *map_get, const BDDNode *branch_checking_map_get_success, bool &success_direction) const;
  bool are_subtrees_equal(const BDDNode *n0, const BDDNode *n1) const;
  std::unordered_set<u16> get_candidate_fwd_devs(const Route *route) const;

  // Tries to find the pattern of a map_get followed by map_puts, but only when
  // the map_get is not successful (i.e. the key is not found).
  // Conditions to meet:
  // (1) Has at least 1 future map_put
  // (2) All map_put happen if the map_get was not successful
  // (3) All map_puts with the target obj also have the same key as the map_get
  bool is_map_get_followed_by_map_puts_on_miss(const Call *map_get, std::vector<const Call *> &map_puts) const;

  const BDDNode *get_node_by_id(bdd_node_id_t id) const;
  BDDNode *get_mutable_node_by_id(bdd_node_id_t id);

  BDDNodeManager &get_mutable_manager() { return manager; }
  const BDDNodeManager &get_manager() const { return manager; }

  SymbolManager *get_mutable_symbol_manager() { return symbol_manager; }
  const SymbolManager *get_symbol_manager() const { return symbol_manager; }

  enum class BranchDeletionAction { KeepOnTrue, KeepOnFalse, DeleteBoth };

  void delete_init_node(bdd_node_id_t target_id);
  BDDNode *delete_non_branch(bdd_node_id_t target_id);
  BDDNode *delete_branch(bdd_node_id_t target_id, BranchDeletionAction branch_deletion_action);
  std::vector<BDDNode *> delete_until(bdd_node_id_t target_id, const bdd_node_ids_t &stopping_points);

  Branch *create_new_branch(klee::ref<klee::Expr> condition);
  Call *create_new_call(const BDDNode *current, const call_t &call, const Symbols &generated_symbols);

  Call *add_new_symbol_generator_function(bdd_node_id_t target_id, const std::string &fn_name, const Symbols &symbols);
  BDDNode *add_cloned_non_branches(bdd_node_id_t target_id, const std::vector<const BDDNode *> &new_nodes);
  Branch *add_cloned_branch(bdd_node_id_t target_id, klee::ref<klee::Expr> condition);

  static BDDNode *delete_non_branch(BDDNode *target, BDDNodeManager &manager);
  static BDDNode *delete_branch(BDDNode *target, BranchDeletionAction branch_deletion_action, BDDNodeManager &manager);
};

std::ostream &operator<<(std::ostream &os, const BDD::inspection_report_t &report);

} // namespace LibBDD