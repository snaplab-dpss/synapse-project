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

#include "util.hpp"

#define SMAC "02:00:00:ca:fe:ee"
#define DMAC "02:00:00:be:ee:ef"
#define SRC_IP "10.10.0.1"
#define DST_IP "10.10.0.2"
#define SRC_PORT 10000
#define DST_PORT 670

#define KEY_SIZE_BYTES 16
#define VALUE_SIZE_BYTES 128

#define ARG_HELP "help"
#define ARG_TOTAL_PACKETS "packets"
#define ARG_TOTAL_KEYS "flows"
#define ARG_TOTAL_CHURN_FPM "churn"
#define ARG_TRAFFIC_UNIFORM "uniform"
#define ARG_TRAFFIC_ZIPF "zipf"
#define ARG_TRAFFIC_ZIPF_PARAM "zipf-param"
#define ARG_RANDOM_SEED "seed"

#define DEFAULT_TOTAL_PACKETS 1'000'000lu
#define DEFAULT_TOTAL_KEYS 65'536lu
#define DEFAULT_TOTAL_CHURN_FPM 0lu
#define DEFAULT_PACKET_SIZES MIN_PKT_SIZE
#define DEFAULT_TRAFFIC_UNIFORM true
#define DEFAULT_TRAFFIC_ZIPF false
#define DEFAULT_TRAFFIC_ZIPF_PARAMETER 1.26 // From Castan [SIGCOMM'18]

enum {
  /* long options mapped to short options: first long only option value must
   * be >= 256, so that it does not conflict with short options.
   */
  ARG_HELP_NUM = 256,
  ARG_TOTAL_PACKETS_NUM,
  ARG_TOTAL_KEYS_NUM,
  ARG_TOTAL_CHURN_FPM_NUM,
  ARG_TRAFFIC_UNIFORM_NUM,
  ARG_TRAFFIC_ZIPF_NUM,
  ARG_TRAFFIC_ZIPF_PARAM_NUM,
  ARG_RANDOM_SEED_NUM,
};

typedef std::array<uint8_t, KEY_SIZE_BYTES> kv_key_t;
typedef std::array<uint8_t, VALUE_SIZE_BYTES> kv_value_t;

struct kv_key_hash_t {
  std::size_t operator()(const kv_key_t &key) const {
    std::size_t res = 0;
    for (size_t i = 0; i < key.size(); i++) {
      res ^= key[i];
    }
    return res;
  }
};

enum kvstore_op {
  KVSTORE_OP_GET = 0,
  KVSTORE_OP_PUT = 1,
  KVSTORE_OP_DEL = 2,
};

struct kvstore_hdr_t {
  uint8_t op;
  uint8_t key[KEY_SIZE_BYTES];
  uint8_t value[VALUE_SIZE_BYTES];
  uint8_t status;
} __attribute__((__packed__));

struct pkt_hdr_t {
  ether_header eth_hdr;
  iphdr ip_hdr;
  udphdr udp_hdr;
  kvstore_hdr_t kvstore_hdr;
} __attribute__((packed));

void build_pkt_template(pkt_hdr_t &pkt) {
  pkt.eth_hdr.ether_type = htons(ETHERTYPE_IP);
  parse_etheraddr(DMAC, (ether_addr *)&pkt.eth_hdr.ether_dhost);
  parse_etheraddr(SMAC, (ether_addr *)&pkt.eth_hdr.ether_shost);

  pkt.ip_hdr.version = 4;
  pkt.ip_hdr.ihl = 5;
  pkt.ip_hdr.tos = 0;
  pkt.ip_hdr.id = 0;
  pkt.ip_hdr.frag_off = 0;
  pkt.ip_hdr.ttl = 64;
  pkt.ip_hdr.protocol = IPPROTO_UDP;
  pkt.ip_hdr.check = 0;
  pkt.ip_hdr.saddr = inet_addr(SRC_IP);
  pkt.ip_hdr.daddr = inet_addr(DST_IP);
  pkt.ip_hdr.tot_len =
      htons(sizeof(pkt.ip_hdr) + sizeof(pkt.udp_hdr) + sizeof(pkt.kvstore_hdr));

  pkt.udp_hdr.source = htons(SRC_PORT);
  pkt.udp_hdr.dest = htons(DST_PORT);
  pkt.udp_hdr.len = htons(sizeof(pkt.udp_hdr) + sizeof(pkt.kvstore_hdr));
  pkt.udp_hdr.check = 0;

  memset(&pkt.kvstore_hdr, 0xff, sizeof(kvstore_hdr_t));
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
  unsigned random_seed;
  uint64_t total_packets;
  uint64_t total_keys;
  uint64_t churn_fpm;
  bool traffic_uniform;
  bool traffic_zipf;
  double traffic_zipf_param;
};

