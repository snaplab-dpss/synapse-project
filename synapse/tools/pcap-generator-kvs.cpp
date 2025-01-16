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

#include "../src/pcap.hpp"

#define NF "kvs"

#define SMAC "02:00:00:ca:fe:ee"
#define DMAC "02:00:00:be:ee:ef"
#define SRC_IP "10.10.0.1"
#define DST_IP "10.10.0.2"
#define SRC_PORT 10000
#define DST_PORT 670 // KVSTORE_PORT

#define KEY_SIZE_BYTES 12
#define VALUE_SIZE_BYTES 128

#define DEFAULT_TOTAL_PACKETS 1'000'000lu
#define DEFAULT_TOTAL_KEYS 65'536lu
#define DEFAULT_TOTAL_CHURN_FPM 0lu
#define DEFAULT_TRAFFIC_UNIFORM true
#define DEFAULT_TRAFFIC_ZIPF false
#define DEFAULT_TRAFFIC_ZIPF_PARAMETER 1.26 // From Castan [SIGCOMM'18]

// Using 1Gbps as the rate because (1) if we go too fast the pcap timestamps can't keep up with the actual time (pcap
// use us instead of ns), and (2) because we want to keep the pcaps as small as possible. We assume the traffic
// characteristics are the same for 10Gbps and 100Gbps.
#define RATE_GBIT 1

using namespace synapse;

typedef std::array<u8, KEY_SIZE_BYTES> kv_key_t;
typedef std::array<u8, VALUE_SIZE_BYTES> kv_value_t;

struct kv_key_hash_t {
  std::size_t operator()(const kv_key_t &key) const {
    std::size_t res = 0;
    for (size_t i = 0; i < key.size(); i++) {
      res ^= key[i];
    }
    return res;
  }
};

enum kvs_op {
  KVS_OP_GET = 0,
  KVS_OP_PUT = 1,
  KVS_OP_DEL = 2,
};

struct kvs_hdr_t {
  u8 op;
  u8 key[KEY_SIZE_BYTES];
  u8 value[VALUE_SIZE_BYTES];
  u8 status;
} __attribute__((__packed__));

struct pkt_hdr_t {
  ether_hdr_t eth_hdr;
  ipv4_hdr_t ip_hdr;
  udp_hdr_t udp_hdr;
  kvs_hdr_t kvs_hdr;
} __attribute__((packed));

pkt_hdr_t build_pkt_template() {
  pkt_hdr_t pkt;

  pkt.eth_hdr.ether_type = htons(ETHERTYPE_IP);
  parse_etheraddr(DMAC, &pkt.eth_hdr.daddr);
  parse_etheraddr(SMAC, &pkt.eth_hdr.saddr);

  pkt.ip_hdr.version         = 4;
  pkt.ip_hdr.ihl             = 5;
  pkt.ip_hdr.type_of_service = 0;
  pkt.ip_hdr.total_length    = htons(sizeof(pkt.ip_hdr) + sizeof(pkt.udp_hdr) + sizeof(pkt.kvs_hdr));
  pkt.ip_hdr.packet_id       = 0;
  pkt.ip_hdr.fragment_offset = 0;
  pkt.ip_hdr.time_to_live    = 64;
  pkt.ip_hdr.next_proto_id   = IPPROTO_UDP;
  pkt.ip_hdr.hdr_checksum    = 0;
  pkt.ip_hdr.src_addr        = inet_addr(SRC_IP);
  pkt.ip_hdr.dst_addr        = inet_addr(DST_IP);

  pkt.udp_hdr.src_port = htons(SRC_PORT);
  pkt.udp_hdr.dst_port = htons(DST_PORT);
  pkt.udp_hdr.len      = htons(sizeof(pkt.udp_hdr) + sizeof(pkt.kvs_hdr));
  pkt.udp_hdr.checksum = 0;

  memset(&pkt.kvs_hdr, 0xff, sizeof(kvs_hdr_t));

  return pkt;
}

void randomize_key(kv_key_t &key) {
  for (int i = 0; i < KEY_SIZE_BYTES; i++) {
    key[i] = rand();
  }
}

void randomize_value(kv_value_t &value) {
  for (int i = 0; i < VALUE_SIZE_BYTES; i++) {
    value[i] = rand();
  }
}

struct config_t {
  u32 random_seed;
  u64 total_packets;
  u64 total_keys;
  u64 churn_fpm;
  bool traffic_uniform;
  bool traffic_zipf;
  double traffic_zipf_param;
};

