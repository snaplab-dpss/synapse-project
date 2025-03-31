#pragma once

#include <LibBDD/BDD.h>
#include <LibBDD/Profile.h>
#include <LibBDD/Reorder.h>
#include <LibCore/Types.h>

#include <optional>
#include <vector>

namespace LibSynapse {

class EP;
class EPNode;

struct flow_stats_t {
  klee::ref<klee::Expr> flow_id;
  u64 pkts;
  u64 flows;
  std::vector<u64> pkts_per_flow;
};

struct fwd_stats_t {
  hit_rate_t drop;
  hit_rate_t flood;
  std::unordered_map<u16, hit_rate_t> ports;

  bool is_drop_only() const {
    bool drop_only = (flood == 0);
    for (const auto &[_, hr] : ports) {
      drop_only &= (hr == 0);
    }
    return drop_only;
  }

  bool is_flood_only() const {
    bool flood_only = (drop == 0);
    for (const auto &[_, hr] : ports) {
      flood_only &= (hr == 0);
    }
    return flood_only;
  }

  bool is_fwd_only() const { return (drop == 0) && (flood == 0); }

  hit_rate_t calculate_total_hr() const {
    hit_rate_t total_hr = drop + flood;
    for (const auto &[_, hr] : ports) {
      total_hr = total_hr + hr;
    }
    return total_hr;
  }
};

struct ProfilerNode {
  klee::ref<klee::Expr> constraint;
  hit_rate_t fraction;
  std::optional<LibBDD::node_id_t> bdd_node_id;
  std::vector<flow_stats_t> flows_stats;
  std::optional<fwd_stats_t> forwarding_stats;

  ProfilerNode *on_true;
  ProfilerNode *on_false;
  ProfilerNode *prev;

  ProfilerNode(klee::ref<klee::Expr> cnstr, hit_rate_t hr);
  ProfilerNode(klee::ref<klee::Expr> cnstr, hit_rate_t hr, LibBDD::node_id_t node);
  ~ProfilerNode();

  ProfilerNode *clone(bool keep_bdd_info) const;
  void debug(int lvl = 0) const;

  flow_stats_t get_flow_stats(klee::ref<klee::Expr> flow_id) const;
};

class Profiler {
private:
  std::shared_ptr<LibBDD::bdd_profile_t> bdd_profile;

  std::shared_ptr<ProfilerNode> root;
  bytes_t avg_pkt_size;

  // Not the prettiest solution, but will do.
  // We cache on reads, and invalidate on writes.
  mutable struct {
    std::unordered_map<LibBDD::node_id_t, ProfilerNode *> n2p;
    std::unordered_map<ProfilerNode *, LibBDD::node_id_t> p2n;
    std::unordered_map<LibBDD::node_id_t, ProfilerNode *> e2p;
    std::unordered_map<ProfilerNode *, LibBDD::node_id_t> p2e;
  } cache;

public:
  Profiler(const LibBDD::BDD *bdd, const LibBDD::bdd_profile_t &bdd_profile);
  Profiler(const LibBDD::BDD *bdd, const std::filesystem::path &bdd_profile_fname);
  Profiler(const LibBDD::BDD *bdd);

  Profiler(const Profiler &other);
  Profiler(Profiler &&other);

  Profiler &operator=(const Profiler &other);

  const LibBDD::bdd_profile_t *get_bdd_profile() const;
  bytes_t get_avg_pkt_bytes() const;

  // Estimation is relative to the parent node.
  // E.g. if the parent node has a hit rate of 0.5, and the estimation_rel is
  // 0.1, the hit rate of the current node will be 0.05.
  // WARNING: this should be called before processing the leaf.
  void insert_relative(const std::vector<klee::ref<klee::Expr>> &cnstrs, klee::ref<klee::Expr> cnstr, hit_rate_t rel_hr_on_true);

  void translate(LibCore::SymbolManager *symbol_manager, const LibBDD::Node *reordered_node,
                 const std::vector<LibBDD::translated_symbol_t> &translated_symbols);
  void replace_constraint(const std::vector<klee::ref<klee::Expr>> &cnstrs, klee::ref<klee::Expr> cnstr);
  void remove(const std::vector<klee::ref<klee::Expr>> &constraints);
  void scale(const std::vector<klee::ref<klee::Expr>> &constraints, double factor);
  void set(const std::vector<klee::ref<klee::Expr>> &constraints, hit_rate_t new_hr);

  hit_rate_t get_hr(const EPNode *node) const;
  hit_rate_t get_hr(const LibBDD::Node *node) const;

  fwd_stats_t get_fwd_stats(const EPNode *node) const;
  fwd_stats_t get_fwd_stats(const LibBDD::Node *node) const;

  flow_stats_t get_flow_stats(const std::vector<klee::ref<klee::Expr>> &cnstrs, klee::ref<klee::Expr> flow) const;
  rw_fractions_t get_cond_map_put_rw_profile_fractions(const LibBDD::Call *map_get) const;

  void clear_cache() const;
  void debug() const;

private:
  ProfilerNode *get_node(const std::vector<klee::ref<klee::Expr>> &cnstrs) const;
  ProfilerNode *get_node(const EPNode *node) const;
  ProfilerNode *get_node(const LibBDD::Node *node) const;

  hit_rate_t get_hr(const std::vector<klee::ref<klee::Expr>> &cnstrs) const;
  fwd_stats_t get_fwd_stats(const std::vector<klee::ref<klee::Expr>> &cnstrs) const;

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

} // namespace LibSynapse