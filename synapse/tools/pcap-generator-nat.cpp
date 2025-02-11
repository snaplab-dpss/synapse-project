#include <LibCore/TrafficGenerator.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <optional>

#include <CLI/CLI.hpp>

LibCore::flow_t random_flow() {
  LibCore::flow_t flow;
  flow.src_ip   = LibCore::random_addr();
  flow.dst_ip   = LibCore::random_addr();
  flow.src_port = LibCore::random_port();
  flow.dst_port = LibCore::random_port();
  return flow;
}

LibCore::flow_t invert_flow(const LibCore::flow_t &flow) {
  LibCore::flow_t inverted_flow;
  inverted_flow.src_ip   = flow.dst_ip;
  inverted_flow.dst_ip   = flow.src_ip;
  inverted_flow.src_port = flow.dst_port;
  inverted_flow.dst_port = flow.src_port;
  return inverted_flow;
}

std::vector<LibCore::flow_t> get_base_flows(const LibCore::TrafficGenerator::config_t &config) {
  std::vector<LibCore::flow_t> flows;
  flows.reserve(config.total_flows);

  for (size_t i = 0; i < config.total_flows; i++) {
    flows.push_back(random_flow());
  }

  return flows;
}

class PortAllocator {
private:
  std::vector<u16> available_ports;
  std::unordered_map<LibCore::flow_t, u64, LibCore::flow_t::flow_hash_t> flow_to_port;

public:
  PortAllocator() {
    u16 port = UINT16_MAX;
    while (1) {
      available_ports.push_back(port);
      if (port == 0) {
        break;
      }
      port--;
    }
  }

  void allocate(const LibCore::flow_t &flow) {
    assert(!available_ports.empty());
    assert(!flow_to_port.contains(flow));
    u16 port = available_ports.back();
    available_ports.pop_back();
    flow_to_port[flow] = port;
  }

  size_t available() const { return available_ports.size(); }
  size_t allocated() const { return flow_to_port.size(); }

  u16 get(const LibCore::flow_t &flow) { return flow_to_port.at(flow); }

  bool free(const LibCore::flow_t &flow) {
    if (!flow_to_port.contains(flow)) {
      return false;
    }

    u16 port = get(flow);
    available_ports.insert(available_ports.begin(), port);
    flow_to_port.erase(flow);

    return true;
  }
};

class NATTrafficGenerator : public LibCore::TrafficGenerator {
private:
  static constexpr const u32 PUBLIC_IP = 0xffffffff;

  std::vector<LibCore::flow_t> flows;
  std::unordered_set<LibCore::flow_t, LibCore::flow_t::flow_hash_t> allocated_flows;
  PortAllocator port_allocator;

public:
  NATTrafficGenerator(const config_t &_config, const std::vector<LibCore::flow_t> &_base_flows)
      : TrafficGenerator("nat", _config), flows(_base_flows) {
    assert(flows.size() <= 65535 && "Too many flows for a NAT (max 65535).");

    for (LibCore::flow_t &flow : flows) {
      u16 lan_dev = get_current_lan_dev();
      u16 wan_dev = lan_to_wan_dev.at(lan_dev);
      advance_lan_dev();

      flow.src_ip = mask_addr_from_dev(flow.src_ip, lan_dev);
      flow.dst_ip = mask_addr_from_dev(flow.dst_ip, wan_dev);
    }

    reset_lan_dev();
  }

  virtual void random_swap_flow(flow_idx_t flow_idx) {
    assert(flow_idx < flows.size());
    u16 lan_dev = flows_to_lan_dev.at(flow_idx);
    u16 wan_dev = lan_to_wan_dev.at(lan_dev);

    LibCore::flow_t old_flow = flows.at(flow_idx);
    LibCore::flow_t new_flow = random_flow();
    new_flow.src_ip          = mask_addr_from_dev(new_flow.src_ip, lan_dev);
    new_flow.dst_ip          = mask_addr_from_dev(new_flow.dst_ip, wan_dev);

    allocated_flows.erase(old_flow);
    port_allocator.free(old_flow);

    flows[flow_idx] = new_flow;
    flows_swapped++;
  }

