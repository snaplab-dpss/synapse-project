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
#include <lib/state/lpm-dir-24-8.h>

#include <lib/util/math.h>
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

struct Vector *vector;
struct Vector *vector2;
struct Vector *vector3;


bool nf_init() {
  int vector_alloc_success = vector_allocate(4, 64, &vector);
  if (!vector_alloc_success) {
    return false;
  }
  int vector_alloc_success2 = vector_allocate(4, 1, &vector2);
  if (!vector_alloc_success2) {
    return false;
  }
  int vector_alloc_success3 = vector_allocate(4, 1, &vector3);
  if (!vector_alloc_success3) {
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
  forwarding_stats_per_route_op.insert({86, PortStats{}});
  forwarding_stats_per_route_op.insert({81, PortStats{}});
  forwarding_stats_per_route_op.insert({78, PortStats{}});
  forwarding_stats_per_route_op.insert({61, PortStats{}});
  forwarding_stats_per_route_op.insert({84, PortStats{}});
  forwarding_stats_per_route_op.insert({58, PortStats{}});
  forwarding_stats_per_route_op.insert({44, PortStats{}});
  forwarding_stats_per_route_op.insert({31, PortStats{}});
  forwarding_stats_per_route_op.insert({41, PortStats{}});
  forwarding_stats_per_route_op.insert({28, PortStats{}});
  forwarding_stats_per_route_op.insert({64, PortStats{}});
  forwarding_stats_per_route_op.insert({38, PortStats{}});
  forwarding_stats_per_route_op.insert({25, PortStats{}});
  node_pkt_counter.insert({86, 0});
  node_pkt_counter.insert({85, 0});
  node_pkt_counter.insert({84, 0});
  node_pkt_counter.insert({83, 0});
  node_pkt_counter.insert({82, 0});
  node_pkt_counter.insert({81, 0});
  node_pkt_counter.insert({80, 0});
  node_pkt_counter.insert({79, 0});
  node_pkt_counter.insert({78, 0});
  node_pkt_counter.insert({77, 0});
  node_pkt_counter.insert({76, 0});
  node_pkt_counter.insert({75, 0});
  node_pkt_counter.insert({74, 0});
  node_pkt_counter.insert({73, 0});
  node_pkt_counter.insert({72, 0});
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
  node_pkt_counter.insert({31, 0});
  node_pkt_counter.insert({30, 0});
  node_pkt_counter.insert({29, 0});
  node_pkt_counter.insert({28, 0});
  node_pkt_counter.insert({27, 0});
  node_pkt_counter.insert({26, 0});
  node_pkt_counter.insert({25, 0});
  node_pkt_counter.insert({24, 0});
  node_pkt_counter.insert({23, 0});
  node_pkt_counter.insert({22, 0});
  node_pkt_counter.insert({21, 0});
  node_pkt_counter.insert({20, 0});
  node_pkt_counter.insert({19, 0});
  node_pkt_counter.insert({18, 0});
  node_pkt_counter.insert({17, 0});
  node_pkt_counter.insert({16, 0});
  node_pkt_counter.insert({3, 0});
  node_pkt_counter.insert({4, 0});
  node_pkt_counter.insert({5, 0});
  node_pkt_counter.insert({6, 0});
  node_pkt_counter.insert({7, 0});
  node_pkt_counter.insert({8, 0});
  node_pkt_counter.insert({9, 0});
  node_pkt_counter.insert({10, 0});
  node_pkt_counter.insert({11, 0});
  node_pkt_counter.insert({12, 0});
  node_pkt_counter.insert({13, 0});
  node_pkt_counter.insert({14, 0});
  node_pkt_counter.insert({15, 0});
  node_pkt_counter.insert({32, 0});
  node_pkt_counter.insert({33, 0});
  node_pkt_counter.insert({34, 0});
  node_pkt_counter.insert({35, 0});
  node_pkt_counter.insert({36, 0});
  node_pkt_counter.insert({37, 0});
  node_pkt_counter.insert({38, 0});
  node_pkt_counter.insert({39, 0});
  node_pkt_counter.insert({40, 0});
  node_pkt_counter.insert({41, 0});
  node_pkt_counter.insert({42, 0});
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
  node_pkt_counter.insert({56, 0});
  node_pkt_counter.insert({57, 0});
  node_pkt_counter.insert({58, 0});
  node_pkt_counter.insert({59, 0});
  node_pkt_counter.insert({60, 0});
  node_pkt_counter.insert({61, 0});
  return true;
}


int nf_process(uint16_t device, uint8_t *buffer, uint16_t packet_length, time_ns_t now) {
  // BDDNode 3
  inc_path_counter(3);
  uint8_t* hdr;
  packet_borrow_next_chunk(buffer, 14, (void**)&hdr);
  // BDDNode 4
  inc_path_counter(4);
  if (((8) == (*(uint16_t*)(uint16_t*)(hdr+12))) & ((20ULL) <= ((uint16_t)((uint32_t)((4294967282) + ((uint16_t)(packet_length & 65535))))))) {
    // BDDNode 5
    inc_path_counter(5);
    uint8_t* hdr2;
    packet_borrow_next_chunk(buffer, 20, (void**)&hdr2);
    // BDDNode 6
    inc_path_counter(6);
    uint8_t obj[8];
    uint64_t hdr2_slice = *(uint64_t*)(hdr2+12);
    *(uint64_t*)obj = hdr2_slice;
    uint32_t hash = hash_obj(obj, 8);
    // BDDNode 7
    inc_path_counter(7);
    uint32_t trailing_zeros = count_trailing_zeros((hash) & (67108863));
    // BDDNode 8
    inc_path_counter(8);
    uint8_t* vector_cell = 0;
    vector_borrow(vector, (hash) >> (26), (void**)&vector_cell);
    uint32_t vector_value_out = *(uint32_t*)vector_cell;
    // BDDNode 9
    inc_path_counter(9);
    if ((trailing_zeros) < (20)) {
      // BDDNode 10
      inc_path_counter(10);
      if ((vector_value_out) < ((1) + (trailing_zeros))) {
        // BDDNode 11
        inc_path_counter(11);
        *(uint32_t*)vector_cell = (1) + (trailing_zeros);
        // BDDNode 12
        inc_path_counter(12);
        uint32_t power = power_of_two((20) - (((vector_value_out) < ((1) + (trailing_zeros))) ? (vector_value_out) : ((1) + (trailing_zeros))));
        // BDDNode 13
        inc_path_counter(13);
        uint32_t power2 = power_of_two((19) - (trailing_zeros));
        // BDDNode 14
        inc_path_counter(14);
        uint8_t* vector_cell2 = 0;
        vector_borrow(vector2, 0, (void**)&vector_cell2);
        uint32_t vector_value_out2 = *(uint32_t*)vector_cell2;
        // BDDNode 15
        inc_path_counter(15);
        *(uint32_t*)vector_cell2 = (vector_value_out2) + ((power) - (power2));
        // BDDNode 16
        inc_path_counter(16);
        uint32_t quotient = divide(3046596202, (67108864) - ((vector_value_out2) + ((power) - (power2))));
        // BDDNode 17
        inc_path_counter(17);
        uint8_t* vector_cell3 = 0;
        vector_borrow(vector3, 0, (void**)&vector_cell3);
        uint32_t vector_value_out3 = *(uint32_t*)vector_cell3;
        // BDDNode 18
        inc_path_counter(18);
        if ((0) != (vector_value_out)) {
          // BDDNode 19
          inc_path_counter(19);
          // BDDNode 20
          inc_path_counter(20);
          if ((quotient) < (160)) {
            // BDDNode 21
            inc_path_counter(21);
            if ((vector_value_out3) < (64)) {
              // BDDNode 22
              inc_path_counter(22);
              uint32_t logarithm = ln((64) - (vector_value_out3), 64);
              ln_stats_per_node[22].update((64) - (vector_value_out3), 64);
              // BDDNode 23
              inc_path_counter(23);
              packet_return_chunk(buffer, hdr2);
              // BDDNode 24
              inc_path_counter(24);
              hdr[6] = (uint32_t)((266) - (logarithm));
              hdr[7] = (uint32_t)((266) - (logarithm)>>8);
              hdr[8] = (uint32_t)((266) - (logarithm)>>16);
              hdr[9] = (uint32_t)((266) - (logarithm)>>24);
              hdr[10] = 0;
              hdr[11] = 0;
              packet_return_chunk(buffer, hdr);
              // BDDNode 25
              inc_path_counter(25);
              forwarding_stats_per_route_op[25].inc_fwd(device & 65535);
              return device & 65535;
            } else {
              // BDDNode 26
              inc_path_counter(26);
              packet_return_chunk(buffer, hdr2);
              // BDDNode 27
              inc_path_counter(27);
              hdr[6] = quotient & 255;
              hdr[7] = (quotient>>8) & 255;
              hdr[8] = (quotient>>16) & 255;
              hdr[9] = (quotient>>24) & 255;
              hdr[10] = 0;
              hdr[11] = 0;
              packet_return_chunk(buffer, hdr);
              // BDDNode 28
              inc_path_counter(28);
              forwarding_stats_per_route_op[28].inc_fwd(device & 65535);
              return device & 65535;
            } // (vector_value_out3) < (64)
          } else {
            // BDDNode 29
            inc_path_counter(29);
            packet_return_chunk(buffer, hdr2);
            // BDDNode 30
            inc_path_counter(30);
            hdr[6] = quotient & 255;
            hdr[7] = (quotient>>8) & 255;
            hdr[8] = (quotient>>16) & 255;
            hdr[9] = (quotient>>24) & 255;
            hdr[10] = 0;
            hdr[11] = 0;
            packet_return_chunk(buffer, hdr);
            // BDDNode 31
            inc_path_counter(31);
            forwarding_stats_per_route_op[31].inc_fwd(device & 65535);
            return device & 65535;
          } // (quotient) < (160)
        } else {
          // BDDNode 32
          inc_path_counter(32);
          *(uint32_t*)vector_cell3 = (1) + (vector_value_out3);
          // BDDNode 33
          inc_path_counter(33);
          if ((quotient) < (160)) {
            // BDDNode 34
            inc_path_counter(34);
            if (((1) + (vector_value_out3)) < (64)) {
              // BDDNode 35
              inc_path_counter(35);
              uint32_t logarithm2 = ln((63) - (vector_value_out3), 64);
              ln_stats_per_node[35].update((63) - (vector_value_out3), 64);
              // BDDNode 36
              inc_path_counter(36);
              packet_return_chunk(buffer, hdr2);
              // BDDNode 37
              inc_path_counter(37);
              hdr[6] = (uint32_t)((266) - (logarithm2));
              hdr[7] = (uint32_t)((266) - (logarithm2)>>8);
              hdr[8] = (uint32_t)((266) - (logarithm2)>>16);
              hdr[9] = (uint32_t)((266) - (logarithm2)>>24);
              hdr[10] = 0;
              hdr[11] = 0;
              packet_return_chunk(buffer, hdr);
              // BDDNode 38
              inc_path_counter(38);
              forwarding_stats_per_route_op[38].inc_fwd(device & 65535);
              return device & 65535;
            } else {
              // BDDNode 39
              inc_path_counter(39);
              packet_return_chunk(buffer, hdr2);
              // BDDNode 40
              inc_path_counter(40);
              hdr[6] = quotient & 255;
              hdr[7] = (quotient>>8) & 255;
              hdr[8] = (quotient>>16) & 255;
              hdr[9] = (quotient>>24) & 255;
              hdr[10] = 0;
              hdr[11] = 0;
              packet_return_chunk(buffer, hdr);
              // BDDNode 41
              inc_path_counter(41);
              forwarding_stats_per_route_op[41].inc_fwd(device & 65535);
              return device & 65535;
            } // ((1) + (vector_value_out3)) < (64)
          } else {
            // BDDNode 42
            inc_path_counter(42);
            packet_return_chunk(buffer, hdr2);
            // BDDNode 43
            inc_path_counter(43);
            hdr[6] = quotient & 255;
            hdr[7] = (quotient>>8) & 255;
            hdr[8] = (quotient>>16) & 255;
            hdr[9] = (quotient>>24) & 255;
            hdr[10] = 0;
            hdr[11] = 0;
            packet_return_chunk(buffer, hdr);
            // BDDNode 44
            inc_path_counter(44);
            forwarding_stats_per_route_op[44].inc_fwd(device & 65535);
            return device & 65535;
          } // (quotient) < (160)
        } // (0) != (vector_value_out)
      } else {
        // BDDNode 45
        inc_path_counter(45);
        // BDDNode 46
        inc_path_counter(46);
        uint32_t power3 = power_of_two((20) - (((vector_value_out) < ((1) + (trailing_zeros))) ? (vector_value_out) : ((1) + (trailing_zeros))));
        // BDDNode 47
        inc_path_counter(47);
        uint32_t power4 = power_of_two((19) - (trailing_zeros));
        // BDDNode 48
        inc_path_counter(48);
        uint8_t* vector_cell4 = 0;
        vector_borrow(vector2, 0, (void**)&vector_cell4);
        uint32_t vector_value_out4 = *(uint32_t*)vector_cell4;
        // BDDNode 49
        inc_path_counter(49);
        *(uint32_t*)vector_cell4 = (vector_value_out4) + ((power3) - (power4));
        // BDDNode 50
        inc_path_counter(50);
        uint32_t quotient2 = divide(3046596202, (67108864) - ((vector_value_out4) + ((power3) - (power4))));
        // BDDNode 51
        inc_path_counter(51);
        uint8_t* vector_cell5 = 0;
        vector_borrow(vector3, 0, (void**)&vector_cell5);
        uint32_t vector_value_out5 = *(uint32_t*)vector_cell5;
        // BDDNode 52
        inc_path_counter(52);
        // BDDNode 53
        inc_path_counter(53);
        if ((quotient2) < (160)) {
          // BDDNode 54
          inc_path_counter(54);
          if ((vector_value_out5) < (64)) {
            // BDDNode 55
            inc_path_counter(55);
            uint32_t logarithm3 = ln((64) - (vector_value_out5), 64);
            ln_stats_per_node[55].update((64) - (vector_value_out5), 64);
            // BDDNode 56
            inc_path_counter(56);
            packet_return_chunk(buffer, hdr2);
            // BDDNode 57
            inc_path_counter(57);
            hdr[6] = (uint32_t)((266) - (logarithm3));
            hdr[7] = (uint32_t)((266) - (logarithm3)>>8);
            hdr[8] = (uint32_t)((266) - (logarithm3)>>16);
            hdr[9] = (uint32_t)((266) - (logarithm3)>>24);
            hdr[10] = 0;
            hdr[11] = 0;
            packet_return_chunk(buffer, hdr);
            // BDDNode 58
            inc_path_counter(58);
            forwarding_stats_per_route_op[58].inc_fwd(device & 65535);
            return device & 65535;
          } else {
            // BDDNode 59
            inc_path_counter(59);
            packet_return_chunk(buffer, hdr2);
            // BDDNode 60
            inc_path_counter(60);
            hdr[6] = quotient2 & 255;
            hdr[7] = (quotient2>>8) & 255;
            hdr[8] = (quotient2>>16) & 255;
            hdr[9] = (quotient2>>24) & 255;
            hdr[10] = 0;
            hdr[11] = 0;
            packet_return_chunk(buffer, hdr);
            // BDDNode 61
            inc_path_counter(61);
            forwarding_stats_per_route_op[61].inc_fwd(device & 65535);
            return device & 65535;
          } // (vector_value_out5) < (64)
        } else {
          // BDDNode 62
          inc_path_counter(62);
          packet_return_chunk(buffer, hdr2);
          // BDDNode 63
          inc_path_counter(63);
          hdr[6] = quotient2 & 255;
          hdr[7] = (quotient2>>8) & 255;
          hdr[8] = (quotient2>>16) & 255;
          hdr[9] = (quotient2>>24) & 255;
          hdr[10] = 0;
          hdr[11] = 0;
          packet_return_chunk(buffer, hdr);
          // BDDNode 64
          inc_path_counter(64);
          forwarding_stats_per_route_op[64].inc_fwd(device & 65535);
          return device & 65535;
        } // (quotient2) < (160)
      } // (vector_value_out) < ((1) + (trailing_zeros))
    } else {
      // BDDNode 65
      inc_path_counter(65);
      // BDDNode 66
      inc_path_counter(66);
      uint32_t power5 = power_of_two((20) - (((vector_value_out) < (0)) ? (vector_value_out) : (0)));
      // BDDNode 67
      inc_path_counter(67);
      uint32_t power6 = power_of_two(20);
      // BDDNode 68
      inc_path_counter(68);
      uint8_t* vector_cell6 = 0;
      vector_borrow(vector2, 0, (void**)&vector_cell6);
      uint32_t vector_value_out6 = *(uint32_t*)vector_cell6;
      // BDDNode 69
      inc_path_counter(69);
      *(uint32_t*)vector_cell6 = (vector_value_out6) + ((power5) - (power6));
      // BDDNode 70
      inc_path_counter(70);
      uint32_t quotient3 = divide(3046596202, (67108864) - ((vector_value_out6) + ((power5) - (power6))));
      // BDDNode 71
      inc_path_counter(71);
      uint8_t* vector_cell7 = 0;
      vector_borrow(vector3, 0, (void**)&vector_cell7);
      uint32_t vector_value_out7 = *(uint32_t*)vector_cell7;
      // BDDNode 72
      inc_path_counter(72);
      *(uint32_t*)vector_cell7 = (1) + (vector_value_out7);
      // BDDNode 73
      inc_path_counter(73);
      if ((quotient3) < (160)) {
        // BDDNode 74
        inc_path_counter(74);
        if (((1) + (vector_value_out7)) < (64)) {
          // BDDNode 75
          inc_path_counter(75);
          uint32_t logarithm4 = ln((63) - (vector_value_out7), 64);
          ln_stats_per_node[75].update((63) - (vector_value_out7), 64);
          // BDDNode 76
          inc_path_counter(76);
          packet_return_chunk(buffer, hdr2);
          // BDDNode 77
          inc_path_counter(77);
          hdr[6] = (uint32_t)((266) - (logarithm4));
          hdr[7] = (uint32_t)((266) - (logarithm4)>>8);
          hdr[8] = (uint32_t)((266) - (logarithm4)>>16);
          hdr[9] = (uint32_t)((266) - (logarithm4)>>24);
          hdr[10] = 0;
          hdr[11] = 0;
          packet_return_chunk(buffer, hdr);
          // BDDNode 78
          inc_path_counter(78);
          forwarding_stats_per_route_op[78].inc_fwd(device & 65535);
          return device & 65535;
        } else {
          // BDDNode 79
          inc_path_counter(79);
          packet_return_chunk(buffer, hdr2);
          // BDDNode 80
          inc_path_counter(80);
          hdr[6] = quotient3 & 255;
          hdr[7] = (quotient3>>8) & 255;
          hdr[8] = (quotient3>>16) & 255;
          hdr[9] = (quotient3>>24) & 255;
          hdr[10] = 0;
          hdr[11] = 0;
          packet_return_chunk(buffer, hdr);
          // BDDNode 81
          inc_path_counter(81);
          forwarding_stats_per_route_op[81].inc_fwd(device & 65535);
          return device & 65535;
        } // ((1) + (vector_value_out7)) < (64)
      } else {
        // BDDNode 82
        inc_path_counter(82);
        packet_return_chunk(buffer, hdr2);
        // BDDNode 83
        inc_path_counter(83);
        hdr[6] = quotient3 & 255;
        hdr[7] = (quotient3>>8) & 255;
        hdr[8] = (quotient3>>16) & 255;
        hdr[9] = (quotient3>>24) & 255;
        hdr[10] = 0;
        hdr[11] = 0;
        packet_return_chunk(buffer, hdr);
        // BDDNode 84
        inc_path_counter(84);
        forwarding_stats_per_route_op[84].inc_fwd(device & 65535);
        return device & 65535;
      } // (quotient3) < (160)
    } // (trailing_zeros) < (20)
  } else {
    // BDDNode 85
    inc_path_counter(85);
    packet_return_chunk(buffer, hdr);
    // BDDNode 86
    inc_path_counter(86);
    forwarding_stats_per_route_op[86].inc_drop();
    return DROP;
  } // ((8) == (*(uint16_t*)(uint16_t*)(hdr+12))) & ((20ULL) <= ((uint16_t)((uint32_t)((4294967282) + ((uint16_t)(packet_length & 65535))))))
}
