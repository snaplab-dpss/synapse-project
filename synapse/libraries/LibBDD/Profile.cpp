#include <LibBDD/Profile.h>
#include <LibBDD/BDD.h>
#include <LibCore/Debug.h>
#include <LibCore/RandomEngine.h>
#include <LibCore/Net.h>
#include <LibCore/Solver.h>

#include <fstream>
#include <nlohmann/json.hpp>

namespace LibBDD {

using LibCore::expr_addr_to_obj_addr;
using LibCore::is_constant;
using LibCore::SingletonRandomEngine;
using LibCore::solver_toolbox;

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

void from_json(const json &j, std::unordered_map<bdd_node_id_t, u64> &counters) {
  for (const auto &kv : j.items()) {
    bdd_node_id_t node_id = std::stoull(kv.key());
    u64 count             = kv.value();
    counters[node_id]     = count;
  }
}

void from_json(const json &j, std::unordered_map<u16, u64> &port_stats) {
  for (const auto &kv : j.items()) {
    u16 port         = std::stoul(kv.key());
    u64 count        = kv.value();
    port_stats[port] = count;
  }
}

void from_json(const json &j, bdd_profile_t::fwd_stats_t &stats) {
  // Use our parser instead of the default one provided by the library. Their
  // one is not working for some reason.
  from_json(j["ports"], stats.ports);

  j.at("drop").get_to(stats.drop);
  j.at("flood").get_to(stats.flood);
}

void from_json(const json &j, std::unordered_map<bdd_node_id_t, bdd_profile_t::fwd_stats_t> &forwarding_stats) {
  for (const auto &kv : j.items()) {
    bdd_node_id_t node_id     = std::stoull(kv.key());
    forwarding_stats[node_id] = kv.value();
  }
}

void from_json(const json &j, bdd_profile_t &report) {
  j.at("config").get_to(report.config);
  j.at("meta").get_to(report.meta);
  j.at("stats_per_map").get_to(report.stats_per_map);

  // Use our parser instead of the default one provided by the library. Theirs
  // is not working for some reason.
  from_json(j["counters"], report.counters);
  from_json(j["forwarding_stats"], report.forwarding_stats);
}

bdd_profile_t parse_bdd_profile(const std::filesystem::path &filename) {
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
  double avg_churn_hr = 0;
  size_t total_epochs = 0;

  for (const bdd_profile_t::map_stats_t::epoch_t &epoch : stats_per_map.at(map).epochs) {
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

    avg_churn_hr += (top_k_new_flows / static_cast<double>(epoch.pkts));
  }

  if (total_epochs > 0) {
    avg_churn_hr /= total_epochs;
  }

  return hit_rate_t(avg_churn_hr);
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

bdd_profile_t build_random_bdd_profile(const BDD *bdd) {
  bdd_profile_t bdd_profile;

  bdd_profile.meta.pkts  = 100'000;
  bdd_profile.meta.bytes = bdd_profile.meta.pkts * 250; // 250B size packets

  const BDDNode *root                   = bdd->get_root();
  bdd_profile.counters[root->get_id()]  = bdd_profile.meta.pkts;
  const std::unordered_set<u16> devices = bdd->get_devices();

  root->visit_nodes([bdd, &bdd_profile, devices](const BDDNode *node) {
    assert(bdd_profile.counters.find(node->get_id()) != bdd_profile.counters.end() && "BDDNode counter not found");
    const u64 current_counter = bdd_profile.counters[node->get_id()];

    switch (node->get_type()) {
    case BDDNodeType::Branch: {
      const Branch *branch = dynamic_cast<const Branch *>(node);

      const BDDNode *on_true  = branch->get_on_true();
      const BDDNode *on_false = branch->get_on_false();

      assert(on_true && "Branch node without on_true");
      assert(on_false && "Branch node without on_false");

      const u64 on_true_counter  = SingletonRandomEngine::generate() % (current_counter + 1);
      const u64 on_false_counter = current_counter - on_true_counter;

      bdd_profile.counters[on_true->get_id()]  = on_true_counter;
      bdd_profile.counters[on_false->get_id()] = on_false_counter;
    } break;
    case BDDNodeType::Call: {
      const Call *call_node = dynamic_cast<const Call *>(node);
      const call_t &call    = call_node->get_call();

      if (call.function_name == "map_get" || call.function_name == "map_put" || call.function_name == "map_erase") {
        klee::ref<klee::Expr> map_addr = call.args.at("map").expr;

        bdd_profile_t::map_stats_t::node_t map_stats;
        map_stats.node  = node->get_id();
        map_stats.pkts  = current_counter;
        map_stats.flows = current_counter == 0 ? 0 : std::max(1ul, SingletonRandomEngine::generate() % current_counter);

        u64 avg_pkts_per_flow = map_stats.flows == 0 ? 0 : current_counter / map_stats.flows;
        for (u64 i = 0; i < map_stats.flows; i++) {
          map_stats.pkts_per_flow.push_back(avg_pkts_per_flow);
        }

        bdd_profile.stats_per_map[expr_addr_to_obj_addr(map_addr)].nodes.push_back(map_stats);
      }

      if (node->get_next()) {
        const BDDNode *next                  = node->get_next();
        bdd_profile.counters[next->get_id()] = current_counter;
      }
    } break;
    case BDDNodeType::Route: {
      if (node->get_next()) {
        const BDDNode *next                  = node->get_next();
        bdd_profile.counters[next->get_id()] = current_counter;
      }

      const Route *route_node = dynamic_cast<const Route *>(node);
      const RouteOp operation = route_node->get_operation();

      switch (operation) {
      case RouteOp::Drop: {
        bdd_profile.forwarding_stats[node->get_id()].drop = current_counter;
      } break;
      case RouteOp::Broadcast: {
        bdd_profile.forwarding_stats[node->get_id()].flood = current_counter;
      } break;
      case RouteOp::Forward: {
        klee::ref<klee::Expr> dst_device = route_node->get_dst_device();
        if (is_constant(dst_device)) {
          const u16 device = solver_toolbox.value_from_expr(dst_device);
          assert(std::find(devices.begin(), devices.end(), device) != devices.end() && "Invalid device");
          bdd_profile.forwarding_stats[node->get_id()].ports[device] = current_counter;
        } else {
          const klee::ConstraintManager constraints = bdd->get_constraints(node);

          std::vector<u16> candidate_devices;
          for (const u16 dev : devices) {
            klee::ref<klee::Expr> handles_port =
                solver_toolbox.exprBuilder->Eq(dst_device, solver_toolbox.exprBuilder->Constant(dev, dst_device->getWidth()));
            if (solver_toolbox.is_expr_maybe_true(constraints, handles_port)) {
              candidate_devices.push_back(dev);
            }
          }

          assert(!candidate_devices.empty() && "No candidate devices found for forwarding");
          for (const u16 dev : candidate_devices) {
            bdd_profile.forwarding_stats[node->get_id()].ports[dev] = current_counter / candidate_devices.size();
          }
        }
      } break;
      }
    } break;
    }

    return BDDNodeVisitAction::Continue;
  });

  return bdd_profile;
}

bdd_profile_t build_uniform_bdd_profile(const BDD *bdd) {
  bdd_profile_t bdd_profile;

  bdd_profile.meta.pkts  = bdd->get_root()->get_leaves().size() * 1000;
  bdd_profile.meta.bytes = bdd_profile.meta.pkts * 250; // 250B size packets

  const BDDNode *root                   = bdd->get_root();
  bdd_profile.counters[root->get_id()]  = bdd_profile.meta.pkts;
  const std::unordered_set<u16> devices = bdd->get_devices();

  root->visit_nodes([bdd, &bdd_profile, &devices](const BDDNode *node) {
    const u64 current_counter = bdd_profile.counters.at(node->get_id());

    switch (node->get_type()) {
    case BDDNodeType::Branch: {
      const Branch *branch = dynamic_cast<const Branch *>(node);

      const BDDNode *on_true  = branch->get_on_true();
      const BDDNode *on_false = branch->get_on_false();

      assert(on_true && "Branch node without on_true");
      assert(on_false && "Branch node without on_false");

      const u64 on_true_counter  = current_counter / 2;
      const u64 on_false_counter = current_counter - on_true_counter;

      bdd_profile.counters[on_true->get_id()]  = on_true_counter;
      bdd_profile.counters[on_false->get_id()] = on_false_counter;
    } break;
    case BDDNodeType::Call: {
      const Call *call_node = dynamic_cast<const Call *>(node);
      const call_t &call    = call_node->get_call();

      if (call.function_name == "map_get" || call.function_name == "map_put" || call.function_name == "map_erase") {
        klee::ref<klee::Expr> map_addr = call.args.at("map").expr;

        bdd_profile_t::map_stats_t::node_t map_stats;
        map_stats.node  = node->get_id();
        map_stats.pkts  = current_counter;
        map_stats.flows = current_counter == 0 ? 0 : std::max(1ul, SingletonRandomEngine::generate() % current_counter);

        u64 avg_pkts_per_flow = map_stats.flows == 0 ? 0 : current_counter / map_stats.flows;
        for (u64 i = 0; i < map_stats.flows; i++) {
          map_stats.pkts_per_flow.push_back(avg_pkts_per_flow);
        }

        bdd_profile.stats_per_map[expr_addr_to_obj_addr(map_addr)].nodes.push_back(map_stats);
      }

      if (node->get_next()) {
        const BDDNode *next                  = node->get_next();
        bdd_profile.counters[next->get_id()] = current_counter;
      }
    } break;
    case BDDNodeType::Route: {
      if (node->get_next()) {
        const BDDNode *next                  = node->get_next();
        bdd_profile.counters[next->get_id()] = current_counter;
      }

      const Route *route_node = dynamic_cast<const Route *>(node);
      const RouteOp operation = route_node->get_operation();

      switch (operation) {
      case RouteOp::Drop: {
        bdd_profile.forwarding_stats[node->get_id()].drop = current_counter;
      } break;
      case RouteOp::Broadcast: {
        bdd_profile.forwarding_stats[node->get_id()].flood = current_counter;
      } break;
      case RouteOp::Forward: {
        klee::ref<klee::Expr> dst_device = route_node->get_dst_device();
        if (is_constant(dst_device)) {
          const u16 device = solver_toolbox.value_from_expr(dst_device);
          assert(devices.contains(device) && "Invalid device");
          bdd_profile.forwarding_stats[node->get_id()].ports[device] = current_counter;
        } else {
          const klee::ConstraintManager constraints = bdd->get_constraints(node);

          std::unordered_set<u16> candidate_devices;
          for (const u16 dev : devices) {
            klee::ref<klee::Expr> handles_port =
                solver_toolbox.exprBuilder->Eq(dst_device, solver_toolbox.exprBuilder->Constant(dev, dst_device->getWidth()));
            if (solver_toolbox.is_expr_maybe_true(constraints, handles_port)) {
              candidate_devices.insert(dev);
            }
          }

          assert(!candidate_devices.empty() && "No candidate devices found for forwarding");
          for (const u16 dev : candidate_devices) {
            bdd_profile.forwarding_stats[node->get_id()].ports[dev] = current_counter / candidate_devices.size();
          }
        }
      } break;
      }
    } break;
    }

    return BDDNodeVisitAction::Continue;
  });

  return bdd_profile;
}

} // namespace LibBDD