void config_print(const config_t &config) {
  printf("----- Config -----\n");
  printf("random seed: %u\n", config.random_seed);
  printf("#packets:    %lu\n", config.total_packets);
  printf("#keys:       %lu\n", config.total_keys);
  printf("churn:       %lu fpm\n", config.churn_fpm);
  if (config.traffic_uniform) {
    printf("traffic:     uniform\n");
  } else if (config.traffic_zipf) {
    printf("traffic:     zipf\n");
    printf("zipf param:  %f\n", config.traffic_zipf_param);
  }
  printf("--- ---------- ---\n");
}

std::string get_base_pcap_fname(const config_t &config) {
  std::stringstream ss;

  ss << NF;
  ss << "-k" << config.total_keys;
  ss << "-c" << config.churn_fpm;
  if (config.traffic_uniform) {
    ss << "-unif";
  } else if (config.traffic_zipf) {
    ss << "-zipf" << config.traffic_zipf_param;
  }

  return ss.str();
}

std::string get_warmup_pcap_fname(const config_t &config) {
  std::stringstream ss;
  ss << get_base_pcap_fname(config);
  ss << "-warmup.pcap";
  return ss.str();
}

std::string get_pcap_fname(const config_t &config) {
  std::stringstream ss;
  ss << get_base_pcap_fname(config);
  ss << ".pcap";
  return ss.str();
}

std::vector<kv_key_t> get_base_keys(const config_t &config) {
  std::vector<kv_key_t> keys;
  keys.reserve(config.total_keys);

  for (size_t i = 0; i < config.total_keys; i++) {
    kv_key_t key;
    randomize_key(key);
    keys.push_back(key);
  }

  return keys;
}

class TrafficGenerator {
private:
  config_t config;
  std::vector<kv_key_t> keys;

  PcapWriter warmup_writer;
  PcapWriter writer;

  RandomEngine uniform_rand;
  RandomZipfEngine zipf_rand;

  pcap_t *pd;
  pcap_dumper_t *pdumper;

  pkt_hdr_t packet_template;

  std::unordered_set<kv_key_t, kv_key_hash_t> allocated_keys;
  std::unordered_map<kv_key_t, u64, kv_key_hash_t> counters;
  u64 keys_swapped;

  time_ns_t current_time;
  time_ns_t alarm_tick;
  time_ns_t next_alarm;

public:
  TrafficGenerator(const config_t &_config, const std::vector<kv_key_t> &_base_keys)
      : config(_config), keys(_base_keys), warmup_writer(get_warmup_pcap_fname(_config)),
        writer(get_pcap_fname(_config)), uniform_rand(_config.random_seed, 0, _config.total_keys - 1),
        zipf_rand(_config.random_seed, _config.traffic_zipf_param, _config.total_keys), pd(NULL), pdumper(NULL),
        packet_template(build_pkt_template()), counters(0), keys_swapped(0), current_time(0), alarm_tick(0),
        next_alarm(-1) {
    for (const kv_key_t &key : keys) {
      counters[key] = 0;
    }

    if (config.churn_fpm > 0) {
      alarm_tick = 60'000'000'000 / config.churn_fpm;
      next_alarm = alarm_tick;
    }
  }

  void dump_warmup() {
    u64 counter  = 0;
    u64 goal     = keys.size();
    int progress = -1;

    printf("Warmup: %s\n", warmup_writer.get_output_fname().c_str());

    for (const kv_key_t &key : keys) {
      pkt_hdr_t pkt = packet_template;

      pkt.kvs_hdr.op = KVS_OP_PUT;

      memcpy(pkt.kvs_hdr.key, key.data(), KEY_SIZE_BYTES);

      kv_value_t value;
      randomize_value(value);
      memcpy(pkt.kvs_hdr.value, value.data(), VALUE_SIZE_BYTES);

      warmup_writer.write((const u8 *)&pkt, sizeof(pkt_hdr_t), sizeof(pkt_hdr_t), current_time);

      counter++;
      int current_progress = (counter * 100) / goal;

      if (current_progress > progress) {
        progress = current_progress;
        printf("\r  Progress: %3d%%", progress);
        fflush(stdout);
      }
    }

    printf("\n");
  }

