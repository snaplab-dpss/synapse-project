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

#define ARG_HELP "help"
#define ARG_TOTAL_PACKETS "packets"
#define ARG_TOTAL_FLOWS "flows"
#define ARG_TOTAL_CHURN_FPM "churn"
#define ARG_TRAFFIC_UNIFORM "uniform"
#define ARG_TRAFFIC_ZIPF "zipf"
#define ARG_TRAFFIC_ZIPF_PARAM "zipf-param"
#define ARG_RANDOM_SEED "seed"
#define ARG_LAN_DEVICES "lan-devs"

#define DEFAULT_TOTAL_PACKETS 1'000'000lu
#define DEFAULT_TOTAL_FLOWS 65'536lu
#define DEFAULT_TOTAL_CHURN_FPM 0lu
#define DEFAULT_TRAFFIC_UNIFORM true
#define DEFAULT_TRAFFIC_ZIPF false
#define DEFAULT_TRAFFIC_ZIPF_PARAMETER 1.26 // From Castan [SIGCOMM'18]
#define DEFAULT_LAN_DEVICES 1

enum {
  /* long options mapped to short options: first long only option value must
   * be >= 256, so that it does not conflict with short options.
   */
  ARG_HELP_NUM = 256,
  ARG_TOTAL_PACKETS_NUM,
  ARG_TOTAL_FLOWS_NUM,
  ARG_TOTAL_CHURN_FPM_NUM,
  ARG_TRAFFIC_UNIFORM_NUM,
  ARG_TRAFFIC_ZIPF_NUM,
  ARG_TRAFFIC_ZIPF_PARAM_NUM,
  ARG_RANDOM_SEED_NUM,
  ARG_LAN_DEVICES_NUM,
};

struct pkt_hdr_t {
  ether_header eth_hdr;
  iphdr ip_hdr;
  udphdr udp_hdr;
} __attribute__((packed));

pkt_hdr_t build_pkt_template() {
  pkt_hdr_t pkt;

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
  pkt.ip_hdr.saddr = 0;
  pkt.ip_hdr.daddr = 0;
  pkt.ip_hdr.tot_len = htons(sizeof(pkt.ip_hdr) + sizeof(pkt.udp_hdr));

  pkt.udp_hdr.source = 0;
  pkt.udp_hdr.dest = 0;
  pkt.udp_hdr.len = htons(sizeof(pkt.udp_hdr));
  pkt.udp_hdr.check = 0;

  return pkt;
}

struct flow_t {
  in_addr_t src_ip;
  in_addr_t dst_ip;
  in_port_t src_port;
  in_port_t dst_port;
};

bool operator==(const flow_t &lhs, const flow_t &rhs) {
  return lhs.src_ip == rhs.src_ip && lhs.dst_ip == rhs.dst_ip &&
         lhs.src_port == rhs.src_port && lhs.dst_port == rhs.dst_port;
}

struct flow_hash_t {
  std::size_t operator()(const flow_t &flow) const {
    std::size_t res = 0;
    res ^= std::hash<in_addr_t>{}(flow.src_ip);
    res ^= std::hash<in_addr_t>{}(flow.dst_ip);
    res ^= std::hash<in_port_t>{}(flow.src_port);
    res ^= std::hash<in_port_t>{}(flow.dst_port);
    return res;
  }
};

flow_t random_flow() {
  flow_t flow;
  flow.src_ip = random_addr();
  flow.dst_ip = random_addr();
  flow.src_port = random_port();
  flow.dst_port = random_port();
  return flow;
}

flow_t invert_flow(const flow_t &flow) {
  flow_t inverted_flow;
  inverted_flow.src_ip = flow.dst_ip;
  inverted_flow.dst_ip = flow.src_ip;
  inverted_flow.src_port = flow.dst_port;
  inverted_flow.dst_port = flow.src_port;
  return inverted_flow;
}

struct config_t {
  unsigned random_seed;
  uint64_t total_packets;
  uint64_t total_flows;
  uint64_t churn_fpm;
  bool traffic_uniform;
  bool traffic_zipf;
  double traffic_zipf_param;
  unsigned lan_devices;
};

