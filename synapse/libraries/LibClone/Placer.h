#pragma once

#include <LibCore/Debug.h>
#include <LibCore/Symbol.h>
#include <LibCore/Types.h>

#include <LibBDD/BDD.h>
#include <LibBDD/Nodes/Node.h>

#include <LibClone/PhysicalNetwork.h>

namespace LibClone {

using LibCore::Symbols;

using LibBDD::BDD;
using LibBDD::bdd_node_id_t;
using LibBDD::bdd_node_ids_t;
using LibBDD::BDDNode;
using LibBDD::Branch;

struct target_roots_t {
  bdd_node_ids_t port_roots;
  bdd_node_ids_t target_roots;
};

class Placer {
private:
  std::shared_ptr<const BDD> bdd;
  const PhysicalNetwork &phys_net;
  std::unordered_map<bdd_node_id_t, Symbols> shared_context;

public:
  Placer(const BDD &_bdd, const PhysicalNetwork &_phys_net);

  Placer(const Placer &)            = delete;
  Placer &operator=(const Placer &) = delete;

  const BDD *get_bdd() const { return bdd.get(); }
  const PhysicalNetwork &get_physical_network() const { return phys_net; }

  std::unordered_map<LibSynapse::TargetType, std::unique_ptr<const BDD>> process();

  void debug() const;
  std::unique_ptr<BDD> add_send_to_device_nodes();

private:
  const std::vector<const BDDNode *> create_send_to_device_node(std::unique_ptr<BDD> &_bdd, bdd_node_id_t current, bdd_node_id_t next_node);
  BDDNode *create_parse_header_cpu_node(std::unique_ptr<BDD> &_bdd);
  std::optional<BDDNode *> create_parse_header_vars_node(std::unique_ptr<BDD> &_bdd, bdd_node_id_t current_id, bdd_node_id_t next_id);
  void handle_branch_node(std::unique_ptr<BDD> &_bdd, bdd_node_id_t branch_id, bdd_node_id_t on_true_id, bdd_node_id_t on_false_id, bool in_root);
  void handle_node(std::unique_ptr<BDD> &_bdd, bdd_node_id_t current_id, bdd_node_id_t next_id);
  std::vector<const BDDNode *> retreive_prev_hdr_parsing_ops(const BDDNode *node);
  Symbols get_relevant_dataplane_state(std::unique_ptr<BDD> &_bdd, const BDDNode *node, const bdd_node_ids_t &target_roots);
  bdd_node_ids_t get_target_global_port_roots(std::unique_ptr<BDD> &_bdd, LibSynapse::TargetType target);
  bdd_node_ids_t get_target_roots(std::unique_ptr<BDD> &_bdd, LibSynapse::TargetType target);
  std::unique_ptr<const BDD> concretize_ports(std::unique_ptr<BDD> &_bdd);
  std::unique_ptr<BDD> extract_target_bdd(std::unique_ptr<BDD> &_bdd, bdd_node_ids_t port_roots, bdd_node_ids_t roots);
  std::unique_ptr<BDD> extract_target_bdd(std::unique_ptr<BDD> &_bdd, bdd_node_ids_t roots);
  std::unordered_map<LibSynapse::TargetType, target_roots_t> get_target_roots(std::unique_ptr<BDD> &_bdd);
};
} // namespace LibClone
