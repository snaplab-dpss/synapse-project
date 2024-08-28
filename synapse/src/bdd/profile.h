#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "nodes/node.h"
#include "../types.h"

struct dev_pcap_t {
  u16 device;
  std::string pcap;
  bool warmup;
};

struct bdd_profile_t {
  struct config_t {
    std::vector<dev_pcap_t> pcaps;
  } config;

  struct meta_t {
    u64 packets;
    u64 bytes;
    int avg_pkt_size;
  } meta;

  struct map_stats_t {
    node_id_t node;
    u64 packets;
    u64 flows;
    u64 avg_pkts_per_flow;
    std::vector<u64> packets_per_flow;
  };

  std::vector<map_stats_t> map_stats;

  std::unordered_map<node_id_t, u64> counters;
};

bdd_profile_t parse_bdd_profile(const std::string &filename);