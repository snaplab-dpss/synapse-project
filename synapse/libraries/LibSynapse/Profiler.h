#pragma once

#include <LibBDD/BDD.h>
#include <LibBDD/Profile.h>
#include <LibBDD/Reorder.h>
#include <LibCore/Types.h>

#include <optional>
#include <vector>

namespace LibSynapse {

using LibBDD::BDD;
using LibBDD::bdd_node_id_t;
using LibBDD::bdd_profile_t;
using LibBDD::BDDNode;
using LibBDD::Call;
using LibBDD::RouteOp;
using LibBDD::symbol_translation_t;

using LibCore::SymbolManager;

class EP;
class EPNode;

struct flow_stats_t {
  klee::ref<klee::Expr> flow_id;
  u64 pkts;
  u64 flows;
  std::vector<u64> pkts_per_flow;

  hit_rate_t calculate_top_k_hit_rate(size_t k) const {
    u64 top_k = 0;
    for (size_t i = 0; i <= k && i < pkts_per_flow.size(); i++) {
      top_k += pkts_per_flow[i];
    }

    assert(top_k <= pkts && "Invalid top_k");
    return hit_rate_t(top_k, pkts);
  }
};

struct fwd_stats_t {
  RouteOp operation;
  hit_rate_t drop;
  hit_rate_t flood;
  std::unordered_map<u16, hit_rate_t> ports;

  hit_rate_t calculate_total_hr() const {
    hit_rate_t total_hr = drop + flood;
    for (const auto &[_, hr] : ports) {
      total_hr = total_hr + hr;
    }
    return total_hr;
  }

  hit_rate_t calculate_fwd_hr() const {
    hit_rate_t total_hr = 0_hr;
    for (const auto &[_, hr] : ports) {
      total_hr = total_hr + hr;
    }
    return total_hr;
  }

  u16 get_most_used_fwd_port() const {
    u16 max_port      = 0;
    hit_rate_t max_hr = 0_hr;
    for (const auto &[port, hr] : ports) {
      if (hr > max_hr) {
        max_hr   = hr;
        max_port = port;
      }
    }
    return max_port;
  }

  fwd_stats_t scale(const double factor) const {
    fwd_stats_t new_fwd_stats;

    new_fwd_stats.operation = operation;
    new_fwd_stats.drop      = drop * factor;
    new_fwd_stats.flood     = flood * factor;
    for (const auto &[port, hr] : ports) {
      new_fwd_stats.ports[port] = hr * factor;
    }

    const hit_rate_t total_old_hr  = calculate_total_hr();
    const hit_rate_t total_goal_hr = total_old_hr * factor;
    const hit_rate_t total_new_hr  = new_fwd_stats.calculate_total_hr();

    if (total_goal_hr != total_new_hr) {
      // We lost precision in the scaling, so we need to adjust the ports.
      const hit_rate_t delta = total_goal_hr - total_new_hr;
      assert(delta >= 0_hr && "Scaling resulted in a negative hit rate");

      const u16 most_used_port            = get_most_used_fwd_port();
      new_fwd_stats.ports[most_used_port] = new_fwd_stats.ports[most_used_port] + delta;
    }

    assert(new_fwd_stats.calculate_total_hr() == total_goal_hr && "Scaling resulted in an incorrect total hit rate");

    return new_fwd_stats;
  }
};

struct ProfilerNode {
  klee::ref<klee::Expr> constraint;
  hit_rate_t fraction;
  std::optional<bdd_node_id_t> bdd_node_id;
  std::vector<flow_stats_t> flows_stats;
  std::optional<fwd_stats_t> forwarding_stats;
  std::optional<fwd_stats_t> original_forwarding_stats;

  // During search, sometimes we need to rebalance the forwarding statistics. Additionally, sometimes we want to extrapolate forwarding statistics
  // (for example, a uniform distribution across _valid_ ports). For that, we need to actually know which ports are valid. This is the main purpose of
  // this "candidate_fwd_ports". It contains all the _valid_ candidate ports for a specific forwarding node on the NF.
  std::unordered_set<u16> candidate_fwd_ports;

  ProfilerNode *on_true;
  ProfilerNode *on_false;
  ProfilerNode *prev;

  ProfilerNode(klee::ref<klee::Expr> cnstr, hit_rate_t hr);
  ProfilerNode(klee::ref<klee::Expr> cnstr, hit_rate_t hr, bdd_node_id_t node);
  ~ProfilerNode();

  ProfilerNode *clone(bool keep_bdd_info) const;
  void debug(int lvl = 0) const;

  flow_stats_t get_flow_stats(klee::ref<klee::Expr> flow_id) const;

