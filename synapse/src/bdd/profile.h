#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "nodes/node.h"
#include "../types.h"

namespace synapse {
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

  struct map_stats_t {
    struct node_t {
      node_id_t node;
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
  std::unordered_map<node_id_t, u64> counters;

  fpm_t churn_top_k_flows(u64 map, u32 k) const;
  hit_rate_t churn_hit_rate_top_k_flows(u64 map, u32 k) const;
  u64 threshold_top_k_flows(u64 map, u32 k) const;
};

bdd_profile_t parse_bdd_profile(const std::string &filename);
} // namespace synapse