void config_usage(char **argv) {
  printf("Usage: %s\n"
         "\t [--" ARG_TOTAL_PACKETS " <#packets> (default=%lu)]\n"
         "\t [--" ARG_TOTAL_FLOWS " <#flows> (default=%lu)]\n"
         "\t [--" ARG_TOTAL_CHURN_FPM " <fpm> (default=%lu)]\n"
         "\t [--" ARG_TRAFFIC_UNIFORM " (default=%s)]\n"
         "\t [--" ARG_TRAFFIC_ZIPF " (default=%s)]\n"
         "\t [--" ARG_TRAFFIC_ZIPF_PARAM " <param> (default=%f)]\n"
         "\t [--" ARG_RANDOM_SEED " <seed>]\n"
         "\t [--" ARG_LAN_DEVICES " <lan-devs> (default=%d)]\n"
         "\t [--" ARG_HELP "]\n"
         "\n"
         "Argument descriptions:\n"
         "\t " ARG_TOTAL_PACKETS ": total number of packets to generate\n"
         "\t " ARG_TOTAL_FLOWS ": total number of flows to generate\n"
         "\t " ARG_TOTAL_CHURN_FPM ": flow churn (fpm)\n"
         "\t " ARG_RANDOM_SEED ": random seed\n"
         "\t " ARG_HELP ": show this menu\n"
         "\n",
         argv[0], DEFAULT_TOTAL_PACKETS, DEFAULT_TOTAL_FLOWS,
         DEFAULT_TOTAL_CHURN_FPM, DEFAULT_TRAFFIC_UNIFORM ? "true" : "false",
         DEFAULT_TRAFFIC_ZIPF ? "true" : "false",
         DEFAULT_TRAFFIC_ZIPF_PARAMETER, DEFAULT_LAN_DEVICES);
}

void config_print(const config_t &config) {
  printf("----- Config -----\n");
  printf("random seed: %u\n", config.random_seed);
  printf("#packets:    %lu\n", config.total_packets);
  printf("#flows:      %lu\n", config.total_flows);
  printf("churn:       %lu fpm\n", config.churn_fpm);
  if (config.traffic_uniform) {
    printf("traffic:     uniform\n");
  } else if (config.traffic_zipf) {
    printf("traffic:     zipf\n");
    printf("zipf param:  %f\n", config.traffic_zipf_param);
  }
  printf("lan devices: %d\n", config.lan_devices);
  printf("--- ---------- ---\n");
}

config_t parse_args(int argc, char **argv) {
  config_t config;

  config.random_seed = std::random_device()();
  config.total_packets = DEFAULT_TOTAL_PACKETS;
  config.total_flows = DEFAULT_TOTAL_FLOWS;
  config.churn_fpm = DEFAULT_TOTAL_CHURN_FPM;
  config.traffic_uniform = DEFAULT_TRAFFIC_UNIFORM;
  config.traffic_zipf = DEFAULT_TRAFFIC_ZIPF;
  config.traffic_zipf_param = DEFAULT_TRAFFIC_ZIPF_PARAMETER;
  config.lan_devices = DEFAULT_LAN_DEVICES;

  const char short_options[] = "";

  struct option long_options[] = {
      {ARG_TOTAL_PACKETS, required_argument, NULL, ARG_TOTAL_PACKETS_NUM},
      {ARG_TOTAL_FLOWS, required_argument, NULL, ARG_TOTAL_FLOWS_NUM},
      {ARG_TOTAL_CHURN_FPM, required_argument, NULL, ARG_TOTAL_CHURN_FPM_NUM},
      {ARG_TRAFFIC_UNIFORM, no_argument, NULL, ARG_TRAFFIC_UNIFORM_NUM},
      {ARG_TRAFFIC_ZIPF, no_argument, NULL, ARG_TRAFFIC_ZIPF_NUM},
      {ARG_TRAFFIC_ZIPF_PARAM, required_argument, NULL,
       ARG_TRAFFIC_ZIPF_PARAM_NUM},
      {ARG_RANDOM_SEED, required_argument, NULL, ARG_RANDOM_SEED_NUM},
      {ARG_LAN_DEVICES, required_argument, NULL, ARG_LAN_DEVICES_NUM},
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
    case ARG_TOTAL_FLOWS_NUM: {
      config.total_flows = parse_int(optarg, "flows", 10, '\0');
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
    case ARG_LAN_DEVICES_NUM: {
      config.lan_devices = parse_int(optarg, "lan-devs", 10, '\0');
    } break;
    default:
      PARSE_ERROR(argv, "Unknown option.\n");
    }
  }

  srand(config.random_seed);

  config_print(config);

  return config;
}