  struct family_t {
    ProfilerNode *node;
    ProfilerNode *parent;
    ProfilerNode *grandparent;
    ProfilerNode *sibling;
  };

  family_t get_family();
};

class Profiler {
private:
  const std::shared_ptr<bdd_profile_t> bdd_profile;

  std::shared_ptr<ProfilerNode> root;
  bytes_t avg_pkt_size;

  // Not the prettiest solution, but will do.
  // We cache on reads, and invalidate on writes.
  mutable struct {
    std::unordered_map<bdd_node_id_t, ProfilerNode *> n2p;
    std::unordered_map<ProfilerNode *, bdd_node_id_t> p2n;
    std::unordered_map<bdd_node_id_t, ProfilerNode *> e2p;
    std::unordered_map<ProfilerNode *, bdd_node_id_t> p2e;
  } cache;

public:
  Profiler(const BDD *bdd, const bdd_profile_t &bdd_profile, const std::unordered_set<u16> &available_devs);

  Profiler(const Profiler &other);
  Profiler(Profiler &&other);

  Profiler &operator=(const Profiler &other);

  const bdd_profile_t *get_bdd_profile() const;
  bytes_t get_avg_pkt_bytes() const;

  // Estimation is relative to the parent node.
  // E.g. if the parent node has a hit rate of 0.5, and the estimation_rel is
  // 0.1, the hit rate of the current node will be 0.05.
  // WARNING: this should be called before processing the leaf.
  void insert_relative(const std::vector<klee::ref<klee::Expr>> &cnstrs, klee::ref<klee::Expr> cnstr, hit_rate_t rel_hr_on_true);

  void translate(SymbolManager *symbol_manager, const BDDNode *reordered_node, const std::vector<symbol_translation_t> &translated_symbols);
  void replace_constraint(const std::vector<klee::ref<klee::Expr>> &cnstrs, klee::ref<klee::Expr> cnstr);
  void remove(const std::vector<klee::ref<klee::Expr>> &constraints);
  void remove_until(const std::vector<klee::ref<klee::Expr>> &target, const std::vector<klee::ref<klee::Expr>> &stopping_constraints);
  void scale(const std::vector<klee::ref<klee::Expr>> &constraints, double factor);
  void set(const std::vector<klee::ref<klee::Expr>> &constraints, hit_rate_t new_hr);
  void set_relative(const std::vector<klee::ref<klee::Expr>> &constraints, hit_rate_t parent_relative_hr);

  hit_rate_t get_hr(const EPNode *node) const;
  hit_rate_t get_hr(const BDDNode *node) const;

  fwd_stats_t get_fwd_stats(const EPNode *node) const;
  fwd_stats_t get_fwd_stats(const BDDNode *node) const;

  std::unordered_set<u16> get_candidate_fwd_ports(const EPNode *node) const;
  std::unordered_set<u16> get_candidate_fwd_ports(const BDDNode *node) const;

  flow_stats_t get_flow_stats(const std::vector<klee::ref<klee::Expr>> &cnstrs, klee::ref<klee::Expr> flow) const;
  rw_fractions_t get_cond_map_put_rw_profile_fractions(const Call *map_get) const;

  void clear_cache() const;
  void debug() const;

private:
  ProfilerNode *get_node(const std::vector<klee::ref<klee::Expr>> &cnstrs) const;
  ProfilerNode *get_node(const EPNode *node) const;
  ProfilerNode *get_node(const BDDNode *node) const;

  hit_rate_t get_hr(const std::vector<klee::ref<klee::Expr>> &cnstrs) const;
  fwd_stats_t get_fwd_stats(const std::vector<klee::ref<klee::Expr>> &cnstrs) const;

  void clone_tree_if_shared();
  void append(ProfilerNode *node, klee::ref<klee::Expr> cnstr, hit_rate_t hr);
  void remove(ProfilerNode *node);
  void remove_until(ProfilerNode *target, ProfilerNode *stopping_node);
  void replace_root(klee::ref<klee::Expr> cnstr, hit_rate_t hr);
  void replace_constraint(ProfilerNode *node, klee::ref<klee::Expr> cnstr);
  void set(ProfilerNode *node, hit_rate_t new_hr);

  bool can_update_fractions(ProfilerNode *node) const;
  bool can_recursive_update_fractions(ProfilerNode *node, hit_rate_t parent_old_fraction) const;

  void recursive_update_fractions(ProfilerNode *node, hit_rate_t parent_old_fraction, hit_rate_t parent_new_fraction);
  void update_fractions(ProfilerNode *node, hit_rate_t new_fraction);
};

} // namespace LibSynapse