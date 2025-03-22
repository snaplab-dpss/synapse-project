#include <LibCore/TrafficGenerator.h>

#include <math.h>
#include <stdint.h>
#include <string.h>

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

constexpr const bytes_t KEY_SIZE_BYTES{16};
constexpr const bytes_t VALUE_SIZE_BYTES{128};

using kv_key_t   = std::array<u8, KEY_SIZE_BYTES>;
using kv_value_t = std::array<u8, VALUE_SIZE_BYTES>;

enum kvs_op {
  KVS_OP_GET = 0,
  KVS_OP_PUT = 1,
  KVS_OP_DEL = 2,
};

enum kvs_status {
  KVS_STATUS_MISS = 0,
  KVS_STATUS_HIT  = 1,
};

struct kvs_hdr_t {
  u8 op;
  u8 key[KEY_SIZE_BYTES];
  u8 value[VALUE_SIZE_BYTES];
  u8 status;
  u16 client_port;
} __attribute__((__packed__));

constexpr const u16 KVSTORE_PORT{670};
constexpr const bytes_t KVS_PKT_SIZE{sizeof(ether_hdr_t) + sizeof(ipv4_hdr_t) + sizeof(udp_hdr_t) + sizeof(kvs_hdr_t)};

kv_key_t random_key() {
  kv_key_t key;
  for (bytes_t i = 0; i < KEY_SIZE_BYTES; i++) {
    key[i] = rand();
  }
  return key;
}

kv_value_t random_value() {
  kv_value_t value;
  for (bytes_t i = 0; i < VALUE_SIZE_BYTES; i++) {
    value[i] = rand();
  }
  return value;
}

std::vector<kv_key_t> get_base_keys(const config_t &config) {
  std::vector<kv_key_t> keys;
  keys.reserve(config.total_flows);

  for (size_t i = 0; i < config.total_flows; i++) {
    keys.push_back(random_key());
  }

  return keys;
}

class KVSTrafficGenerator : public LibCore::TrafficGenerator {
private:
  std::vector<kv_key_t> keys;

public:
  KVSTrafficGenerator(const config_t &_config, const std::vector<kv_key_t> &_base_keys)
      : TrafficGenerator("kvs", _config), keys(_base_keys) {}

  virtual void random_swap_flow(flow_idx_t flow_idx) override {
    assert(flow_idx < keys.size());
    keys[flow_idx] = random_key();
    flows_swapped++;
  }

  virtual pkt_t build_packet(device_t dev, flow_idx_t flow_idx) override {
    pkt_t pkt          = template_packet;
    const kv_key_t key = keys[flow_idx];

    pkt.udp_hdr.dst_port = bswap16(KVSTORE_PORT);

    kvs_hdr_t *kvs_hdr   = (kvs_hdr_t *)pkt.payload;
    kvs_hdr->op          = KVS_OP_PUT;
    kvs_hdr->status      = KVS_STATUS_MISS;
    kvs_hdr->client_port = 0;

    std::memcpy(kvs_hdr->key, key.data(), key.size());
    std::memset(kvs_hdr->value, 0, sizeof(kvs_hdr->value));

    return pkt;
  }

  virtual std::optional<device_t> get_response_dev(device_t dev, flow_idx_t flow_idx) const override { return std::nullopt; }
};

int main(int argc, char *argv[]) {
  CLI::App app{"Traffic generator for the kvs nf."};

  LibCore::TrafficGenerator::config_t config;

  app.add_option("--out", config.out_dir, "Output directory.")->default_val(TrafficGenerator::DEFAULT_OUTPUT_DIR);
  app.add_option("--packets", config.total_packets, "Total packets.")->default_val(TrafficGenerator::DEFAULT_TOTAL_PACKETS);
  app.add_option("--flows", config.total_flows, "Total flows.")->default_val(TrafficGenerator::DEFAULT_TOTAL_FLOWS);
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

  config.packet_size    = KVS_PKT_SIZE;
  config.client_devices = config.devices;

  srand(config.random_seed);

  config.print();
  if (config.dry_run) {
    return 0;
  }

  std::vector<kv_key_t> base_keys = get_base_keys(config);
  KVSTrafficGenerator generator(config, base_keys);

  generator.generate_warmup();
  generator.generate();

  return 0;
}