  virtual pkt_t build_lan_packet(u16 lan_dev, flow_idx_t flow_idx) {
    const LibCore::flow_t &flow = flows[flow_idx];

    if (allocated_flows.find(flow) == allocated_flows.end()) {
      allocated_flows.insert(flow);
      port_allocator.allocate(flow);
      assert(port_allocator.allocated() <= flows.size());
    }

    pkt_t pkt            = template_packet;
    pkt.ip_hdr.src_addr  = flow.src_ip;
    pkt.ip_hdr.dst_addr  = flow.dst_ip;
    pkt.udp_hdr.src_port = flow.src_port;
    pkt.udp_hdr.dst_port = flow.dst_port;

    return pkt;
  }

  virtual pkt_t build_wan_packet(u16 wan_dev, flow_idx_t flow_idx) {
    const LibCore::flow_t &flow   = flows[flow_idx];
    LibCore::flow_t inverted_flow = invert_flow(flow);

    u16 allocated_port = port_allocator.get(flow);

    pkt_t pkt            = template_packet;
    pkt.ip_hdr.src_addr  = inverted_flow.src_ip;
    pkt.udp_hdr.src_port = inverted_flow.src_port;

    // No ntohs, the original NAT doesn't care.
    pkt.ip_hdr.dst_addr  = PUBLIC_IP;
    pkt.udp_hdr.dst_port = allocated_port;

    return pkt;
  }
};

int main(int argc, char *argv[]) {
  CLI::App app{"Traffic generator for the nat nf."};

  LibCore::TrafficGenerator::config_t config;

  app.add_option("--out", config.out_dir, "Output directory.")->default_val(LibCore::TrafficGenerator::DEFAULT_OUTPUT_DIR);
  app.add_option("--packets", config.total_packets, "Total packets.")->default_val(LibCore::TrafficGenerator::DEFAULT_TOTAL_PACKETS);
  app.add_option("--flows", config.total_flows, "Total flows.")->default_val(LibCore::TrafficGenerator::DEFAULT_TOTAL_FLOWS);
  app.add_option("--packet-size", config.packet_size, "Packet size (bytes).")->default_val(LibCore::TrafficGenerator::DEFAULT_PACKET_SIZE);
  app.add_option("--churn", config.churn_fpm, "Total churn (fpm).")->default_val(LibCore::TrafficGenerator::DEFAULT_TOTAL_CHURN_FPM);
  app.add_option("--traffic", config.traffic_type, "Traffic distribution.")
      ->default_val(LibCore::TrafficGenerator::DEFAULT_TRAFFIC_TYPE)
      ->transform(CLI::CheckedTransformer(
          std::unordered_map<std::string, LibCore::TrafficGenerator::TrafficType>{
              {"uniform", LibCore::TrafficGenerator::TrafficType::Uniform},
              {"zipf", LibCore::TrafficGenerator::TrafficType::Zipf},
          },
          CLI::ignore_case));
  app.add_option("--zipf-param", config.zipf_param, "Zipf parameter.")->default_val(LibCore::TrafficGenerator::DEFAULT_ZIPF_PARAM);
  app.add_option("--devs", config.lan_wan_pairs, "LAN/WAN pairs.")->delimiter(',')->required();
  app.add_option("--seed", config.random_seed, "Random seed.")->default_val(std::random_device()());
  app.add_flag("--dry-run", config.dry_run, "Print out the configuration values without generating the pcaps.")->default_val(false);

  CLI11_PARSE(app, argc, argv);

  srand(config.random_seed);

  config.print();
  if (config.dry_run) {
    return 0;
  }

  std::vector<LibCore::flow_t> base_flows = get_base_flows(config);
  NATTrafficGenerator generator(config, base_flows);

  generator.generate_warmup();
  generator.generate();

  return 0;
}