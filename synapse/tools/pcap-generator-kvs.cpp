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

using namespace LibCore;

using device_t    = TrafficGenerator::device_t;
using config_t    = TrafficGenerator::config_t;
using TrafficType = TrafficGenerator::TrafficType;

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

struct kvs_op_ratio_t {
  u64 get;
  u64 put;
};

static kvs_op_ratio_t calculate_kvs_op_ratio(double get_ratio) {
  struct kvs_op_ratio_t ratio = {.get = 0, .put = 0};

  u64 total = 1;

  double lhs = -1;
  while (std::modf(get_ratio, &lhs) != 0) {
    total *= 10;
    get_ratio *= 10;
  }

  ratio.get = static_cast<u64>(get_ratio);
  ratio.put = total - ratio.get;

  assert(ratio.get + ratio.put > 0 && "Invalid KVS ratio");

  return ratio;
}

class KVSTrafficGenerator : public TrafficGenerator {
private:
  const kvs_op_ratio_t kvs_op_ratio;

  std::vector<kv_key_t> keys;
  std::vector<u64> kvs_op_seq;

public:
  KVSTrafficGenerator(const config_t &_config, const kvs_op_ratio_t &_kvs_op_ratio, const std::vector<kv_key_t> &_base_keys)
      : TrafficGenerator("kvs", _config, true), kvs_op_ratio(_kvs_op_ratio), keys(_base_keys), kvs_op_seq(_base_keys.size(), kvs_op_ratio.get) {}

  virtual bytes_t get_hdrs_len() const override { return sizeof(ether_hdr_t) + sizeof(ipv4_hdr_t) + sizeof(udp_hdr_t) + sizeof(kvs_hdr_t); }

  virtual void random_swap_flow(flow_idx_t flow_idx) override {
    assert(flow_idx < keys.size());
    keys[flow_idx]       = random_key();
    kvs_op_seq[flow_idx] = kvs_op_ratio.get;
    flows_swapped++;
  }

  virtual std::optional<pkt_t> build_packet(device_t dev, flow_idx_t flow_idx) override {
    pkt_t pkt          = template_packet;
    const kv_key_t key = keys[flow_idx];
    const kvs_op_t op  = get_next_op(flow_idx);

    pkt.udp_hdr.dst_port = bswap16(KVSTORE_PORT);

    kvs_hdr_t *kvs_hdr = reinterpret_cast<kvs_hdr_t *>(pkt.payload);
    kvs_hdr->op        = op;
    std::memcpy(kvs_hdr->key, key.data(), sizeof(kvs_hdr->key));
    std::memset(kvs_hdr->value, 0, sizeof(kvs_hdr->value));
    kvs_hdr->status      = KVS_STATUS_MISS;
    kvs_hdr->client_port = 0;

    return pkt;
  }

  virtual pkt_t build_warmup_packet(device_t dev, flow_idx_t flow_idx) override {
    pkt_t pkt          = template_packet;
    const kv_key_t key = keys[flow_idx];

    pkt.udp_hdr.dst_port = bswap16(KVSTORE_PORT);

    kvs_hdr_t *kvs_hdr = reinterpret_cast<kvs_hdr_t *>(pkt.payload);
    kvs_hdr->op        = KVS_OP_PUT;
    std::memcpy(kvs_hdr->key, key.data(), sizeof(kvs_hdr->key));
    std::memset(kvs_hdr->value, 0, sizeof(kvs_hdr->value));
    kvs_hdr->status      = KVS_STATUS_MISS;
    kvs_hdr->client_port = 0;

    return pkt;
  }

  virtual std::optional<device_t> get_response_dev(device_t dev, flow_idx_t flow_idx) const override { return {}; }

private:
  kvs_op_t get_next_op(flow_idx_t flow_idx) {
    const kvs_op_t op    = kvs_op_seq[flow_idx] < kvs_op_ratio.get ? KVS_OP_GET : KVS_OP_PUT;
    kvs_op_seq[flow_idx] = (kvs_op_seq[flow_idx] + 1) % (kvs_op_ratio.get + kvs_op_ratio.put);
    return op;
  }
};

int main(int argc, char *argv[]) {
  CLI::App app{"Traffic generator for the kvs nf."};

  TrafficGenerator::config_t config;
  double get_ratio{0.99};

  app.add_option("--out", config.out_dir, "Output directory.")->default_val(TrafficGenerator::DEFAULT_OUTPUT_DIR);
  app.add_option("--packets", config.total_packets, "Total packets.")->default_val(TrafficGenerator::DEFAULT_TOTAL_PACKETS);
  app.add_option("--flows", config.total_flows, "Total flows.")->default_val(TrafficGenerator::DEFAULT_TOTAL_FLOWS);
  app.add_option("--rate", config.rate, "Rate (bps).")->default_val(TrafficGenerator::DEFAULT_RATE);
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
  app.add_option("--get-ratio", get_ratio, "Ratio of GET/PUT requests.");
  app.add_flag("--dry-run", config.dry_run, "Print out the configuration values without generating the pcaps.")->default_val(false);

  CLI11_PARSE(app, argc, argv);

  config.packet_size_without_crc = std::max(KVS_PKT_SIZE + CRC_SIZE_BYTES, MIN_PKT_SIZE_BYTES) - CRC_SIZE_BYTES;
  config.client_devices          = config.devices;

  srand(config.random_seed);

  config.print();
  if (config.dry_run) {
    return 0;
  }

  const kvs_op_ratio_t kvs_ratio  = calculate_kvs_op_ratio(get_ratio);
  std::vector<kv_key_t> base_keys = get_base_keys(config);
  KVSTrafficGenerator generator(config, kvs_ratio, base_keys);

  generator.generate_warmup();
  generator.generate();

  return 0;
}