void config_usage(char **argv) {
  printf("Usage: %s\n"
         "\t [--" ARG_TOTAL_PACKETS " <#packets> (default=%lu)]\n"
         "\t [--" ARG_TOTAL_KEYS " <#flows> (default=%lu)]\n"
         "\t [--" ARG_TOTAL_CHURN_FPM " <fpm> (default=%lu)]\n"
         "\t [--" ARG_TRAFFIC_UNIFORM " (default=%s)]\n"
         "\t [--" ARG_TRAFFIC_ZIPF " (default=%s)]\n"
         "\t [--" ARG_TRAFFIC_ZIPF_PARAM " <param> (default=%f)]\n"
         "\t [--" ARG_RANDOM_SEED " <seed>]\n"
         "\t [--" ARG_HELP "]\n"
         "\n"
         "Argument descriptions:\n"
         "\t " ARG_TOTAL_PACKETS ": total number of packets to generate\n"
         "\t " ARG_TOTAL_KEYS ": total number of flows to generate\n"
         "\t " ARG_TOTAL_CHURN_FPM ": flow churn (fpm)\n"
         "\t " ARG_RANDOM_SEED ": random seed\n"
         "\t " ARG_HELP ": show this menu\n"
         "\n",
         argv[0], DEFAULT_TOTAL_PACKETS, DEFAULT_TOTAL_KEYS,
         DEFAULT_TOTAL_CHURN_FPM, DEFAULT_TRAFFIC_UNIFORM ? "true" : "false",
         DEFAULT_TRAFFIC_ZIPF ? "true" : "false",
         DEFAULT_TRAFFIC_ZIPF_PARAMETER);
}

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

config_t parse_args(int argc, char **argv) {
  config_t config;

  config.random_seed = std::random_device()();
  config.total_packets = DEFAULT_TOTAL_PACKETS;
  config.total_keys = DEFAULT_TOTAL_KEYS;
  config.churn_fpm = DEFAULT_TOTAL_CHURN_FPM;
  config.traffic_uniform = DEFAULT_TRAFFIC_UNIFORM;
  config.traffic_zipf = DEFAULT_TRAFFIC_ZIPF;
  config.traffic_zipf_param = DEFAULT_TRAFFIC_ZIPF_PARAMETER;

  const char short_options[] = "";

  struct option long_options[] = {
      {ARG_TOTAL_PACKETS, required_argument, NULL, ARG_TOTAL_PACKETS_NUM},
      {ARG_TOTAL_KEYS, required_argument, NULL, ARG_TOTAL_KEYS_NUM},
      {ARG_TOTAL_CHURN_FPM, required_argument, NULL, ARG_TOTAL_CHURN_FPM_NUM},
      {ARG_TRAFFIC_UNIFORM, no_argument, NULL, ARG_TRAFFIC_UNIFORM_NUM},
      {ARG_TRAFFIC_ZIPF, no_argument, NULL, ARG_TRAFFIC_ZIPF_NUM},
      {ARG_TRAFFIC_ZIPF_PARAM, required_argument, NULL,
       ARG_TRAFFIC_ZIPF_PARAM_NUM},
      {ARG_RANDOM_SEED, required_argument, NULL, ARG_RANDOM_SEED_NUM},
      {ARG_HELP, no_argument, NULL, ARG_HELP_NUM},
      {NULL, 0, NULL, 0}};

  int opt;

  while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) !=
         EOF) {
    switch (opt) {
    case ARG_HELP_NUM: {
      config_usage(argv);
      exit(EXIT_SUCCESS);
    } break;
    case ARG_TOTAL_PACKETS_NUM: {
      config.total_packets = parse_int(optarg, "packets", 10, '\0');
    } break;
    case ARG_TOTAL_KEYS_NUM: {
      config.total_keys = parse_int(optarg, "keys", 10, '\0');
    } break;
    case ARG_TOTAL_CHURN_FPM_NUM: {
      config.churn_fpm = parse_int(optarg, "churn", 10, '\0');
    } break;
    case ARG_TRAFFIC_UNIFORM_NUM: {
      config.traffic_uniform = true;
      config.traffic_zipf = false;
    } break;
    case ARG_TRAFFIC_ZIPF_NUM: {
      config.traffic_uniform = false;
      config.traffic_zipf = true;
    } break;
    case ARG_TRAFFIC_ZIPF_PARAM_NUM: {
      config.traffic_zipf_param = strtof(optarg, NULL);
    } break;
    case ARG_RANDOM_SEED_NUM: {
      config.random_seed = parse_int(optarg, "seed", 10, '\0');
    } break;
    default:
      PARSE_ERROR(argv, "Unknown option.\n");
    }
  }

  srand(config.random_seed);

  config_print(config);

  return config;
}

