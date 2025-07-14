#pragma once

#include <LibCore/Types.h>
#include <LibBDD/Nodes/Node.h>

#include <filesystem>
#include <cstdbool>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace LibBDD {

struct dev_pcap_t {
  u16 device;
  std::string pcap;
  bool warmup;
};

struct bdd_profile_t {
  struct config_t {
    std::vector<dev_pcap_t> pcaps;
  };

  struct meta_t {
    u64 pkts;
    u64 bytes;
  };

  struct fwd_stats_t {
    std::unordered_map<u16, u64> ports;
    u64 drop;
    u64 flood;

    u64 get_total_fwd() const {
      u64 total = 0;
      for (const auto &[_, count] : ports) {
        total += count;
      }
      return total;
    }
  };

  struct map_stats_t {
    struct node_t {
      bdd_node_id_t node;
      u64 pkts;
      u64 flows;
      std::vector<u64> pkts_per_flow; // Sorted by descending order.
      std::unordered_map<u32, u32> crc32_hashes_per_mask;
    };

    struct epoch_t {
      time_ns_t dt_ns;
      bool warmup;
      u64 pkts;
      u64 flows;
      std::vector<u64> pkts_per_persistent_flow; // Sorted by descending order.
      std::vector<u64> pkts_per_new_flow;        // Sorted by descending order.
    };

    std::vector<node_t> nodes;
    std::vector<epoch_t> epochs;
  };

  config_t config;
  meta_t meta;
  std::unordered_map<u64, map_stats_t> stats_per_map;
  std::unordered_map<bdd_node_id_t, u64> counters;
  std::unordered_map<bdd_node_id_t, fwd_stats_t> forwarding_stats;

  fpm_t churn_top_k_flows(u64 map, u32 k) const;
  hit_rate_t churn_hit_rate_top_k_flows(u64 map, u32 k) const;
  u64 threshold_top_k_flows(u64 map, u32 k) const;
};

bdd_profile_t parse_bdd_profile(const std::filesystem::path &filename);
bdd_profile_t build_random_bdd_profile(const BDD *bdd);

} // namespace LibBDD