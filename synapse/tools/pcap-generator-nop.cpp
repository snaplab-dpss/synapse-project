#include <LibCore/Pcap.h>
#include <LibCore/RandomEngine.h>

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

#define NF "nop"

#define SMAC "02:00:00:ca:fe:ee"
#define DMAC "02:00:00:be:ee:ef"

#define DEFAULT_TOTAL_PACKETS 1'000'000lu
#define DEFAULT_TOTAL_FLOWS 65'536lu
#define DEFAULT_TOTAL_CHURN_FPM 0lu
#define DEFAULT_TRAFFIC_UNIFORM true
#define DEFAULT_TRAFFIC_ZIPF false
#define DEFAULT_TRAFFIC_ZIPF_PARAMETER 1.26 // From Castan [SIGCOMM'18]
#define DEFAULT_LAN_WAN_PAIRS "0,1 2,3 4,5 6,7 8,9 10,11 12,13 14,15 16,17 18,19 20,21 22,23 24,25 26,27 28,29"

// Using 1Gbps as the rate because (1) if we go too fast the pcap timestamps can't keep up with the actual time (pcap
// use us instead of ns), and (2) because we want to keep the pcaps as small as possible. We assume the traffic
// characteristics are the same for 10Gbps and 100Gbps.
#define RATE_GBIT 1

struct pkt_hdr_t {
  ether_hdr_t eth_hdr;
  ipv4_hdr_t ip_hdr;
  udp_hdr_t udp_hdr;
  u8 payload[22];
} __attribute__((packed));

pkt_hdr_t build_pkt_template() {
  pkt_hdr_t pkt;

  pkt.eth_hdr.ether_type = htons(ETHERTYPE_IP);
  LibCore::parse_etheraddr(DMAC, &pkt.eth_hdr.daddr);
  LibCore::parse_etheraddr(SMAC, &pkt.eth_hdr.saddr);

  pkt.ip_hdr.version         = 4;
  pkt.ip_hdr.ihl             = 5;
  pkt.ip_hdr.type_of_service = 0;
  pkt.ip_hdr.total_length    = htons(sizeof(pkt.ip_hdr) + sizeof(pkt.udp_hdr) + sizeof(pkt.payload));
  pkt.ip_hdr.packet_id       = 0;
  pkt.ip_hdr.fragment_offset = 0;
  pkt.ip_hdr.time_to_live    = 64;
  pkt.ip_hdr.next_proto_id   = IPPROTO_UDP;
  pkt.ip_hdr.hdr_checksum    = 0;
  pkt.ip_hdr.src_addr        = 0;
  pkt.ip_hdr.dst_addr        = 0;

  pkt.udp_hdr.src_port = 0;
  pkt.udp_hdr.dst_port = 0;
  pkt.udp_hdr.len      = htons(sizeof(pkt.udp_hdr) + sizeof(pkt.payload));
  pkt.udp_hdr.checksum = 0;

  memset(pkt.payload, 0x42, sizeof(pkt.payload));

  return pkt;
}

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

enum class TrafficType { Uniform = 0, Zipf = 1 };
const std::unordered_map<std::string, TrafficType> traffic_type_from_str{{"uniform", TrafficType::Uniform}, {"zipf", TrafficType::Zipf}};

struct config_t {
  std::filesystem::path out_dir;
  u64 total_packets;
  u64 total_flows;
  u64 churn_fpm;
  TrafficType traffic_type;
  double traffic_zipf_param;
  std::vector<std::pair<u16, u16>> lan_wan_pairs;
  u32 random_seed;
  bool dry_run;
};

void config_print(const config_t &config) {
  printf("----- Config -----\n");
  printf("out dir:     %s\n", config.out_dir.c_str());
  printf("random seed: %u\n", config.random_seed);
  printf("#packets:    %lu\n", config.total_packets);
  printf("#flows:      %lu\n", config.total_flows);
  printf("churn:       %lu fpm\n", config.churn_fpm);
  switch (config.traffic_type) {
  case TrafficType::Uniform:
    printf("traffic:     uniform\n");
    break;
  case TrafficType::Zipf:
    printf("traffic:     zipf\n");
    printf("zipf param:  %f\n", config.traffic_zipf_param);
    break;
  }
  printf("LAN/WAN pairs:\n");
  for (const auto &lan_wan_pair : config.lan_wan_pairs) {
    printf("  %d -> %d\n", lan_wan_pair.first, lan_wan_pair.second);
  }
  printf("--- ---------- ---\n");

  if (config.dry_run) {
    exit(0);
  }
}

