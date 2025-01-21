#include <LibBDD/Profile.h>
#include <LibCore/Debug.h>

#include <fstream>
#include <nlohmann/json.hpp>

namespace LibBDD {

using json = nlohmann::json;

void from_json(const json &j, bdd_profile_t::config_t &config) {
  for (const auto &dev_pcap : j["pcaps"]) {
    dev_pcap_t elem;
    dev_pcap.at("device").get_to(elem.device);
    dev_pcap.at("pcap").get_to(elem.pcap);
    dev_pcap.at("warmup").get_to(elem.warmup);
    config.pcaps.push_back(elem);
  }
}

void from_json(const json &j, bdd_profile_t::meta_t &meta) {
  j.at("pkts").get_to(meta.pkts);
  j.at("bytes").get_to(meta.bytes);
}

void from_json(const json &j, std::unordered_map<u32, u32> &crc32_hashes_per_mask) {
  for (const auto &kv : j.items()) {
    u32 mask                    = std::stoul(kv.key());
    u32 count                   = kv.value();
    crc32_hashes_per_mask[mask] = count;
  }
}

void from_json(const json &j, bdd_profile_t::map_stats_t::node_t &node) {
  j.at("node").get_to(node.node);
  j.at("pkts").get_to(node.pkts);
  j.at("flows").get_to(node.flows);
  j.at("pkts_per_flow").get_to(node.pkts_per_flow);

  // Use our parser instead of the default one provided by the library. Their
  // one is not working for some reason.
  from_json(j["crc32_hashes_per_mask"], node.crc32_hashes_per_mask);

  std::sort(node.pkts_per_flow.begin(), node.pkts_per_flow.end(), std::greater<u64>());
}

void from_json(const json &j, bdd_profile_t::map_stats_t::epoch_t &epoch) {
  j.at("dt_ns").get_to(epoch.dt_ns);
  j.at("warmup").get_to(epoch.warmup);
  j.at("pkts").get_to(epoch.pkts);
  j.at("flows").get_to(epoch.flows);
  j.at("pkts_per_persistent_flow").get_to(epoch.pkts_per_persistent_flow);
  j.at("pkts_per_new_flow").get_to(epoch.pkts_per_new_flow);

  std::sort(epoch.pkts_per_persistent_flow.begin(), epoch.pkts_per_persistent_flow.end(), std::greater<u64>());
  std::sort(epoch.pkts_per_new_flow.begin(), epoch.pkts_per_new_flow.end(), std::greater<u64>());
}

void from_json(const json &j, bdd_profile_t::map_stats_t &map_stats) {
  j.at("nodes").get_to(map_stats.nodes);
  j.at("epochs").get_to(map_stats.epochs);
}

void from_json(const json &j, std::unordered_map<u64, bdd_profile_t::map_stats_t> &stats_per_map) {
  for (const auto &kv : j.items()) {
    u64 map_addr                         = std::stoull(kv.key());
    bdd_profile_t::map_stats_t map_stats = kv.value();
    stats_per_map[map_addr]              = map_stats;
  }
}

void from_json(const json &j, std::unordered_map<node_id_t, u64> &counters) {
  for (const auto &kv : j.items()) {
    node_id_t node_id = std::stoull(kv.key());
    u64 count         = kv.value();
    counters[node_id] = count;
  }
}

void from_json(const json &j, bdd_profile_t &report) {
  j.at("config").get_to(report.config);
  j.at("meta").get_to(report.meta);
  j.at("stats_per_map").get_to(report.stats_per_map);

  // Use our parser instead of the default one provided by the library. Theirs
  // is not working for some reason.
  from_json(j["counters"], report.counters);
}

bdd_profile_t parse_bdd_profile(const std::string &filename) {
  std::ifstream file(filename);

  if (!file.is_open()) {
    panic("Failed to open file: %s", filename.c_str());
  }

  json j               = json::parse(file);
  bdd_profile_t report = j.get<bdd_profile_t>();

  return report;
}

fpm_t bdd_profile_t::churn_top_k_flows(u64 map, u32 k) const {
  fpm_t avg_churn     = 0;
  size_t total_epochs = 0;

  for (const auto &epoch : stats_per_map.at(map).epochs) {
    if (epoch.warmup) {
      continue;
    }

    total_epochs++;

    size_t i               = 0;
    size_t j               = 0;
    size_t top_k_new_flows = 0;
    while (i < epoch.pkts_per_persistent_flow.size() && j < epoch.pkts_per_new_flow.size() && i + j < k) {
      if (epoch.pkts_per_persistent_flow[i] > epoch.pkts_per_new_flow[j]) {
        i++;
      } else {
        j++;
        top_k_new_flows++;
      }
    }

    if (epoch.dt_ns > 0) {
      fpm_t churn = (60.0 * top_k_new_flows) / (epoch.dt_ns / 1e9);
      avg_churn += churn;
    }
  }

  if (total_epochs > 0) {
    avg_churn /= total_epochs;
  }

  return avg_churn;
}

hit_rate_t bdd_profile_t::churn_hit_rate_top_k_flows(u64 map, u32 k) const {
  hit_rate_t avg_churn_hr = 0;
  size_t total_epochs     = 0;

  for (const auto &epoch : stats_per_map.at(map).epochs) {
    if (epoch.warmup) {
      continue;
    }

    total_epochs++;

    size_t i               = 0;
    size_t j               = 0;
    size_t top_k_new_flows = 0;
    while (i < epoch.pkts_per_persistent_flow.size() && j < epoch.pkts_per_new_flow.size() && i + j < k) {
      if (epoch.pkts_per_persistent_flow[i] > epoch.pkts_per_new_flow[j]) {
        i++;
      } else {
        j++;
        top_k_new_flows++;
      }
    }

    avg_churn_hr += static_cast<hit_rate_t>(top_k_new_flows) / epoch.pkts;
  }

  if (total_epochs > 0) {
    avg_churn_hr /= total_epochs;
  }

  return avg_churn_hr;
}

u64 bdd_profile_t::threshold_top_k_flows(u64 map, u32 k) const {
  u64 threshold = UINT64_MAX;

  for (const auto &epoch : stats_per_map.at(map).epochs) {
    if (epoch.warmup) {
      continue;
    }

    size_t i = 0;
    size_t j = 0;
    while (i < epoch.pkts_per_persistent_flow.size() && j < epoch.pkts_per_new_flow.size() && i + j < k) {
      if (epoch.pkts_per_persistent_flow[i] > epoch.pkts_per_new_flow[j]) {
        threshold = std::min(threshold, epoch.pkts_per_persistent_flow[i]);
        i++;
      } else {
        threshold = std::min(threshold, epoch.pkts_per_new_flow[j]);
        j++;
      }
    }
  }

  return threshold;
}

} // namespace LibBDD