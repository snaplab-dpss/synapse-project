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

std::vector<LibCore::flow_t> get_base_flows(const config_t &config) {
  std::vector<LibCore::flow_t> flows;
  flows.reserve(config.total_flows);

  for (size_t i = 0; i < config.total_flows; i++) {
    flows.push_back(random_flow());
  }

  return flows;
}

class EchoTrafficGenerator : public TrafficGenerator {
private:
  std::vector<LibCore::flow_t> flows;

public:
  EchoTrafficGenerator(const config_t &_config, const std::vector<LibCore::flow_t> &_base_flows)
      : TrafficGenerator("echo", _config, true), flows(_base_flows) {}

  virtual bytes_t get_hdrs_len() const override { return sizeof(ether_hdr_t) + sizeof(ipv4_hdr_t) + sizeof(udp_hdr_t); }

  virtual void random_swap_flow(flow_idx_t flow_idx) override {
    assert(flow_idx < flows.size());
    flows[flow_idx] = random_flow();
  }

  virtual pkt_t build_packet(device_t dev, flow_idx_t flow_idx) override {
    pkt_t pkt                   = template_packet;
    const LibCore::flow_t &flow = flows[flow_idx];
    pkt.ip_hdr.src_addr         = flow.src_ip;
    pkt.ip_hdr.dst_addr         = flow.dst_ip;
    pkt.udp_hdr.src_port        = flow.src_port;
    pkt.udp_hdr.dst_port        = flow.dst_port;
    return pkt;
  }

  virtual std::optional<device_t> get_response_dev(device_t dev, flow_idx_t flow_idx) const override { return std::nullopt; }
};

int main(int argc, char *argv[]) {
  CLI::App app{"Traffic generator for the echo nf."};

  config_t config;

  app.add_option("--out", config.out_dir, "Output directory.")->default_val(TrafficGenerator::DEFAULT_OUTPUT_DIR);
  app.add_option("--packets", config.total_packets, "Total packets.")->default_val(TrafficGenerator::DEFAULT_TOTAL_PACKETS);
  app.add_option("--flows", config.total_flows, "Total flows.")->default_val(TrafficGenerator::DEFAULT_TOTAL_FLOWS);
  app.add_option("--packet-size", config.packet_size, "Packet size (bytes).")->default_val(TrafficGenerator::DEFAULT_PACKET_SIZE);
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
  app.add_option("--devs", config.devices, "Devices.")->required();
  app.add_option("--seed", config.random_seed, "Random seed.")->default_val(std::random_device()());
  app.add_flag("--dry-run", config.dry_run, "Print out the configuration values without generating the pcaps.")->default_val(false);

  CLI11_PARSE(app, argc, argv);

  config.client_devices = config.devices;

  srand(config.random_seed);

  config.print();
  if (config.dry_run) {
    return 0;
  }

  std::vector<LibCore::flow_t> base_flows = get_base_flows(config);
  EchoTrafficGenerator generator(config, base_flows);

  generator.generate();

  return 0;
}