  void dump() {
    u64 counter  = 0;
    u64 goal     = config.total_packets;
    int progress = -1;

    printf("Traffic: %s\n", writer.get_output_fname().c_str());

    for (u64 i = 0; i < config.total_packets; i++) {
      pkt_hdr_t pkt = packet_template;

      if (next_alarm >= 0 && current_time >= next_alarm) {
        u64 chosen_swap_key_idx = uniform_rand.generate();
        random_swap_key(chosen_swap_key_idx);
        next_alarm += alarm_tick;
      }

      u64 key_idx = config.traffic_uniform ? uniform_rand.generate() : zipf_rand.generate();
      assert(key_idx < keys.size());

      const kv_key_t &key = keys[key_idx];
      memcpy(pkt.kvs_hdr.key, key.data(), KEY_SIZE_BYTES);

      pkt.kvs_hdr.op = KVS_OP_PUT;

      if (allocated_keys.find(key) == allocated_keys.end()) {
        kv_value_t value;
        randomize_value(value);
        memcpy(pkt.kvs_hdr.value, value.data(), VALUE_SIZE_BYTES);
        allocated_keys.insert(key);
      }

      counters[key]++;
      writer.write((const u8 *)&pkt, sizeof(pkt_hdr_t), sizeof(pkt_hdr_t), current_time);

      tick();

      counter++;
      int current_progress = (counter * 100) / goal;

      if (current_progress > progress) {
        progress = current_progress;
        printf("\r  Progress: %3d%%", progress);
        fflush(stdout);
      }
    }

    printf("\n");

    report();
  }

private:
  void report() const {
    std::vector<u64> counters_values;
    counters_values.reserve(counters.size());
    for (const auto &kv : counters) {
      counters_values.push_back(kv.second);
    }
    std::sort(counters_values.begin(), counters_values.end(), std::greater{});

    u64 total_keys = counters.size();

    u64 hh         = 0;
    u64 hh_packets = 0;
    for (size_t i = 0; i < total_keys; i++) {
      hh++;
      hh_packets += counters_values[i];
      if (hh_packets >= config.total_packets * 0.8) {
        break;
      }
    }

    printf("\n");
    printf("Base keys: %ld\n", keys.size());
    printf("Total keys: %ld\n", total_keys);
    printf("Swapped keys: %ld\n", keys_swapped);
    printf("HH: %ld keys (%.2f%%) %.2f%% volume\n", hh, 100.0 * hh / total_keys,
           100.0 * hh_packets / config.total_packets);
    printf("Top 10 keys:\n");
    for (size_t i = 0; i < config.total_keys; i++) {
      printf("  key %ld: %ld\n", i, counters_values[i]);

      if (i >= 9) {
        break;
      }
    }
  }

  void random_swap_key(u64 key_idx) {
    assert(key_idx < keys.size());
    kv_key_t new_key;
    randomize_key(new_key);

    counters[new_key] = 0;
    keys[key_idx]     = new_key;

    keys_swapped++;
  }

  void tick() {
    // To obtain the time in seconds:
    // (pkt.size * 8) / (gbps * 1e9)
    // We want in ns, not seconds, so we multiply by 1e9.
    // This cancels with the 1e9 on the bottom side of the operation.
    // So actually, result in ns = (pkt.size * 8) / gbps
    // Also, don't forget to take the inter packet gap and CRC
    // into consideration.
    constexpr const bytes_t bytes = PREAMBLE_SIZE_BYTES + sizeof(pkt_hdr_t) + CRC_SIZE_BYTES + IPG_SIZE_BYTES;
    constexpr const time_ns_t dt  = (bytes * 8) / RATE_GBIT;
    current_time += dt;
    if (alarm_tick > 0 && alarm_tick < dt) {
      fprintf(stderr, "Churn is too high: alarm tick (%luns) is smaller than the time step (%luns)\n", alarm_tick, dt);
      exit(1);
    }
  }
};

int main(int argc, char *argv[]) {
  CLI::App app{"Traffic generator for the " NF " nf."};

  config_t config;

  app.add_option("--packets", config.total_packets, "Total packets.")->default_val(DEFAULT_TOTAL_PACKETS);
  app.add_option("--flows", config.total_keys, "Total keys.")->default_val(DEFAULT_TOTAL_KEYS);
  app.add_option("--churn", config.churn_fpm, "Total churn (fpm).")->default_val(DEFAULT_TOTAL_CHURN_FPM);
  app.add_flag("--uniform", config.traffic_uniform, "Uniform traffic.")->default_val(DEFAULT_TRAFFIC_UNIFORM);
  app.add_flag("--zipf", config.traffic_zipf, "Zipf traffic.")->default_val(DEFAULT_TRAFFIC_ZIPF);
  app.add_option("--zipf-param", config.traffic_zipf_param, "Zipf parameter.")
      ->default_val(DEFAULT_TRAFFIC_ZIPF_PARAMETER);
  app.add_option("--seed", config.random_seed, "Random seed.")->default_val(std::random_device()());

  CLI11_PARSE(app, argc, argv);

  srand(config.random_seed);

  config_print(config);

  std::vector<kv_key_t> base_keys = get_base_keys(config);
  TrafficGenerator generator(config, base_keys);

  generator.dump_warmup();
  generator.dump();

  return 0;
}