#pragma once

#include <optional>

#include "bdd/bdd.h"
#include "types.h"

typedef std::vector<klee::ref<klee::Expr>> constraints_t;

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

  ProfilerNode(klee::ref<klee::Expr> _constraint, hit_rate_t _fraction);
  ProfilerNode(klee::ref<klee::Expr> _constraint, hit_rate_t _fraction,
               node_id_t _bdd_node_id);
  ~ProfilerNode();

  ProfilerNode *clone(bool keep_bdd_info) const;
  void log_debug(int lvl = 0) const;

  bool get_flow_stats(klee::ref<klee::Expr> flow_id,
                      FlowStats &flow_stats) const;
};

class Profiler {
private:
  bdd_profile_t bdd_profile;
  ProfilerNode *root;

public:
  Profiler(const BDD *bdd, const bdd_profile_t &_bdd_profile);
  Profiler(const BDD *bdd, const std::string &_bdd_profile_fname);
  Profiler(const BDD *bdd);

  Profiler(const Profiler &other);
  Profiler(Profiler &&other);

  ~Profiler();

  int get_avg_pkt_bytes() const;

  void insert(const constraints_t &constraints,
              klee::ref<klee::Expr> constraint, hit_rate_t fraction_on_true);
  void insert_relative(const constraints_t &constraints,
                       klee::ref<klee::Expr> constraint,
                       hit_rate_t rel_fraction_on_true);
  void remove(const constraints_t &constraints);
  void scale(const constraints_t &constraints, hit_rate_t factor);

  const ProfilerNode *get_root() const { return root; }
  std::optional<hit_rate_t>
  get_fraction(const constraints_t &constraints) const;
  std::optional<FlowStats> get_flow_stats(const constraints_t &constraints,
                                          klee::ref<klee::Expr> flow_id) const;

  void log_debug() const;

private:
  ProfilerNode *get_node(const constraints_t &constraints) const;

  void append(ProfilerNode *node, klee::ref<klee::Expr> constraint,
              hit_rate_t fraction);
  void remove(ProfilerNode *node);
  void replace_root(klee::ref<klee::Expr> constraint, hit_rate_t fraction);
};