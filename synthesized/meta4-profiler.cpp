#ifdef __cplusplus
extern "C" {
#endif
#include <lib/state/map.h>
#include <lib/state/vector.h>
#include <lib/state/double-chain.h>
#include <lib/state/cht.h>
#include <lib/state/cms.h>
#include <lib/state/bloom-filter.h>
#include <lib/state/token-bucket.h>
#include <lib/state/lpm.h>

#include <lib/util/math.h>
#include <lib/util/expirator.h>
#include <lib/util/packet-io.h>
#include <lib/util/dns_hdr.h>
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
#include <set>
#include <utility>

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

struct LnStats {
  std::set<std::pair<uint32_t, uint32_t>> inputs; // distinct (x, scale) pairs

  void update(uint32_t x, uint32_t scale) {
    if (!warmup) {
      inputs.insert({x, scale});
    }
  }
};

PcapReader warmup_reader;
PcapReader reader;
std::unordered_map<int, MapStats> stats_per_map;
std::unordered_map<int, PortStats> forwarding_stats_per_route_op;
std::unordered_map<uint64_t, uint64_t> node_pkt_counter;
std::unordered_map<int, LnStats> ln_stats_per_node;
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

  report["ln_inputs"] = json::object();
  for (const auto &[node_id, ln_stats] : ln_stats_per_node) {
    json entries = json::array();
    for (const auto &[x, scale] : ln_stats.inputs) {
      json entry;
      entry["x"]     = x;
      entry["scale"] = scale;
      entries.push_back(entry);
    }
    report["ln_inputs"][std::to_string(node_id)] = entries;
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

struct LPM *lpm;
struct LPM *lpm2;
struct Map *map;
struct Vector *vector;
struct Vector *vector2;
struct DoubleChain *dchain;
struct Vector *vector3;
struct Vector *vector4;
struct Vector *vector5;
struct Vector *vector6;


bool nf_init() {
  int lpm_alloc_success = lpm_allocate(2048, 65, &lpm);
  if (!lpm_alloc_success) {
    return false;
  }
  int lpm_alloc_success2 = lpm_allocate(2048, 4, &lpm2);
  if (!lpm_alloc_success2) {
    return false;
  }
  int map_allocation_succeeded = map_allocate(65536, 8, &map);
  if (!map_allocation_succeeded) {
    return false;
  }
  int vector_alloc_success = vector_allocate(8, 65536, &vector);
  if (!vector_alloc_success) {
    return false;
  }
  int vector_alloc_success2 = vector_allocate(4, 65536, &vector2);
  if (!vector_alloc_success2) {
    return false;
  }
  int is_dchain_allocated = dchain_allocate(65536, &dchain);
  if (!is_dchain_allocated) {
    return false;
  }
  int vector_alloc_success3 = vector_allocate(4, 2048, &vector3);
  if (!vector_alloc_success3) {
    return false;
  }
  int vector_alloc_success4 = vector_allocate(4, 2048, &vector4);
  if (!vector_alloc_success4) {
    return false;
  }
  int vector_alloc_success5 = vector_allocate(4, 2048, &vector5);
  if (!vector_alloc_success5) {
    return false;
  }
  int vector_alloc_success6 = vector_allocate(4, 2048, &vector6);
  if (!vector_alloc_success6) {
    return false;
  }
  uint8_t lpm_prefix[65];
  lpm_prefix[0] = 3;
  lpm_prefix[1] = 3;
  lpm_prefix[2] = 99;
  lpm_prefix[3] = 111;
  lpm_prefix[4] = 109;
  lpm_prefix[5] = 0;
  lpm_prefix[6] = 0;
  lpm_prefix[7] = 0;
  lpm_prefix[8] = 0;
  lpm_prefix[9] = 0;
  lpm_prefix[10] = 0;
  lpm_prefix[11] = 0;
  lpm_prefix[12] = 0;
  lpm_prefix[13] = 0;
  lpm_prefix[14] = 0;
  lpm_prefix[15] = 0;
  lpm_prefix[16] = 0;
  lpm_prefix[17] = 6;
  lpm_prefix[18] = 103;
  lpm_prefix[19] = 111;
  lpm_prefix[20] = 111;
  lpm_prefix[21] = 103;
  lpm_prefix[22] = 108;
  lpm_prefix[23] = 101;
  lpm_prefix[24] = 0;
  lpm_prefix[25] = 0;
  lpm_prefix[26] = 0;
  lpm_prefix[27] = 0;
  lpm_prefix[28] = 0;
  lpm_prefix[29] = 0;
  lpm_prefix[30] = 0;
  lpm_prefix[31] = 0;
  lpm_prefix[32] = 0;
  lpm_prefix[33] = 0;
  lpm_prefix[34] = 0;
  lpm_prefix[35] = 0;
  lpm_prefix[36] = 0;
  lpm_prefix[37] = 0;
  lpm_prefix[38] = 0;
  lpm_prefix[39] = 0;
  lpm_prefix[40] = 0;
  lpm_prefix[41] = 0;
  lpm_prefix[42] = 0;
  lpm_prefix[43] = 0;
  lpm_prefix[44] = 0;
  lpm_prefix[45] = 0;
  lpm_prefix[46] = 0;
  lpm_prefix[47] = 0;
  lpm_prefix[48] = 0;
  lpm_prefix[49] = 0;
  lpm_prefix[50] = 0;
  lpm_prefix[51] = 0;
  lpm_prefix[52] = 0;
  lpm_prefix[53] = 0;
  lpm_prefix[54] = 0;
  lpm_prefix[55] = 0;
  lpm_prefix[56] = 0;
  lpm_prefix[57] = 0;
  lpm_prefix[58] = 0;
  lpm_prefix[59] = 0;
  lpm_prefix[60] = 0;
  lpm_prefix[61] = 0;
  lpm_prefix[62] = 0;
  lpm_prefix[63] = 0;
  lpm_prefix[64] = 0;
  int lpm_update_elem_success = lpm_update(lpm, lpm_prefix, 264, 0);
  if (!lpm_update_elem_success) {
    return false;
  }
  uint8_t lpm_prefix2[65];
  lpm_prefix2[0] = 3;
  lpm_prefix2[1] = 3;
  lpm_prefix2[2] = 99;
  lpm_prefix2[3] = 111;
  lpm_prefix2[4] = 109;
  lpm_prefix2[5] = 0;
  lpm_prefix2[6] = 0;
  lpm_prefix2[7] = 0;
  lpm_prefix2[8] = 0;
  lpm_prefix2[9] = 0;
  lpm_prefix2[10] = 0;
  lpm_prefix2[11] = 0;
  lpm_prefix2[12] = 0;
  lpm_prefix2[13] = 0;
  lpm_prefix2[14] = 0;
  lpm_prefix2[15] = 0;
  lpm_prefix2[16] = 0;
  lpm_prefix2[17] = 8;
  lpm_prefix2[18] = 102;
  lpm_prefix2[19] = 97;
  lpm_prefix2[20] = 99;
  lpm_prefix2[21] = 101;
  lpm_prefix2[22] = 98;
  lpm_prefix2[23] = 111;
  lpm_prefix2[24] = 111;
  lpm_prefix2[25] = 107;
  lpm_prefix2[26] = 0;
  lpm_prefix2[27] = 0;
  lpm_prefix2[28] = 0;
  lpm_prefix2[29] = 0;
  lpm_prefix2[30] = 0;
  lpm_prefix2[31] = 0;
  lpm_prefix2[32] = 0;
  lpm_prefix2[33] = 0;
  lpm_prefix2[34] = 0;
  lpm_prefix2[35] = 0;
  lpm_prefix2[36] = 0;
  lpm_prefix2[37] = 0;
  lpm_prefix2[38] = 0;
  lpm_prefix2[39] = 0;
  lpm_prefix2[40] = 0;
  lpm_prefix2[41] = 0;
  lpm_prefix2[42] = 0;
  lpm_prefix2[43] = 0;
  lpm_prefix2[44] = 0;
  lpm_prefix2[45] = 0;
  lpm_prefix2[46] = 0;
  lpm_prefix2[47] = 0;
  lpm_prefix2[48] = 0;
  lpm_prefix2[49] = 0;
  lpm_prefix2[50] = 0;
  lpm_prefix2[51] = 0;
  lpm_prefix2[52] = 0;
  lpm_prefix2[53] = 0;
  lpm_prefix2[54] = 0;
  lpm_prefix2[55] = 0;
  lpm_prefix2[56] = 0;
  lpm_prefix2[57] = 0;
  lpm_prefix2[58] = 0;
  lpm_prefix2[59] = 0;
  lpm_prefix2[60] = 0;
  lpm_prefix2[61] = 0;
  lpm_prefix2[62] = 0;
  lpm_prefix2[63] = 0;
  lpm_prefix2[64] = 0;
  int lpm_update_elem_success2 = lpm_update(lpm, lpm_prefix2, 264, 1);
  if (!lpm_update_elem_success2) {
    return false;
  }
  uint8_t lpm_prefix3[65];
  lpm_prefix3[0] = 3;
  lpm_prefix3[1] = 3;
  lpm_prefix3[2] = 99;
  lpm_prefix3[3] = 111;
  lpm_prefix3[4] = 109;
  lpm_prefix3[5] = 0;
  lpm_prefix3[6] = 0;
  lpm_prefix3[7] = 0;
  lpm_prefix3[8] = 0;
  lpm_prefix3[9] = 0;
  lpm_prefix3[10] = 0;
  lpm_prefix3[11] = 0;
  lpm_prefix3[12] = 0;
  lpm_prefix3[13] = 0;
  lpm_prefix3[14] = 0;
  lpm_prefix3[15] = 0;
  lpm_prefix3[16] = 0;
  lpm_prefix3[17] = 5;
  lpm_prefix3[18] = 97;
  lpm_prefix3[19] = 112;
  lpm_prefix3[20] = 112;
  lpm_prefix3[21] = 108;
  lpm_prefix3[22] = 101;
  lpm_prefix3[23] = 0;
  lpm_prefix3[24] = 0;
  lpm_prefix3[25] = 0;
  lpm_prefix3[26] = 0;
  lpm_prefix3[27] = 0;
  lpm_prefix3[28] = 0;
  lpm_prefix3[29] = 0;
  lpm_prefix3[30] = 0;
  lpm_prefix3[31] = 0;
  lpm_prefix3[32] = 0;
  lpm_prefix3[33] = 0;
  lpm_prefix3[34] = 0;
  lpm_prefix3[35] = 0;
  lpm_prefix3[36] = 0;
  lpm_prefix3[37] = 0;
  lpm_prefix3[38] = 0;
  lpm_prefix3[39] = 0;
  lpm_prefix3[40] = 0;
  lpm_prefix3[41] = 0;
  lpm_prefix3[42] = 0;
  lpm_prefix3[43] = 0;
  lpm_prefix3[44] = 0;
  lpm_prefix3[45] = 0;
  lpm_prefix3[46] = 0;
  lpm_prefix3[47] = 0;
  lpm_prefix3[48] = 0;
  lpm_prefix3[49] = 0;
  lpm_prefix3[50] = 0;
  lpm_prefix3[51] = 0;
  lpm_prefix3[52] = 0;
  lpm_prefix3[53] = 0;
  lpm_prefix3[54] = 0;
  lpm_prefix3[55] = 0;
  lpm_prefix3[56] = 0;
  lpm_prefix3[57] = 0;
  lpm_prefix3[58] = 0;
  lpm_prefix3[59] = 0;
  lpm_prefix3[60] = 0;
  lpm_prefix3[61] = 0;
  lpm_prefix3[62] = 0;
  lpm_prefix3[63] = 0;
  lpm_prefix3[64] = 0;
  int lpm_update_elem_success3 = lpm_update(lpm, lpm_prefix3, 264, 2);
  if (!lpm_update_elem_success3) {
    return false;
  }
  uint8_t lpm_prefix4[65];
  lpm_prefix4[0] = 3;
  lpm_prefix4[1] = 3;
  lpm_prefix4[2] = 99;
  lpm_prefix4[3] = 111;
  lpm_prefix4[4] = 109;
  lpm_prefix4[5] = 0;
  lpm_prefix4[6] = 0;
  lpm_prefix4[7] = 0;
  lpm_prefix4[8] = 0;
  lpm_prefix4[9] = 0;
  lpm_prefix4[10] = 0;
  lpm_prefix4[11] = 0;
  lpm_prefix4[12] = 0;
  lpm_prefix4[13] = 0;
  lpm_prefix4[14] = 0;
  lpm_prefix4[15] = 0;
  lpm_prefix4[16] = 0;
  lpm_prefix4[17] = 9;
  lpm_prefix4[18] = 109;
  lpm_prefix4[19] = 105;
  lpm_prefix4[20] = 99;
  lpm_prefix4[21] = 114;
  lpm_prefix4[22] = 111;
  lpm_prefix4[23] = 115;
  lpm_prefix4[24] = 111;
  lpm_prefix4[25] = 102;
  lpm_prefix4[26] = 116;
  lpm_prefix4[27] = 0;
  lpm_prefix4[28] = 0;
  lpm_prefix4[29] = 0;
  lpm_prefix4[30] = 0;
  lpm_prefix4[31] = 0;
  lpm_prefix4[32] = 0;
  lpm_prefix4[33] = 0;
  lpm_prefix4[34] = 0;
  lpm_prefix4[35] = 0;
  lpm_prefix4[36] = 0;
  lpm_prefix4[37] = 0;
  lpm_prefix4[38] = 0;
  lpm_prefix4[39] = 0;
  lpm_prefix4[40] = 0;
  lpm_prefix4[41] = 0;
  lpm_prefix4[42] = 0;
  lpm_prefix4[43] = 0;
  lpm_prefix4[44] = 0;
  lpm_prefix4[45] = 0;
  lpm_prefix4[46] = 0;
  lpm_prefix4[47] = 0;
  lpm_prefix4[48] = 0;
  lpm_prefix4[49] = 0;
  lpm_prefix4[50] = 0;
  lpm_prefix4[51] = 0;
  lpm_prefix4[52] = 0;
  lpm_prefix4[53] = 0;
  lpm_prefix4[54] = 0;
  lpm_prefix4[55] = 0;
  lpm_prefix4[56] = 0;
  lpm_prefix4[57] = 0;
  lpm_prefix4[58] = 0;
  lpm_prefix4[59] = 0;
  lpm_prefix4[60] = 0;
  lpm_prefix4[61] = 0;
  lpm_prefix4[62] = 0;
  lpm_prefix4[63] = 0;
  lpm_prefix4[64] = 0;
  int lpm_update_elem_success4 = lpm_update(lpm, lpm_prefix4, 264, 3);
  if (!lpm_update_elem_success4) {
    return false;
  }
  uint8_t lpm_prefix5[65];
  lpm_prefix5[0] = 4;
  lpm_prefix5[1] = 3;
  lpm_prefix5[2] = 99;
  lpm_prefix5[3] = 111;
  lpm_prefix5[4] = 109;
  lpm_prefix5[5] = 0;
  lpm_prefix5[6] = 0;
  lpm_prefix5[7] = 0;
  lpm_prefix5[8] = 0;
  lpm_prefix5[9] = 0;
  lpm_prefix5[10] = 0;
  lpm_prefix5[11] = 0;
  lpm_prefix5[12] = 0;
  lpm_prefix5[13] = 0;
  lpm_prefix5[14] = 0;
  lpm_prefix5[15] = 0;
  lpm_prefix5[16] = 0;
  lpm_prefix5[17] = 5;
  lpm_prefix5[18] = 115;
  lpm_prefix5[19] = 107;
  lpm_prefix5[20] = 121;
  lpm_prefix5[21] = 112;
  lpm_prefix5[22] = 101;
  lpm_prefix5[23] = 0;
  lpm_prefix5[24] = 0;
  lpm_prefix5[25] = 0;
  lpm_prefix5[26] = 0;
  lpm_prefix5[27] = 0;
  lpm_prefix5[28] = 0;
  lpm_prefix5[29] = 0;
  lpm_prefix5[30] = 0;
  lpm_prefix5[31] = 0;
  lpm_prefix5[32] = 0;
  lpm_prefix5[33] = 0;
  lpm_prefix5[34] = 0;
  lpm_prefix5[35] = 0;
  lpm_prefix5[36] = 0;
  lpm_prefix5[37] = 0;
  lpm_prefix5[38] = 0;
  lpm_prefix5[39] = 0;
  lpm_prefix5[40] = 0;
  lpm_prefix5[41] = 0;
  lpm_prefix5[42] = 0;
  lpm_prefix5[43] = 0;
  lpm_prefix5[44] = 0;
  lpm_prefix5[45] = 0;
  lpm_prefix5[46] = 0;
  lpm_prefix5[47] = 0;
  lpm_prefix5[48] = 0;
  lpm_prefix5[49] = 0;
  lpm_prefix5[50] = 0;
  lpm_prefix5[51] = 0;
  lpm_prefix5[52] = 0;
  lpm_prefix5[53] = 0;
  lpm_prefix5[54] = 0;
  lpm_prefix5[55] = 0;
  lpm_prefix5[56] = 0;
  lpm_prefix5[57] = 0;
  lpm_prefix5[58] = 0;
  lpm_prefix5[59] = 0;
  lpm_prefix5[60] = 0;
  lpm_prefix5[61] = 0;
  lpm_prefix5[62] = 0;
  lpm_prefix5[63] = 0;
  lpm_prefix5[64] = 0;
  int lpm_update_elem_success5 = lpm_update(lpm, lpm_prefix5, 264, 4);
  if (!lpm_update_elem_success5) {
    return false;
  }
  uint8_t lpm_prefix6[65];
  lpm_prefix6[0] = 3;
  lpm_prefix6[1] = 3;
  lpm_prefix6[2] = 99;
  lpm_prefix6[3] = 111;
  lpm_prefix6[4] = 109;
  lpm_prefix6[5] = 0;
  lpm_prefix6[6] = 0;
  lpm_prefix6[7] = 0;
  lpm_prefix6[8] = 0;
  lpm_prefix6[9] = 0;
  lpm_prefix6[10] = 0;
  lpm_prefix6[11] = 0;
  lpm_prefix6[12] = 0;
  lpm_prefix6[13] = 0;
  lpm_prefix6[14] = 0;
  lpm_prefix6[15] = 0;
  lpm_prefix6[16] = 0;
  lpm_prefix6[17] = 5;
  lpm_prefix6[18] = 115;
  lpm_prefix6[19] = 107;
  lpm_prefix6[20] = 121;
  lpm_prefix6[21] = 112;
  lpm_prefix6[22] = 101;
  lpm_prefix6[23] = 0;
  lpm_prefix6[24] = 0;
  lpm_prefix6[25] = 0;
  lpm_prefix6[26] = 0;
  lpm_prefix6[27] = 0;
  lpm_prefix6[28] = 0;
  lpm_prefix6[29] = 0;
  lpm_prefix6[30] = 0;
  lpm_prefix6[31] = 0;
  lpm_prefix6[32] = 0;
  lpm_prefix6[33] = 0;
  lpm_prefix6[34] = 0;
  lpm_prefix6[35] = 0;
  lpm_prefix6[36] = 0;
  lpm_prefix6[37] = 0;
  lpm_prefix6[38] = 0;
  lpm_prefix6[39] = 0;
  lpm_prefix6[40] = 0;
  lpm_prefix6[41] = 0;
  lpm_prefix6[42] = 0;
  lpm_prefix6[43] = 0;
  lpm_prefix6[44] = 0;
  lpm_prefix6[45] = 0;
  lpm_prefix6[46] = 0;
  lpm_prefix6[47] = 0;
  lpm_prefix6[48] = 0;
  lpm_prefix6[49] = 0;
  lpm_prefix6[50] = 0;
  lpm_prefix6[51] = 0;
  lpm_prefix6[52] = 0;
  lpm_prefix6[53] = 0;
  lpm_prefix6[54] = 0;
  lpm_prefix6[55] = 0;
  lpm_prefix6[56] = 0;
  lpm_prefix6[57] = 0;
  lpm_prefix6[58] = 0;
  lpm_prefix6[59] = 0;
  lpm_prefix6[60] = 0;
  lpm_prefix6[61] = 0;
  lpm_prefix6[62] = 0;
  lpm_prefix6[63] = 0;
  lpm_prefix6[64] = 0;
  int lpm_update_elem_success6 = lpm_update(lpm, lpm_prefix6, 264, 5);
  if (!lpm_update_elem_success6) {
    return false;
  }
  uint8_t lpm_prefix7[65];
  lpm_prefix7[0] = 3;
  lpm_prefix7[1] = 3;
  lpm_prefix7[2] = 99;
  lpm_prefix7[3] = 111;
  lpm_prefix7[4] = 109;
  lpm_prefix7[5] = 0;
  lpm_prefix7[6] = 0;
  lpm_prefix7[7] = 0;
  lpm_prefix7[8] = 0;
  lpm_prefix7[9] = 0;
  lpm_prefix7[10] = 0;
  lpm_prefix7[11] = 0;
  lpm_prefix7[12] = 0;
  lpm_prefix7[13] = 0;
  lpm_prefix7[14] = 0;
  lpm_prefix7[15] = 0;
  lpm_prefix7[16] = 0;
  lpm_prefix7[17] = 3;
  lpm_prefix7[18] = 109;
  lpm_prefix7[19] = 115;
  lpm_prefix7[20] = 110;
  lpm_prefix7[21] = 0;
  lpm_prefix7[22] = 0;
  lpm_prefix7[23] = 0;
  lpm_prefix7[24] = 0;
  lpm_prefix7[25] = 0;
  lpm_prefix7[26] = 0;
  lpm_prefix7[27] = 0;
  lpm_prefix7[28] = 0;
  lpm_prefix7[29] = 0;
  lpm_prefix7[30] = 0;
  lpm_prefix7[31] = 0;
  lpm_prefix7[32] = 0;
  lpm_prefix7[33] = 0;
  lpm_prefix7[34] = 0;
  lpm_prefix7[35] = 0;
  lpm_prefix7[36] = 0;
  lpm_prefix7[37] = 0;
  lpm_prefix7[38] = 0;
  lpm_prefix7[39] = 0;
  lpm_prefix7[40] = 0;
  lpm_prefix7[41] = 0;
  lpm_prefix7[42] = 0;
  lpm_prefix7[43] = 0;
  lpm_prefix7[44] = 0;
  lpm_prefix7[45] = 0;
  lpm_prefix7[46] = 0;
  lpm_prefix7[47] = 0;
  lpm_prefix7[48] = 0;
  lpm_prefix7[49] = 0;
  lpm_prefix7[50] = 0;
  lpm_prefix7[51] = 0;
  lpm_prefix7[52] = 0;
  lpm_prefix7[53] = 0;
  lpm_prefix7[54] = 0;
  lpm_prefix7[55] = 0;
  lpm_prefix7[56] = 0;
  lpm_prefix7[57] = 0;
  lpm_prefix7[58] = 0;
  lpm_prefix7[59] = 0;
  lpm_prefix7[60] = 0;
  lpm_prefix7[61] = 0;
  lpm_prefix7[62] = 0;
  lpm_prefix7[63] = 0;
  lpm_prefix7[64] = 0;
  int lpm_update_elem_success7 = lpm_update(lpm, lpm_prefix7, 264, 6);
  if (!lpm_update_elem_success7) {
    return false;
  }
  uint8_t lpm_prefix8[65];
  lpm_prefix8[0] = 3;
  lpm_prefix8[1] = 3;
  lpm_prefix8[2] = 99;
  lpm_prefix8[3] = 111;
  lpm_prefix8[4] = 109;
  lpm_prefix8[5] = 0;
  lpm_prefix8[6] = 0;
  lpm_prefix8[7] = 0;
  lpm_prefix8[8] = 0;
  lpm_prefix8[9] = 0;
  lpm_prefix8[10] = 0;
  lpm_prefix8[11] = 0;
  lpm_prefix8[12] = 0;
  lpm_prefix8[13] = 0;
  lpm_prefix8[14] = 0;
  lpm_prefix8[15] = 0;
  lpm_prefix8[16] = 0;
  lpm_prefix8[17] = 4;
  lpm_prefix8[18] = 108;
  lpm_prefix8[19] = 105;
  lpm_prefix8[20] = 118;
  lpm_prefix8[21] = 101;
  lpm_prefix8[22] = 0;
  lpm_prefix8[23] = 0;
  lpm_prefix8[24] = 0;
  lpm_prefix8[25] = 0;
  lpm_prefix8[26] = 0;
  lpm_prefix8[27] = 0;
  lpm_prefix8[28] = 0;
  lpm_prefix8[29] = 0;
  lpm_prefix8[30] = 0;
  lpm_prefix8[31] = 0;
  lpm_prefix8[32] = 0;
  lpm_prefix8[33] = 0;
  lpm_prefix8[34] = 0;
  lpm_prefix8[35] = 0;
  lpm_prefix8[36] = 0;
  lpm_prefix8[37] = 0;
  lpm_prefix8[38] = 0;
  lpm_prefix8[39] = 0;
  lpm_prefix8[40] = 0;
  lpm_prefix8[41] = 0;
  lpm_prefix8[42] = 0;
  lpm_prefix8[43] = 0;
  lpm_prefix8[44] = 0;
  lpm_prefix8[45] = 0;
  lpm_prefix8[46] = 0;
  lpm_prefix8[47] = 0;
  lpm_prefix8[48] = 0;
  lpm_prefix8[49] = 0;
  lpm_prefix8[50] = 0;
  lpm_prefix8[51] = 0;
  lpm_prefix8[52] = 0;
  lpm_prefix8[53] = 0;
  lpm_prefix8[54] = 0;
  lpm_prefix8[55] = 0;
  lpm_prefix8[56] = 0;
  lpm_prefix8[57] = 0;
  lpm_prefix8[58] = 0;
  lpm_prefix8[59] = 0;
  lpm_prefix8[60] = 0;
  lpm_prefix8[61] = 0;
  lpm_prefix8[62] = 0;
  lpm_prefix8[63] = 0;
  lpm_prefix8[64] = 0;
  int lpm_update_elem_success8 = lpm_update(lpm, lpm_prefix8, 264, 7);
  if (!lpm_update_elem_success8) {
    return false;
  }
  uint8_t lpm_prefix9[65];
  lpm_prefix9[0] = 3;
  lpm_prefix9[1] = 3;
  lpm_prefix9[2] = 99;
  lpm_prefix9[3] = 111;
  lpm_prefix9[4] = 109;
  lpm_prefix9[5] = 0;
  lpm_prefix9[6] = 0;
  lpm_prefix9[7] = 0;
  lpm_prefix9[8] = 0;
  lpm_prefix9[9] = 0;
  lpm_prefix9[10] = 0;
  lpm_prefix9[11] = 0;
  lpm_prefix9[12] = 0;
  lpm_prefix9[13] = 0;
  lpm_prefix9[14] = 0;
  lpm_prefix9[15] = 0;
  lpm_prefix9[16] = 0;
  lpm_prefix9[17] = 6;
  lpm_prefix9[18] = 103;
  lpm_prefix9[19] = 111;
  lpm_prefix9[20] = 111;
  lpm_prefix9[21] = 103;
  lpm_prefix9[22] = 108;
  lpm_prefix9[23] = 101;
  lpm_prefix9[24] = 0;
  lpm_prefix9[25] = 0;
  lpm_prefix9[26] = 0;
  lpm_prefix9[27] = 0;
  lpm_prefix9[28] = 0;
  lpm_prefix9[29] = 0;
  lpm_prefix9[30] = 0;
  lpm_prefix9[31] = 0;
  lpm_prefix9[32] = 0;
  lpm_prefix9[33] = 3;
  lpm_prefix9[34] = 119;
  lpm_prefix9[35] = 119;
  lpm_prefix9[36] = 119;
  lpm_prefix9[37] = 0;
  lpm_prefix9[38] = 0;
  lpm_prefix9[39] = 0;
  lpm_prefix9[40] = 0;
  lpm_prefix9[41] = 0;
  lpm_prefix9[42] = 0;
  lpm_prefix9[43] = 0;
  lpm_prefix9[44] = 0;
  lpm_prefix9[45] = 0;
  lpm_prefix9[46] = 0;
  lpm_prefix9[47] = 0;
  lpm_prefix9[48] = 0;
  lpm_prefix9[49] = 0;
  lpm_prefix9[50] = 0;
  lpm_prefix9[51] = 0;
  lpm_prefix9[52] = 0;
  lpm_prefix9[53] = 0;
  lpm_prefix9[54] = 0;
  lpm_prefix9[55] = 0;
  lpm_prefix9[56] = 0;
  lpm_prefix9[57] = 0;
  lpm_prefix9[58] = 0;
  lpm_prefix9[59] = 0;
  lpm_prefix9[60] = 0;
  lpm_prefix9[61] = 0;
  lpm_prefix9[62] = 0;
  lpm_prefix9[63] = 0;
  lpm_prefix9[64] = 0;
  int lpm_update_elem_success9 = lpm_update(lpm, lpm_prefix9, 392, 8);
  if (!lpm_update_elem_success9) {
    return false;
  }
  uint8_t lpm_prefix10[65];
  lpm_prefix10[0] = 3;
  lpm_prefix10[1] = 3;
  lpm_prefix10[2] = 99;
  lpm_prefix10[3] = 111;
  lpm_prefix10[4] = 109;
  lpm_prefix10[5] = 0;
  lpm_prefix10[6] = 0;
  lpm_prefix10[7] = 0;
  lpm_prefix10[8] = 0;
  lpm_prefix10[9] = 0;
  lpm_prefix10[10] = 0;
  lpm_prefix10[11] = 0;
  lpm_prefix10[12] = 0;
  lpm_prefix10[13] = 0;
  lpm_prefix10[14] = 0;
  lpm_prefix10[15] = 0;
  lpm_prefix10[16] = 0;
  lpm_prefix10[17] = 7;
  lpm_prefix10[18] = 121;
  lpm_prefix10[19] = 111;
  lpm_prefix10[20] = 117;
  lpm_prefix10[21] = 116;
  lpm_prefix10[22] = 117;
  lpm_prefix10[23] = 98;
  lpm_prefix10[24] = 101;
  lpm_prefix10[25] = 0;
  lpm_prefix10[26] = 0;
  lpm_prefix10[27] = 0;
  lpm_prefix10[28] = 0;
  lpm_prefix10[29] = 0;
  lpm_prefix10[30] = 0;
  lpm_prefix10[31] = 0;
  lpm_prefix10[32] = 0;
  lpm_prefix10[33] = 3;
  lpm_prefix10[34] = 119;
  lpm_prefix10[35] = 119;
  lpm_prefix10[36] = 119;
  lpm_prefix10[37] = 0;
  lpm_prefix10[38] = 0;
  lpm_prefix10[39] = 0;
  lpm_prefix10[40] = 0;
  lpm_prefix10[41] = 0;
  lpm_prefix10[42] = 0;
  lpm_prefix10[43] = 0;
  lpm_prefix10[44] = 0;
  lpm_prefix10[45] = 0;
  lpm_prefix10[46] = 0;
  lpm_prefix10[47] = 0;
  lpm_prefix10[48] = 0;
  lpm_prefix10[49] = 0;
  lpm_prefix10[50] = 0;
  lpm_prefix10[51] = 0;
  lpm_prefix10[52] = 0;
  lpm_prefix10[53] = 0;
  lpm_prefix10[54] = 0;
  lpm_prefix10[55] = 0;
  lpm_prefix10[56] = 0;
  lpm_prefix10[57] = 0;
  lpm_prefix10[58] = 0;
  lpm_prefix10[59] = 0;
  lpm_prefix10[60] = 0;
  lpm_prefix10[61] = 0;
  lpm_prefix10[62] = 0;
  lpm_prefix10[63] = 0;
  lpm_prefix10[64] = 0;
  int lpm_update_elem_success10 = lpm_update(lpm, lpm_prefix10, 392, 9);
  if (!lpm_update_elem_success10) {
    return false;
  }
  uint8_t lpm_prefix11[65];
  lpm_prefix11[0] = 3;
  lpm_prefix11[1] = 3;
  lpm_prefix11[2] = 99;
  lpm_prefix11[3] = 111;
  lpm_prefix11[4] = 109;
  lpm_prefix11[5] = 0;
  lpm_prefix11[6] = 0;
  lpm_prefix11[7] = 0;
  lpm_prefix11[8] = 0;
  lpm_prefix11[9] = 0;
  lpm_prefix11[10] = 0;
  lpm_prefix11[11] = 0;
  lpm_prefix11[12] = 0;
  lpm_prefix11[13] = 0;
  lpm_prefix11[14] = 0;
  lpm_prefix11[15] = 0;
  lpm_prefix11[16] = 0;
  lpm_prefix11[17] = 6;
  lpm_prefix11[18] = 103;
  lpm_prefix11[19] = 111;
  lpm_prefix11[20] = 111;
  lpm_prefix11[21] = 103;
  lpm_prefix11[22] = 108;
  lpm_prefix11[23] = 101;
  lpm_prefix11[24] = 0;
  lpm_prefix11[25] = 0;
  lpm_prefix11[26] = 0;
  lpm_prefix11[27] = 0;
  lpm_prefix11[28] = 0;
  lpm_prefix11[29] = 0;
  lpm_prefix11[30] = 0;
  lpm_prefix11[31] = 0;
  lpm_prefix11[32] = 0;
  lpm_prefix11[33] = 4;
  lpm_prefix11[34] = 109;
  lpm_prefix11[35] = 97;
  lpm_prefix11[36] = 105;
  lpm_prefix11[37] = 108;
  lpm_prefix11[38] = 0;
  lpm_prefix11[39] = 0;
  lpm_prefix11[40] = 0;
  lpm_prefix11[41] = 0;
  lpm_prefix11[42] = 0;
  lpm_prefix11[43] = 0;
  lpm_prefix11[44] = 0;
  lpm_prefix11[45] = 0;
  lpm_prefix11[46] = 0;
  lpm_prefix11[47] = 0;
  lpm_prefix11[48] = 0;
  lpm_prefix11[49] = 0;
  lpm_prefix11[50] = 0;
  lpm_prefix11[51] = 0;
  lpm_prefix11[52] = 0;
  lpm_prefix11[53] = 0;
  lpm_prefix11[54] = 0;
  lpm_prefix11[55] = 0;
  lpm_prefix11[56] = 0;
  lpm_prefix11[57] = 0;
  lpm_prefix11[58] = 0;
  lpm_prefix11[59] = 0;
  lpm_prefix11[60] = 0;
  lpm_prefix11[61] = 0;
  lpm_prefix11[62] = 0;
  lpm_prefix11[63] = 0;
  lpm_prefix11[64] = 0;
  int lpm_update_elem_success11 = lpm_update(lpm, lpm_prefix11, 392, 10);
  if (!lpm_update_elem_success11) {
    return false;
  }
  uint8_t lpm_prefix12[65];
  lpm_prefix12[0] = 3;
  lpm_prefix12[1] = 3;
  lpm_prefix12[2] = 99;
  lpm_prefix12[3] = 111;
  lpm_prefix12[4] = 109;
  lpm_prefix12[5] = 0;
  lpm_prefix12[6] = 0;
  lpm_prefix12[7] = 0;
  lpm_prefix12[8] = 0;
  lpm_prefix12[9] = 0;
  lpm_prefix12[10] = 0;
  lpm_prefix12[11] = 0;
  lpm_prefix12[12] = 0;
  lpm_prefix12[13] = 0;
  lpm_prefix12[14] = 0;
  lpm_prefix12[15] = 0;
  lpm_prefix12[16] = 0;
  lpm_prefix12[17] = 6;
  lpm_prefix12[18] = 103;
  lpm_prefix12[19] = 111;
  lpm_prefix12[20] = 111;
  lpm_prefix12[21] = 103;
  lpm_prefix12[22] = 108;
  lpm_prefix12[23] = 101;
  lpm_prefix12[24] = 0;
  lpm_prefix12[25] = 0;
  lpm_prefix12[26] = 0;
  lpm_prefix12[27] = 0;
  lpm_prefix12[28] = 0;
  lpm_prefix12[29] = 0;
  lpm_prefix12[30] = 0;
  lpm_prefix12[31] = 0;
  lpm_prefix12[32] = 0;
  lpm_prefix12[33] = 4;
  lpm_prefix12[34] = 112;
  lpm_prefix12[35] = 108;
  lpm_prefix12[36] = 97;
  lpm_prefix12[37] = 121;
  lpm_prefix12[38] = 0;
  lpm_prefix12[39] = 0;
  lpm_prefix12[40] = 0;
  lpm_prefix12[41] = 0;
  lpm_prefix12[42] = 0;
  lpm_prefix12[43] = 0;
  lpm_prefix12[44] = 0;
  lpm_prefix12[45] = 0;
  lpm_prefix12[46] = 0;
  lpm_prefix12[47] = 0;
  lpm_prefix12[48] = 0;
  lpm_prefix12[49] = 0;
  lpm_prefix12[50] = 0;
  lpm_prefix12[51] = 0;
  lpm_prefix12[52] = 0;
  lpm_prefix12[53] = 0;
  lpm_prefix12[54] = 0;
  lpm_prefix12[55] = 0;
  lpm_prefix12[56] = 0;
  lpm_prefix12[57] = 0;
  lpm_prefix12[58] = 0;
  lpm_prefix12[59] = 0;
  lpm_prefix12[60] = 0;
  lpm_prefix12[61] = 0;
  lpm_prefix12[62] = 0;
  lpm_prefix12[63] = 0;
  lpm_prefix12[64] = 0;
  int lpm_update_elem_success12 = lpm_update(lpm, lpm_prefix12, 392, 11);
  if (!lpm_update_elem_success12) {
    return false;
  }
  uint8_t lpm_prefix13[65];
  lpm_prefix13[0] = 3;
  lpm_prefix13[1] = 3;
  lpm_prefix13[2] = 99;
  lpm_prefix13[3] = 111;
  lpm_prefix13[4] = 109;
  lpm_prefix13[5] = 0;
  lpm_prefix13[6] = 0;
  lpm_prefix13[7] = 0;
  lpm_prefix13[8] = 0;
  lpm_prefix13[9] = 0;
  lpm_prefix13[10] = 0;
  lpm_prefix13[11] = 0;
  lpm_prefix13[12] = 0;
  lpm_prefix13[13] = 0;
  lpm_prefix13[14] = 0;
  lpm_prefix13[15] = 0;
  lpm_prefix13[16] = 0;
  lpm_prefix13[17] = 6;
  lpm_prefix13[18] = 103;
  lpm_prefix13[19] = 111;
  lpm_prefix13[20] = 111;
  lpm_prefix13[21] = 103;
  lpm_prefix13[22] = 108;
  lpm_prefix13[23] = 101;
  lpm_prefix13[24] = 0;
  lpm_prefix13[25] = 0;
  lpm_prefix13[26] = 0;
  lpm_prefix13[27] = 0;
  lpm_prefix13[28] = 0;
  lpm_prefix13[29] = 0;
  lpm_prefix13[30] = 0;
  lpm_prefix13[31] = 0;
  lpm_prefix13[32] = 0;
  lpm_prefix13[33] = 8;
  lpm_prefix13[34] = 104;
  lpm_prefix13[35] = 97;
  lpm_prefix13[36] = 110;
  lpm_prefix13[37] = 103;
  lpm_prefix13[38] = 111;
  lpm_prefix13[39] = 117;
  lpm_prefix13[40] = 116;
  lpm_prefix13[41] = 115;
  lpm_prefix13[42] = 0;
  lpm_prefix13[43] = 0;
  lpm_prefix13[44] = 0;
  lpm_prefix13[45] = 0;
  lpm_prefix13[46] = 0;
  lpm_prefix13[47] = 0;
  lpm_prefix13[48] = 0;
  lpm_prefix13[49] = 0;
  lpm_prefix13[50] = 0;
  lpm_prefix13[51] = 0;
  lpm_prefix13[52] = 0;
  lpm_prefix13[53] = 0;
  lpm_prefix13[54] = 0;
  lpm_prefix13[55] = 0;
  lpm_prefix13[56] = 0;
  lpm_prefix13[57] = 0;
  lpm_prefix13[58] = 0;
  lpm_prefix13[59] = 0;
  lpm_prefix13[60] = 0;
  lpm_prefix13[61] = 0;
  lpm_prefix13[62] = 0;
  lpm_prefix13[63] = 0;
  lpm_prefix13[64] = 0;
  int lpm_update_elem_success13 = lpm_update(lpm, lpm_prefix13, 392, 12);
  if (!lpm_update_elem_success13) {
    return false;
  }
  uint8_t lpm_prefix14[65];
  lpm_prefix14[0] = 3;
  lpm_prefix14[1] = 3;
  lpm_prefix14[2] = 99;
  lpm_prefix14[3] = 111;
  lpm_prefix14[4] = 109;
  lpm_prefix14[5] = 0;
  lpm_prefix14[6] = 0;
  lpm_prefix14[7] = 0;
  lpm_prefix14[8] = 0;
  lpm_prefix14[9] = 0;
  lpm_prefix14[10] = 0;
  lpm_prefix14[11] = 0;
  lpm_prefix14[12] = 0;
  lpm_prefix14[13] = 0;
  lpm_prefix14[14] = 0;
  lpm_prefix14[15] = 0;
  lpm_prefix14[16] = 0;
  lpm_prefix14[17] = 8;
  lpm_prefix14[18] = 102;
  lpm_prefix14[19] = 97;
  lpm_prefix14[20] = 99;
  lpm_prefix14[21] = 101;
  lpm_prefix14[22] = 98;
  lpm_prefix14[23] = 111;
  lpm_prefix14[24] = 111;
  lpm_prefix14[25] = 107;
  lpm_prefix14[26] = 0;
  lpm_prefix14[27] = 0;
  lpm_prefix14[28] = 0;
  lpm_prefix14[29] = 0;
  lpm_prefix14[30] = 0;
  lpm_prefix14[31] = 0;
  lpm_prefix14[32] = 0;
  lpm_prefix14[33] = 3;
  lpm_prefix14[34] = 119;
  lpm_prefix14[35] = 119;
  lpm_prefix14[36] = 119;
  lpm_prefix14[37] = 0;
  lpm_prefix14[38] = 0;
  lpm_prefix14[39] = 0;
  lpm_prefix14[40] = 0;
  lpm_prefix14[41] = 0;
  lpm_prefix14[42] = 0;
  lpm_prefix14[43] = 0;
  lpm_prefix14[44] = 0;
  lpm_prefix14[45] = 0;
  lpm_prefix14[46] = 0;
  lpm_prefix14[47] = 0;
  lpm_prefix14[48] = 0;
  lpm_prefix14[49] = 0;
  lpm_prefix14[50] = 0;
  lpm_prefix14[51] = 0;
  lpm_prefix14[52] = 0;
  lpm_prefix14[53] = 0;
  lpm_prefix14[54] = 0;
  lpm_prefix14[55] = 0;
  lpm_prefix14[56] = 0;
  lpm_prefix14[57] = 0;
  lpm_prefix14[58] = 0;
  lpm_prefix14[59] = 0;
  lpm_prefix14[60] = 0;
  lpm_prefix14[61] = 0;
  lpm_prefix14[62] = 0;
  lpm_prefix14[63] = 0;
  lpm_prefix14[64] = 0;
  int lpm_update_elem_success14 = lpm_update(lpm, lpm_prefix14, 392, 13);
  if (!lpm_update_elem_success14) {
    return false;
  }
  uint8_t lpm_prefix15[65];
  lpm_prefix15[0] = 2;
  lpm_prefix15[1] = 3;
  lpm_prefix15[2] = 99;
  lpm_prefix15[3] = 111;
  lpm_prefix15[4] = 109;
  lpm_prefix15[5] = 0;
  lpm_prefix15[6] = 0;
  lpm_prefix15[7] = 0;
  lpm_prefix15[8] = 0;
  lpm_prefix15[9] = 0;
  lpm_prefix15[10] = 0;
  lpm_prefix15[11] = 0;
  lpm_prefix15[12] = 0;
  lpm_prefix15[13] = 0;
  lpm_prefix15[14] = 0;
  lpm_prefix15[15] = 0;
  lpm_prefix15[16] = 0;
  lpm_prefix15[17] = 8;
  lpm_prefix15[18] = 102;
  lpm_prefix15[19] = 97;
  lpm_prefix15[20] = 99;
  lpm_prefix15[21] = 101;
  lpm_prefix15[22] = 98;
  lpm_prefix15[23] = 111;
  lpm_prefix15[24] = 111;
  lpm_prefix15[25] = 107;
  lpm_prefix15[26] = 0;
  lpm_prefix15[27] = 0;
  lpm_prefix15[28] = 0;
  lpm_prefix15[29] = 0;
  lpm_prefix15[30] = 0;
  lpm_prefix15[31] = 0;
  lpm_prefix15[32] = 0;
  lpm_prefix15[33] = 0;
  lpm_prefix15[34] = 0;
  lpm_prefix15[35] = 0;
  lpm_prefix15[36] = 0;
  lpm_prefix15[37] = 0;
  lpm_prefix15[38] = 0;
  lpm_prefix15[39] = 0;
  lpm_prefix15[40] = 0;
  lpm_prefix15[41] = 0;
  lpm_prefix15[42] = 0;
  lpm_prefix15[43] = 0;
  lpm_prefix15[44] = 0;
  lpm_prefix15[45] = 0;
  lpm_prefix15[46] = 0;
  lpm_prefix15[47] = 0;
  lpm_prefix15[48] = 0;
  lpm_prefix15[49] = 0;
  lpm_prefix15[50] = 0;
  lpm_prefix15[51] = 0;
  lpm_prefix15[52] = 0;
  lpm_prefix15[53] = 0;
  lpm_prefix15[54] = 0;
  lpm_prefix15[55] = 0;
  lpm_prefix15[56] = 0;
  lpm_prefix15[57] = 0;
  lpm_prefix15[58] = 0;
  lpm_prefix15[59] = 0;
  lpm_prefix15[60] = 0;
  lpm_prefix15[61] = 0;
  lpm_prefix15[62] = 0;
  lpm_prefix15[63] = 0;
  lpm_prefix15[64] = 0;
  int lpm_update_elem_success15 = lpm_update(lpm, lpm_prefix15, 264, 14);
  if (!lpm_update_elem_success15) {
    return false;
  }
  uint8_t lpm_prefix16[65];
  lpm_prefix16[0] = 3;
  lpm_prefix16[1] = 3;
  lpm_prefix16[2] = 99;
  lpm_prefix16[3] = 111;
  lpm_prefix16[4] = 109;
  lpm_prefix16[5] = 0;
  lpm_prefix16[6] = 0;
  lpm_prefix16[7] = 0;
  lpm_prefix16[8] = 0;
  lpm_prefix16[9] = 0;
  lpm_prefix16[10] = 0;
  lpm_prefix16[11] = 0;
  lpm_prefix16[12] = 0;
  lpm_prefix16[13] = 0;
  lpm_prefix16[14] = 0;
  lpm_prefix16[15] = 0;
  lpm_prefix16[16] = 0;
  lpm_prefix16[17] = 9;
  lpm_prefix16[18] = 109;
  lpm_prefix16[19] = 101;
  lpm_prefix16[20] = 115;
  lpm_prefix16[21] = 115;
  lpm_prefix16[22] = 101;
  lpm_prefix16[23] = 110;
  lpm_prefix16[24] = 103;
  lpm_prefix16[25] = 101;
  lpm_prefix16[26] = 114;
  lpm_prefix16[27] = 0;
  lpm_prefix16[28] = 0;
  lpm_prefix16[29] = 0;
  lpm_prefix16[30] = 0;
  lpm_prefix16[31] = 0;
  lpm_prefix16[32] = 0;
  lpm_prefix16[33] = 3;
  lpm_prefix16[34] = 119;
  lpm_prefix16[35] = 119;
  lpm_prefix16[36] = 119;
  lpm_prefix16[37] = 0;
  lpm_prefix16[38] = 0;
  lpm_prefix16[39] = 0;
  lpm_prefix16[40] = 0;
  lpm_prefix16[41] = 0;
  lpm_prefix16[42] = 0;
  lpm_prefix16[43] = 0;
  lpm_prefix16[44] = 0;
  lpm_prefix16[45] = 0;
  lpm_prefix16[46] = 0;
  lpm_prefix16[47] = 0;
  lpm_prefix16[48] = 0;
  lpm_prefix16[49] = 0;
  lpm_prefix16[50] = 0;
  lpm_prefix16[51] = 0;
  lpm_prefix16[52] = 0;
  lpm_prefix16[53] = 0;
  lpm_prefix16[54] = 0;
  lpm_prefix16[55] = 0;
  lpm_prefix16[56] = 0;
  lpm_prefix16[57] = 0;
  lpm_prefix16[58] = 0;
  lpm_prefix16[59] = 0;
  lpm_prefix16[60] = 0;
  lpm_prefix16[61] = 0;
  lpm_prefix16[62] = 0;
  lpm_prefix16[63] = 0;
  lpm_prefix16[64] = 0;
  int lpm_update_elem_success16 = lpm_update(lpm, lpm_prefix16, 392, 15);
  if (!lpm_update_elem_success16) {
    return false;
  }
  uint8_t lpm_prefix17[65];
  lpm_prefix17[0] = 3;
  lpm_prefix17[1] = 3;
  lpm_prefix17[2] = 99;
  lpm_prefix17[3] = 111;
  lpm_prefix17[4] = 109;
  lpm_prefix17[5] = 0;
  lpm_prefix17[6] = 0;
  lpm_prefix17[7] = 0;
  lpm_prefix17[8] = 0;
  lpm_prefix17[9] = 0;
  lpm_prefix17[10] = 0;
  lpm_prefix17[11] = 0;
  lpm_prefix17[12] = 0;
  lpm_prefix17[13] = 0;
  lpm_prefix17[14] = 0;
  lpm_prefix17[15] = 0;
  lpm_prefix17[16] = 0;
  lpm_prefix17[17] = 5;
  lpm_prefix17[18] = 97;
  lpm_prefix17[19] = 112;
  lpm_prefix17[20] = 112;
  lpm_prefix17[21] = 108;
  lpm_prefix17[22] = 101;
  lpm_prefix17[23] = 0;
  lpm_prefix17[24] = 0;
  lpm_prefix17[25] = 0;
  lpm_prefix17[26] = 0;
  lpm_prefix17[27] = 0;
  lpm_prefix17[28] = 0;
  lpm_prefix17[29] = 0;
  lpm_prefix17[30] = 0;
  lpm_prefix17[31] = 0;
  lpm_prefix17[32] = 0;
  lpm_prefix17[33] = 3;
  lpm_prefix17[34] = 119;
  lpm_prefix17[35] = 119;
  lpm_prefix17[36] = 119;
  lpm_prefix17[37] = 0;
  lpm_prefix17[38] = 0;
  lpm_prefix17[39] = 0;
  lpm_prefix17[40] = 0;
  lpm_prefix17[41] = 0;
  lpm_prefix17[42] = 0;
  lpm_prefix17[43] = 0;
  lpm_prefix17[44] = 0;
  lpm_prefix17[45] = 0;
  lpm_prefix17[46] = 0;
  lpm_prefix17[47] = 0;
  lpm_prefix17[48] = 0;
  lpm_prefix17[49] = 0;
  lpm_prefix17[50] = 0;
  lpm_prefix17[51] = 0;
  lpm_prefix17[52] = 0;
  lpm_prefix17[53] = 0;
  lpm_prefix17[54] = 0;
  lpm_prefix17[55] = 0;
  lpm_prefix17[56] = 0;
  lpm_prefix17[57] = 0;
  lpm_prefix17[58] = 0;
  lpm_prefix17[59] = 0;
  lpm_prefix17[60] = 0;
  lpm_prefix17[61] = 0;
  lpm_prefix17[62] = 0;
  lpm_prefix17[63] = 0;
  lpm_prefix17[64] = 0;
  int lpm_update_elem_success17 = lpm_update(lpm, lpm_prefix17, 392, 16);
  if (!lpm_update_elem_success17) {
    return false;
  }
  uint8_t lpm_prefix18[65];
  lpm_prefix18[0] = 3;
  lpm_prefix18[1] = 3;
  lpm_prefix18[2] = 99;
  lpm_prefix18[3] = 111;
  lpm_prefix18[4] = 109;
  lpm_prefix18[5] = 0;
  lpm_prefix18[6] = 0;
  lpm_prefix18[7] = 0;
  lpm_prefix18[8] = 0;
  lpm_prefix18[9] = 0;
  lpm_prefix18[10] = 0;
  lpm_prefix18[11] = 0;
  lpm_prefix18[12] = 0;
  lpm_prefix18[13] = 0;
  lpm_prefix18[14] = 0;
  lpm_prefix18[15] = 0;
  lpm_prefix18[16] = 0;
  lpm_prefix18[17] = 5;
  lpm_prefix18[18] = 97;
  lpm_prefix18[19] = 112;
  lpm_prefix18[20] = 112;
  lpm_prefix18[21] = 108;
  lpm_prefix18[22] = 101;
  lpm_prefix18[23] = 0;
  lpm_prefix18[24] = 0;
  lpm_prefix18[25] = 0;
  lpm_prefix18[26] = 0;
  lpm_prefix18[27] = 0;
  lpm_prefix18[28] = 0;
  lpm_prefix18[29] = 0;
  lpm_prefix18[30] = 0;
  lpm_prefix18[31] = 0;
  lpm_prefix18[32] = 0;
  lpm_prefix18[33] = 6;
  lpm_prefix18[34] = 105;
  lpm_prefix18[35] = 116;
  lpm_prefix18[36] = 117;
  lpm_prefix18[37] = 110;
  lpm_prefix18[38] = 101;
  lpm_prefix18[39] = 115;
  lpm_prefix18[40] = 0;
  lpm_prefix18[41] = 0;
  lpm_prefix18[42] = 0;
  lpm_prefix18[43] = 0;
  lpm_prefix18[44] = 0;
  lpm_prefix18[45] = 0;
  lpm_prefix18[46] = 0;
  lpm_prefix18[47] = 0;
  lpm_prefix18[48] = 0;
  lpm_prefix18[49] = 0;
  lpm_prefix18[50] = 0;
  lpm_prefix18[51] = 0;
  lpm_prefix18[52] = 0;
  lpm_prefix18[53] = 0;
  lpm_prefix18[54] = 0;
  lpm_prefix18[55] = 0;
  lpm_prefix18[56] = 0;
  lpm_prefix18[57] = 0;
  lpm_prefix18[58] = 0;
  lpm_prefix18[59] = 0;
  lpm_prefix18[60] = 0;
  lpm_prefix18[61] = 0;
  lpm_prefix18[62] = 0;
  lpm_prefix18[63] = 0;
  lpm_prefix18[64] = 0;
  int lpm_update_elem_success18 = lpm_update(lpm, lpm_prefix18, 392, 17);
  if (!lpm_update_elem_success18) {
    return false;
  }
  uint8_t lpm_prefix19[65];
  lpm_prefix19[0] = 3;
  lpm_prefix19[1] = 3;
  lpm_prefix19[2] = 99;
  lpm_prefix19[3] = 111;
  lpm_prefix19[4] = 109;
  lpm_prefix19[5] = 0;
  lpm_prefix19[6] = 0;
  lpm_prefix19[7] = 0;
  lpm_prefix19[8] = 0;
  lpm_prefix19[9] = 0;
  lpm_prefix19[10] = 0;
  lpm_prefix19[11] = 0;
  lpm_prefix19[12] = 0;
  lpm_prefix19[13] = 0;
  lpm_prefix19[14] = 0;
  lpm_prefix19[15] = 0;
  lpm_prefix19[16] = 0;
  lpm_prefix19[17] = 8;
  lpm_prefix19[18] = 108;
  lpm_prefix19[19] = 105;
  lpm_prefix19[20] = 110;
  lpm_prefix19[21] = 107;
  lpm_prefix19[22] = 101;
  lpm_prefix19[23] = 100;
  lpm_prefix19[24] = 105;
  lpm_prefix19[25] = 110;
  lpm_prefix19[26] = 0;
  lpm_prefix19[27] = 0;
  lpm_prefix19[28] = 0;
  lpm_prefix19[29] = 0;
  lpm_prefix19[30] = 0;
  lpm_prefix19[31] = 0;
  lpm_prefix19[32] = 0;
  lpm_prefix19[33] = 3;
  lpm_prefix19[34] = 119;
  lpm_prefix19[35] = 119;
  lpm_prefix19[36] = 119;
  lpm_prefix19[37] = 0;
  lpm_prefix19[38] = 0;
  lpm_prefix19[39] = 0;
  lpm_prefix19[40] = 0;
  lpm_prefix19[41] = 0;
  lpm_prefix19[42] = 0;
  lpm_prefix19[43] = 0;
  lpm_prefix19[44] = 0;
  lpm_prefix19[45] = 0;
  lpm_prefix19[46] = 0;
  lpm_prefix19[47] = 0;
  lpm_prefix19[48] = 0;
  lpm_prefix19[49] = 0;
  lpm_prefix19[50] = 0;
  lpm_prefix19[51] = 0;
  lpm_prefix19[52] = 0;
  lpm_prefix19[53] = 0;
  lpm_prefix19[54] = 0;
  lpm_prefix19[55] = 0;
  lpm_prefix19[56] = 0;
  lpm_prefix19[57] = 0;
  lpm_prefix19[58] = 0;
  lpm_prefix19[59] = 0;
  lpm_prefix19[60] = 0;
  lpm_prefix19[61] = 0;
  lpm_prefix19[62] = 0;
  lpm_prefix19[63] = 0;
  lpm_prefix19[64] = 0;
  int lpm_update_elem_success19 = lpm_update(lpm, lpm_prefix19, 392, 18);
  if (!lpm_update_elem_success19) {
    return false;
  }
  uint8_t lpm_prefix20[65];
  lpm_prefix20[0] = 3;
  lpm_prefix20[1] = 3;
  lpm_prefix20[2] = 99;
  lpm_prefix20[3] = 111;
  lpm_prefix20[4] = 109;
  lpm_prefix20[5] = 0;
  lpm_prefix20[6] = 0;
  lpm_prefix20[7] = 0;
  lpm_prefix20[8] = 0;
  lpm_prefix20[9] = 0;
  lpm_prefix20[10] = 0;
  lpm_prefix20[11] = 0;
  lpm_prefix20[12] = 0;
  lpm_prefix20[13] = 0;
  lpm_prefix20[14] = 0;
  lpm_prefix20[15] = 0;
  lpm_prefix20[16] = 0;
  lpm_prefix20[17] = 4;
  lpm_prefix20[18] = 98;
  lpm_prefix20[19] = 105;
  lpm_prefix20[20] = 110;
  lpm_prefix20[21] = 103;
  lpm_prefix20[22] = 0;
  lpm_prefix20[23] = 0;
  lpm_prefix20[24] = 0;
  lpm_prefix20[25] = 0;
  lpm_prefix20[26] = 0;
  lpm_prefix20[27] = 0;
  lpm_prefix20[28] = 0;
  lpm_prefix20[29] = 0;
  lpm_prefix20[30] = 0;
  lpm_prefix20[31] = 0;
  lpm_prefix20[32] = 0;
  lpm_prefix20[33] = 3;
  lpm_prefix20[34] = 119;
  lpm_prefix20[35] = 119;
  lpm_prefix20[36] = 119;
  lpm_prefix20[37] = 0;
  lpm_prefix20[38] = 0;
  lpm_prefix20[39] = 0;
  lpm_prefix20[40] = 0;
  lpm_prefix20[41] = 0;
  lpm_prefix20[42] = 0;
  lpm_prefix20[43] = 0;
  lpm_prefix20[44] = 0;
  lpm_prefix20[45] = 0;
  lpm_prefix20[46] = 0;
  lpm_prefix20[47] = 0;
  lpm_prefix20[48] = 0;
  lpm_prefix20[49] = 0;
  lpm_prefix20[50] = 0;
  lpm_prefix20[51] = 0;
  lpm_prefix20[52] = 0;
  lpm_prefix20[53] = 0;
  lpm_prefix20[54] = 0;
  lpm_prefix20[55] = 0;
  lpm_prefix20[56] = 0;
  lpm_prefix20[57] = 0;
  lpm_prefix20[58] = 0;
  lpm_prefix20[59] = 0;
  lpm_prefix20[60] = 0;
  lpm_prefix20[61] = 0;
  lpm_prefix20[62] = 0;
  lpm_prefix20[63] = 0;
  lpm_prefix20[64] = 0;
  int lpm_update_elem_success20 = lpm_update(lpm, lpm_prefix20, 392, 19);
  if (!lpm_update_elem_success20) {
    return false;
  }
  uint8_t lpm_prefix21[65];
  lpm_prefix21[0] = 3;
  lpm_prefix21[1] = 3;
  lpm_prefix21[2] = 99;
  lpm_prefix21[3] = 111;
  lpm_prefix21[4] = 109;
  lpm_prefix21[5] = 0;
  lpm_prefix21[6] = 0;
  lpm_prefix21[7] = 0;
  lpm_prefix21[8] = 0;
  lpm_prefix21[9] = 0;
  lpm_prefix21[10] = 0;
  lpm_prefix21[11] = 0;
  lpm_prefix21[12] = 0;
  lpm_prefix21[13] = 0;
  lpm_prefix21[14] = 0;
  lpm_prefix21[15] = 0;
  lpm_prefix21[16] = 0;
  lpm_prefix21[17] = 9;
  lpm_prefix21[18] = 109;
  lpm_prefix21[19] = 105;
  lpm_prefix21[20] = 99;
  lpm_prefix21[21] = 114;
  lpm_prefix21[22] = 111;
  lpm_prefix21[23] = 115;
  lpm_prefix21[24] = 111;
  lpm_prefix21[25] = 102;
  lpm_prefix21[26] = 116;
  lpm_prefix21[27] = 0;
  lpm_prefix21[28] = 0;
  lpm_prefix21[29] = 0;
  lpm_prefix21[30] = 0;
  lpm_prefix21[31] = 0;
  lpm_prefix21[32] = 0;
  lpm_prefix21[33] = 5;
  lpm_prefix21[34] = 116;
  lpm_prefix21[35] = 101;
  lpm_prefix21[36] = 97;
  lpm_prefix21[37] = 109;
  lpm_prefix21[38] = 115;
  lpm_prefix21[39] = 0;
  lpm_prefix21[40] = 0;
  lpm_prefix21[41] = 0;
  lpm_prefix21[42] = 0;
  lpm_prefix21[43] = 0;
  lpm_prefix21[44] = 0;
  lpm_prefix21[45] = 0;
  lpm_prefix21[46] = 0;
  lpm_prefix21[47] = 0;
  lpm_prefix21[48] = 0;
  lpm_prefix21[49] = 0;
  lpm_prefix21[50] = 0;
  lpm_prefix21[51] = 0;
  lpm_prefix21[52] = 0;
  lpm_prefix21[53] = 0;
  lpm_prefix21[54] = 0;
  lpm_prefix21[55] = 0;
  lpm_prefix21[56] = 0;
  lpm_prefix21[57] = 0;
  lpm_prefix21[58] = 0;
  lpm_prefix21[59] = 0;
  lpm_prefix21[60] = 0;
  lpm_prefix21[61] = 0;
  lpm_prefix21[62] = 0;
  lpm_prefix21[63] = 0;
  lpm_prefix21[64] = 0;
  int lpm_update_elem_success21 = lpm_update(lpm, lpm_prefix21, 392, 20);
  if (!lpm_update_elem_success21) {
    return false;
  }
  uint8_t lpm_prefix22[65];
  lpm_prefix22[0] = 3;
  lpm_prefix22[1] = 3;
  lpm_prefix22[2] = 99;
  lpm_prefix22[3] = 111;
  lpm_prefix22[4] = 109;
  lpm_prefix22[5] = 0;
  lpm_prefix22[6] = 0;
  lpm_prefix22[7] = 0;
  lpm_prefix22[8] = 0;
  lpm_prefix22[9] = 0;
  lpm_prefix22[10] = 0;
  lpm_prefix22[11] = 0;
  lpm_prefix22[12] = 0;
  lpm_prefix22[13] = 0;
  lpm_prefix22[14] = 0;
  lpm_prefix22[15] = 0;
  lpm_prefix22[16] = 0;
  lpm_prefix22[17] = 9;
  lpm_prefix22[18] = 111;
  lpm_prefix22[19] = 102;
  lpm_prefix22[20] = 102;
  lpm_prefix22[21] = 105;
  lpm_prefix22[22] = 99;
  lpm_prefix22[23] = 101;
  lpm_prefix22[24] = 51;
  lpm_prefix22[25] = 54;
  lpm_prefix22[26] = 53;
  lpm_prefix22[27] = 0;
  lpm_prefix22[28] = 0;
  lpm_prefix22[29] = 0;
  lpm_prefix22[30] = 0;
  lpm_prefix22[31] = 0;
  lpm_prefix22[32] = 0;
  lpm_prefix22[33] = 7;
  lpm_prefix22[34] = 111;
  lpm_prefix22[35] = 117;
  lpm_prefix22[36] = 116;
  lpm_prefix22[37] = 108;
  lpm_prefix22[38] = 111;
  lpm_prefix22[39] = 111;
  lpm_prefix22[40] = 107;
  lpm_prefix22[41] = 0;
  lpm_prefix22[42] = 0;
  lpm_prefix22[43] = 0;
  lpm_prefix22[44] = 0;
  lpm_prefix22[45] = 0;
  lpm_prefix22[46] = 0;
  lpm_prefix22[47] = 0;
  lpm_prefix22[48] = 0;
  lpm_prefix22[49] = 0;
  lpm_prefix22[50] = 0;
  lpm_prefix22[51] = 0;
  lpm_prefix22[52] = 0;
  lpm_prefix22[53] = 0;
  lpm_prefix22[54] = 0;
  lpm_prefix22[55] = 0;
  lpm_prefix22[56] = 0;
  lpm_prefix22[57] = 0;
  lpm_prefix22[58] = 0;
  lpm_prefix22[59] = 0;
  lpm_prefix22[60] = 0;
  lpm_prefix22[61] = 0;
  lpm_prefix22[62] = 0;
  lpm_prefix22[63] = 0;
  lpm_prefix22[64] = 0;
  int lpm_update_elem_success22 = lpm_update(lpm, lpm_prefix22, 392, 21);
  if (!lpm_update_elem_success22) {
    return false;
  }
  uint8_t lpm_prefix23[65];
  lpm_prefix23[0] = 2;
  lpm_prefix23[1] = 3;
  lpm_prefix23[2] = 99;
  lpm_prefix23[3] = 111;
  lpm_prefix23[4] = 109;
  lpm_prefix23[5] = 0;
  lpm_prefix23[6] = 0;
  lpm_prefix23[7] = 0;
  lpm_prefix23[8] = 0;
  lpm_prefix23[9] = 0;
  lpm_prefix23[10] = 0;
  lpm_prefix23[11] = 0;
  lpm_prefix23[12] = 0;
  lpm_prefix23[13] = 0;
  lpm_prefix23[14] = 0;
  lpm_prefix23[15] = 0;
  lpm_prefix23[16] = 0;
  lpm_prefix23[17] = 5;
  lpm_prefix23[18] = 115;
  lpm_prefix23[19] = 107;
  lpm_prefix23[20] = 121;
  lpm_prefix23[21] = 112;
  lpm_prefix23[22] = 101;
  lpm_prefix23[23] = 0;
  lpm_prefix23[24] = 0;
  lpm_prefix23[25] = 0;
  lpm_prefix23[26] = 0;
  lpm_prefix23[27] = 0;
  lpm_prefix23[28] = 0;
  lpm_prefix23[29] = 0;
  lpm_prefix23[30] = 0;
  lpm_prefix23[31] = 0;
  lpm_prefix23[32] = 0;
  lpm_prefix23[33] = 0;
  lpm_prefix23[34] = 0;
  lpm_prefix23[35] = 0;
  lpm_prefix23[36] = 0;
  lpm_prefix23[37] = 0;
  lpm_prefix23[38] = 0;
  lpm_prefix23[39] = 0;
  lpm_prefix23[40] = 0;
  lpm_prefix23[41] = 0;
  lpm_prefix23[42] = 0;
  lpm_prefix23[43] = 0;
  lpm_prefix23[44] = 0;
  lpm_prefix23[45] = 0;
  lpm_prefix23[46] = 0;
  lpm_prefix23[47] = 0;
  lpm_prefix23[48] = 0;
  lpm_prefix23[49] = 0;
  lpm_prefix23[50] = 0;
  lpm_prefix23[51] = 0;
  lpm_prefix23[52] = 0;
  lpm_prefix23[53] = 0;
  lpm_prefix23[54] = 0;
  lpm_prefix23[55] = 0;
  lpm_prefix23[56] = 0;
  lpm_prefix23[57] = 0;
  lpm_prefix23[58] = 0;
  lpm_prefix23[59] = 0;
  lpm_prefix23[60] = 0;
  lpm_prefix23[61] = 0;
  lpm_prefix23[62] = 0;
  lpm_prefix23[63] = 0;
  lpm_prefix23[64] = 0;
  int lpm_update_elem_success23 = lpm_update(lpm, lpm_prefix23, 264, 22);
  if (!lpm_update_elem_success23) {
    return false;
  }
  uint8_t lpm_prefix24[65];
  lpm_prefix24[0] = 3;
  lpm_prefix24[1] = 3;
  lpm_prefix24[2] = 99;
  lpm_prefix24[3] = 111;
  lpm_prefix24[4] = 109;
  lpm_prefix24[5] = 0;
  lpm_prefix24[6] = 0;
  lpm_prefix24[7] = 0;
  lpm_prefix24[8] = 0;
  lpm_prefix24[9] = 0;
  lpm_prefix24[10] = 0;
  lpm_prefix24[11] = 0;
  lpm_prefix24[12] = 0;
  lpm_prefix24[13] = 0;
  lpm_prefix24[14] = 0;
  lpm_prefix24[15] = 0;
  lpm_prefix24[16] = 0;
  lpm_prefix24[17] = 5;
  lpm_prefix24[18] = 115;
  lpm_prefix24[19] = 107;
  lpm_prefix24[20] = 121;
  lpm_prefix24[21] = 112;
  lpm_prefix24[22] = 101;
  lpm_prefix24[23] = 0;
  lpm_prefix24[24] = 0;
  lpm_prefix24[25] = 0;
  lpm_prefix24[26] = 0;
  lpm_prefix24[27] = 0;
  lpm_prefix24[28] = 0;
  lpm_prefix24[29] = 0;
  lpm_prefix24[30] = 0;
  lpm_prefix24[31] = 0;
  lpm_prefix24[32] = 0;
  lpm_prefix24[33] = 5;
  lpm_prefix24[34] = 116;
  lpm_prefix24[35] = 101;
  lpm_prefix24[36] = 97;
  lpm_prefix24[37] = 109;
  lpm_prefix24[38] = 115;
  lpm_prefix24[39] = 0;
  lpm_prefix24[40] = 0;
  lpm_prefix24[41] = 0;
  lpm_prefix24[42] = 0;
  lpm_prefix24[43] = 0;
  lpm_prefix24[44] = 0;
  lpm_prefix24[45] = 0;
  lpm_prefix24[46] = 0;
  lpm_prefix24[47] = 0;
  lpm_prefix24[48] = 0;
  lpm_prefix24[49] = 0;
  lpm_prefix24[50] = 0;
  lpm_prefix24[51] = 0;
  lpm_prefix24[52] = 0;
  lpm_prefix24[53] = 0;
  lpm_prefix24[54] = 0;
  lpm_prefix24[55] = 0;
  lpm_prefix24[56] = 0;
  lpm_prefix24[57] = 0;
  lpm_prefix24[58] = 0;
  lpm_prefix24[59] = 0;
  lpm_prefix24[60] = 0;
  lpm_prefix24[61] = 0;
  lpm_prefix24[62] = 0;
  lpm_prefix24[63] = 0;
  lpm_prefix24[64] = 0;
  int lpm_update_elem_success24 = lpm_update(lpm, lpm_prefix24, 392, 23);
  if (!lpm_update_elem_success24) {
    return false;
  }
  uint8_t lpm_prefix25[65];
  lpm_prefix25[0] = 4;
  lpm_prefix25[1] = 3;
  lpm_prefix25[2] = 99;
  lpm_prefix25[3] = 111;
  lpm_prefix25[4] = 109;
  lpm_prefix25[5] = 0;
  lpm_prefix25[6] = 0;
  lpm_prefix25[7] = 0;
  lpm_prefix25[8] = 0;
  lpm_prefix25[9] = 0;
  lpm_prefix25[10] = 0;
  lpm_prefix25[11] = 0;
  lpm_prefix25[12] = 0;
  lpm_prefix25[13] = 0;
  lpm_prefix25[14] = 0;
  lpm_prefix25[15] = 0;
  lpm_prefix25[16] = 0;
  lpm_prefix25[17] = 5;
  lpm_prefix25[18] = 115;
  lpm_prefix25[19] = 107;
  lpm_prefix25[20] = 121;
  lpm_prefix25[21] = 112;
  lpm_prefix25[22] = 101;
  lpm_prefix25[23] = 0;
  lpm_prefix25[24] = 0;
  lpm_prefix25[25] = 0;
  lpm_prefix25[26] = 0;
  lpm_prefix25[27] = 0;
  lpm_prefix25[28] = 0;
  lpm_prefix25[29] = 0;
  lpm_prefix25[30] = 0;
  lpm_prefix25[31] = 0;
  lpm_prefix25[32] = 0;
  lpm_prefix25[33] = 5;
  lpm_prefix25[34] = 116;
  lpm_prefix25[35] = 101;
  lpm_prefix25[36] = 97;
  lpm_prefix25[37] = 109;
  lpm_prefix25[38] = 115;
  lpm_prefix25[39] = 0;
  lpm_prefix25[40] = 0;
  lpm_prefix25[41] = 0;
  lpm_prefix25[42] = 0;
  lpm_prefix25[43] = 0;
  lpm_prefix25[44] = 0;
  lpm_prefix25[45] = 0;
  lpm_prefix25[46] = 0;
  lpm_prefix25[47] = 0;
  lpm_prefix25[48] = 0;
  lpm_prefix25[49] = 3;
  lpm_prefix25[50] = 97;
  lpm_prefix25[51] = 112;
  lpm_prefix25[52] = 105;
  lpm_prefix25[53] = 0;
  lpm_prefix25[54] = 0;
  lpm_prefix25[55] = 0;
  lpm_prefix25[56] = 0;
  lpm_prefix25[57] = 0;
  lpm_prefix25[58] = 0;
  lpm_prefix25[59] = 0;
  lpm_prefix25[60] = 0;
  lpm_prefix25[61] = 0;
  lpm_prefix25[62] = 0;
  lpm_prefix25[63] = 0;
  lpm_prefix25[64] = 0;
  int lpm_update_elem_success25 = lpm_update(lpm, lpm_prefix25, 520, 24);
  if (!lpm_update_elem_success25) {
    return false;
  }
  uint8_t lpm_prefix26[65];
  lpm_prefix26[0] = 4;
  lpm_prefix26[1] = 3;
  lpm_prefix26[2] = 99;
  lpm_prefix26[3] = 111;
  lpm_prefix26[4] = 109;
  lpm_prefix26[5] = 0;
  lpm_prefix26[6] = 0;
  lpm_prefix26[7] = 0;
  lpm_prefix26[8] = 0;
  lpm_prefix26[9] = 0;
  lpm_prefix26[10] = 0;
  lpm_prefix26[11] = 0;
  lpm_prefix26[12] = 0;
  lpm_prefix26[13] = 0;
  lpm_prefix26[14] = 0;
  lpm_prefix26[15] = 0;
  lpm_prefix26[16] = 0;
  lpm_prefix26[17] = 5;
  lpm_prefix26[18] = 115;
  lpm_prefix26[19] = 107;
  lpm_prefix26[20] = 121;
  lpm_prefix26[21] = 112;
  lpm_prefix26[22] = 101;
  lpm_prefix26[23] = 0;
  lpm_prefix26[24] = 0;
  lpm_prefix26[25] = 0;
  lpm_prefix26[26] = 0;
  lpm_prefix26[27] = 0;
  lpm_prefix26[28] = 0;
  lpm_prefix26[29] = 0;
  lpm_prefix26[30] = 0;
  lpm_prefix26[31] = 0;
  lpm_prefix26[32] = 0;
  lpm_prefix26[33] = 5;
  lpm_prefix26[34] = 116;
  lpm_prefix26[35] = 101;
  lpm_prefix26[36] = 97;
  lpm_prefix26[37] = 109;
  lpm_prefix26[38] = 115;
  lpm_prefix26[39] = 0;
  lpm_prefix26[40] = 0;
  lpm_prefix26[41] = 0;
  lpm_prefix26[42] = 0;
  lpm_prefix26[43] = 0;
  lpm_prefix26[44] = 0;
  lpm_prefix26[45] = 0;
  lpm_prefix26[46] = 0;
  lpm_prefix26[47] = 0;
  lpm_prefix26[48] = 0;
  lpm_prefix26[49] = 3;
  lpm_prefix26[50] = 105;
  lpm_prefix26[51] = 109;
  lpm_prefix26[52] = 103;
  lpm_prefix26[53] = 0;
  lpm_prefix26[54] = 0;
  lpm_prefix26[55] = 0;
  lpm_prefix26[56] = 0;
  lpm_prefix26[57] = 0;
  lpm_prefix26[58] = 0;
  lpm_prefix26[59] = 0;
  lpm_prefix26[60] = 0;
  lpm_prefix26[61] = 0;
  lpm_prefix26[62] = 0;
  lpm_prefix26[63] = 0;
  lpm_prefix26[64] = 0;
  int lpm_update_elem_success26 = lpm_update(lpm, lpm_prefix26, 520, 25);
  if (!lpm_update_elem_success26) {
    return false;
  }
  uint8_t lpm_prefix27[65];
  lpm_prefix27[0] = 2;
  lpm_prefix27[1] = 3;
  lpm_prefix27[2] = 99;
  lpm_prefix27[3] = 111;
  lpm_prefix27[4] = 109;
  lpm_prefix27[5] = 0;
  lpm_prefix27[6] = 0;
  lpm_prefix27[7] = 0;
  lpm_prefix27[8] = 0;
  lpm_prefix27[9] = 0;
  lpm_prefix27[10] = 0;
  lpm_prefix27[11] = 0;
  lpm_prefix27[12] = 0;
  lpm_prefix27[13] = 0;
  lpm_prefix27[14] = 0;
  lpm_prefix27[15] = 0;
  lpm_prefix27[16] = 0;
  lpm_prefix27[17] = 5;
  lpm_prefix27[18] = 119;
  lpm_prefix27[19] = 101;
  lpm_prefix27[20] = 98;
  lpm_prefix27[21] = 101;
  lpm_prefix27[22] = 120;
  lpm_prefix27[23] = 0;
  lpm_prefix27[24] = 0;
  lpm_prefix27[25] = 0;
  lpm_prefix27[26] = 0;
  lpm_prefix27[27] = 0;
  lpm_prefix27[28] = 0;
  lpm_prefix27[29] = 0;
  lpm_prefix27[30] = 0;
  lpm_prefix27[31] = 0;
  lpm_prefix27[32] = 0;
  lpm_prefix27[33] = 0;
  lpm_prefix27[34] = 0;
  lpm_prefix27[35] = 0;
  lpm_prefix27[36] = 0;
  lpm_prefix27[37] = 0;
  lpm_prefix27[38] = 0;
  lpm_prefix27[39] = 0;
  lpm_prefix27[40] = 0;
  lpm_prefix27[41] = 0;
  lpm_prefix27[42] = 0;
  lpm_prefix27[43] = 0;
  lpm_prefix27[44] = 0;
  lpm_prefix27[45] = 0;
  lpm_prefix27[46] = 0;
  lpm_prefix27[47] = 0;
  lpm_prefix27[48] = 0;
  lpm_prefix27[49] = 0;
  lpm_prefix27[50] = 0;
  lpm_prefix27[51] = 0;
  lpm_prefix27[52] = 0;
  lpm_prefix27[53] = 0;
  lpm_prefix27[54] = 0;
  lpm_prefix27[55] = 0;
  lpm_prefix27[56] = 0;
  lpm_prefix27[57] = 0;
  lpm_prefix27[58] = 0;
  lpm_prefix27[59] = 0;
  lpm_prefix27[60] = 0;
  lpm_prefix27[61] = 0;
  lpm_prefix27[62] = 0;
  lpm_prefix27[63] = 0;
  lpm_prefix27[64] = 0;
  int lpm_update_elem_success27 = lpm_update(lpm, lpm_prefix27, 264, 26);
  if (!lpm_update_elem_success27) {
    return false;
  }
  uint8_t lpm_prefix28[65];
  lpm_prefix28[0] = 2;
  lpm_prefix28[1] = 2;
  lpm_prefix28[2] = 117;
  lpm_prefix28[3] = 115;
  lpm_prefix28[4] = 0;
  lpm_prefix28[5] = 0;
  lpm_prefix28[6] = 0;
  lpm_prefix28[7] = 0;
  lpm_prefix28[8] = 0;
  lpm_prefix28[9] = 0;
  lpm_prefix28[10] = 0;
  lpm_prefix28[11] = 0;
  lpm_prefix28[12] = 0;
  lpm_prefix28[13] = 0;
  lpm_prefix28[14] = 0;
  lpm_prefix28[15] = 0;
  lpm_prefix28[16] = 0;
  lpm_prefix28[17] = 4;
  lpm_prefix28[18] = 122;
  lpm_prefix28[19] = 111;
  lpm_prefix28[20] = 111;
  lpm_prefix28[21] = 109;
  lpm_prefix28[22] = 0;
  lpm_prefix28[23] = 0;
  lpm_prefix28[24] = 0;
  lpm_prefix28[25] = 0;
  lpm_prefix28[26] = 0;
  lpm_prefix28[27] = 0;
  lpm_prefix28[28] = 0;
  lpm_prefix28[29] = 0;
  lpm_prefix28[30] = 0;
  lpm_prefix28[31] = 0;
  lpm_prefix28[32] = 0;
  lpm_prefix28[33] = 0;
  lpm_prefix28[34] = 0;
  lpm_prefix28[35] = 0;
  lpm_prefix28[36] = 0;
  lpm_prefix28[37] = 0;
  lpm_prefix28[38] = 0;
  lpm_prefix28[39] = 0;
  lpm_prefix28[40] = 0;
  lpm_prefix28[41] = 0;
  lpm_prefix28[42] = 0;
  lpm_prefix28[43] = 0;
  lpm_prefix28[44] = 0;
  lpm_prefix28[45] = 0;
  lpm_prefix28[46] = 0;
  lpm_prefix28[47] = 0;
  lpm_prefix28[48] = 0;
  lpm_prefix28[49] = 0;
  lpm_prefix28[50] = 0;
  lpm_prefix28[51] = 0;
  lpm_prefix28[52] = 0;
  lpm_prefix28[53] = 0;
  lpm_prefix28[54] = 0;
  lpm_prefix28[55] = 0;
  lpm_prefix28[56] = 0;
  lpm_prefix28[57] = 0;
  lpm_prefix28[58] = 0;
  lpm_prefix28[59] = 0;
  lpm_prefix28[60] = 0;
  lpm_prefix28[61] = 0;
  lpm_prefix28[62] = 0;
  lpm_prefix28[63] = 0;
  lpm_prefix28[64] = 0;
  int lpm_update_elem_success28 = lpm_update(lpm, lpm_prefix28, 264, 27);
  if (!lpm_update_elem_success28) {
    return false;
  }
  uint8_t lpm_prefix29[65];
  lpm_prefix29[0] = 2;
  lpm_prefix29[1] = 4;
  lpm_prefix29[2] = 116;
  lpm_prefix29[3] = 111;
  lpm_prefix29[4] = 119;
  lpm_prefix29[5] = 110;
  lpm_prefix29[6] = 0;
  lpm_prefix29[7] = 0;
  lpm_prefix29[8] = 0;
  lpm_prefix29[9] = 0;
  lpm_prefix29[10] = 0;
  lpm_prefix29[11] = 0;
  lpm_prefix29[12] = 0;
  lpm_prefix29[13] = 0;
  lpm_prefix29[14] = 0;
  lpm_prefix29[15] = 0;
  lpm_prefix29[16] = 0;
  lpm_prefix29[17] = 6;
  lpm_prefix29[18] = 103;
  lpm_prefix29[19] = 97;
  lpm_prefix29[20] = 116;
  lpm_prefix29[21] = 104;
  lpm_prefix29[22] = 101;
  lpm_prefix29[23] = 114;
  lpm_prefix29[24] = 0;
  lpm_prefix29[25] = 0;
  lpm_prefix29[26] = 0;
  lpm_prefix29[27] = 0;
  lpm_prefix29[28] = 0;
  lpm_prefix29[29] = 0;
  lpm_prefix29[30] = 0;
  lpm_prefix29[31] = 0;
  lpm_prefix29[32] = 0;
  lpm_prefix29[33] = 0;
  lpm_prefix29[34] = 0;
  lpm_prefix29[35] = 0;
  lpm_prefix29[36] = 0;
  lpm_prefix29[37] = 0;
  lpm_prefix29[38] = 0;
  lpm_prefix29[39] = 0;
  lpm_prefix29[40] = 0;
  lpm_prefix29[41] = 0;
  lpm_prefix29[42] = 0;
  lpm_prefix29[43] = 0;
  lpm_prefix29[44] = 0;
  lpm_prefix29[45] = 0;
  lpm_prefix29[46] = 0;
  lpm_prefix29[47] = 0;
  lpm_prefix29[48] = 0;
  lpm_prefix29[49] = 0;
  lpm_prefix29[50] = 0;
  lpm_prefix29[51] = 0;
  lpm_prefix29[52] = 0;
  lpm_prefix29[53] = 0;
  lpm_prefix29[54] = 0;
  lpm_prefix29[55] = 0;
  lpm_prefix29[56] = 0;
  lpm_prefix29[57] = 0;
  lpm_prefix29[58] = 0;
  lpm_prefix29[59] = 0;
  lpm_prefix29[60] = 0;
  lpm_prefix29[61] = 0;
  lpm_prefix29[62] = 0;
  lpm_prefix29[63] = 0;
  lpm_prefix29[64] = 0;
  int lpm_update_elem_success29 = lpm_update(lpm, lpm_prefix29, 264, 28);
  if (!lpm_update_elem_success29) {
    return false;
  }
  uint8_t lpm_prefix30[65];
  lpm_prefix30[0] = 2;
  lpm_prefix30[1] = 2;
  lpm_prefix30[2] = 105;
  lpm_prefix30[3] = 111;
  lpm_prefix30[4] = 0;
  lpm_prefix30[5] = 0;
  lpm_prefix30[6] = 0;
  lpm_prefix30[7] = 0;
  lpm_prefix30[8] = 0;
  lpm_prefix30[9] = 0;
  lpm_prefix30[10] = 0;
  lpm_prefix30[11] = 0;
  lpm_prefix30[12] = 0;
  lpm_prefix30[13] = 0;
  lpm_prefix30[14] = 0;
  lpm_prefix30[15] = 0;
  lpm_prefix30[16] = 0;
  lpm_prefix30[17] = 8;
  lpm_prefix30[18] = 103;
  lpm_prefix30[19] = 97;
  lpm_prefix30[20] = 116;
  lpm_prefix30[21] = 104;
  lpm_prefix30[22] = 101;
  lpm_prefix30[23] = 114;
  lpm_prefix30[24] = 108;
  lpm_prefix30[25] = 121;
  lpm_prefix30[26] = 0;
  lpm_prefix30[27] = 0;
  lpm_prefix30[28] = 0;
  lpm_prefix30[29] = 0;
  lpm_prefix30[30] = 0;
  lpm_prefix30[31] = 0;
  lpm_prefix30[32] = 0;
  lpm_prefix30[33] = 0;
  lpm_prefix30[34] = 0;
  lpm_prefix30[35] = 0;
  lpm_prefix30[36] = 0;
  lpm_prefix30[37] = 0;
  lpm_prefix30[38] = 0;
  lpm_prefix30[39] = 0;
  lpm_prefix30[40] = 0;
  lpm_prefix30[41] = 0;
  lpm_prefix30[42] = 0;
  lpm_prefix30[43] = 0;
  lpm_prefix30[44] = 0;
  lpm_prefix30[45] = 0;
  lpm_prefix30[46] = 0;
  lpm_prefix30[47] = 0;
  lpm_prefix30[48] = 0;
  lpm_prefix30[49] = 0;
  lpm_prefix30[50] = 0;
  lpm_prefix30[51] = 0;
  lpm_prefix30[52] = 0;
  lpm_prefix30[53] = 0;
  lpm_prefix30[54] = 0;
  lpm_prefix30[55] = 0;
  lpm_prefix30[56] = 0;
  lpm_prefix30[57] = 0;
  lpm_prefix30[58] = 0;
  lpm_prefix30[59] = 0;
  lpm_prefix30[60] = 0;
  lpm_prefix30[61] = 0;
  lpm_prefix30[62] = 0;
  lpm_prefix30[63] = 0;
  lpm_prefix30[64] = 0;
  int lpm_update_elem_success30 = lpm_update(lpm, lpm_prefix30, 264, 29);
  if (!lpm_update_elem_success30) {
    return false;
  }
  uint8_t lpm_prefix31[65];
  lpm_prefix31[0] = 3;
  lpm_prefix31[1] = 3;
  lpm_prefix31[2] = 99;
  lpm_prefix31[3] = 111;
  lpm_prefix31[4] = 109;
  lpm_prefix31[5] = 0;
  lpm_prefix31[6] = 0;
  lpm_prefix31[7] = 0;
  lpm_prefix31[8] = 0;
  lpm_prefix31[9] = 0;
  lpm_prefix31[10] = 0;
  lpm_prefix31[11] = 0;
  lpm_prefix31[12] = 0;
  lpm_prefix31[13] = 0;
  lpm_prefix31[14] = 0;
  lpm_prefix31[15] = 0;
  lpm_prefix31[16] = 0;
  lpm_prefix31[17] = 7;
  lpm_prefix31[18] = 103;
  lpm_prefix31[19] = 115;
  lpm_prefix31[20] = 116;
  lpm_prefix31[21] = 97;
  lpm_prefix31[22] = 116;
  lpm_prefix31[23] = 105;
  lpm_prefix31[24] = 99;
  lpm_prefix31[25] = 0;
  lpm_prefix31[26] = 0;
  lpm_prefix31[27] = 0;
  lpm_prefix31[28] = 0;
  lpm_prefix31[29] = 0;
  lpm_prefix31[30] = 0;
  lpm_prefix31[31] = 0;
  lpm_prefix31[32] = 0;
  lpm_prefix31[33] = 3;
  lpm_prefix31[34] = 119;
  lpm_prefix31[35] = 119;
  lpm_prefix31[36] = 119;
  lpm_prefix31[37] = 0;
  lpm_prefix31[38] = 0;
  lpm_prefix31[39] = 0;
  lpm_prefix31[40] = 0;
  lpm_prefix31[41] = 0;
  lpm_prefix31[42] = 0;
  lpm_prefix31[43] = 0;
  lpm_prefix31[44] = 0;
  lpm_prefix31[45] = 0;
  lpm_prefix31[46] = 0;
  lpm_prefix31[47] = 0;
  lpm_prefix31[48] = 0;
  lpm_prefix31[49] = 0;
  lpm_prefix31[50] = 0;
  lpm_prefix31[51] = 0;
  lpm_prefix31[52] = 0;
  lpm_prefix31[53] = 0;
  lpm_prefix31[54] = 0;
  lpm_prefix31[55] = 0;
  lpm_prefix31[56] = 0;
  lpm_prefix31[57] = 0;
  lpm_prefix31[58] = 0;
  lpm_prefix31[59] = 0;
  lpm_prefix31[60] = 0;
  lpm_prefix31[61] = 0;
  lpm_prefix31[62] = 0;
  lpm_prefix31[63] = 0;
  lpm_prefix31[64] = 0;
  int lpm_update_elem_success31 = lpm_update(lpm, lpm_prefix31, 392, 30);
  if (!lpm_update_elem_success31) {
    return false;
  }
  uint8_t lpm_prefix32[65];
  lpm_prefix32[0] = 3;
  lpm_prefix32[1] = 3;
  lpm_prefix32[2] = 99;
  lpm_prefix32[3] = 111;
  lpm_prefix32[4] = 109;
  lpm_prefix32[5] = 0;
  lpm_prefix32[6] = 0;
  lpm_prefix32[7] = 0;
  lpm_prefix32[8] = 0;
  lpm_prefix32[9] = 0;
  lpm_prefix32[10] = 0;
  lpm_prefix32[11] = 0;
  lpm_prefix32[12] = 0;
  lpm_prefix32[13] = 0;
  lpm_prefix32[14] = 0;
  lpm_prefix32[15] = 0;
  lpm_prefix32[16] = 0;
  lpm_prefix32[17] = 9;
  lpm_prefix32[18] = 98;
  lpm_prefix32[19] = 108;
  lpm_prefix32[20] = 111;
  lpm_prefix32[21] = 111;
  lpm_prefix32[22] = 109;
  lpm_prefix32[23] = 98;
  lpm_prefix32[24] = 101;
  lpm_prefix32[25] = 114;
  lpm_prefix32[26] = 103;
  lpm_prefix32[27] = 0;
  lpm_prefix32[28] = 0;
  lpm_prefix32[29] = 0;
  lpm_prefix32[30] = 0;
  lpm_prefix32[31] = 0;
  lpm_prefix32[32] = 0;
  lpm_prefix32[33] = 3;
  lpm_prefix32[34] = 119;
  lpm_prefix32[35] = 119;
  lpm_prefix32[36] = 119;
  lpm_prefix32[37] = 0;
  lpm_prefix32[38] = 0;
  lpm_prefix32[39] = 0;
  lpm_prefix32[40] = 0;
  lpm_prefix32[41] = 0;
  lpm_prefix32[42] = 0;
  lpm_prefix32[43] = 0;
  lpm_prefix32[44] = 0;
  lpm_prefix32[45] = 0;
  lpm_prefix32[46] = 0;
  lpm_prefix32[47] = 0;
  lpm_prefix32[48] = 0;
  lpm_prefix32[49] = 0;
  lpm_prefix32[50] = 0;
  lpm_prefix32[51] = 0;
  lpm_prefix32[52] = 0;
  lpm_prefix32[53] = 0;
  lpm_prefix32[54] = 0;
  lpm_prefix32[55] = 0;
  lpm_prefix32[56] = 0;
  lpm_prefix32[57] = 0;
  lpm_prefix32[58] = 0;
  lpm_prefix32[59] = 0;
  lpm_prefix32[60] = 0;
  lpm_prefix32[61] = 0;
  lpm_prefix32[62] = 0;
  lpm_prefix32[63] = 0;
  lpm_prefix32[64] = 0;
  int lpm_update_elem_success32 = lpm_update(lpm, lpm_prefix32, 392, 31);
  if (!lpm_update_elem_success32) {
    return false;
  }
  uint8_t lpm_prefix33[4];
  lpm_prefix33[0] = 10;
  lpm_prefix33[1] = 9;
  lpm_prefix33[2] = 0;
  lpm_prefix33[3] = 0;
  int lpm_update_elem_success33 = lpm_update(lpm2, lpm_prefix33, 24, 1);
  if (!lpm_update_elem_success33) {
    return false;
  }
  ports.push_back(31);
  ports.push_back(30);
  ports.push_back(29);
  ports.push_back(12);
  ports.push_back(11);
  ports.push_back(10);
  ports.push_back(9);
  ports.push_back(8);
  ports.push_back(7);
  ports.push_back(6);
  ports.push_back(5);
  ports.push_back(4);
  ports.push_back(3);
  ports.push_back(2);
  ports.push_back(1);
  ports.push_back(0);
  ports.push_back(13);
  ports.push_back(14);
  ports.push_back(15);
  ports.push_back(16);
  ports.push_back(17);
  ports.push_back(18);
  ports.push_back(19);
  ports.push_back(20);
  ports.push_back(21);
  ports.push_back(22);
  ports.push_back(23);
  ports.push_back(24);
  ports.push_back(25);
  ports.push_back(26);
  ports.push_back(27);
  ports.push_back(28);
  stats_per_map[1074083024ULL].init(136);
  stats_per_map[1074083024ULL].init(101);
  stats_per_map[1074083024ULL].init(89);
  stats_per_map[1074083024ULL].init(50);
  forwarding_stats_per_route_op.insert({152, PortStats{}});
  forwarding_stats_per_route_op.insert({66, PortStats{}});
  forwarding_stats_per_route_op.insert({131, PortStats{}});
  forwarding_stats_per_route_op.insert({55, PortStats{}});
  forwarding_stats_per_route_op.insert({84, PortStats{}});
  forwarding_stats_per_route_op.insert({99, PortStats{}});
  forwarding_stats_per_route_op.insert({109, PortStats{}});
  forwarding_stats_per_route_op.insert({122, PortStats{}});
  forwarding_stats_per_route_op.insert({77, PortStats{}});
  forwarding_stats_per_route_op.insert({135, PortStats{}});
  forwarding_stats_per_route_op.insert({117, PortStats{}});
  forwarding_stats_per_route_op.insert({127, PortStats{}});
  forwarding_stats_per_route_op.insert({140, PortStats{}});
  forwarding_stats_per_route_op.insert({150, PortStats{}});
  node_pkt_counter.insert({152, 0});
  node_pkt_counter.insert({151, 0});
  node_pkt_counter.insert({150, 0});
  node_pkt_counter.insert({149, 0});
  node_pkt_counter.insert({148, 0});
  node_pkt_counter.insert({147, 0});
  node_pkt_counter.insert({146, 0});
  node_pkt_counter.insert({145, 0});
  node_pkt_counter.insert({144, 0});
  node_pkt_counter.insert({143, 0});
  node_pkt_counter.insert({142, 0});
  node_pkt_counter.insert({141, 0});
  node_pkt_counter.insert({140, 0});
  node_pkt_counter.insert({139, 0});
  node_pkt_counter.insert({138, 0});
  node_pkt_counter.insert({137, 0});
  node_pkt_counter.insert({136, 0});
  node_pkt_counter.insert({135, 0});
  node_pkt_counter.insert({134, 0});
  node_pkt_counter.insert({133, 0});
  node_pkt_counter.insert({132, 0});
  node_pkt_counter.insert({131, 0});
  node_pkt_counter.insert({130, 0});
  node_pkt_counter.insert({129, 0});
  node_pkt_counter.insert({128, 0});
  node_pkt_counter.insert({127, 0});
  node_pkt_counter.insert({126, 0});
  node_pkt_counter.insert({125, 0});
  node_pkt_counter.insert({124, 0});
  node_pkt_counter.insert({123, 0});
  node_pkt_counter.insert({122, 0});
  node_pkt_counter.insert({121, 0});
  node_pkt_counter.insert({120, 0});
  node_pkt_counter.insert({119, 0});
  node_pkt_counter.insert({118, 0});
  node_pkt_counter.insert({117, 0});
  node_pkt_counter.insert({116, 0});
  node_pkt_counter.insert({115, 0});
  node_pkt_counter.insert({114, 0});
  node_pkt_counter.insert({113, 0});
  node_pkt_counter.insert({112, 0});
  node_pkt_counter.insert({111, 0});
  node_pkt_counter.insert({110, 0});
  node_pkt_counter.insert({109, 0});
  node_pkt_counter.insert({108, 0});
  node_pkt_counter.insert({107, 0});
  node_pkt_counter.insert({106, 0});
  node_pkt_counter.insert({105, 0});
  node_pkt_counter.insert({104, 0});
  node_pkt_counter.insert({103, 0});
  node_pkt_counter.insert({102, 0});
  node_pkt_counter.insert({71, 0});
  node_pkt_counter.insert({70, 0});
  node_pkt_counter.insert({69, 0});
  node_pkt_counter.insert({68, 0});
  node_pkt_counter.insert({67, 0});
  node_pkt_counter.insert({66, 0});
  node_pkt_counter.insert({65, 0});
  node_pkt_counter.insert({64, 0});
  node_pkt_counter.insert({63, 0});
  node_pkt_counter.insert({62, 0});
  node_pkt_counter.insert({61, 0});
  node_pkt_counter.insert({60, 0});
  node_pkt_counter.insert({59, 0});
  node_pkt_counter.insert({58, 0});
  node_pkt_counter.insert({57, 0});
  node_pkt_counter.insert({56, 0});
  node_pkt_counter.insert({43, 0});
  node_pkt_counter.insert({44, 0});
  node_pkt_counter.insert({45, 0});
  node_pkt_counter.insert({46, 0});
  node_pkt_counter.insert({47, 0});
  node_pkt_counter.insert({48, 0});
  node_pkt_counter.insert({49, 0});
  node_pkt_counter.insert({50, 0});
  node_pkt_counter.insert({51, 0});
  node_pkt_counter.insert({52, 0});
  node_pkt_counter.insert({53, 0});
  node_pkt_counter.insert({54, 0});
  node_pkt_counter.insert({55, 0});
  node_pkt_counter.insert({72, 0});
  node_pkt_counter.insert({73, 0});
  node_pkt_counter.insert({74, 0});
  node_pkt_counter.insert({75, 0});
  node_pkt_counter.insert({76, 0});
  node_pkt_counter.insert({77, 0});
  node_pkt_counter.insert({78, 0});
  node_pkt_counter.insert({79, 0});
  node_pkt_counter.insert({80, 0});
  node_pkt_counter.insert({81, 0});
  node_pkt_counter.insert({82, 0});
  node_pkt_counter.insert({83, 0});
  node_pkt_counter.insert({84, 0});
  node_pkt_counter.insert({85, 0});
  node_pkt_counter.insert({86, 0});
  node_pkt_counter.insert({87, 0});
  node_pkt_counter.insert({88, 0});
  node_pkt_counter.insert({89, 0});
  node_pkt_counter.insert({90, 0});
  node_pkt_counter.insert({91, 0});
  node_pkt_counter.insert({92, 0});
  node_pkt_counter.insert({93, 0});
  node_pkt_counter.insert({94, 0});
  node_pkt_counter.insert({95, 0});
  node_pkt_counter.insert({96, 0});
  node_pkt_counter.insert({97, 0});
  node_pkt_counter.insert({98, 0});
  node_pkt_counter.insert({99, 0});
  node_pkt_counter.insert({100, 0});
  node_pkt_counter.insert({101, 0});
  return true;
}


int nf_process(uint16_t device, uint8_t *buffer, uint16_t packet_length, time_ns_t now) {
  // BDDNode 43
  inc_path_counter(43);
  int freed_flows = profiler_expire_items_single_map(dchain, vector, map, now);
  expiration_tracker.update(freed_flows, now);
  // BDDNode 44
  inc_path_counter(44);
  uint8_t* hdr;
  packet_borrow_next_chunk(buffer, 14, (void**)&hdr);
  // BDDNode 45
  inc_path_counter(45);
  if (((8) == (*(uint16_t*)(uint16_t*)(hdr+12))) & ((20ULL) <= ((uint16_t)((uint32_t)((4294967282) + ((uint16_t)(packet_length & 65535))))))) {
    // BDDNode 46
    inc_path_counter(46);
    uint8_t* hdr2;
    packet_borrow_next_chunk(buffer, 20, (void**)&hdr2);
    // BDDNode 47
    inc_path_counter(47);
    if (((17) == (*(hdr2+9))) & ((8ULL) <= ((uint32_t)((4294967262) + ((uint16_t)(packet_length & 65535)))))) {
      // BDDNode 48
      inc_path_counter(48);
      uint8_t* hdr3;
      packet_borrow_next_chunk(buffer, 8, (void**)&hdr3);
      // BDDNode 49
      inc_path_counter(49);
      if (((13568) != (*(uint16_t*)(uint16_t*)(hdr3+2))) & ((13568) != (*(uint16_t*)(uint16_t*)(hdr3+0)))) {
        // BDDNode 50
        inc_path_counter(50);
        uint8_t key[8];
        key[0] = *(hdr2+16);
        key[1] = *(hdr2+17);
        key[2] = *(hdr2+18);
        key[3] = *(hdr2+19);
        key[4] = *(hdr2+12);
        key[5] = *(hdr2+13);
        key[6] = *(hdr2+14);
        key[7] = *(hdr2+15);
        int value;
        int map_hit = map_get(map, key, &value);
        stats_per_map[1074083024ULL].update(50, key, 8, now);
        // BDDNode 51
        inc_path_counter(51);
        if ((0) == (map_hit)) {
          // BDDNode 52
          inc_path_counter(52);
          packet_return_chunk(buffer, hdr3);
          // BDDNode 53
          inc_path_counter(53);
          packet_return_chunk(buffer, hdr2);
          // BDDNode 54
          inc_path_counter(54);
          packet_return_chunk(buffer, hdr);
          // BDDNode 55
          inc_path_counter(55);
          forwarding_stats_per_route_op[55].inc_fwd(device & 65535);
          return device & 65535;
        } else {
          // BDDNode 56
          inc_path_counter(56);
          dchain_rejuvenate_index(dchain, value, now);
          // BDDNode 57
          inc_path_counter(57);
          uint8_t* vector_cell = 0;
          vector_borrow(vector2, value, (void**)&vector_cell);
          uint32_t vector_value_out = *(uint32_t*)vector_cell;
          // BDDNode 58
          inc_path_counter(58);
          // BDDNode 59
          inc_path_counter(59);
          uint8_t* vector_cell2 = 0;
          vector_borrow(vector5, vector_value_out, (void**)&vector_cell2);
          uint32_t vector_value_out2 = *(uint32_t*)vector_cell2;
          // BDDNode 60
          inc_path_counter(60);
          *(uint32_t*)vector_cell2 = (1) + (vector_value_out2);
          // BDDNode 61
          inc_path_counter(61);
          uint8_t* vector_cell3 = 0;
          vector_borrow(vector6, vector_value_out, (void**)&vector_cell3);
          uint32_t vector_value_out3 = *(uint32_t*)vector_cell3;
          // BDDNode 62
          inc_path_counter(62);
          *(uint32_t*)vector_cell3 = (vector_value_out3) + ((uint16_t)(packet_length & 65535));
          // BDDNode 63
          inc_path_counter(63);
          packet_return_chunk(buffer, hdr3);
          // BDDNode 64
          inc_path_counter(64);
          packet_return_chunk(buffer, hdr2);
          // BDDNode 65
          inc_path_counter(65);
          packet_return_chunk(buffer, hdr);
          // BDDNode 66
          inc_path_counter(66);
          forwarding_stats_per_route_op[66].inc_fwd(device & 65535);
          return device & 65535;
        } // (0) == (map_hit)
      } else {
        // BDDNode 67
        inc_path_counter(67);
        if ((20ULL) <= ((uint16_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5)))))) {
          // BDDNode 68
          inc_path_counter(68);
          if (((uint16_t)(((255) < ((uint16_t)((uint64_t)((-8LL) + ((uint16_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5))))))))) ? (255) : ((uint64_t)((-8LL) + ((uint16_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5))))))))) <= ((4294967254) + ((uint16_t)(packet_length & 65535)))) {
            // BDDNode 69
            inc_path_counter(69);
            uint8_t* hdr4;
            packet_borrow_next_chunk(buffer, (uint16_t)(((255) < ((uint16_t)((uint64_t)((-8LL) + ((uint16_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5))))))))) ? (255) : ((uint64_t)((-8LL) + ((uint16_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5)))))))), (void**)&hdr4);
            // BDDNode 70
            inc_path_counter(70);
            if ((0) != (((uint8_t)(*(hdr4+2))) & (128))) {
              // BDDNode 71
              inc_path_counter(71);
              uint8_t dns_name[65];
              uint8_t dns_address[4];
              int dns_response_found = dns_get_response((struct dns_hdr*)hdr4, ((255) < ((uint16_t)((uint64_t)((-8LL) + ((uint16_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5))))))))) ? (255) : ((uint64_t)((-8LL) + ((uint16_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5))))))), (struct dns_name*)dns_name, (uint32_t*)dns_address);
              // BDDNode 72
              inc_path_counter(72);
              if ((0) == (dns_response_found)) {
                // BDDNode 73
                inc_path_counter(73);
                packet_return_chunk(buffer, hdr4);
                // BDDNode 74
                inc_path_counter(74);
                packet_return_chunk(buffer, hdr3);
                // BDDNode 75
                inc_path_counter(75);
                packet_return_chunk(buffer, hdr2);
                // BDDNode 76
                inc_path_counter(76);
                packet_return_chunk(buffer, hdr);
                // BDDNode 77
                inc_path_counter(77);
                forwarding_stats_per_route_op[77].inc_fwd(device & 65535);
                return device & 65535;
              } else {
                // BDDNode 78
                inc_path_counter(78);
                int lpm_value;
                int lpm_lookup_match = lpm_lookup(lpm, dns_name, &lpm_value);
                // BDDNode 79
                inc_path_counter(79);
                if ((0) == (lpm_lookup_match)) {
                  // BDDNode 80
                  inc_path_counter(80);
                  packet_return_chunk(buffer, hdr4);
                  // BDDNode 81
                  inc_path_counter(81);
                  packet_return_chunk(buffer, hdr3);
                  // BDDNode 82
                  inc_path_counter(82);
                  packet_return_chunk(buffer, hdr2);
                  // BDDNode 83
                  inc_path_counter(83);
                  packet_return_chunk(buffer, hdr);
                  // BDDNode 84
                  inc_path_counter(84);
                  forwarding_stats_per_route_op[84].inc_fwd(device & 65535);
                  return device & 65535;
                } else {
                  // BDDNode 85
                  inc_path_counter(85);
                  uint8_t lpm_key[4];
                  uint32_t hdr2_slice = *(uint32_t*)(hdr2+16);
                  *(uint32_t*)lpm_key = hdr2_slice;
                  int lpm_value2;
                  int lpm_lookup_match2 = lpm_lookup(lpm2, lpm_key, &lpm_value2);
                  // BDDNode 86
                  inc_path_counter(86);
                  if ((0) == (lpm_lookup_match2)) {
                    // BDDNode 87
                    inc_path_counter(87);
                    uint8_t* vector_cell4 = 0;
                    vector_borrow(vector3, lpm_value, (void**)&vector_cell4);
                    uint32_t vector_value_out4 = *(uint32_t*)vector_cell4;
                    // BDDNode 88
                    inc_path_counter(88);
                    *(uint32_t*)vector_cell4 = (1) + (vector_value_out4);
                    // BDDNode 89
                    inc_path_counter(89);
                    uint8_t key2[8];
                    key2[0] = hdr2_slice & 255;
                    key2[1] = (hdr2_slice>>8) & 255;
                    key2[2] = (hdr2_slice>>16) & 255;
                    key2[3] = (hdr2_slice>>24) & 255;
                    key2[4] = *(dns_address+0);
                    key2[5] = *(dns_address+1);
                    key2[6] = *(dns_address+2);
                    key2[7] = *(dns_address+3);
                    int value2;
                    int map_hit2 = map_get(map, key2, &value2);
                    stats_per_map[1074083024ULL].update(89, key2, 8, now);
                    // BDDNode 90
                    inc_path_counter(90);
                    if ((0) == (map_hit2)) {
                      // BDDNode 91
                      inc_path_counter(91);
                      int index;
                      int not_out_of_space = dchain_allocate_new_index(dchain, &index, now);
                      // BDDNode 92
                      inc_path_counter(92);
                      if ((0) == (not_out_of_space)) {
                        // BDDNode 93
                        inc_path_counter(93);
                        uint8_t* vector_cell5 = 0;
                        vector_borrow(vector4, lpm_value, (void**)&vector_cell5);
                        uint32_t vector_value_out5 = *(uint32_t*)vector_cell5;
                        // BDDNode 94
                        inc_path_counter(94);
                        *(uint32_t*)vector_cell5 = (1) + (vector_value_out5);
                        // BDDNode 95
                        inc_path_counter(95);
                        packet_return_chunk(buffer, hdr4);
                        // BDDNode 96
                        inc_path_counter(96);
                        packet_return_chunk(buffer, hdr3);
                        // BDDNode 97
                        inc_path_counter(97);
                        packet_return_chunk(buffer, hdr2);
                        // BDDNode 98
                        inc_path_counter(98);
                        packet_return_chunk(buffer, hdr);
                        // BDDNode 99
                        inc_path_counter(99);
                        forwarding_stats_per_route_op[99].inc_fwd(device & 65535);
                        return device & 65535;
                      } else {
                        // BDDNode 100
                        inc_path_counter(100);
                        uint8_t* vector_cell6 = 0;
                        vector_borrow(vector, index, (void**)&vector_cell6);
                        uint64_t vector_value_out6 = *(uint64_t*)vector_cell6;
                        // BDDNode 101
                        inc_path_counter(101);
                        *(uint64_t*)vector_cell6 = *(uint64_t*)key2;
                        map_put(map, vector_cell6, index);
                        stats_per_map[1074083024ULL].update(101, vector_cell6, 8, now);
                        // BDDNode 102
                        inc_path_counter(102);
                        // BDDNode 103
                        inc_path_counter(103);
                        uint8_t* vector_cell7 = 0;
                        vector_borrow(vector2, index, (void**)&vector_cell7);
                        uint32_t vector_value_out7 = *(uint32_t*)vector_cell7;
                        // BDDNode 104
                        inc_path_counter(104);
                        memcpy((void*)vector_cell7, (void*)&lpm_value, 4);
                        // BDDNode 105
                        inc_path_counter(105);
                        packet_return_chunk(buffer, hdr4);
                        // BDDNode 106
                        inc_path_counter(106);
                        packet_return_chunk(buffer, hdr3);
                        // BDDNode 107
                        inc_path_counter(107);
                        packet_return_chunk(buffer, hdr2);
                        // BDDNode 108
                        inc_path_counter(108);
                        packet_return_chunk(buffer, hdr);
                        // BDDNode 109
                        inc_path_counter(109);
                        forwarding_stats_per_route_op[109].inc_fwd(device & 65535);
                        return device & 65535;
                      } // (0) == (not_out_of_space)
                    } else {
                      // BDDNode 110
                      inc_path_counter(110);
                      dchain_rejuvenate_index(dchain, value2, now);
                      // BDDNode 111
                      inc_path_counter(111);
                      uint8_t* vector_cell8 = 0;
                      vector_borrow(vector2, value2, (void**)&vector_cell8);
                      uint32_t vector_value_out8 = *(uint32_t*)vector_cell8;
                      // BDDNode 112
                      inc_path_counter(112);
                      memcpy((void*)vector_cell8, (void*)&lpm_value, 4);
                      // BDDNode 113
                      inc_path_counter(113);
                      packet_return_chunk(buffer, hdr4);
                      // BDDNode 114
                      inc_path_counter(114);
                      packet_return_chunk(buffer, hdr3);
                      // BDDNode 115
                      inc_path_counter(115);
                      packet_return_chunk(buffer, hdr2);
                      // BDDNode 116
                      inc_path_counter(116);
                      packet_return_chunk(buffer, hdr);
                      // BDDNode 117
                      inc_path_counter(117);
                      forwarding_stats_per_route_op[117].inc_fwd(device & 65535);
                      return device & 65535;
                    } // (0) == (map_hit2)
                  } else {
                    // BDDNode 118
                    inc_path_counter(118);
                    packet_return_chunk(buffer, hdr4);
                    // BDDNode 119
                    inc_path_counter(119);
                    packet_return_chunk(buffer, hdr3);
                    // BDDNode 120
                    inc_path_counter(120);
                    packet_return_chunk(buffer, hdr2);
                    // BDDNode 121
                    inc_path_counter(121);
                    packet_return_chunk(buffer, hdr);
                    // BDDNode 122
                    inc_path_counter(122);
                    forwarding_stats_per_route_op[122].inc_fwd(device & 65535);
                    return device & 65535;
                  } // (0) == (lpm_lookup_match2)
                } // (0) == (lpm_lookup_match)
              } // (0) == (dns_response_found)
            } else {
              // BDDNode 123
              inc_path_counter(123);
              packet_return_chunk(buffer, hdr4);
              // BDDNode 124
              inc_path_counter(124);
              packet_return_chunk(buffer, hdr3);
              // BDDNode 125
              inc_path_counter(125);
              packet_return_chunk(buffer, hdr2);
              // BDDNode 126
              inc_path_counter(126);
              packet_return_chunk(buffer, hdr);
              // BDDNode 127
              inc_path_counter(127);
              forwarding_stats_per_route_op[127].inc_fwd(device & 65535);
              return device & 65535;
            } // (0) != (((uint8_t)(*(hdr4+2))) & (128))
          } else {
            // BDDNode 128
            inc_path_counter(128);
            packet_return_chunk(buffer, hdr3);
            // BDDNode 129
            inc_path_counter(129);
            packet_return_chunk(buffer, hdr2);
            // BDDNode 130
            inc_path_counter(130);
            packet_return_chunk(buffer, hdr);
            // BDDNode 131
            inc_path_counter(131);
            forwarding_stats_per_route_op[131].inc_fwd(device & 65535);
            return device & 65535;
          } // ((uint16_t)(((255) < ((uint16_t)((uint64_t)((-8LL) + ((uint16_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5))))))))) ? (255) : ((uint64_t)((-8LL) + ((uint16_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5))))))))) <= ((4294967254) + ((uint16_t)(packet_length & 65535)))
        } else {
          // BDDNode 132
          inc_path_counter(132);
          packet_return_chunk(buffer, hdr3);
          // BDDNode 133
          inc_path_counter(133);
          packet_return_chunk(buffer, hdr2);
          // BDDNode 134
          inc_path_counter(134);
          packet_return_chunk(buffer, hdr);
          // BDDNode 135
          inc_path_counter(135);
          forwarding_stats_per_route_op[135].inc_fwd(device & 65535);
          return device & 65535;
        } // (20ULL) <= ((uint16_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5)))))
      } // ((13568) != (*(uint16_t*)(uint16_t*)(hdr3+2))) & ((13568) != (*(uint16_t*)(uint16_t*)(hdr3+0)))
    } else {
      // BDDNode 136
      inc_path_counter(136);
      uint8_t key3[8];
      key3[0] = *(hdr2+16);
      key3[1] = *(hdr2+17);
      key3[2] = *(hdr2+18);
      key3[3] = *(hdr2+19);
      key3[4] = *(hdr2+12);
      key3[5] = *(hdr2+13);
      key3[6] = *(hdr2+14);
      key3[7] = *(hdr2+15);
      int value3;
      int map_hit3 = map_get(map, key3, &value3);
      stats_per_map[1074083024ULL].update(136, key3, 8, now);
      // BDDNode 137
      inc_path_counter(137);
      if ((0) == (map_hit3)) {
        // BDDNode 138
        inc_path_counter(138);
        packet_return_chunk(buffer, hdr2);
        // BDDNode 139
        inc_path_counter(139);
        packet_return_chunk(buffer, hdr);
        // BDDNode 140
        inc_path_counter(140);
        forwarding_stats_per_route_op[140].inc_fwd(device & 65535);
        return device & 65535;
      } else {
        // BDDNode 141
        inc_path_counter(141);
        dchain_rejuvenate_index(dchain, value3, now);
        // BDDNode 142
        inc_path_counter(142);
        uint8_t* vector_cell9 = 0;
        vector_borrow(vector2, value3, (void**)&vector_cell9);
        uint32_t vector_value_out9 = *(uint32_t*)vector_cell9;
        // BDDNode 143
        inc_path_counter(143);
        // BDDNode 144
        inc_path_counter(144);
        uint8_t* vector_cell10 = 0;
        vector_borrow(vector5, vector_value_out9, (void**)&vector_cell10);
        uint32_t vector_value_out10 = *(uint32_t*)vector_cell10;
        // BDDNode 145
        inc_path_counter(145);
        *(uint32_t*)vector_cell10 = (1) + (vector_value_out10);
        // BDDNode 146
        inc_path_counter(146);
        uint8_t* vector_cell11 = 0;
        vector_borrow(vector6, vector_value_out9, (void**)&vector_cell11);
        uint32_t vector_value_out11 = *(uint32_t*)vector_cell11;
        // BDDNode 147
        inc_path_counter(147);
        *(uint32_t*)vector_cell11 = (vector_value_out11) + ((uint16_t)(packet_length & 65535));
        // BDDNode 148
        inc_path_counter(148);
        packet_return_chunk(buffer, hdr2);
        // BDDNode 149
        inc_path_counter(149);
        packet_return_chunk(buffer, hdr);
        // BDDNode 150
        inc_path_counter(150);
        forwarding_stats_per_route_op[150].inc_fwd(device & 65535);
        return device & 65535;
      } // (0) == (map_hit3)
    } // ((17) == (*(hdr2+9))) & ((8ULL) <= ((uint32_t)((4294967262) + ((uint16_t)(packet_length & 65535)))))
  } else {
    // BDDNode 151
    inc_path_counter(151);
    packet_return_chunk(buffer, hdr);
    // BDDNode 152
    inc_path_counter(152);
    forwarding_stats_per_route_op[152].inc_fwd(device & 65535);
    return device & 65535;
  } // ((8) == (*(uint16_t*)(uint16_t*)(hdr+12))) & ((20ULL) <= ((uint16_t)((uint32_t)((4294967282) + ((uint16_t)(packet_length & 65535))))))
}
