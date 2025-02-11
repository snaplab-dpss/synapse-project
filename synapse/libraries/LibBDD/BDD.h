#pragma once

#include <LibBDD/CallPath.h>
#include <LibBDD/Profile.h>
#include <LibBDD/Nodes/Nodes.h>
#include <LibBDD/Nodes/NodeManager.h>
#include <LibBDD/Config.h>

namespace LibBDD {

class BDDVisitor;

class BDD {
private:
  node_id_t id;

  LibCore::symbol_t device;
  LibCore::symbol_t packet_len;
  LibCore::symbol_t time;

  std::vector<Call *> init;
  Node *root;

  NodeManager manager;
  LibCore::SymbolManager *symbol_manager;

public:
  BDD();

  BDD(const call_paths_view_t &call_paths);
  BDD(const std::filesystem::path &bdd_file, LibCore::SymbolManager *symbol_manager);

  BDD(const BDD &other);
  BDD(BDD &&other);

  BDD &operator=(const BDD &other);

  node_id_t get_id() const { return id; }
  node_id_t &get_mutable_id() { return id; }

  LibCore::symbol_t get_device() const { return device; }
  LibCore::symbol_t get_packet_len() const { return packet_len; }
  LibCore::symbol_t get_time() const { return time; }

  const std::vector<Call *> &get_init() const { return init; }
  const Node *get_root() const { return root; }

  std::string hash() const { return root->hash(true); }
  size_t size() const { return root->count_children(true) + 1; }
  std::vector<u16> get_devices() const;

  LibCore::Symbols get_generated_symbols(const Node *node) const;
  void visit(BDDVisitor &visitor) const;
  void serialize(const std::filesystem::path &fpath) const;
  void deserialize(const std::filesystem::path &fpath);
  void assert_integrity() const;
  int get_node_depth(node_id_t id) const;

  bool get_map_coalescing_objs(addr_t obj, map_coalescing_objs_t &data) const;
  bool get_map_coalescing_objs_from_map_op(const Call *map_op, map_coalescing_objs_t &map_objs) const;
  bool get_map_coalescing_objs_from_dchain_op(const Call *dchain_op, map_coalescing_objs_t &map_objs) const;
  bool is_index_alloc_on_unsuccessful_map_get(const Call *dchain_allocate_new_index) const;
  bool is_map_update_with_dchain(const Call *dchain_allocate_new_index, std::vector<const Call *> &map_puts) const;
  bool is_fwd_pattern_depending_on_lpm(const Node *node, std::vector<const Node *> &fwd_logic) const;

  const Node *get_node_by_id(node_id_t id) const;
  Node *get_mutable_node_by_id(node_id_t id);

  NodeManager &get_mutable_manager() { return manager; }
  const NodeManager &get_manager() const { return manager; }

  LibCore::SymbolManager *get_mutable_symbol_manager() { return symbol_manager; }
  const LibCore::SymbolManager *get_symbol_manager() const { return symbol_manager; }

  Node *delete_non_branch(node_id_t target_id);
  Node *delete_branch(node_id_t target_id, bool direction_to_keep);
  Node *clone_and_add_non_branches(const Node *current, const std::vector<const Node *> &new_nodes);
  Branch *clone_and_add_branch(const Node *current, klee::ref<klee::Expr> condition);
};

} // namespace LibBDD