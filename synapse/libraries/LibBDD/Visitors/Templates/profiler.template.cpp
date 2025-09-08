#ifdef __cplusplus
extern "C" {
#endif
#include <lib/state/map.h>
#include <lib/state/vector.h>
#include <lib/state/double-chain.h>
#include <lib/state/cht.h>
#include <lib/state/cms.h>
#include <lib/state/token-bucket.h>
#include <lib/state/lpm-dir-24-8.h>

#include <lib/util/hash.h>
#include <lib/util/expirator.h>
#include <lib/util/packet-io.h>
#include <lib/util/tcpudp_hdr.h>
#include <lib/util/time.h>
#ifdef __cplusplus
}
#endif

#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>

#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_random.h>
#include <rte_hash_crc.h>

#include <pcap.h>
#include <cstdbool>
#include <unistd.h>

#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <vector>
#include <unordered_map>
#include <unordered_set>

using json = nlohmann::json;

constexpr const uint16_t DROP = ((uint16_t)-1);
constexpr const uint16_t FLOOD = ((uint16_t)-2);

constexpr const uint16_t CRC_SIZE_BYTES = 4;
constexpr const uint16_t MIN_PKT_SIZE_BYTES = 64; // With CRC
constexpr const uint16_t MAX_PKT_SIZE_BYTES = 1518; // With CRC

constexpr const char* const DEFAULT_SRC_MAC = "90:e2:ba:8e:4f:6c";
constexpr const char* const DEFAULT_DST_MAC = "90:e2:ba:8e:4f:6d";

constexpr const time_ns_t PROFILING_EXPIRATION_TIME_NS = 1'000'000'000LL; // 1 second

