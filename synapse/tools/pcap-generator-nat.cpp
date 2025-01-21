#include <LibCore/Pcap.h>
#include <LibCore/RandomEngine.h>

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

#define NF "nat"

#define SMAC "02:00:00:ca:fe:ee"
#define DMAC "02:00:00:be:ee:ef"
#define PUBLIC_IP "1.2.3.4"

#define DEFAULT_TOTAL_PACKETS 1'000'000lu
#define DEFAULT_TOTAL_FLOWS 65'536lu
#define DEFAULT_TOTAL_CHURN_FPM 0lu
#define DEFAULT_TRAFFIC_UNIFORM true
#define DEFAULT_TRAFFIC_ZIPF false
#define DEFAULT_TRAFFIC_ZIPF_PARAMETER 1.26 // From Castan [SIGCOMM'18]
#define DEFAULT_LAN_DEVICES 1

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

struct config_t {
  u64 total_packets;
  u64 total_flows;
  u64 churn_fpm;
  bool traffic_uniform;
  bool traffic_zipf;
  double traffic_zipf_param;
  u32 lan_devices;
  u32 random_seed;
};

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

std::string get_base_pcap_fname(const config_t &config) {
  std::stringstream ss;

  ss << NF;
  ss << "-f" << config.total_flows;
  ss << "-c" << config.churn_fpm;
  if (config.traffic_uniform) {
    ss << "-unif";
  } else if (config.traffic_zipf) {
    ss << "-zipf" << config.traffic_zipf_param;
  }

  return ss.str();
}

std::string get_warmup_pcap_fname(const config_t &config, u16 dev) {
  std::stringstream ss;
  ss << get_base_pcap_fname(config);
  ss << "-dev" << dev;
  ss << "-warmup.pcap";
  return ss.str();
}

std::string get_pcap_fname(const config_t &config, u16 dev) {
  std::stringstream ss;
  ss << get_base_pcap_fname(config);
  ss << "-dev" << dev;
  ss << ".pcap";
  return ss.str();
}

std::vector<LibCore::flow_t> get_base_flows(const config_t &config) {
  std::vector<LibCore::flow_t> flows;
  flows.reserve(config.total_flows);

  for (size_t i = 0; i < config.total_flows; i++) {
    flows.push_back(random_flow());
  }

  return flows;
}

enum class Dev {
  WAN,
  LAN,
};

class PortAllocator {
private:
  std::vector<u16> available_ports;
  std::unordered_map<LibCore::flow_t, u64, LibCore::flow_t::flow_hash_t> flow_to_port;

public:
  PortAllocator() {
    u16 port = 65535;
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

  u16 get(const LibCore::flow_t &flow) { return flow_to_port.at(flow); }

  void free(const LibCore::flow_t &flow) {
    if (!flow_to_port.contains(flow)) {
      return;
    }

    u16 port = get(flow);
    available_ports.insert(available_ports.begin(), port);
    flow_to_port.erase(flow);
  }
};

class TrafficGenerator {
private:
  config_t config;
  std::vector<LibCore::flow_t> flows;

  LibCore::PcapWriter warmup_writer;
  LibCore::PcapWriter wan_writer;
  std::vector<LibCore::PcapWriter> lan_writers;

  LibCore::RandomUniformEngine uniform_rand;
  LibCore::RandomZipfEngine zipf_rand;

  pcap_t *pd;
  pcap_dumper_t *pdumper;

  pkt_hdr_t packet_template;

  u16 current_lan_dev;
  std::unordered_map<LibCore::flow_t, Dev, LibCore::flow_t::flow_hash_t> flows_dev_turn;
  std::unordered_map<LibCore::flow_t, u16, LibCore::flow_t::flow_hash_t> flows_to_lan_dev;
  std::unordered_set<LibCore::flow_t, LibCore::flow_t::flow_hash_t> allocated_flows;
  std::unordered_map<LibCore::flow_t, u64, LibCore::flow_t::flow_hash_t> counters;
  u64 flows_swapped;
  PortAllocator port_allocator;

  time_ns_t current_time;
  time_ns_t alarm_tick;
  time_ns_t next_alarm;

public:
  TrafficGenerator(const config_t &_config, const std::vector<LibCore::flow_t> &_base_flows)
      : config(_config), flows(_base_flows), warmup_writer(get_warmup_pcap_fname(_config, 0)),
        wan_writer(get_pcap_fname(_config, config.lan_devices)), uniform_rand(_config.random_seed, 0, _config.total_flows - 1),
        zipf_rand(_config.random_seed, _config.traffic_zipf_param, _config.total_flows), pd(NULL), pdumper(NULL),
        packet_template(build_pkt_template()), current_lan_dev(0), counters(0), flows_swapped(0), current_time(0), alarm_tick(0),
        next_alarm(-1) {
    // Because of the port allocator.
    assert(flows.size() <= 65535);

    for (u32 i = 0; i < config.lan_devices; i++) {
      lan_writers.emplace_back(get_pcap_fname(config, i));
    }

    for (const LibCore::flow_t &flow : flows) {
      flows_dev_turn[flow]   = Dev::LAN;
      flows_to_lan_dev[flow] = current_lan_dev;
      counters[flow]         = 0;
      advance_lan_dev();
    }

    if (config.churn_fpm > 0) {
      alarm_tick = 60'000'000'000 / config.churn_fpm;
      next_alarm = alarm_tick;
    }
  }