std::string get_output_pcap_fname(const config_t &config) {
  std::stringstream ss;

  ss << "kvstore";
  ss << "-seed-" << config.random_seed;
  ss << "-packets-" << config.total_packets;
  ss << "-keys-" << config.total_keys;
  ss << "-churn-" << config.churn_fpm;
  if (config.traffic_uniform) {
    ss << "-uniform";
  } else if (config.traffic_zipf) {
    ss << "-zipf-" << config.traffic_zipf_param;
  }
  ss << ".pcap";

  return ss.str();
}

std::string get_warmup_pcap_fname(const std::string &pcap_fname) {
  std::filesystem::path path(pcap_fname);
  std::string stem = path.stem().string();
  std::string extension = path.extension().string();

  std::stringstream ss;
  ss << stem << "-warmup" << extension;

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
  RandomRealEngine zipf_rand;

  pcap_t *pd;
  pcap_dumper_t *pdumper;

  pkt_hdr_t packet_template;
  std::unordered_map<kv_key_t, uint64_t, kv_key_hash_t> counters;
  uint64_t keys_swapped;

  time_ns_t current_time;
  time_ns_t alarm_tick;
  time_ns_t next_alarm;

public:
  TrafficGenerator(const config_t &_config,
                   const std::vector<kv_key_t> &_base_keys,
                   const std::string &_warmup_pcap_fname,
                   const std::string &_output_pcap_fname)
      : config(_config), keys(_base_keys), warmup_writer(_warmup_pcap_fname),
        writer(_output_pcap_fname),
        uniform_rand(_config.random_seed, 0, _config.total_keys - 1),
        zipf_rand(_config.random_seed, 0, 1), pd(NULL), pdumper(NULL),
        counters(0), keys_swapped(0), current_time(0), alarm_tick(0),
        next_alarm(-1) {
    build_pkt_template(packet_template);

    for (const kv_key_t &key : keys) {
      counters[key] = 0;
    }

    if (config.churn_fpm > 0) {
      alarm_tick = 60'000'000'000 / config.churn_fpm;
      next_alarm = alarm_tick;
    }
  }

  void dump_warmup() {
    uint64_t counter = 0;
    uint64_t goal = keys.size();
    int progress = -1;

    printf("Warmup: %s\n", warmup_writer.get_output_fname().c_str());

    for (const kv_key_t &key : keys) {
      pkt_hdr_t pkt = packet_template;

      pkt.kvstore_hdr.op = KVSTORE_OP_PUT;

      memcpy(pkt.kvstore_hdr.key, key.data(), KEY_SIZE_BYTES);

      kv_value_t value;
      randomize_value(value);
      memcpy(pkt.kvstore_hdr.value, value.data(), VALUE_SIZE_BYTES);

      counters[key]++;

      warmup_writer.write((const u_char *)&pkt, sizeof(pkt_hdr_t),
                          current_time);

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
    uint64_t counter = 0;
    uint64_t goal = config.total_packets;
    int progress = -1;

    printf("Traffic: %s\n", writer.get_output_fname().c_str());

    for (uint64_t i = 0; i < config.total_packets; i++) {
      pkt_hdr_t pkt = packet_template;

      if (next_alarm >= 0 && current_time >= next_alarm) {
        uint64_t chosen_swap_key_idx = uniform_rand.generate();
        random_swap_key(chosen_swap_key_idx);
        next_alarm += alarm_tick;
      }

      uint64_t key_idx = 0;
      if (config.traffic_uniform) {
        key_idx = get_next_key_uniform();
      } else {
        key_idx = get_next_key_zipf();
      }

      assert(key_idx < keys.size());

      const kv_key_t &key = keys[key_idx];
      memcpy(pkt.kvstore_hdr.key, key.data(), KEY_SIZE_BYTES);

      if (counters[key] == 0) {
        pkt.kvstore_hdr.op = KVSTORE_OP_PUT;

        kv_value_t value;
        randomize_value(value);
        memcpy(pkt.kvstore_hdr.value, value.data(), VALUE_SIZE_BYTES);
      } else {
        pkt.kvstore_hdr.op = KVSTORE_OP_GET;
      }

      counters[key]++;
      writer.write((const u_char *)&pkt, sizeof(pkt_hdr_t), current_time);

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
    std::vector<uint64_t> counters_values;
    counters_values.reserve(counters.size());
    for (const auto &kv : counters) {
      counters_values.push_back(kv.second);
    }
    std::sort(counters_values.begin(), counters_values.end(), std::greater{});

    uint64_t total_keys = counters.size();

    uint64_t hh = 0;
    uint64_t hh_packets = 0;
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

  void random_swap_key(uint64_t key_idx) {
    assert(key_idx < keys.size());
    kv_key_t new_key;
    randomize_key(new_key);

    counters[new_key] = 0;
    keys[key_idx] = new_key;

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

    constexpr int rate_gbps = 100;
    constexpr int CRC = 4;
    constexpr int IPG = 20;
    constexpr int bytes = sizeof(pkt_hdr_t) + CRC + IPG;

    current_time += (bytes * 8) / rate_gbps;
  }

  uint64_t get_next_key_uniform() { return uniform_rand.generate(); }

  // From Castan [SIGCOMM'18]
  // Source:
  // https://github.com/nal-epfl/castan/blob/master/scripts/pcap_tools/create_zipfian_distribution_pcap.py
  uint64_t get_next_key_zipf() {
    double probability = zipf_rand.generate();
    assert(probability >= 0 && probability <= 1);

    double p = probability;
    uint64_t N = config.total_keys + 1;
    double s = config.traffic_zipf_param;
    double tolerance = 0.01;
    double x = (double)N / 2.0;

    double D = p * (12.0 * (pow(N, 1.0 - s) - 1) / (1.0 - s) + 6.0 -
                    6.0 * pow(N, -s) + s - pow(N, -1.0 - s) * s);

    while (true) {
      double m = pow(x, -2 - s);
      double mx = m * x;
      double mxx = mx * x;
      double mxxx = mxx * x;

      double a = 12.0 * (mxxx - 1) / (1.0 - s) + 6.0 * (1.0 - mxx) +
                 (s - (mx * s)) - D;
      double b = 12.0 * mxx + 6.0 * (s * mx) + (m * s * (s + 1.0));
      double newx = std::max(1.0, x - a / b);

      if (std::abs(newx - x) <= tolerance) {
        int i = newx - 1;
        assert(i >= 0 && i < config.total_keys);
        return i;
      }

      x = newx;
    }
  }
};

int main(int argc, char *argv[]) {
  config_t config = parse_args(argc, argv);

  std::string output_fname = get_output_pcap_fname(config);
  std::string warmup_fname = get_warmup_pcap_fname(output_fname);

  std::vector<kv_key_t> base_keys = get_base_keys(config);
  TrafficGenerator generator(config, base_keys, warmup_fname, output_fname);

  generator.dump_warmup();
  generator.dump();

  return 0;
}