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
std::unordered_map<uint64_t, MapStats> stats_per_map; // by the map's address
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

struct Vector *vector;
struct Vector *vector2;
struct Vector *vector3;
struct Vector *vector4;
struct Vector *vector5;
struct Vector *vector6;
struct Vector *vector7;
struct Vector *vector8;
struct crc32_hasher *hasher;
struct crc32_hasher *hasher2;
struct crc32_hasher *hasher3;
struct crc32_hasher *hasher4;
struct crc32_hasher *hasher5;
struct crc32_hasher *hasher6;
struct Vector *vector9;
struct Vector *vector10;


bool nf_init() {
  int vector_alloc_success = vector_allocate(4, 256, &vector);
  if (!vector_alloc_success) {
    return false;
  }
  int vector_alloc_success2 = vector_allocate(4, 65536, &vector2);
  if (!vector_alloc_success2) {
    return false;
  }
  int vector_alloc_success3 = vector_allocate(8, 1024, &vector3);
  if (!vector_alloc_success3) {
    return false;
  }
  int vector_alloc_success4 = vector_allocate(8, 1024, &vector4);
  if (!vector_alloc_success4) {
    return false;
  }
  int vector_alloc_success5 = vector_allocate(8, 1024, &vector5);
  if (!vector_alloc_success5) {
    return false;
  }
  int vector_alloc_success6 = vector_allocate(8, 1024, &vector6);
  if (!vector_alloc_success6) {
    return false;
  }
  int vector_alloc_success7 = vector_allocate(8, 1024, &vector7);
  if (!vector_alloc_success7) {
    return false;
  }
  int vector_alloc_success8 = vector_allocate(8, 1024, &vector8);
  if (!vector_alloc_success8) {
    return false;
  }
  hasher = (struct crc32_hasher *)malloc(sizeof(struct crc32_hasher));
  const struct crc32_config hasher_config = {"", 79764919, 1, 4294967295, 4294967295};
  crc32_hasher_init(hasher, &hasher_config);
  hasher2 = (struct crc32_hasher *)malloc(sizeof(struct crc32_hasher));
  const struct crc32_config hasher2_config = {"", 2065146783, 1, 4294967295, 4294967295};
  crc32_hasher_init(hasher2, &hasher2_config);
  hasher3 = (struct crc32_hasher *)malloc(sizeof(struct crc32_hasher));
  const struct crc32_config hasher3_config = {"", 2582813357, 1, 4294967295, 4294967295};
  crc32_hasher_init(hasher3, &hasher3_config);
  hasher4 = (struct crc32_hasher *)malloc(sizeof(struct crc32_hasher));
  const struct crc32_config hasher4_config = {"", 566010563, 1, 4294967295, 4294967295};
  crc32_hasher_init(hasher4, &hasher4_config);
  hasher5 = (struct crc32_hasher *)malloc(sizeof(struct crc32_hasher));
  const struct crc32_config hasher5_config = {"", 207185993, 1, 4294967295, 4294967295};
  crc32_hasher_init(hasher5, &hasher5_config);
  hasher6 = (struct crc32_hasher *)malloc(sizeof(struct crc32_hasher));
  const struct crc32_config hasher6_config = {"", 894686999, 1, 4294967295, 4294967295};
  crc32_hasher_init(hasher6, &hasher6_config);
  int vector_alloc_success9 = vector_allocate(4, 32, &vector9);
  if (!vector_alloc_success9) {
    return false;
  }
  int vector_alloc_success10 = vector_allocate(2, 32, &vector10);
  if (!vector_alloc_success10) {
    return false;
  }
  uint8_t* vector_cell = 0;
  vector_borrow(vector9, 0, (void**)&vector_cell);
  uint32_t vector_value_out = *(uint32_t*)vector_cell;
  *(uint32_t*)vector_cell = 1;
  uint8_t* vector_cell2 = 0;
  vector_borrow(vector10, 0, (void**)&vector_cell2);
  uint16_t vector_value_out2 = *(uint16_t*)vector_cell2;
  *(uint16_t*)vector_cell2 = 1;
  uint8_t* vector_cell3 = 0;
  vector_borrow(vector9, 1, (void**)&vector_cell3);
  uint32_t vector_value_out3 = *(uint32_t*)vector_cell3;
  *(uint32_t*)vector_cell3 = 0;
  uint8_t* vector_cell4 = 0;
  vector_borrow(vector10, 1, (void**)&vector_cell4);
  uint16_t vector_value_out4 = *(uint16_t*)vector_cell4;
  *(uint16_t*)vector_cell4 = 0;
  uint8_t* vector_cell5 = 0;
  vector_borrow(vector9, 2, (void**)&vector_cell5);
  uint32_t vector_value_out5 = *(uint32_t*)vector_cell5;
  *(uint32_t*)vector_cell5 = 1;
  uint8_t* vector_cell6 = 0;
  vector_borrow(vector10, 2, (void**)&vector_cell6);
  uint16_t vector_value_out6 = *(uint16_t*)vector_cell6;
  *(uint16_t*)vector_cell6 = 3;
  uint8_t* vector_cell7 = 0;
  vector_borrow(vector9, 3, (void**)&vector_cell7);
  uint32_t vector_value_out7 = *(uint32_t*)vector_cell7;
  *(uint32_t*)vector_cell7 = 0;
  uint8_t* vector_cell8 = 0;
  vector_borrow(vector10, 3, (void**)&vector_cell8);
  uint16_t vector_value_out8 = *(uint16_t*)vector_cell8;
  *(uint16_t*)vector_cell8 = 2;
  uint8_t* vector_cell9 = 0;
  vector_borrow(vector9, 4, (void**)&vector_cell9);
  uint32_t vector_value_out9 = *(uint32_t*)vector_cell9;
  *(uint32_t*)vector_cell9 = 1;
  uint8_t* vector_cell10 = 0;
  vector_borrow(vector10, 4, (void**)&vector_cell10);
  uint16_t vector_value_out10 = *(uint16_t*)vector_cell10;
  *(uint16_t*)vector_cell10 = 5;
  uint8_t* vector_cell11 = 0;
  vector_borrow(vector9, 5, (void**)&vector_cell11);
  uint32_t vector_value_out11 = *(uint32_t*)vector_cell11;
  *(uint32_t*)vector_cell11 = 0;
  uint8_t* vector_cell12 = 0;
  vector_borrow(vector10, 5, (void**)&vector_cell12);
  uint16_t vector_value_out12 = *(uint16_t*)vector_cell12;
  *(uint16_t*)vector_cell12 = 4;
  uint8_t* vector_cell13 = 0;
  vector_borrow(vector9, 6, (void**)&vector_cell13);
  uint32_t vector_value_out13 = *(uint32_t*)vector_cell13;
  *(uint32_t*)vector_cell13 = 1;
  uint8_t* vector_cell14 = 0;
  vector_borrow(vector10, 6, (void**)&vector_cell14);
  uint16_t vector_value_out14 = *(uint16_t*)vector_cell14;
  *(uint16_t*)vector_cell14 = 7;
  uint8_t* vector_cell15 = 0;
  vector_borrow(vector9, 7, (void**)&vector_cell15);
  uint32_t vector_value_out15 = *(uint32_t*)vector_cell15;
  *(uint32_t*)vector_cell15 = 0;
  uint8_t* vector_cell16 = 0;
  vector_borrow(vector10, 7, (void**)&vector_cell16);
  uint16_t vector_value_out16 = *(uint16_t*)vector_cell16;
  *(uint16_t*)vector_cell16 = 6;
  uint8_t* vector_cell17 = 0;
  vector_borrow(vector9, 8, (void**)&vector_cell17);
  uint32_t vector_value_out17 = *(uint32_t*)vector_cell17;
  *(uint32_t*)vector_cell17 = 1;
  uint8_t* vector_cell18 = 0;
  vector_borrow(vector10, 8, (void**)&vector_cell18);
  uint16_t vector_value_out18 = *(uint16_t*)vector_cell18;
  *(uint16_t*)vector_cell18 = 9;
  uint8_t* vector_cell19 = 0;
  vector_borrow(vector9, 9, (void**)&vector_cell19);
  uint32_t vector_value_out19 = *(uint32_t*)vector_cell19;
  *(uint32_t*)vector_cell19 = 0;
  uint8_t* vector_cell20 = 0;
  vector_borrow(vector10, 9, (void**)&vector_cell20);
  uint16_t vector_value_out20 = *(uint16_t*)vector_cell20;
  *(uint16_t*)vector_cell20 = 8;
  uint8_t* vector_cell21 = 0;
  vector_borrow(vector9, 10, (void**)&vector_cell21);
  uint32_t vector_value_out21 = *(uint32_t*)vector_cell21;
  *(uint32_t*)vector_cell21 = 1;
  uint8_t* vector_cell22 = 0;
  vector_borrow(vector10, 10, (void**)&vector_cell22);
  uint16_t vector_value_out22 = *(uint16_t*)vector_cell22;
  *(uint16_t*)vector_cell22 = 11;
  uint8_t* vector_cell23 = 0;
  vector_borrow(vector9, 11, (void**)&vector_cell23);
  uint32_t vector_value_out23 = *(uint32_t*)vector_cell23;
  *(uint32_t*)vector_cell23 = 0;
  uint8_t* vector_cell24 = 0;
  vector_borrow(vector10, 11, (void**)&vector_cell24);
  uint16_t vector_value_out24 = *(uint16_t*)vector_cell24;
  *(uint16_t*)vector_cell24 = 10;
  uint8_t* vector_cell25 = 0;
  vector_borrow(vector9, 12, (void**)&vector_cell25);
  uint32_t vector_value_out25 = *(uint32_t*)vector_cell25;
  *(uint32_t*)vector_cell25 = 1;
  uint8_t* vector_cell26 = 0;
  vector_borrow(vector10, 12, (void**)&vector_cell26);
  uint16_t vector_value_out26 = *(uint16_t*)vector_cell26;
  *(uint16_t*)vector_cell26 = 13;
  uint8_t* vector_cell27 = 0;
  vector_borrow(vector9, 13, (void**)&vector_cell27);
  uint32_t vector_value_out27 = *(uint32_t*)vector_cell27;
  *(uint32_t*)vector_cell27 = 0;
  uint8_t* vector_cell28 = 0;
  vector_borrow(vector10, 13, (void**)&vector_cell28);
  uint16_t vector_value_out28 = *(uint16_t*)vector_cell28;
  *(uint16_t*)vector_cell28 = 12;
  uint8_t* vector_cell29 = 0;
  vector_borrow(vector9, 14, (void**)&vector_cell29);
  uint32_t vector_value_out29 = *(uint32_t*)vector_cell29;
  *(uint32_t*)vector_cell29 = 1;
  uint8_t* vector_cell30 = 0;
  vector_borrow(vector10, 14, (void**)&vector_cell30);
  uint16_t vector_value_out30 = *(uint16_t*)vector_cell30;
  *(uint16_t*)vector_cell30 = 15;
  uint8_t* vector_cell31 = 0;
  vector_borrow(vector9, 15, (void**)&vector_cell31);
  uint32_t vector_value_out31 = *(uint32_t*)vector_cell31;
  *(uint32_t*)vector_cell31 = 0;
  uint8_t* vector_cell32 = 0;
  vector_borrow(vector10, 15, (void**)&vector_cell32);
  uint16_t vector_value_out32 = *(uint16_t*)vector_cell32;
  *(uint16_t*)vector_cell32 = 14;
  uint8_t* vector_cell33 = 0;
  vector_borrow(vector9, 16, (void**)&vector_cell33);
  uint32_t vector_value_out33 = *(uint32_t*)vector_cell33;
  *(uint32_t*)vector_cell33 = 1;
  uint8_t* vector_cell34 = 0;
  vector_borrow(vector10, 16, (void**)&vector_cell34);
  uint16_t vector_value_out34 = *(uint16_t*)vector_cell34;
  *(uint16_t*)vector_cell34 = 17;
  uint8_t* vector_cell35 = 0;
  vector_borrow(vector9, 17, (void**)&vector_cell35);
  uint32_t vector_value_out35 = *(uint32_t*)vector_cell35;
  *(uint32_t*)vector_cell35 = 0;
  uint8_t* vector_cell36 = 0;
  vector_borrow(vector10, 17, (void**)&vector_cell36);
  uint16_t vector_value_out36 = *(uint16_t*)vector_cell36;
  *(uint16_t*)vector_cell36 = 16;
  uint8_t* vector_cell37 = 0;
  vector_borrow(vector9, 18, (void**)&vector_cell37);
  uint32_t vector_value_out37 = *(uint32_t*)vector_cell37;
  *(uint32_t*)vector_cell37 = 1;
  uint8_t* vector_cell38 = 0;
  vector_borrow(vector10, 18, (void**)&vector_cell38);
  uint16_t vector_value_out38 = *(uint16_t*)vector_cell38;
  *(uint16_t*)vector_cell38 = 19;
  uint8_t* vector_cell39 = 0;
  vector_borrow(vector9, 19, (void**)&vector_cell39);
  uint32_t vector_value_out39 = *(uint32_t*)vector_cell39;
  *(uint32_t*)vector_cell39 = 0;
  uint8_t* vector_cell40 = 0;
  vector_borrow(vector10, 19, (void**)&vector_cell40);
  uint16_t vector_value_out40 = *(uint16_t*)vector_cell40;
  *(uint16_t*)vector_cell40 = 18;
  uint8_t* vector_cell41 = 0;
  vector_borrow(vector9, 20, (void**)&vector_cell41);
  uint32_t vector_value_out41 = *(uint32_t*)vector_cell41;
  *(uint32_t*)vector_cell41 = 1;
  uint8_t* vector_cell42 = 0;
  vector_borrow(vector10, 20, (void**)&vector_cell42);
  uint16_t vector_value_out42 = *(uint16_t*)vector_cell42;
  *(uint16_t*)vector_cell42 = 21;
  uint8_t* vector_cell43 = 0;
  vector_borrow(vector9, 21, (void**)&vector_cell43);
  uint32_t vector_value_out43 = *(uint32_t*)vector_cell43;
  *(uint32_t*)vector_cell43 = 0;
  uint8_t* vector_cell44 = 0;
  vector_borrow(vector10, 21, (void**)&vector_cell44);
  uint16_t vector_value_out44 = *(uint16_t*)vector_cell44;
  *(uint16_t*)vector_cell44 = 20;
  uint8_t* vector_cell45 = 0;
  vector_borrow(vector9, 22, (void**)&vector_cell45);
  uint32_t vector_value_out45 = *(uint32_t*)vector_cell45;
  *(uint32_t*)vector_cell45 = 1;
  uint8_t* vector_cell46 = 0;
  vector_borrow(vector10, 22, (void**)&vector_cell46);
  uint16_t vector_value_out46 = *(uint16_t*)vector_cell46;
  *(uint16_t*)vector_cell46 = 23;
  uint8_t* vector_cell47 = 0;
  vector_borrow(vector9, 23, (void**)&vector_cell47);
  uint32_t vector_value_out47 = *(uint32_t*)vector_cell47;
  *(uint32_t*)vector_cell47 = 0;
  uint8_t* vector_cell48 = 0;
  vector_borrow(vector10, 23, (void**)&vector_cell48);
  uint16_t vector_value_out48 = *(uint16_t*)vector_cell48;
  *(uint16_t*)vector_cell48 = 22;
  uint8_t* vector_cell49 = 0;
  vector_borrow(vector9, 24, (void**)&vector_cell49);
  uint32_t vector_value_out49 = *(uint32_t*)vector_cell49;
  *(uint32_t*)vector_cell49 = 1;
  uint8_t* vector_cell50 = 0;
  vector_borrow(vector10, 24, (void**)&vector_cell50);
  uint16_t vector_value_out50 = *(uint16_t*)vector_cell50;
  *(uint16_t*)vector_cell50 = 25;
  uint8_t* vector_cell51 = 0;
  vector_borrow(vector9, 25, (void**)&vector_cell51);
  uint32_t vector_value_out51 = *(uint32_t*)vector_cell51;
  *(uint32_t*)vector_cell51 = 0;
  uint8_t* vector_cell52 = 0;
  vector_borrow(vector10, 25, (void**)&vector_cell52);
  uint16_t vector_value_out52 = *(uint16_t*)vector_cell52;
  *(uint16_t*)vector_cell52 = 24;
  uint8_t* vector_cell53 = 0;
  vector_borrow(vector9, 26, (void**)&vector_cell53);
  uint32_t vector_value_out53 = *(uint32_t*)vector_cell53;
  *(uint32_t*)vector_cell53 = 1;
  uint8_t* vector_cell54 = 0;
  vector_borrow(vector10, 26, (void**)&vector_cell54);
  uint16_t vector_value_out54 = *(uint16_t*)vector_cell54;
  *(uint16_t*)vector_cell54 = 27;
  uint8_t* vector_cell55 = 0;
  vector_borrow(vector9, 27, (void**)&vector_cell55);
  uint32_t vector_value_out55 = *(uint32_t*)vector_cell55;
  *(uint32_t*)vector_cell55 = 0;
  uint8_t* vector_cell56 = 0;
  vector_borrow(vector10, 27, (void**)&vector_cell56);
  uint16_t vector_value_out56 = *(uint16_t*)vector_cell56;
  *(uint16_t*)vector_cell56 = 26;
  uint8_t* vector_cell57 = 0;
  vector_borrow(vector9, 28, (void**)&vector_cell57);
  uint32_t vector_value_out57 = *(uint32_t*)vector_cell57;
  *(uint32_t*)vector_cell57 = 1;
  uint8_t* vector_cell58 = 0;
  vector_borrow(vector10, 28, (void**)&vector_cell58);
  uint16_t vector_value_out58 = *(uint16_t*)vector_cell58;
  *(uint16_t*)vector_cell58 = 29;
  uint8_t* vector_cell59 = 0;
  vector_borrow(vector9, 29, (void**)&vector_cell59);
  uint32_t vector_value_out59 = *(uint32_t*)vector_cell59;
  *(uint32_t*)vector_cell59 = 0;
  uint8_t* vector_cell60 = 0;
  vector_borrow(vector10, 29, (void**)&vector_cell60);
  uint16_t vector_value_out60 = *(uint16_t*)vector_cell60;
  *(uint16_t*)vector_cell60 = 28;
  uint8_t* vector_cell61 = 0;
  vector_borrow(vector9, 30, (void**)&vector_cell61);
  uint32_t vector_value_out61 = *(uint32_t*)vector_cell61;
  *(uint32_t*)vector_cell61 = 1;
  uint8_t* vector_cell62 = 0;
  vector_borrow(vector10, 30, (void**)&vector_cell62);
  uint16_t vector_value_out62 = *(uint16_t*)vector_cell62;
  *(uint16_t*)vector_cell62 = 31;
  uint8_t* vector_cell63 = 0;
  vector_borrow(vector9, 31, (void**)&vector_cell63);
  uint32_t vector_value_out63 = *(uint32_t*)vector_cell63;
  *(uint32_t*)vector_cell63 = 0;
  uint8_t* vector_cell64 = 0;
  vector_borrow(vector10, 31, (void**)&vector_cell64);
  uint16_t vector_value_out64 = *(uint16_t*)vector_cell64;
  *(uint16_t*)vector_cell64 = 30;
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
  forwarding_stats_per_route_op.insert({194, PortStats{}});
  forwarding_stats_per_route_op.insert({191, PortStats{}});
  forwarding_stats_per_route_op.insert({190, PortStats{}});
  forwarding_stats_per_route_op.insert({196, PortStats{}});
  forwarding_stats_per_route_op.insert({183, PortStats{}});
  forwarding_stats_per_route_op.insert({182, PortStats{}});
  node_pkt_counter.insert({196, 0});
  node_pkt_counter.insert({195, 0});
  node_pkt_counter.insert({194, 0});
  node_pkt_counter.insert({193, 0});
  node_pkt_counter.insert({192, 0});
  node_pkt_counter.insert({191, 0});
  node_pkt_counter.insert({190, 0});
  node_pkt_counter.insert({189, 0});
  node_pkt_counter.insert({188, 0});
  node_pkt_counter.insert({187, 0});
  node_pkt_counter.insert({186, 0});
  node_pkt_counter.insert({185, 0});
  node_pkt_counter.insert({184, 0});
  node_pkt_counter.insert({183, 0});
  node_pkt_counter.insert({182, 0});
  node_pkt_counter.insert({181, 0});
  node_pkt_counter.insert({180, 0});
  node_pkt_counter.insert({179, 0});
  node_pkt_counter.insert({178, 0});
  node_pkt_counter.insert({177, 0});
  node_pkt_counter.insert({176, 0});
  node_pkt_counter.insert({175, 0});
  node_pkt_counter.insert({174, 0});
  node_pkt_counter.insert({173, 0});
  node_pkt_counter.insert({156, 0});
  node_pkt_counter.insert({155, 0});
  node_pkt_counter.insert({154, 0});
  node_pkt_counter.insert({153, 0});
  node_pkt_counter.insert({152, 0});
  node_pkt_counter.insert({151, 0});
  node_pkt_counter.insert({150, 0});
  node_pkt_counter.insert({149, 0});
  node_pkt_counter.insert({148, 0});
  node_pkt_counter.insert({147, 0});
  node_pkt_counter.insert({146, 0});
  node_pkt_counter.insert({145, 0});
  node_pkt_counter.insert({144, 0});
  node_pkt_counter.insert({157, 0});
  node_pkt_counter.insert({158, 0});
  node_pkt_counter.insert({159, 0});
  node_pkt_counter.insert({160, 0});
  node_pkt_counter.insert({161, 0});
  node_pkt_counter.insert({162, 0});
  node_pkt_counter.insert({163, 0});
  node_pkt_counter.insert({164, 0});
  node_pkt_counter.insert({165, 0});
  node_pkt_counter.insert({166, 0});
  node_pkt_counter.insert({167, 0});
  node_pkt_counter.insert({168, 0});
  node_pkt_counter.insert({169, 0});
  node_pkt_counter.insert({170, 0});
  node_pkt_counter.insert({171, 0});
  node_pkt_counter.insert({172, 0});
  return true;
}