std::string get_base_pcap_fname(const config_t &config) {
  std::stringstream ss;

  ss << "fw";
  ss << "-f" << config.total_flows;
  ss << "-c" << config.churn_fpm;
  if (config.traffic_uniform) {
    ss << "-unif";
  } else if (config.traffic_zipf) {
    ss << "-zipf" << config.traffic_zipf_param;
  }

  return ss.str();
}

std::string get_warmup_pcap_fname(const config_t &config, uint16_t dev) {
  std::stringstream ss;
  ss << get_base_pcap_fname(config);
  ss << "-dev" << dev;
  ss << "-warmup.pcap";
  return ss.str();
}

std::string get_pcap_fname(const config_t &config, uint16_t dev) {
  std::stringstream ss;
  ss << get_base_pcap_fname(config);
  ss << "-dev" << dev;
  ss << ".pcap";
  return ss.str();
}

std::vector<flow_t> get_base_flows(const config_t &config) {
  std::vector<flow_t> flows;
  flows.reserve(config.total_flows);

  for (size_t i = 0; i < config.total_flows; i++) {
    flows.push_back(random_flow());
  }

  return flows;
}

enum class DevTurn {
  WAN,
  LAN,
};

class TrafficGenerator {
private:
  config_t config;
  std::vector<flow_t> flows;

  PcapWriter warmup_writer;
  PcapWriter wan_writer;
  std::vector<PcapWriter> lan_writers;

  RandomEngine uniform_rand;
  RandomZipfEngine zipf_rand;

  pcap_t *pd;
  pcap_dumper_t *pdumper;

  pkt_hdr_t packet_template;

  uint16_t current_lan_dev;
  std::unordered_map<flow_t, DevTurn, flow_hash_t> flows_dev_turn;
  std::unordered_map<flow_t, uint16_t, flow_hash_t> flows_to_lan_dev;
  std::unordered_set<flow_t, flow_hash_t> allocated_flows;
  std::unordered_map<flow_t, uint64_t, flow_hash_t> counters;
  uint64_t flows_swapped;

  time_ns_t current_time;
  time_ns_t alarm_tick;
  time_ns_t next_alarm;

public:
  TrafficGenerator(const config_t &_config,
                   const std::vector<flow_t> &_base_flows)
      : config(_config), flows(_base_flows),
        warmup_writer(get_warmup_pcap_fname(_config, 0)),
        wan_writer(get_pcap_fname(_config, 0)),
        uniform_rand(_config.random_seed, 0, _config.total_flows - 1),
        zipf_rand(_config.random_seed, _config.traffic_zipf_param,
                  _config.total_flows),
        pd(NULL), pdumper(NULL), packet_template(build_pkt_template()),
        current_lan_dev(1), counters(0), flows_swapped(0), current_time(0),
        alarm_tick(0), next_alarm(-1) {
    for (unsigned i = 0; i < config.lan_devices; i++) {
      lan_writers.emplace_back(get_pcap_fname(config, i + 1));
    }

    for (const flow_t &flow : flows) {
      flows_dev_turn[flow] = DevTurn::WAN;
      flows_to_lan_dev[invert_flow(flow)] = current_lan_dev;
      counters[flow] = 0;
      advance_lan_dev();
    }

    if (config.churn_fpm > 0) {
      alarm_tick = 60'000'000'000 / config.churn_fpm;
      next_alarm = alarm_tick;
    }
  }

