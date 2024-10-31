#pragma once

#include <optional>

#include "bdd/bdd.h"
#include "types.h"

class EP;
class EPNode;

struct FlowStats {
  klee::ref<klee::Expr> flow_id;
  u64 packets;
  u64 flows;
  u64 avg_pkts_per_flow;
  std::vector<u64> packets_per_flow;
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

  bool get_flow_stats(klee::ref<klee::Expr> flow_id,
                      FlowStats &flow_stats) const;
};

class Profiler {
private:
  std::shared_ptr<ProfilerNode> root;
  int avg_pkt_size;

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

  int get_avg_pkt_bytes() const;

  void insert(const constraints_t &cnstrs, klee::ref<klee::Expr> cnstr,
              hit_rate_t hr_on_true);
  void insert_relative(const constraints_t &cnstrs, klee::ref<klee::Expr> cnstr,
                       hit_rate_t rel_hr_on_true);
  void remove(const constraints_t &constraints);
  void scale(const constraints_t &constraints, hit_rate_t factor);

  hit_rate_t get_hr(const Node *node) const;
  hit_rate_t get_hr(const EP *ep, const EPNode *node) const;

  std::optional<FlowStats> get_flow_stats(const constraints_t &cnstrs,
                                          klee::ref<klee::Expr> flow) const;

  void clear_cache() const;
  void debug() const;

private:
  ProfilerNode *get_node(const constraints_t &cnstrs) const;
  hit_rate_t get_hr(const constraints_t &cnstrs) const;

  void clone_tree_if_shared();
  void append(ProfilerNode *node, klee::ref<klee::Expr> cnstr, hit_rate_t hr);
  void remove(ProfilerNode *node);
  void replace_root(klee::ref<klee::Expr> cnstr, hit_rate_t hr);
};