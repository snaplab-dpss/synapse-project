#pragma once

#include <optional>

#include "bdd/bdd.hpp"
#include "types.hpp"

namespace synapse {
class EP;
class EPNode;

struct FlowStats {
  klee::ref<klee::Expr> flow_id;
  u64 pkts;
  u64 flows;
  std::vector<u64> pkts_per_flow;
};

struct ProfilerNode {
  klee::ref<klee::Expr> constraint;
  hit_rate_t fraction;
  std::optional<node_id_t> bdd_node_id;
  std::vector<FlowStats> flows_stats;

  ProfilerNode *on_true;
  ProfilerNode *on_false;
  ProfilerNode *prev;

  ProfilerNode(klee::ref<klee::Expr> cnstr, hit_rate_t hr);
  ProfilerNode(klee::ref<klee::Expr> cnstr, hit_rate_t hr, node_id_t node);
  ~ProfilerNode();

  ProfilerNode *clone(bool keep_bdd_info) const;
  void debug(int lvl = 0) const;

  FlowStats get_flow_stats(klee::ref<klee::Expr> flow_id) const;
};

class Profiler {
private:
  std::shared_ptr<bdd_profile_t> bdd_profile;

  std::shared_ptr<ProfilerNode> root;
  bytes_t avg_pkt_size;

  // Not the prettiest solution, but will do.
  // We cache on reads, and invalidate on writes.
  mutable struct {
    std::unordered_map<node_id_t, const ProfilerNode *> n2p;
    std::unordered_map<const ProfilerNode *, node_id_t> p2n;
    std::unordered_map<ep_node_id_t, const ProfilerNode *> e2p;
    std::unordered_map<const ProfilerNode *, ep_node_id_t> p2e;
  } cache;

public:
  Profiler(const BDD *bdd, const bdd_profile_t &bdd_profile);
  Profiler(const BDD *bdd, const std::string &bdd_profile_fname);
  Profiler(const BDD *bdd);

  Profiler(const Profiler &other);
  Profiler(Profiler &&other);

  Profiler &operator=(const Profiler &other);

  const bdd_profile_t *get_bdd_profile() const;
  bytes_t get_avg_pkt_bytes() const;

  // Estimation is relative to the parent node.
  // E.g. if the parent node has a hit rate of 0.5, and the estimation_rel is
  // 0.1, the hit rate of the current node will be 0.05.
  // WARNING: this should be called before processing the leaf.
  void insert_relative(const constraints_t &cnstrs, klee::ref<klee::Expr> cnstr, hit_rate_t rel_hr_on_true);

  void replace_constraint(const constraints_t &cnstrs, klee::ref<klee::Expr> cnstr);
  void remove(const constraints_t &constraints);
  void scale(const constraints_t &constraints, hit_rate_t factor);
  void set(const constraints_t &constraints, hit_rate_t new_hr);

  hit_rate_t get_hr(const Node *node) const;
  hit_rate_t get_hr(const EPNode *node) const;

  FlowStats get_flow_stats(const constraints_t &cnstrs, klee::ref<klee::Expr> flow) const;

  void clear_cache() const;
  void debug() const;

private:
  ProfilerNode *get_node(const constraints_t &cnstrs) const;
  hit_rate_t get_hr(const constraints_t &cnstrs) const;

  struct family_t {
    ProfilerNode *node;
    ProfilerNode *parent;
    ProfilerNode *grandparent;
    ProfilerNode *sibling;
  };

  family_t get_family(ProfilerNode *node) const;

  void clone_tree_if_shared();
  void append(ProfilerNode *node, klee::ref<klee::Expr> cnstr, hit_rate_t hr);
  void remove(ProfilerNode *node);
  void replace_root(klee::ref<klee::Expr> cnstr, hit_rate_t hr);
  void replace_constraint(ProfilerNode *node, klee::ref<klee::Expr> cnstr);
};
} // namespace synapse