std::string get_base_pcap_fname(const config_t &config) {
  std::stringstream ss;

  ss << NF;
  ss << "-f" << config.total_flows;
  ss << "-c" << config.churn_fpm;
  switch (config.traffic_type) {
  case TrafficType::Uniform:
    ss << "-unif";
    break;
  case TrafficType::Zipf:
    ss << "-zipf" << config.traffic_zipf_param;
    break;
  }

  return ss.str();
}

std::filesystem::path get_warmup_pcap_fname(const config_t &config, u16 dev) {
  std::stringstream ss;
  ss << get_base_pcap_fname(config);
  ss << "-dev" << dev;
  ss << "-warmup.pcap";
  return config.out_dir / ss.str();
}

std::filesystem::path get_pcap_fname(const config_t &config, u16 dev) {
  std::stringstream ss;
  ss << get_base_pcap_fname(config);
  ss << "-dev" << dev;
  ss << ".pcap";
  return config.out_dir / ss.str();
}

std::vector<LibCore::flow_t> get_base_flows(const config_t &config) {
  std::vector<LibCore::flow_t> flows;
  flows.reserve(config.total_flows);

  for (size_t i = 0; i < config.total_flows; i++) {
    flows.push_back(random_flow());
  }

  return flows;
}

enum class Dev { WAN, LAN };

class TrafficGenerator {
private:
  config_t config;
  std::vector<LibCore::flow_t> flows;

  std::unordered_map<u16, LibCore::PcapWriter> wan_writers;
  std::unordered_map<u16, LibCore::PcapWriter> lan_writers;

  LibCore::RandomUniformEngine uniform_rand;
  LibCore::RandomZipfEngine zipf_rand;

  pcap_t *pd;
  pcap_dumper_t *pdumper;

  pkt_hdr_t packet_template;

  const std::unordered_map<u16, u16> lan_to_wan_dev;
  const std::vector<u16> lan_devices;
  const std::vector<u16> wan_devices;
  u16 current_lan_dev_it;

  std::unordered_map<LibCore::flow_t, Dev, LibCore::flow_t::flow_hash_t> flows_dev_turn;
  std::unordered_map<LibCore::flow_t, u16, LibCore::flow_t::flow_hash_t> flows_to_lan_dev;
  std::unordered_set<LibCore::flow_t, LibCore::flow_t::flow_hash_t> allocated_flows;
  std::unordered_map<LibCore::flow_t, u64, LibCore::flow_t::flow_hash_t> counters;
  u64 flows_swapped;

  time_ns_t current_time;
  time_ns_t alarm_tick;
  time_ns_t next_alarm;

public:
  TrafficGenerator(const config_t &_config, const std::vector<LibCore::flow_t> &_base_flows)
      : config(_config), flows(_base_flows), uniform_rand(_config.random_seed, 0, _config.total_flows - 1),
        zipf_rand(_config.random_seed, _config.traffic_zipf_param, _config.total_flows), pd(NULL), pdumper(NULL),
        packet_template(build_pkt_template()), lan_to_wan_dev(build_lan_to_wan(config.lan_wan_pairs)),
        lan_devices(build_lan_devices(config.lan_wan_pairs)), wan_devices(build_wan_devices(config.lan_wan_pairs)), current_lan_dev_it(0),
        counters(0), flows_swapped(0), current_time(0), alarm_tick(0), next_alarm(-1) {
    for (u16 lan_dev : lan_devices) {
      lan_writers.emplace(lan_dev, get_pcap_fname(config, lan_dev));
    }

    for (u16 wan_dev : wan_devices) {
      wan_writers.emplace(wan_dev, get_pcap_fname(config, wan_dev));
    }

    for (LibCore::flow_t &flow : flows) {
      u16 lan_dev = get_current_lan_dev();
      u16 wan_dev = lan_to_wan_dev.at(lan_dev);
      advance_lan_dev();

      flow.src_ip = mask_addr_from_dev(flow.src_ip, lan_dev);
      flow.dst_ip = mask_addr_from_dev(flow.dst_ip, wan_dev);
      allocated_flows.insert(flow);

      flows_dev_turn[flow]   = Dev::LAN;
      flows_to_lan_dev[flow] = lan_dev;
      counters[flow]         = 0;
    }

    if (config.churn_fpm > 0) {
      alarm_tick = 60'000'000'000 / config.churn_fpm;
      next_alarm = alarm_tick;
    }
  }