#define NF_INFO(text, ...)                                                                                             \
  printf(text "\n", ##__VA_ARGS__);                                                                                    \
  fflush(stdout);

#ifdef ENABLE_LOG
#define NF_DEBUG(text, ...)                                                                                            \
  fprintf(stderr, "DEBUG: " text "\n", ##__VA_ARGS__);                                                                 \
  fflush(stderr);
#else // ENABLE_LOG
#define NF_DEBUG(...)
#endif // ENABLE_LOG



#define PARSE_ERROR(argv, format, ...)                                                                                 \
  nf_config_usage(argv);                                                                                               \
  fprintf(stderr, format, ##__VA_ARGS__);                                                                              \
  exit(EXIT_FAILURE);

#define PARSER_ASSERT(cond, fmt, ...)                                                                                  \
  if (!(cond))                                                                                                         \
    rte_exit(EXIT_FAILURE, fmt, ##__VA_ARGS__);

bool nf_init(void);
int nf_process(uint16_t device, uint8_t *buffer, uint16_t packet_length, time_ns_t now);

uintmax_t nf_util_parse_int(const char *str, const char *name, int base, char next) {
  char *temp;
  intmax_t result = strtoimax(str, &temp, base);

  // There's also a weird failure case with overflows, but let's not care
  if (temp == str || *temp != next) {
    rte_exit(EXIT_FAILURE, "Error while parsing '%s': %s\n", name, str);
  }

  return result;
}

bool nf_parse_etheraddr(const char *str, struct rte_ether_addr *addr) {
  return sscanf(str, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX", addr->addr_bytes + 0, addr->addr_bytes + 1,
                addr->addr_bytes + 2, addr->addr_bytes + 3, addr->addr_bytes + 4, addr->addr_bytes + 5) == 6;
}

struct pkt_t {
  uint8_t data[MAX_PKT_SIZE_BYTES];
  uint32_t len;
  time_ns_t ts;
};

struct dev_pcap_t {
  uint16_t device;
  std::filesystem::path pcap;
  bool warmup;
};

struct config_t {
  std::filesystem::path report_fname;
  std::vector<dev_pcap_t> pcaps;
} config;

struct pcap_data_t {
  const uint8_t *data;
  const struct pcap_pkthdr *header;
};

struct next_packet_t {
  uint16_t device;
  pkt_t pkt;
};

struct pcap_info_t {
  pcap_t* pcap;
  bool assume_ip;
  long start_offset;
  uint64_t total_packets;
  uint64_t total_bytes;
  pkt_t first_packet;
  std::unordered_set<uint16_t> devices;
};

class PcapReader {
private:
  std::unordered_map<std::string, pcap_t*> fname_to_pcap;
  std::unordered_map<pcap_t*, pcap_info_t> pcap_infos;
  std::map<pcap_t *, pkt_t> pending_pkts_per_pcap;
  int64_t last_ts;
  
  // Meta
  uint64_t total_packets;
  uint64_t total_bytes;
  uint64_t processed_packets;
  uint64_t processed_bytes;
  int last_percentage_report;

public:
  PcapReader() {}

  uint64_t get_processed_packets() { return processed_packets; }
  uint64_t get_processed_bytes() { return processed_bytes; }

  void setup(const std::vector<dev_pcap_t> &_pcaps) {
    last_ts                = -1;
    total_packets          = 0;
    total_bytes            = 0;
    processed_packets      = 0;
    last_percentage_report = -1;

    for (const auto &dev_pcap : _pcaps) {
      auto fname_to_pcap_it = fname_to_pcap.find(dev_pcap.pcap.string());
      if (fname_to_pcap_it != fname_to_pcap.end()) {
        pcap_t* pcap = fname_to_pcap_it->second;
        pcap_infos[pcap].devices.insert(dev_pcap.device);
        continue;
      }

      char errbuf[PCAP_ERRBUF_SIZE];
      pcap_t* pcap = pcap_open_offline(dev_pcap.pcap.c_str(), errbuf);

      fname_to_pcap[dev_pcap.pcap.string()] = pcap;
      pcap_infos[pcap] = pcap_info_t();

      pcap_info_t &pcap_info = pcap_infos.at(pcap);

      pcap_info.pcap = pcap;
      pcap_info.devices.insert(dev_pcap.device);

      if (pcap_info.pcap == NULL) {
        rte_exit(EXIT_FAILURE, "pcap_open_offline() failed: %s\n", errbuf);
      }

      int link_hdr_type = pcap_datalink(pcap_info.pcap);

      switch (link_hdr_type) {
      case DLT_EN10MB:
        // Normal ethernet, as expected.
        pcap_info.assume_ip = false;
        break;
      case DLT_RAW:
        // Contains raw IP packets.
        pcap_info.assume_ip = true;
        break;
      default: {
        fprintf(stderr, "Unknown header type (%d)", link_hdr_type);
        exit(1);
      }
      }

      FILE *pcap_fptr = pcap_file(pcap_info.pcap);
      assert(pcap_fptr && "Invalid pcap file pointer");
      pcap_info.start_offset = ftell(pcap_fptr);

      pcap_info.total_packets = 0;
      pcap_info.total_bytes   = 0;

      pkt_t pkt;
      while (read(pcap_info.pcap, pkt)) {
        if (pcap_info.total_packets == 0) {
          pcap_info.first_packet = pkt;
        }

        pcap_info.total_packets++;
        pcap_info.total_bytes += pkt.len + CRC_SIZE_BYTES;
      }
      
      total_packets += pcap_info.total_packets;
      total_bytes += pcap_info.total_bytes;

      pending_pkts_per_pcap[pcap_info.pcap] = pcap_info.first_packet;
    }
  }

  std::vector<next_packet_t> get_next_packets() {
    int64_t ts = -1;
    for (const auto& [pending_pcap, pending_pkt] : pending_pkts_per_pcap) {
      if (ts == -1 || pending_pkt.ts < ts) {
        ts = pending_pkt.ts;
      }
    }

    if (ts == -1) {
      return {};
    }

    pcap_t* chosen_pcap = nullptr;
    std::vector<next_packet_t> next_packets;
    for (const auto& [pending_pcap, pending_pkt] : pending_pkts_per_pcap) {
      if (pending_pkt.ts != ts) {
        continue;
      }

      for (uint16_t dev : pcap_infos[pending_pcap].devices) {
        next_packet_t next_pkt = {
          .device = dev,
          .pkt = pending_pkt
        };
        next_packets.push_back(next_pkt);

        processed_packets += 1;
        processed_bytes += pending_pkt.len + CRC_SIZE_BYTES;
      }

      chosen_pcap = pending_pcap;
      break;
    }

    last_ts = ts;

    show_progress();

    pkt_t new_pkt;
    if (read(chosen_pcap, new_pkt)) {
      pending_pkts_per_pcap[chosen_pcap] = new_pkt;
    } else {
      pending_pkts_per_pcap.erase(chosen_pcap);
    }

    return next_packets;
  }

private:
  bool read(pcap_t* pcap, pkt_t &pkt) {
    const uint8_t *data;
    struct pcap_pkthdr *hdr;

    if (pcap_next_ex(pcap, &hdr, &data) != 1) {
      rewind(pcap);
      return false;
    }

    uint8_t *pkt_data = pkt.data;

    pkt.len = hdr->len;

    if (pcap_infos.at(pcap).assume_ip) {
      struct rte_ether_hdr *eth_hdr = (struct rte_ether_hdr *)pkt_data;
      nf_parse_etheraddr(DEFAULT_DST_MAC, &eth_hdr->dst_addr);
      nf_parse_etheraddr(DEFAULT_SRC_MAC, &eth_hdr->src_addr);
      eth_hdr->ether_type = rte_bswap16(RTE_ETHER_TYPE_IPV4);
      pkt_data += sizeof(struct rte_ether_hdr);
      pkt.len += sizeof(struct rte_ether_hdr);
    }

    memcpy(pkt_data, data, hdr->caplen);
    pkt.ts  = hdr->ts.tv_sec * 1e9 + hdr->ts.tv_usec * 1e3;

    return true;
  }

  // WARNING: this does not work on windows!
  // https://winpcap-users.winpcap.narkive.com/scCKD3x2/packet-random-access-using-file-seek
  void rewind(pcap_t* pcap) {
    long pcap_start = pcap_infos.at(pcap).start_offset;
    FILE *pcap_fptr = pcap_file(pcap);
    fseek(pcap_fptr, pcap_start, SEEK_SET);
  }

  void show_progress() {
    int progress = 100.0 * processed_packets / total_packets;

    if (progress <= last_percentage_report) {
      return;
    }

    last_percentage_report = progress;
    printf("\r[Progress %3d%%]", progress);
    if (progress == 100)
      printf("\n");
    fflush(stdout);
  }
};

void nf_log_pkt(time_ns_t time, uint16_t device, uint8_t *packet, uint16_t packet_length) {
  struct rte_ether_hdr *rte_ether_header = (struct rte_ether_hdr *)(packet);
  struct rte_ipv4_hdr *rte_ipv4_header   = (struct rte_ipv4_hdr *)(packet + sizeof(struct rte_ether_hdr));
  struct tcpudp_hdr *tcpudp_header =
      (struct tcpudp_hdr *)(packet + sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));

  NF_DEBUG("[%lu:%u] %u.%u.%u.%u:%u -> %u.%u.%u.%u:%u", time, device, (rte_ipv4_header->src_addr >> 0) & 0xff,
           (rte_ipv4_header->src_addr >> 8) & 0xff, (rte_ipv4_header->src_addr >> 16) & 0xff,
           (rte_ipv4_header->src_addr >> 24) & 0xff, rte_bswap16(tcpudp_header->src_port),
           (rte_ipv4_header->dst_addr >> 0) & 0xff, (rte_ipv4_header->dst_addr >> 8) & 0xff,
           (rte_ipv4_header->dst_addr >> 16) & 0xff, (rte_ipv4_header->dst_addr >> 24) & 0xff,
           rte_bswap16(tcpudp_header->dst_port));
}

void nf_config_usage(char **argv) {
  NF_INFO("Usage: %s <JSON output filename> [[--warmup] dev0:pcap0] "
          "[[--warmup] dev1:pcap1] ...\n",
          argv[0]);
}

void nf_config_print(void) {
  NF_INFO("----- Config -----");
  NF_INFO("report: %s", config.report_fname.c_str());
  for (const auto &dev_pcap : config.pcaps) {
    NF_INFO("device: %u | pcap: %s | warmup: %s", dev_pcap.device, dev_pcap.pcap.filename().c_str(),
            dev_pcap.warmup ? "yes" : "no");
  }
  NF_INFO("--- ---------- ---");
}

void nf_config_init(int argc, char **argv) {
  if (argc < 3) {
    PARSE_ERROR(argv, "Insufficient arguments.\n");
  }

  config.report_fname = argv[1];

  bool incoming_warmup = false;

  // split the arguments into device and pcap pairs joined by a :
  for (int i = 2; i < argc; i++) {
    char *arg = argv[i];

    if (strcmp(arg, "--warmup") == 0) {
      incoming_warmup = true;
      continue;
    }

    char *device_str = strtok(arg, ":");
    char *pcap_str   = strtok(NULL, ":");

    if (!device_str || !pcap_str) {
      PARSE_ERROR(argv, "Invalid argument format: %s\n", arg);
    }

    dev_pcap_t dev_pcap;
    dev_pcap.device = nf_util_parse_int(device_str, "device", 10, '\0');
    dev_pcap.pcap   = pcap_str;
    dev_pcap.warmup = incoming_warmup;

    config.pcaps.push_back(dev_pcap);

    incoming_warmup = false;
  }

  nf_config_print();
}

bool warmup;

int profiler_expire_items_single_map(struct DoubleChain *dchain, struct Vector *vector, struct Map *map, time_ns_t time)  {
  if (!warmup)
    return expire_items_single_map(dchain, vector, map, time - PROFILING_EXPIRATION_TIME_NS);
  return 0;
}

struct Stats {
  struct key_t {
    uint8_t *data;
    uint32_t len;

    key_t(const uint8_t *_data, uint32_t _len) : len(_len) {
      data = new uint8_t[len];
      memcpy(data, _data, len);
    }

    key_t(const key_t &other) : len(other.len) {
      data = new uint8_t[len];
      memcpy(data, other.data, len);
    }

    bool operator==(const key_t &other) const { return len == other.len && memcmp(data, other.data, len) == 0; }

    ~key_t() { delete[] data; }
  };

  struct KeyHasher {
    std::size_t operator()(const key_t &key) const { return hash_obj((void *)key.data, key.len); }
  };

  std::unordered_map<key_t, uint64_t, KeyHasher> key_counter;
  std::unordered_map<uint32_t, std::unordered_set<uint32_t>> mask_to_crc32;
  uint64_t total_count;

  Stats() : total_count(0) {
    uint32_t mask = 0;
    while (1) {
      mask                = (mask << 1) | 1;
      mask_to_crc32[mask] = {};
      if (mask == 0xffffffff) {
        break;
      }
    }
  }

  void update(const void *key, uint32_t len) {
    key_t k((uint8_t *)key, len);
    key_counter[k]++;
    total_count++;

    uint32_t crc32 = rte_hash_crc(k.data, k.len, 0xffffffff);
    for (auto &[mask, hashes] : mask_to_crc32) {
      hashes.insert(crc32 & mask);
    }
  }
};

struct MapStats {
  struct epoch_t {
    Stats stats;
    time_ns_t start;
    time_ns_t end;
    bool warmup;

    epoch_t(time_ns_t _start, bool _warmup) : start(_start), end(-1), warmup(_warmup) {}
  };

  std::unordered_map<int, Stats> stats_per_node;
  std::vector<epoch_t> epochs;
  time_ns_t epoch_duration;

  MapStats() : epoch_duration(PROFILING_EXPIRATION_TIME_NS) {}

  void init(int op) { stats_per_node.insert({op, Stats()}); }

  void update(int op, const void *key, uint32_t len, time_ns_t now) {
    if (epochs.empty() || (epochs.back().warmup && !warmup) || now - epochs.back().start > epoch_duration) {
      epochs.emplace_back(now, warmup);
    }

    stats_per_node.at(op).update(key, len);
    epochs.back().stats.update(key, len);
    epochs.back().end = now;
  }
};

std::vector<uint16_t> ports;

struct PortStats {
  std::unordered_map<uint16_t, uint64_t> counters_per_port;
  uint64_t drop_counter;
  uint64_t flood_counter;

  PortStats() : drop_counter(0), flood_counter(0) {
    for (uint16_t port : ports) {
      counters_per_port[port] = 0;
    }
  }

  void inc_fwd(uint16_t port) {
    if (!warmup) {
      auto found_it = counters_per_port.find(port);
      if (found_it == counters_per_port.end()) {
        counters_per_port[port] = 1;
      } else {
        found_it->second++;
      }
    }
  }

  void inc_drop() {
    if (!warmup) {
      drop_counter++;
    }
  }

  void inc_flood() {
    if (!warmup) {
      flood_counter++;
    }
  }
};

struct expiration_tracker_t {
  struct epoch_t {
    time_ns_t start;
    time_ns_t end;
    bool warmup;
    uint64_t expirations;
  
    epoch_t(time_ns_t _start, bool _warmup) : start(_start), end(-1), warmup(_warmup), expirations(0) {}
  };

  std::vector<epoch_t> epochs;

  void update(uint64_t expirations, time_ns_t now) {
    if (epochs.empty() || (epochs.back().warmup && !warmup) || now - epochs.back().start > PROFILING_EXPIRATION_TIME_NS) {
      epochs.emplace_back(now, warmup);
    }

    if (!warmup) {
      epochs.back().expirations += expirations;
    }

    if (!epochs.empty()) {
      epochs.back().end = now;
    }
  }
};

PcapReader warmup_reader;
PcapReader reader;
std::unordered_map<int, MapStats> stats_per_map;
std::unordered_map<int, PortStats> forwarding_stats_per_route_op;
std::unordered_map<uint64_t, uint64_t> node_pkt_counter;
time_ns_t elapsed_time;
expiration_tracker_t expiration_tracker;

void inc_path_counter(int i) {
  if (warmup) {
    return;
  }

  node_pkt_counter[i]++;
}

void generate_report() {
  json report;

  report["config"]          = json::object();
  report["config"]["pcaps"] = json::array();
  for (const auto &dev_pcap : config.pcaps) {
    json dev_pcap_elem;
    dev_pcap_elem["device"] = dev_pcap.device;
    dev_pcap_elem["pcap"]   = dev_pcap.pcap.filename().stem().string();
    dev_pcap_elem["warmup"] = dev_pcap.warmup;
    report["config"]["pcaps"].push_back(dev_pcap_elem);
  }

  report["forwarding_stats"] = json::object();
  for (const auto&[route_op, port_stats] : forwarding_stats_per_route_op) {
    report["forwarding_stats"][std::to_string(route_op)] = json::object();
    report["forwarding_stats"][std::to_string(route_op)]["drop"]  = port_stats.drop_counter;
    report["forwarding_stats"][std::to_string(route_op)]["flood"] = port_stats.flood_counter;
    report["forwarding_stats"][std::to_string(route_op)]["ports"] = json::object();
    for (const auto&[port, count] : port_stats.counters_per_port) {
      report["forwarding_stats"][std::to_string(route_op)]["ports"][std::to_string(port)] = count;
    }
  }

  report["counters"] = json::object();
  for (const auto& [node_id, count] : node_pkt_counter) {
    report["counters"][std::to_string(node_id)] = count;
  }

  report["meta"]            = json::object();
  report["meta"]["elapsed"] = elapsed_time;
  report["meta"]["pkts"]    = reader.get_processed_packets();
  report["meta"]["bytes"]   = reader.get_processed_bytes();
  
  report["expirations_per_epoch"] = json::array();
  for (const auto &epoch : expiration_tracker.epochs) {
    report["expirations_per_epoch"].push_back(epoch.expirations);
  }

  report["stats_per_map"] = json::object();

  for (const auto &[map, map_stats] : stats_per_map) {
    json map_stats_json;

    map_stats_json["nodes"] = json::array();
    for (const auto &[map_op, stats] : map_stats.stats_per_node) {
      json map_op_stats_json;
      map_op_stats_json["node"]          = map_op;
      map_op_stats_json["pkts_per_flow"] = json::array();
      map_op_stats_json["flows"]         = stats.key_counter.size();

      map_op_stats_json["crc32_hashes_per_mask"] = json::object();
      for (const auto &[mask, crc32_hashes] : stats.mask_to_crc32) {
        map_op_stats_json["crc32_hashes_per_mask"][std::to_string(mask)] = crc32_hashes.size();
      }

      auto build_pkts_per_flow = [&stats] {
        auto pkts_per_flow = json::array();
        std::vector<uint64_t> ppf;
        for (const auto &map_key_stats : stats.key_counter) {
          ppf.push_back(map_key_stats.second);
        }
        std::sort(ppf.begin(), ppf.end(), std::greater<>());
        for (uint64_t packets : ppf) {
          pkts_per_flow.push_back(packets);
        }
        return pkts_per_flow;
      };

      map_op_stats_json["pkts_per_flow"] = build_pkts_per_flow();
      map_op_stats_json["pkts"]          = stats.total_count;

      map_stats_json["nodes"].push_back(map_op_stats_json);
    }

    map_stats_json["epochs"] = json::array();
    for (size_t i = 0; i < map_stats.epochs.size(); i++) {
      const auto &epoch = map_stats.epochs[i];

      json epoch_json;
      epoch_json["dt_ns"]                    = epoch.end - epoch.start;
      epoch_json["warmup"]                   = epoch.warmup;
      epoch_json["pkts"]                     = epoch.stats.total_count;
      epoch_json["flows"]                    = epoch.stats.key_counter.size();
      epoch_json["pkts_per_persistent_flow"] = json::array();
      epoch_json["pkts_per_new_flow"]        = json::array();

      std::vector<uint64_t> pf;
      std::vector<uint64_t> nf;
      for (const auto &[key, pkts] : epoch.stats.key_counter) {
        if (i == 0 ||
            (map_stats.epochs[i - 1].stats.key_counter.find(key) == map_stats.epochs[i - 1].stats.key_counter.end())) {
          nf.push_back(pkts);
        } else {
          pf.push_back(pkts);
        }
      }
      std::sort(pf.begin(), pf.end(), std::greater<>());
      std::sort(nf.begin(), nf.end(), std::greater<>());

      for (uint64_t packets : pf) {
        epoch_json["pkts_per_persistent_flow"].push_back(packets);
      }

      for (uint64_t packets : nf) {
        epoch_json["pkts_per_new_flow"].push_back(packets);
      }

      map_stats_json["epochs"].push_back(epoch_json);
    }

    report["stats_per_map"][std::to_string(map)] = map_stats_json;
  }

  if (config.report_fname.has_parent_path() && !std::filesystem::exists(config.report_fname.parent_path())) {
    std::filesystem::create_directories(config.report_fname.parent_path());
  }

  std::ofstream os = std::ofstream(config.report_fname);
  os << report.dump(2);
  os.flush();
  os.close();

  NF_INFO("Generated report %s", config.report_fname.c_str());
}

// Main worker method (for now used on a single thread...)
static void worker_main() {
  if (!nf_init()) {
    rte_exit(EXIT_FAILURE, "Error initializing NF");
  }

  std::vector<dev_pcap_t> warmup_pcaps;
  std::vector<dev_pcap_t> pcaps;

  for (const auto &dev_pcap : config.pcaps) {
    if (dev_pcap.warmup) {
      warmup_pcaps.push_back(dev_pcap);
    } else {
      pcaps.push_back(dev_pcap);
    }
  }

  puts("Setting up pcap readers...");

  warmup_reader.setup(warmup_pcaps);
  reader.setup(pcaps);

  puts("Processing warmup packets...");

  // First process warmup packets
  warmup = true;
  std::vector<next_packet_t> next_pkts;
  while (!(next_pkts = warmup_reader.get_next_packets()).empty()) {
    for (next_packet_t& next_pkt : next_pkts) {
      nf_process(next_pkt.device, next_pkt.pkt.data, next_pkt.pkt.len, next_pkt.pkt.ts);
    }
  }
  warmup = false;

  puts("Processing NF packets...");

  // Generate the first packet manually to record the starting time
  next_pkts = reader.get_next_packets();
  assert(!next_pkts.empty() && "Failed to generate the first packet");

  time_ns_t first_pkt_time = next_pkts.front().pkt.ts;
  time_ns_t start_time = first_pkt_time;
  time_ns_t last_time  = first_pkt_time;

  while (!next_pkts.empty()) {
    // Ignore destination device, we don't forward anywhere
    for (next_packet_t& next_pkt : next_pkts) {
      nf_process(next_pkt.device, next_pkt.pkt.data, next_pkt.pkt.len, next_pkt.pkt.ts);
    }
    
    elapsed_time += next_pkts.back().pkt.ts - last_time;
    last_time = next_pkts.back().pkt.ts;

    next_pkts = reader.get_next_packets();
  }

  NF_INFO("Elapsed virtual time: %lf s", (double)elapsed_time / 1e9);
}

int main(int argc, char **argv) {
  nf_config_init(argc, argv);
  worker_main();
  generate_report();
  return 0;
}

/*@{NF_STATE}@*/

/*@{NF_INIT}@*/

/*@{NF_PROCESS}@*/