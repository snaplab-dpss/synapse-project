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

using LibCore::TrafficGenerator;
using device_t    = TrafficGenerator::device_t;
using config_t    = TrafficGenerator::config_t;
using TrafficType = TrafficGenerator::TrafficType;

std::vector<LibCore::flow_t> get_base_flows(const LibCore::TrafficGenerator::config_t &config) {
  std::vector<LibCore::flow_t> flows;
  flows.reserve(config.total_flows);

  for (size_t i = 0; i < config.total_flows; i++) {
    flows.push_back(LibCore::random_flow());
  }

  return flows;
}

class PortAllocator {
private:
  std::vector<device_t> available_ports;
  std::unordered_map<LibCore::flow_t, u64, LibCore::flow_t::flow_hash_t> flow_to_port;

public:
  PortAllocator() {
    device_t port = UINT16_MAX;
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
    device_t port = available_ports.back();
    available_ports.pop_back();
    flow_to_port[flow] = port;
  }

  size_t available() const { return available_ports.size(); }
  size_t allocated() const { return flow_to_port.size(); }

  device_t get(const LibCore::flow_t &flow) { return flow_to_port.at(flow); }

  bool free(const LibCore::flow_t &flow) {
    if (!flow_to_port.contains(flow)) {
      return false;
    }

    device_t port = get(flow);
    available_ports.insert(available_ports.begin(), port);
    flow_to_port.erase(flow);

    return true;
  }
};

class NATTrafficGenerator : public LibCore::TrafficGenerator {
private:
  static constexpr const u32 PUBLIC_IP = 0xffffffff;

  const std::vector<std::pair<device_t, device_t>> lan_wan_pairs;
  const std::unordered_map<device_t, device_t> connections;
  const std::unordered_set<device_t> lan_devs;

  std::vector<LibCore::flow_t> flows;
  std::unordered_set<LibCore::flow_t, LibCore::flow_t::flow_hash_t> allocated_flows;
  PortAllocator port_allocator;

public:
  NATTrafficGenerator(const config_t &_config, const std::vector<std::pair<device_t, device_t>> &_lan_wan_pairs,
                      const std::vector<LibCore::flow_t> &_base_flows)
      : TrafficGenerator("nat", _config, true), lan_wan_pairs(_lan_wan_pairs), connections(build_connections(_lan_wan_pairs)),
        lan_devs(build_lan_devices(_lan_wan_pairs)), flows(_base_flows) {
    assert(flows.size() <= 65535 && "Too many flows for a NAT (max 65535).");

    for (LibCore::flow_t &flow : flows) {
      const device_t lan_dev = get_current_client_dev();
      const device_t wan_dev = connections.at(lan_dev);
      advance_client_dev();

      flow.five_tuple.src_ip = mask_addr_from_dev(flow.five_tuple.src_ip, lan_dev);
      flow.five_tuple.dst_ip = mask_addr_from_dev(flow.five_tuple.dst_ip, wan_dev);
    }

    reset_client_dev();
  }

  virtual bytes_t get_hdrs_len() const override { return sizeof(ether_hdr_t) + sizeof(ipv4_hdr_t) + sizeof(udp_hdr_t); }

  virtual void random_swap_flow(flow_idx_t flow_idx) override {
    assert(flow_idx < flows.size());

    const device_t lan_dev = get_client_dev_from_flow(flow_idx);
    const device_t wan_dev = connections.at(lan_dev);

    const LibCore::flow_t old_flow = flows[flow_idx];
    LibCore::flow_t new_flow       = LibCore::random_flow();
    new_flow.five_tuple.src_ip     = mask_addr_from_dev(new_flow.five_tuple.src_ip, lan_dev);
    new_flow.five_tuple.dst_ip     = mask_addr_from_dev(new_flow.five_tuple.dst_ip, wan_dev);

    allocated_flows.erase(old_flow);
    port_allocator.free(old_flow);

    flows[flow_idx] = new_flow;
    flows_swapped++;
  }