int nf_process(uint16_t device, uint8_t *buffer, uint16_t packet_length, time_ns_t now) {
  // BDDNode 144
  inc_path_counter(144);
  uint8_t* hdr;
  packet_borrow_next_chunk(buffer, 14, (void**)&hdr);
  // BDDNode 145
  inc_path_counter(145);
  if (((8) == (*(uint16_t*)(uint16_t*)(hdr+12))) & ((20ULL) <= ((uint16_t)((uint32_t)((4294967282) + ((uint16_t)(packet_length & 65535))))))) {
    // BDDNode 146
    inc_path_counter(146);
    uint8_t* hdr2;
    packet_borrow_next_chunk(buffer, 20, (void**)&hdr2);
    // BDDNode 147
    inc_path_counter(147);
    if ((((6) == (*(hdr2+9))) | ((17) == (*(hdr2+9)))) & ((4ULL) <= ((uint32_t)((4294967262) + ((uint16_t)(packet_length & 65535)))))) {
      // BDDNode 148
      inc_path_counter(148);
      uint8_t* hdr3;
      packet_borrow_next_chunk(buffer, 4, (void**)&hdr3);
      // BDDNode 149
      inc_path_counter(149);
      int cleared = vector_periodic_clear(vector, now, 1000000000ULL);
      // BDDNode 150
      inc_path_counter(150);
      int cleared2 = vector_periodic_clear(vector2, now, 1000000000ULL);
      // BDDNode 151
      inc_path_counter(151);
      int cleared3 = vector_periodic_clear(vector3, now, 1000000000ULL);
      // BDDNode 152
      inc_path_counter(152);
      int cleared4 = vector_periodic_clear(vector4, now, 1000000000ULL);
      // BDDNode 153
      inc_path_counter(153);
      int cleared5 = vector_periodic_clear(vector5, now, 1000000000ULL);
      // BDDNode 154
      inc_path_counter(154);
      int cleared6 = vector_periodic_clear(vector6, now, 1000000000ULL);
      // BDDNode 155
      inc_path_counter(155);
      int cleared7 = vector_periodic_clear(vector7, now, 1000000000ULL);
      // BDDNode 156
      inc_path_counter(156);
      int cleared8 = vector_periodic_clear(vector8, now, 1000000000ULL);
      // BDDNode 157
      inc_path_counter(157);
      uint8_t* vector_cell65 = 0;
      vector_borrow(vector9, (uint16_t)(device & 65535), (void**)&vector_cell65);
      uint32_t vector_value_out65 = *(uint32_t*)vector_cell65;
      // BDDNode 158
      inc_path_counter(158);
      // BDDNode 159
      inc_path_counter(159);
      if ((0) == (vector_value_out65)) {
        // BDDNode 160
        inc_path_counter(160);
        uint8_t* vector_cell66 = 0;
        vector_borrow(vector, (*(uint32_t*)(uint32_t*)(hdr2+12)) & (255), (void**)&vector_cell66);
        uint32_t vector_value_out66 = *(uint32_t*)vector_cell66;
        // BDDNode 161
        inc_path_counter(161);
        *(uint32_t*)vector_cell66 = (1) + (vector_value_out66);
        // BDDNode 162
        inc_path_counter(162);
        uint8_t* vector_cell67 = 0;
        vector_borrow(vector2, (*(uint32_t*)(uint32_t*)(hdr2+12)) & (65535), (void**)&vector_cell67);
        uint32_t vector_value_out67 = *(uint32_t*)vector_cell67;
        // BDDNode 163
        inc_path_counter(163);
        *(uint32_t*)vector_cell67 = (1) + (vector_value_out67);
        // BDDNode 164
        inc_path_counter(164);
        uint8_t data[4];
        data[0] = (uint32_t)((*(uint32_t*)(uint32_t*)(hdr2+12)) & (16777215));
        data[1] = (uint32_t)((*(uint32_t*)(uint32_t*)(hdr2+12)) & (16777215)>>8);
        data[2] = (uint32_t)((*(uint32_t*)(uint32_t*)(hdr2+12)) & (16777215)>>16);
        data[3] = (uint32_t)((*(uint32_t*)(uint32_t*)(hdr2+12)) & (16777215)>>24);
        uint32_t hash = crc32_hasher_hash(hasher, data, 4);
        // BDDNode 165
        inc_path_counter(165);
        uint8_t pair[8];
        pair[0] = (uint64_t)(((uint64_t)(0) << 56 | (uint64_t)(((uint64_t)(0) << 48 | (uint64_t)(((uint64_t)(0) << 40 | (uint64_t)(((uint64_t)(1) << 32 | (uint64_t)((*(uint32_t*)(uint32_t*)(hdr2+12)) & (16777215))))))))));
        pair[1] = (uint64_t)(((uint64_t)(0) << 56 | (uint64_t)(((uint64_t)(0) << 48 | (uint64_t)(((uint64_t)(0) << 40 | (uint64_t)(((uint64_t)(1) << 32 | (uint64_t)((*(uint32_t*)(uint32_t*)(hdr2+12)) & (16777215)))))))))>>8);
        pair[2] = (uint64_t)(((uint64_t)(0) << 56 | (uint64_t)(((uint64_t)(0) << 48 | (uint64_t)(((uint64_t)(0) << 40 | (uint64_t)(((uint64_t)(1) << 32 | (uint64_t)((*(uint32_t*)(uint32_t*)(hdr2+12)) & (16777215)))))))))>>16);
        pair[3] = (uint64_t)(((uint64_t)(0) << 56 | (uint64_t)(((uint64_t)(0) << 48 | (uint64_t)(((uint64_t)(0) << 40 | (uint64_t)(((uint64_t)(1) << 32 | (uint64_t)((*(uint32_t*)(uint32_t*)(hdr2+12)) & (16777215)))))))))>>24);
        pair[4] = 1;
        pair[5] = 0;
        pair[6] = 0;
        pair[7] = 0;
        vector_inc_or_swap(vector3, (hash) & (1023), pair, 4, 4, 1);
        // BDDNode 166
        inc_path_counter(166);
        uint32_t hash2 = crc32_hasher_hash(hasher2, pair, 4);
        // BDDNode 167
        inc_path_counter(167);
        vector_inc_or_swap(vector4, (hash2) & (1023), pair, 4, 4, 0);
        // BDDNode 168
        inc_path_counter(168);
        uint32_t hash3 = crc32_hasher_hash(hasher3, pair, 4);
        // BDDNode 169
        inc_path_counter(169);
        vector_inc_or_swap(vector5, (hash3) & (1023), pair, 4, 4, 0);
        // BDDNode 170
        inc_path_counter(170);
        uint32_t hash4 = crc32_hasher_hash(hasher4, pair, 4);
        // BDDNode 171
        inc_path_counter(171);
        vector_inc_or_swap(vector6, (hash4) & (1023), pair, 4, 4, 0);
        // BDDNode 172
        inc_path_counter(172);
        uint32_t hash5 = crc32_hasher_hash(hasher5, pair, 4);
        // BDDNode 173
        inc_path_counter(173);
        vector_inc_or_swap(vector7, (hash5) & (1023), pair, 4, 4, 0);
        // BDDNode 174
        inc_path_counter(174);
        uint32_t hash6 = crc32_hasher_hash(hasher6, pair, 4);
        // BDDNode 175
        inc_path_counter(175);
        vector_inc_or_swap(vector8, (hash6) & (1023), pair, 4, 4, 0);
        // BDDNode 176
        inc_path_counter(176);
        uint8_t* vector_cell68 = 0;
        vector_borrow(vector10, (uint16_t)(device & 65535), (void**)&vector_cell68);
        uint16_t vector_value_out68 = *(uint16_t*)vector_cell68;
        // BDDNode 177
        inc_path_counter(177);
        // BDDNode 178
        inc_path_counter(178);
        packet_return_chunk(buffer, hdr3);
        // BDDNode 179
        inc_path_counter(179);
        packet_return_chunk(buffer, hdr2);
        // BDDNode 180
        inc_path_counter(180);
        packet_return_chunk(buffer, hdr);
        // BDDNode 181
        inc_path_counter(181);
        if ((device & 65535) != (vector_value_out68)) {
          // BDDNode 182
          inc_path_counter(182);
          forwarding_stats_per_route_op[182].inc_fwd(vector_value_out68);
          return vector_value_out68;
        } else {
          // BDDNode 183
          inc_path_counter(183);
          forwarding_stats_per_route_op[183].inc_drop();
          return DROP;
        } // (device & 65535) != (vector_value_out68)
      } else {
        // BDDNode 184
        inc_path_counter(184);
        uint8_t* vector_cell69 = 0;
        vector_borrow(vector10, (uint16_t)(device & 65535), (void**)&vector_cell69);
        uint16_t vector_value_out69 = *(uint16_t*)vector_cell69;
        // BDDNode 185
        inc_path_counter(185);
        // BDDNode 186
        inc_path_counter(186);
        packet_return_chunk(buffer, hdr3);
        // BDDNode 187
        inc_path_counter(187);
        packet_return_chunk(buffer, hdr2);
        // BDDNode 188
        inc_path_counter(188);
        packet_return_chunk(buffer, hdr);
        // BDDNode 189
        inc_path_counter(189);
        if ((device & 65535) != (vector_value_out69)) {
          // BDDNode 190
          inc_path_counter(190);
          forwarding_stats_per_route_op[190].inc_fwd(vector_value_out69);
          return vector_value_out69;
        } else {
          // BDDNode 191
          inc_path_counter(191);
          forwarding_stats_per_route_op[191].inc_drop();
          return DROP;
        } // (device & 65535) != (vector_value_out69)
      } // (0) == (vector_value_out65)
    } else {
      // BDDNode 192
      inc_path_counter(192);
      packet_return_chunk(buffer, hdr2);
      // BDDNode 193
      inc_path_counter(193);
      packet_return_chunk(buffer, hdr);
      // BDDNode 194
      inc_path_counter(194);
      forwarding_stats_per_route_op[194].inc_drop();
      return DROP;
    } // (((6) == (*(hdr2+9))) | ((17) == (*(hdr2+9)))) & ((4ULL) <= ((uint32_t)((4294967262) + ((uint16_t)(packet_length & 65535)))))
  } else {
    // BDDNode 195
    inc_path_counter(195);
    packet_return_chunk(buffer, hdr);
    // BDDNode 196
    inc_path_counter(196);
    forwarding_stats_per_route_op[196].inc_drop();
    return DROP;
  } // ((8) == (*(uint16_t*)(uint16_t*)(hdr+12))) & ((20ULL) <= ((uint16_t)((uint32_t)((4294967282) + ((uint16_t)(packet_length & 65535))))))
}
