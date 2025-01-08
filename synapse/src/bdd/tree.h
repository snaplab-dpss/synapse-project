#pragma once

#include <filesystem>

#include "nodes/node.h"
#include "nodes/manager.h"
#include "visitor.h"
#include "symbex.h"

namespace synapse {
class BDDVisitor;

class BDD {
private:
  node_id_t id;

  symbol_t device;
  symbol_t packet_len;
  symbol_t time;

  std::vector<call_t> init;
  Node *root;

  NodeManager manager;
  SymbolManager *symbol_manager;

public:
  BDD();

  BDD(const call_paths_view_t &call_paths);
  BDD(const std::filesystem::path &bdd_file, SymbolManager *symbol_manager);

  BDD(const BDD &other);
  BDD(BDD &&other);

  BDD &operator=(const BDD &other);

  node_id_t get_id() const { return id; }
  node_id_t &get_mutable_id() { return id; }

  symbol_t get_device() const { return device; }
  symbol_t get_packet_len() const { return packet_len; }
  symbol_t get_time() const { return time; }

  const std::vector<call_t> &get_init() const { return init; }
  const Node *get_root() const { return root; }

  std::string hash() const { return root->hash(true); }
  size_t size() const { return root->count_children(true) + 1; }

  Symbols get_generated_symbols(const Node *node) const;
  void visit(BDDVisitor &visitor) const;
  void serialize(const std::filesystem::path &fpath) const;
  void deserialize(const std::filesystem::path &fpath);
  void assert_integrity() const;
  int get_node_depth(node_id_t id) const;

  const Node *get_node_by_id(node_id_t id) const;
  Node *get_mutable_node_by_id(node_id_t id);

  NodeManager &get_mutable_manager() { return manager; }
  const NodeManager &get_manager() const { return manager; }

  SymbolManager *get_mutable_symbol_manager() { return symbol_manager; }
  const SymbolManager *get_symbol_manager() const { return symbol_manager; }

  Node *delete_non_branch(node_id_t target_id);
  Node *delete_branch(node_id_t target_id, bool direction_to_keep);
  Node *clone_and_add_non_branches(const Node *current, const std::vector<const Node *> &new_nodes);
  Branch *clone_and_add_branch(const Node *current, klee::ref<klee::Expr> condition);
};
} // namespace synapse