  virtual pkt_t build_packet(device_t dev, flow_idx_t flow_idx) override {
    const LibCore::flow_t &flow = flows[flow_idx];

    if (lan_devs.contains(dev)) {
      if (allocated_flows.find(flow) == allocated_flows.end()) {
        allocated_flows.insert(flow);
        port_allocator.allocate(flow);
        assert(port_allocator.allocated() <= flows.size());
      }

      pkt_t pkt            = template_packet;
      pkt.ip_hdr.src_addr  = flow.five_tuple.src_ip;
      pkt.ip_hdr.dst_addr  = flow.five_tuple.dst_ip;
      pkt.udp_hdr.src_port = flow.five_tuple.src_port;
      pkt.udp_hdr.dst_port = flow.five_tuple.dst_port;

      return pkt;
    } else {
      LibCore::flow_t inverted_flow = flow.invert();

      const device_t allocated_port = port_allocator.get(flow);

      pkt_t pkt            = template_packet;
      pkt.ip_hdr.src_addr  = inverted_flow.five_tuple.src_ip;
      pkt.udp_hdr.src_port = inverted_flow.five_tuple.src_port;

      // No ntohs, the original NAT doesn't care.
      pkt.ip_hdr.dst_addr  = PUBLIC_IP;
      pkt.udp_hdr.dst_port = allocated_port;

      return pkt;
    }
  }

  virtual pkt_t build_warmup_packet(device_t dev, flow_idx_t flow_idx) override { return build_packet(dev, flow_idx); }

  virtual std::optional<device_t> get_response_dev(device_t dev, flow_idx_t flow_idx) const override { return connections.at(dev); }

private:
  static std::unordered_map<device_t, device_t> build_connections(const std::vector<std::pair<device_t, device_t>> &lan_wan_pairs) {
    std::unordered_map<device_t, device_t> connections;
    for (const auto &[lan_dev, wan_dev] : lan_wan_pairs) {
      connections[lan_dev] = wan_dev;
      connections[wan_dev] = lan_dev;
    }
    return connections;
  }

  static std::unordered_set<device_t> build_lan_devices(const std::vector<std::pair<device_t, device_t>> &lan_wan_pairs) {
    std::unordered_set<device_t> lan_devs;
    for (const auto &[lan_dev, _] : lan_wan_pairs) {
      lan_devs.insert(lan_dev);
    }
    return lan_devs;
  }
};

int main(int argc, char *argv[]) {
  CLI::App app{"Traffic generator for the nat nf."};

  LibCore::TrafficGenerator::config_t config;
  std::vector<std::pair<device_t, device_t>> lan_wan_pairs;
  bytes_t packet_size;

  app.add_option("--out", config.out_dir, "Output directory.")->default_val(TrafficGenerator::DEFAULT_OUTPUT_DIR);
  app.add_option("--packets", config.total_packets, "Total packets.")->default_val(TrafficGenerator::DEFAULT_TOTAL_PACKETS);
  app.add_option("--flows", config.total_flows, "Total flows.")->default_val(TrafficGenerator::DEFAULT_TOTAL_FLOWS);
  app.add_option("--rate", config.rate, "Rate (bps).")->default_val(TrafficGenerator::DEFAULT_RATE);
  app.add_option("--packet-size", packet_size, "Packet size without (bytes).")->default_val(TrafficGenerator::DEFAULT_PACKET_SIZE);
  app.add_option("--churn", config.churn, "Total churn (fpm).")->default_val(TrafficGenerator::DEFAULT_TOTAL_CHURN_FPM);
  app.add_option("--traffic", config.traffic_type, "Traffic distribution.")
      ->default_val(TrafficGenerator::DEFAULT_TRAFFIC_TYPE)
      ->transform(CLI::CheckedTransformer(
          std::unordered_map<std::string, TrafficType>{
              {"uniform", TrafficType::Uniform},
              {"zipf", TrafficType::Zipf},
          },
          CLI::ignore_case));
  app.add_option("--zipf-param", config.zipf_param, "Zipf parameter.")->default_val(TrafficGenerator::DEFAULT_ZIPF_PARAM);
  app.add_option("--devs", lan_wan_pairs, "LAN/WAN pairs.")->delimiter(',')->required();
  app.add_option("--seed", config.random_seed, "Random seed.")->default_val(std::random_device()());
  app.add_flag("--dry-run", config.dry_run, "Print out the configuration values without generating the pcaps.")->default_val(false);

  CLI11_PARSE(app, argc, argv);

  config.packet_size_without_crc = std::max(packet_size, MIN_PKT_SIZE_BYTES) - CRC_SIZE_BYTES;

  for (const auto &[lan_dev, wan_dev] : lan_wan_pairs) {
    config.devices.push_back(lan_dev);
    config.devices.push_back(wan_dev);
    config.client_devices.push_back(lan_dev);
  }

  srand(config.random_seed);

  config.print();
  if (config.dry_run) {
    return 0;
  }

  std::vector<LibCore::flow_t> base_flows = get_base_flows(config);
  NATTrafficGenerator generator(config, lan_wan_pairs, base_flows);

  generator.generate_warmup();
  generator.generate();

  return 0;
}