  void dump_warmup() {
    uint64_t counter = 0;
    uint64_t goal = flows.size();
    int progress = -1;

    printf("Warmup: %s\n", warmup_writer.get_output_fname().c_str());

    for (const flow_t &flow : flows) {
      pkt_hdr_t pkt = packet_template;

      pkt.ip_hdr.saddr = flow.src_ip;
      pkt.ip_hdr.daddr = flow.dst_ip;
      pkt.udp_hdr.source = flow.src_port;
      pkt.udp_hdr.dest = flow.dst_port;

      allocated_flows.insert(flow);

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

    printf("Traffic: %s\n", wan_writer.get_output_fname().c_str());
    for (unsigned i = 0; i < lan_writers.size(); i++) {
      printf("Traffic: %s\n", lan_writers[i].get_output_fname().c_str());
    }

    for (uint64_t i = 0; i < config.total_packets; i++) {
      pkt_hdr_t pkt = packet_template;

      if (next_alarm >= 0 && current_time >= next_alarm) {
        uint64_t chosen_swap_flow_idx = uniform_rand.generate();
        random_swap_flow(chosen_swap_flow_idx);
        next_alarm += alarm_tick;

        const flow_t &new_flow = flows[chosen_swap_flow_idx];
        flows_dev_turn[new_flow] = DevTurn::WAN;
        flows_to_lan_dev[invert_flow(new_flow)] = current_lan_dev;
        advance_lan_dev();
        counters[new_flow] = 0;
      }

      uint64_t flow_idx = 0;
      if (config.traffic_uniform) {
        flow_idx = uniform_rand.generate();
      } else {
        flow_idx = zipf_rand.generate();
      }

      assert(flow_idx < flows.size());

      const flow_t &flow = flows[flow_idx];
      bool new_flow = allocated_flows.find(flow) == allocated_flows.end();

      if (new_flow) {
        flows_to_lan_dev[invert_flow(flow)] = current_lan_dev;
        allocated_flows.insert(flow);
      }

      if (new_flow || flows_dev_turn[flow] == DevTurn::WAN) {
        pkt.ip_hdr.saddr = flow.src_ip;
        pkt.ip_hdr.daddr = flow.dst_ip;
        pkt.udp_hdr.source = flow.src_port;
        pkt.udp_hdr.dest = flow.dst_port;

        flows_dev_turn[flow] = DevTurn::LAN;

        wan_writer.write((const u_char *)&pkt, sizeof(pkt_hdr_t), current_time);
      } else {
        flow_t inverted_flow = invert_flow(flow);

        pkt.ip_hdr.saddr = inverted_flow.src_ip;
        pkt.ip_hdr.daddr = inverted_flow.dst_ip;
        pkt.udp_hdr.source = inverted_flow.src_port;
        pkt.udp_hdr.dest = inverted_flow.dst_port;

        flows_dev_turn[flow] = DevTurn::WAN;

        uint16_t lan_dev = flows_to_lan_dev.at(inverted_flow);
        lan_writers[lan_dev - 1].write((const u_char *)&pkt, sizeof(pkt_hdr_t),
                                       current_time);
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
  void advance_lan_dev() {
    current_lan_dev = (current_lan_dev + 1) % (config.lan_devices + 1);
    if (current_lan_dev == 0) {
      current_lan_dev = 1;
    }
  }

  void report() const {
    std::vector<uint64_t> counters_values;
    counters_values.reserve(counters.size());
    for (const auto &kv : counters) {
      // Take warmup into account.
      counters_values.push_back(kv.second);
    }
    std::sort(counters_values.begin(), counters_values.end(), std::greater{});

    uint64_t total_flows = counters.size();

    uint64_t hh = 0;
    uint64_t hh_packets = 0;
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
    printf("HH: %ld flows (%.2f%%) %.2f%% volume\n", hh,
           100.0 * hh / total_flows, 100.0 * hh_packets / config.total_packets);
    printf("Top 10 flows:\n");
    for (size_t i = 0; i < config.total_flows; i++) {
      printf("  flow %ld: %ld\n", i, counters_values[i]);

      if (i >= 9) {
        break;
      }
    }
  }

  void random_swap_flow(uint64_t flow_idx) {
    assert(flow_idx < flows.size());
    flow_t new_flow = random_flow();

    counters[new_flow] = 0;
    flows[flow_idx] = new_flow;

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

    // Using 1Gbps as the rate because we go too fast the pcap timestamps can't
    // keep up with the actual time (pcap use us instead of ns).
    constexpr int rate_gbps = 1;
    constexpr int CRC = 4;
    constexpr int IPG = 20;
    constexpr int bytes = sizeof(pkt_hdr_t) + CRC + IPG;

    current_time += (bytes * 8) / rate_gbps;
  }
};

int main(int argc, char *argv[]) {
  config_t config = parse_args(argc, argv);

  std::vector<flow_t> base_flows = get_base_flows(config);
  TrafficGenerator generator(config, base_flows);

  generator.dump_warmup();
  generator.dump();

  return 0;
}