  void dump_warmup() {
    u64 counter  = 0;
    u64 goal     = flows.size();
    int progress = -1;

    printf("Warmup: %s\n", warmup_writer.get_output_fname().c_str());

    for (const LibCore::flow_t &flow : flows) {
      pkt_hdr_t pkt = packet_template;

      pkt.ip_hdr.src_addr  = flow.src_ip;
      pkt.ip_hdr.dst_addr  = flow.dst_ip;
      pkt.udp_hdr.src_port = flow.src_port;
      pkt.udp_hdr.dst_port = flow.dst_port;

      port_allocator.allocate(flow);
      allocated_flows.insert(flow);

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

    printf("Traffic: %s\n", wan_writer.get_output_fname().c_str());
    for (u32 i = 0; i < lan_writers.size(); i++) {
      printf("Traffic: %s\n", lan_writers[i].get_output_fname().c_str());
    }

    for (u64 i = 0; i < config.total_packets; i++) {
      pkt_hdr_t pkt = packet_template;

      if (next_alarm >= 0 && current_time >= next_alarm) {
        u64 chosen_swap_flow_idx = uniform_rand.generate();
        LibCore::flow_t old_flow = random_swap_flow(chosen_swap_flow_idx);
        next_alarm += alarm_tick;

        const LibCore::flow_t &new_flow = flows[chosen_swap_flow_idx];
        flows_dev_turn[new_flow]        = Dev::WAN;
        flows_to_lan_dev[new_flow]      = current_lan_dev;
        advance_lan_dev();
        counters[new_flow] = 0;
        allocated_flows.erase(old_flow);
        port_allocator.free(old_flow);
      }

      u64 flow_idx = config.traffic_uniform ? uniform_rand.generate() : zipf_rand.generate();
      assert(flow_idx < flows.size());

      const LibCore::flow_t &flow = flows[flow_idx];
      bool new_flow               = allocated_flows.find(flow) == allocated_flows.end();

      if (new_flow) {
        flows_to_lan_dev[flow] = current_lan_dev;
        port_allocator.allocate(flow);
        allocated_flows.insert(flow);
      }

      u16 allocated_port = port_allocator.get(flow);

      if (new_flow || flows_dev_turn[flow] == Dev::LAN) {
        pkt.ip_hdr.src_addr  = flow.src_ip;
        pkt.ip_hdr.dst_addr  = flow.dst_ip;
        pkt.udp_hdr.src_port = flow.src_port;
        pkt.udp_hdr.dst_port = flow.dst_port;

        flows_dev_turn[flow] = Dev::WAN;

        u16 lan_dev = flows_to_lan_dev.at(flow);
        lan_writers[lan_dev].write((const u8 *)&pkt, sizeof(pkt_hdr_t), sizeof(pkt_hdr_t), current_time);
      } else {
        LibCore::flow_t inverted_flow = invert_flow(flow);

        u32 public_ipv4_addr = 0;
        bool success         = LibCore::parse_ipv4addr(PUBLIC_IP, &public_ipv4_addr);
        assert(success && "Invalid IPv4 format");

        pkt.ip_hdr.src_addr  = inverted_flow.src_ip;
        pkt.udp_hdr.src_port = inverted_flow.src_port;

        // No ntohs, the original NAT doesn't care.
        pkt.ip_hdr.dst_addr  = public_ipv4_addr;
        pkt.udp_hdr.dst_port = allocated_port;

        flows_dev_turn[flow] = Dev::LAN;

        wan_writer.write((const u8 *)&pkt, sizeof(pkt_hdr_t), sizeof(pkt_hdr_t), current_time);
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
  void advance_lan_dev() { current_lan_dev = (current_lan_dev + 1) % config.lan_devices; }

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

  // Return old flow
  LibCore::flow_t random_swap_flow(u64 flow_idx) {
    assert(flow_idx < flows.size());

    LibCore::flow_t old_flow = flows[flow_idx];
    LibCore::flow_t new_flow = random_flow();

    counters[new_flow] = 0;
    flows[flow_idx]    = new_flow;

    flows_swapped++;

    return old_flow;
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
  app.add_option("--flows", config.total_flows, "Total flows.")->default_val(DEFAULT_TOTAL_FLOWS);
  app.add_option("--churn", config.churn_fpm, "Total churn (fpm).")->default_val(DEFAULT_TOTAL_CHURN_FPM);
  app.add_flag("--uniform", config.traffic_uniform, "Uniform traffic.")->default_val(DEFAULT_TRAFFIC_UNIFORM);
  app.add_flag("--zipf", config.traffic_zipf, "Zipf traffic.")->default_val(DEFAULT_TRAFFIC_ZIPF);
  app.add_option("--zipf-param", config.traffic_zipf_param, "Zipf parameter.")->default_val(DEFAULT_TRAFFIC_ZIPF_PARAMETER);
  app.add_option("--lan-devs", config.lan_devices, "LAN devices.")->default_val(DEFAULT_LAN_DEVICES);
  app.add_option("--seed", config.random_seed, "Random seed.")->default_val(std::random_device()());

  CLI11_PARSE(app, argc, argv);

  srand(config.random_seed);

  config_print(config);

  std::vector<LibCore::flow_t> base_flows = get_base_flows(config);
  TrafficGenerator generator(config, base_flows);

  generator.dump_warmup();
  generator.dump();

  return 0;
}