  void dump() {
    u64 counter  = 0;
    u64 goal     = config.total_packets;
    int progress = -1;

    for (const auto &[lan_dev, lan_writer] : lan_writers) {
      printf("LAN dev %u: %s\n", lan_dev, lan_writer.get_output_fname().c_str());
    }

    for (const auto &[wan_dev, wan_writer] : wan_writers) {
      printf("WAN dev %u: %s\n", wan_dev, wan_writer.get_output_fname().c_str());
    }

    for (u64 i = 0; i < config.total_packets; i++) {
      pkt_hdr_t pkt = packet_template;

      if (next_alarm >= 0 && current_time >= next_alarm) {
        u16 lan_dev = get_current_lan_dev();
        u16 wan_dev = lan_to_wan_dev.at(lan_dev);
        advance_lan_dev();

        u64 chosen_swap_flow_idx = uniform_rand.generate();
        random_swap_flow(chosen_swap_flow_idx, lan_dev, wan_dev);
        next_alarm += alarm_tick;

        const LibCore::flow_t &new_flow = flows[chosen_swap_flow_idx];
        flows_dev_turn[new_flow]        = Dev::LAN;
        flows_to_lan_dev[new_flow]      = lan_dev;
        counters[new_flow]              = 0;
      }

      u64 flow_idx;
      switch (config.traffic_type) {
      case TrafficType::Uniform:
        flow_idx = uniform_rand.generate();
        break;
      case TrafficType::Zipf:
        flow_idx = zipf_rand.generate();
        break;
      }
      assert(flow_idx < flows.size());
      const LibCore::flow_t &flow = flows[flow_idx];

      if (flows_dev_turn[flow] == Dev::LAN) {
        u16 lan_dev          = flows_to_lan_dev.at(flow);
        flows_dev_turn[flow] = Dev::WAN;

        pkt.ip_hdr.src_addr  = flow.src_ip;
        pkt.ip_hdr.dst_addr  = flow.dst_ip;
        pkt.udp_hdr.src_port = flow.src_port;
        pkt.udp_hdr.dst_port = flow.dst_port;

        lan_writers.at(lan_dev).write((const u8 *)&pkt, sizeof(pkt_hdr_t), sizeof(pkt_hdr_t), current_time);
      } else {
        u16 lan_dev          = flows_to_lan_dev.at(flow);
        u16 wan_dev          = lan_to_wan_dev.at(lan_dev);
        flows_dev_turn[flow] = Dev::LAN;

        LibCore::flow_t inverted_flow = invert_flow(flow);

        pkt.ip_hdr.src_addr  = inverted_flow.src_ip;
        pkt.ip_hdr.dst_addr  = inverted_flow.dst_ip;
        pkt.udp_hdr.src_port = inverted_flow.src_port;
        pkt.udp_hdr.dst_port = inverted_flow.dst_port;

        wan_writers.at(wan_dev).write((const u8 *)&pkt, sizeof(pkt_hdr_t), sizeof(pkt_hdr_t), current_time);
      }

      counters[flow]++;

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
  static std::unordered_map<u16, u16> build_lan_to_wan(const std::vector<std::pair<u16, u16>> &lan_wan_pairs) {
    std::unordered_map<u16, u16> lan_to_wan_dev;
    for (const auto &lan_wan_pair : lan_wan_pairs) {
      lan_to_wan_dev[lan_wan_pair.first] = lan_wan_pair.second;
    }
    return lan_to_wan_dev;
  }

  static std::vector<u16> build_lan_devices(const std::vector<std::pair<u16, u16>> &lan_wan_pairs) {
    std::vector<u16> lan_devices;
    for (const auto &lan_wan_pair : lan_wan_pairs) {
      lan_devices.push_back(lan_wan_pair.first);
    }
    return lan_devices;
  }

  static std::vector<u16> build_wan_devices(const std::vector<std::pair<u16, u16>> &lan_wan_pairs) {
    std::vector<u16> wan_devices;
    for (const auto &lan_wan_pair : lan_wan_pairs) {
      wan_devices.push_back(lan_wan_pair.second);
    }
    return wan_devices;
  }

  static in_addr_t mask_addr_from_dev(in_addr_t addr, u16 dev) { return LibCore::ipv4_set_prefix(addr, dev, 5); }

  void advance_lan_dev() { current_lan_dev_it = (current_lan_dev_it + 1) % lan_devices.size(); }
  u16 get_current_lan_dev() const { return lan_devices.at(current_lan_dev_it); }

  void report() const {
    std::vector<u64> counters_values;
    counters_values.reserve(counters.size());
    for (const auto &kv : counters) {
      // Take warmup into account.
      counters_values.push_back(kv.second);
    }
    std::sort(counters_values.begin(), counters_values.end(), std::greater{});

    u64 total_flows = counters.size();

    u64 hh         = 0;
    u64 hh_packets = 0;
    for (size_t i = 0; i < total_flows; i++) {
      hh++;
      hh_packets += counters_values[i];
      if (hh_packets >= config.total_packets * 0.8) {
        break;
      }
    }

    printf("\n");
    printf("Base flows: %ld\n", flows.size());
    printf("Total flows: %ld\n", total_flows);
    printf("Swapped flows: %ld\n", flows_swapped);
    printf("HH: %ld flows (%.2f%%) %.2f%% volume\n", hh, 100.0 * hh / total_flows, 100.0 * hh_packets / config.total_packets);
    printf("Top 10 flows:\n");
    for (size_t i = 0; i < config.total_flows; i++) {
      printf("  flow %ld: %ld\n", i, counters_values[i]);

      if (i >= 9) {
        break;
      }
    }
  }

  void random_swap_flow(u64 flow_idx, u16 lan_dev, u16 wan_dev) {
    assert(flow_idx < flows.size());
    LibCore::flow_t new_flow = random_flow();
    new_flow.src_ip          = mask_addr_from_dev(new_flow.src_ip, lan_dev);
    new_flow.dst_ip          = mask_addr_from_dev(new_flow.dst_ip, wan_dev);

    counters[new_flow] = 0;
    flows[flow_idx]    = new_flow;

    flows_swapped++;
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

  app.add_option("--out", config.out_dir, "Output directory.")->default_val(".");
  app.add_option("--packets", config.total_packets, "Total packets.")->default_val(DEFAULT_TOTAL_PACKETS);
  app.add_option("--flows", config.total_flows, "Total flows.")->default_val(DEFAULT_TOTAL_FLOWS);
  app.add_option("--churn", config.churn_fpm, "Total churn (fpm).")->default_val(DEFAULT_TOTAL_CHURN_FPM);
  app.add_option("--traffic", config.traffic_type, "Traffic distribution.")
      ->required()
      ->transform(CLI::CheckedTransformer(traffic_type_from_str, CLI::ignore_case));
  app.add_option("--zipf-param", config.traffic_zipf_param, "Zipf parameter.")->default_val(DEFAULT_TRAFFIC_ZIPF_PARAMETER);
  app.add_option("--devs", config.lan_wan_pairs, "LAN/WAN pairs.")->delimiter(',')->required();
  app.add_option("--seed", config.random_seed, "Random seed.")->default_val(std::random_device()());
  app.add_flag("--dry-run", config.dry_run, "Print out the configuration values without generating the pcaps.")->default_val(false);

  CLI11_PARSE(app, argc, argv);

  srand(config.random_seed);

  config_print(config);

  std::vector<LibCore::flow_t> base_flows = get_base_flows(config);
  TrafficGenerator generator(config, base_flows);

  generator.dump();

  return 0;
}