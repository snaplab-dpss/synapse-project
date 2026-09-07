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

struct BloomFilter *bf;
struct Vector *vector;


bool nf_init() {
  int bf_allocation_succeeded = bf_allocate(2, 1048576, 12, 0ULL, &bf);
  if (!bf_allocation_succeeded) {
    return false;
  }
  int vector_alloc_success = vector_allocate(4, 1, &vector);
  if (!vector_alloc_success) {
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
  forwarding_stats_per_route_op.insert({230, PortStats{}});
  forwarding_stats_per_route_op.insert({228, PortStats{}});
  forwarding_stats_per_route_op.insert({217, PortStats{}});
  forwarding_stats_per_route_op.insert({95, PortStats{}});
  forwarding_stats_per_route_op.insert({225, PortStats{}});
  forwarding_stats_per_route_op.insert({99, PortStats{}});
  forwarding_stats_per_route_op.insert({103, PortStats{}});
  forwarding_stats_per_route_op.insert({9, PortStats{}});
  forwarding_stats_per_route_op.insert({183, PortStats{}});
  forwarding_stats_per_route_op.insert({209, PortStats{}});
  forwarding_stats_per_route_op.insert({187, PortStats{}});
  forwarding_stats_per_route_op.insert({200, PortStats{}});
  forwarding_stats_per_route_op.insert({197, PortStats{}});
  forwarding_stats_per_route_op.insert({192, PortStats{}});
  forwarding_stats_per_route_op.insert({221, PortStats{}});
  node_pkt_counter.insert({230, 0});
  node_pkt_counter.insert({229, 0});
  node_pkt_counter.insert({228, 0});
  node_pkt_counter.insert({227, 0});
  node_pkt_counter.insert({226, 0});
  node_pkt_counter.insert({225, 0});
  node_pkt_counter.insert({224, 0});
  node_pkt_counter.insert({223, 0});
  node_pkt_counter.insert({222, 0});
  node_pkt_counter.insert({221, 0});
  node_pkt_counter.insert({220, 0});
  node_pkt_counter.insert({219, 0});
  node_pkt_counter.insert({218, 0});
  node_pkt_counter.insert({217, 0});
  node_pkt_counter.insert({216, 0});
  node_pkt_counter.insert({215, 0});
  node_pkt_counter.insert({214, 0});
  node_pkt_counter.insert({213, 0});
  node_pkt_counter.insert({212, 0});
  node_pkt_counter.insert({458, 0});
  node_pkt_counter.insert({211, 0});
  node_pkt_counter.insert({210, 0});
  node_pkt_counter.insert({209, 0});
  node_pkt_counter.insert({208, 0});
  node_pkt_counter.insert({207, 0});
  node_pkt_counter.insert({206, 0});
  node_pkt_counter.insert({205, 0});
  node_pkt_counter.insert({204, 0});
  node_pkt_counter.insert({203, 0});
  node_pkt_counter.insert({202, 0});
  node_pkt_counter.insert({201, 0});
  node_pkt_counter.insert({200, 0});
  node_pkt_counter.insert({199, 0});
  node_pkt_counter.insert({198, 0});
  node_pkt_counter.insert({197, 0});
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
  node_pkt_counter.insert({457, 0});
  node_pkt_counter.insert({456, 0});
  node_pkt_counter.insert({455, 0});
  node_pkt_counter.insert({454, 0});
  node_pkt_counter.insert({453, 0});
  node_pkt_counter.insert({452, 0});
  node_pkt_counter.insert({451, 0});
  node_pkt_counter.insert({450, 0});
  node_pkt_counter.insert({449, 0});
  node_pkt_counter.insert({448, 0});
  node_pkt_counter.insert({447, 0});
  node_pkt_counter.insert({179, 0});
  node_pkt_counter.insert({178, 0});
  node_pkt_counter.insert({446, 0});
  node_pkt_counter.insert({177, 0});
  node_pkt_counter.insert({445, 0});
  node_pkt_counter.insert({444, 0});
  node_pkt_counter.insert({176, 0});
  node_pkt_counter.insert({443, 0});
  node_pkt_counter.insert({175, 0});
  node_pkt_counter.insert({442, 0});
  node_pkt_counter.insert({174, 0});
  node_pkt_counter.insert({441, 0});
  node_pkt_counter.insert({440, 0});
  node_pkt_counter.insert({173, 0});
  node_pkt_counter.insert({439, 0});
  node_pkt_counter.insert({172, 0});
  node_pkt_counter.insert({438, 0});
  node_pkt_counter.insert({171, 0});
  node_pkt_counter.insert({437, 0});
  node_pkt_counter.insert({436, 0});
  node_pkt_counter.insert({170, 0});
  node_pkt_counter.insert({435, 0});
  node_pkt_counter.insert({169, 0});
  node_pkt_counter.insert({434, 0});
  node_pkt_counter.insert({168, 0});
  node_pkt_counter.insert({433, 0});
  node_pkt_counter.insert({432, 0});
  node_pkt_counter.insert({167, 0});
  node_pkt_counter.insert({431, 0});
  node_pkt_counter.insert({166, 0});
  node_pkt_counter.insert({430, 0});
  node_pkt_counter.insert({165, 0});
  node_pkt_counter.insert({429, 0});
  node_pkt_counter.insert({428, 0});
  node_pkt_counter.insert({164, 0});
  node_pkt_counter.insert({427, 0});
  node_pkt_counter.insert({163, 0});
  node_pkt_counter.insert({426, 0});
  node_pkt_counter.insert({162, 0});
  node_pkt_counter.insert({425, 0});
  node_pkt_counter.insert({424, 0});
  node_pkt_counter.insert({161, 0});
  node_pkt_counter.insert({423, 0});
  node_pkt_counter.insert({160, 0});
  node_pkt_counter.insert({422, 0});
  node_pkt_counter.insert({159, 0});
  node_pkt_counter.insert({421, 0});
  node_pkt_counter.insert({420, 0});
  node_pkt_counter.insert({158, 0});
  node_pkt_counter.insert({419, 0});
  node_pkt_counter.insert({157, 0});
  node_pkt_counter.insert({418, 0});
  node_pkt_counter.insert({417, 0});
  node_pkt_counter.insert({156, 0});
  node_pkt_counter.insert({416, 0});
  node_pkt_counter.insert({415, 0});
  node_pkt_counter.insert({155, 0});
  node_pkt_counter.insert({414, 0});
  node_pkt_counter.insert({154, 0});
  node_pkt_counter.insert({413, 0});
  node_pkt_counter.insert({153, 0});
  node_pkt_counter.insert({412, 0});
  node_pkt_counter.insert({411, 0});
  node_pkt_counter.insert({152, 0});
  node_pkt_counter.insert({410, 0});
  node_pkt_counter.insert({151, 0});
  node_pkt_counter.insert({409, 0});
  node_pkt_counter.insert({150, 0});
  node_pkt_counter.insert({408, 0});
  node_pkt_counter.insert({407, 0});
  node_pkt_counter.insert({149, 0});
  node_pkt_counter.insert({406, 0});
  node_pkt_counter.insert({148, 0});
  node_pkt_counter.insert({405, 0});
  node_pkt_counter.insert({147, 0});
  node_pkt_counter.insert({404, 0});
  node_pkt_counter.insert({403, 0});
  node_pkt_counter.insert({146, 0});
  node_pkt_counter.insert({402, 0});
  node_pkt_counter.insert({145, 0});
  node_pkt_counter.insert({401, 0});
  node_pkt_counter.insert({400, 0});
  node_pkt_counter.insert({144, 0});
  node_pkt_counter.insert({399, 0});
  node_pkt_counter.insert({398, 0});
  node_pkt_counter.insert({397, 0});
  node_pkt_counter.insert({143, 0});
  node_pkt_counter.insert({396, 0});
  node_pkt_counter.insert({142, 0});
  node_pkt_counter.insert({395, 0});
  node_pkt_counter.insert({141, 0});
  node_pkt_counter.insert({394, 0});
  node_pkt_counter.insert({393, 0});
  node_pkt_counter.insert({140, 0});
  node_pkt_counter.insert({392, 0});
  node_pkt_counter.insert({139, 0});
  node_pkt_counter.insert({391, 0});
  node_pkt_counter.insert({138, 0});
  node_pkt_counter.insert({390, 0});
  node_pkt_counter.insert({389, 0});
  node_pkt_counter.insert({137, 0});
  node_pkt_counter.insert({388, 0});
  node_pkt_counter.insert({136, 0});
  node_pkt_counter.insert({387, 0});
  node_pkt_counter.insert({135, 0});
  node_pkt_counter.insert({386, 0});
  node_pkt_counter.insert({385, 0});
  node_pkt_counter.insert({134, 0});
  node_pkt_counter.insert({384, 0});
  node_pkt_counter.insert({133, 0});
  node_pkt_counter.insert({383, 0});
  node_pkt_counter.insert({382, 0});
  node_pkt_counter.insert({132, 0});
  node_pkt_counter.insert({381, 0});
  node_pkt_counter.insert({380, 0});
  node_pkt_counter.insert({379, 0});
  node_pkt_counter.insert({378, 0});
  node_pkt_counter.insert({377, 0});
  node_pkt_counter.insert({131, 0});
  node_pkt_counter.insert({376, 0});
  node_pkt_counter.insert({130, 0});
  node_pkt_counter.insert({375, 0});
  node_pkt_counter.insert({129, 0});
  node_pkt_counter.insert({374, 0});
  node_pkt_counter.insert({373, 0});
  node_pkt_counter.insert({128, 0});
  node_pkt_counter.insert({372, 0});
  node_pkt_counter.insert({127, 0});
  node_pkt_counter.insert({371, 0});
  node_pkt_counter.insert({126, 0});
  node_pkt_counter.insert({370, 0});
  node_pkt_counter.insert({369, 0});
  node_pkt_counter.insert({125, 0});
  node_pkt_counter.insert({368, 0});
  node_pkt_counter.insert({124, 0});
  node_pkt_counter.insert({367, 0});
  node_pkt_counter.insert({123, 0});
  node_pkt_counter.insert({61, 0});
  node_pkt_counter.insert({318, 0});
  node_pkt_counter.insert({60, 0});
  node_pkt_counter.insert({317, 0});
  node_pkt_counter.insert({59, 0});
  node_pkt_counter.insert({316, 0});
  node_pkt_counter.insert({58, 0});
  node_pkt_counter.insert({315, 0});
  node_pkt_counter.insert({57, 0});
  node_pkt_counter.insert({314, 0});
  node_pkt_counter.insert({56, 0});
  node_pkt_counter.insert({313, 0});
  node_pkt_counter.insert({55, 0});
  node_pkt_counter.insert({312, 0});
  node_pkt_counter.insert({54, 0});
  node_pkt_counter.insert({311, 0});
  node_pkt_counter.insert({53, 0});
  node_pkt_counter.insert({310, 0});
  node_pkt_counter.insert({52, 0});
  node_pkt_counter.insert({309, 0});
  node_pkt_counter.insert({51, 0});
  node_pkt_counter.insert({308, 0});
  node_pkt_counter.insert({50, 0});
  node_pkt_counter.insert({307, 0});
  node_pkt_counter.insert({49, 0});
  node_pkt_counter.insert({306, 0});
  node_pkt_counter.insert({48, 0});
  node_pkt_counter.insert({305, 0});
  node_pkt_counter.insert({47, 0});
  node_pkt_counter.insert({304, 0});
  node_pkt_counter.insert({46, 0});
  node_pkt_counter.insert({303, 0});
  node_pkt_counter.insert({45, 0});
  node_pkt_counter.insert({302, 0});
  node_pkt_counter.insert({44, 0});
  node_pkt_counter.insert({301, 0});
  node_pkt_counter.insert({297, 0});
  node_pkt_counter.insert({40, 0});
  node_pkt_counter.insert({43, 0});
  node_pkt_counter.insert({300, 0});
  node_pkt_counter.insert({296, 0});
  node_pkt_counter.insert({39, 0});
  node_pkt_counter.insert({42, 0});
  node_pkt_counter.insert({299, 0});
  node_pkt_counter.insert({295, 0});
  node_pkt_counter.insert({38, 0});
  node_pkt_counter.insert({41, 0});
  node_pkt_counter.insert({298, 0});
  node_pkt_counter.insert({294, 0});
  node_pkt_counter.insert({37, 0});
  node_pkt_counter.insert({293, 0});
  node_pkt_counter.insert({36, 0});
  node_pkt_counter.insert({292, 0});
  node_pkt_counter.insert({35, 0});
  node_pkt_counter.insert({291, 0});
  node_pkt_counter.insert({34, 0});
  node_pkt_counter.insert({290, 0});
  node_pkt_counter.insert({33, 0});
  node_pkt_counter.insert({277, 0});
  node_pkt_counter.insert({20, 0});
  node_pkt_counter.insert({23, 0});
  node_pkt_counter.insert({280, 0});
  node_pkt_counter.insert({276, 0});
  node_pkt_counter.insert({19, 0});
  node_pkt_counter.insert({22, 0});
  node_pkt_counter.insert({279, 0});
  node_pkt_counter.insert({275, 0});
  node_pkt_counter.insert({18, 0});
  node_pkt_counter.insert({21, 0});
  node_pkt_counter.insert({278, 0});
  node_pkt_counter.insert({233, 0});
  node_pkt_counter.insert({274, 0});
  node_pkt_counter.insert({17, 0});
  node_pkt_counter.insert({232, 0});
  node_pkt_counter.insert({231, 0});
  node_pkt_counter.insert({273, 0});
  node_pkt_counter.insert({16, 0});
  node_pkt_counter.insert({255, 0});
  node_pkt_counter.insert({272, 0});
  node_pkt_counter.insert({15, 0});
  node_pkt_counter.insert({254, 0});
  node_pkt_counter.insert({271, 0});
  node_pkt_counter.insert({14, 0});
  node_pkt_counter.insert({253, 0});
  node_pkt_counter.insert({270, 0});
  node_pkt_counter.insert({13, 0});
  node_pkt_counter.insert({252, 0});
  node_pkt_counter.insert({269, 0});
  node_pkt_counter.insert({12, 0});
  node_pkt_counter.insert({251, 0});
  node_pkt_counter.insert({234, 0});
  node_pkt_counter.insert({256, 0});
  node_pkt_counter.insert({2, 0});
  node_pkt_counter.insert({259, 0});
  node_pkt_counter.insert({238, 0});
  node_pkt_counter.insert({235, 0});
  node_pkt_counter.insert({257, 0});
  node_pkt_counter.insert({3, 0});
  node_pkt_counter.insert({260, 0});
  node_pkt_counter.insert({239, 0});
  node_pkt_counter.insert({236, 0});
  node_pkt_counter.insert({258, 0});
  node_pkt_counter.insert({4, 0});
  node_pkt_counter.insert({261, 0});
  node_pkt_counter.insert({240, 0});
  node_pkt_counter.insert({237, 0});
  node_pkt_counter.insert({5, 0});
  node_pkt_counter.insert({262, 0});
  node_pkt_counter.insert({241, 0});
  node_pkt_counter.insert({6, 0});
  node_pkt_counter.insert({263, 0});
  node_pkt_counter.insert({242, 0});
  node_pkt_counter.insert({7, 0});
  node_pkt_counter.insert({264, 0});
  node_pkt_counter.insert({243, 0});
  node_pkt_counter.insert({8, 0});
  node_pkt_counter.insert({265, 0});
  node_pkt_counter.insert({244, 0});
  node_pkt_counter.insert({9, 0});
  node_pkt_counter.insert({266, 0});
  node_pkt_counter.insert({245, 0});
  node_pkt_counter.insert({10, 0});
  node_pkt_counter.insert({267, 0});
  node_pkt_counter.insert({246, 0});
  node_pkt_counter.insert({11, 0});
  node_pkt_counter.insert({268, 0});
  node_pkt_counter.insert({247, 0});
  node_pkt_counter.insert({248, 0});
  node_pkt_counter.insert({249, 0});
  node_pkt_counter.insert({250, 0});
  node_pkt_counter.insert({24, 0});
  node_pkt_counter.insert({281, 0});
  node_pkt_counter.insert({25, 0});
  node_pkt_counter.insert({282, 0});
  node_pkt_counter.insert({26, 0});
  node_pkt_counter.insert({283, 0});
  node_pkt_counter.insert({27, 0});
  node_pkt_counter.insert({284, 0});
  node_pkt_counter.insert({28, 0});
  node_pkt_counter.insert({285, 0});
  node_pkt_counter.insert({29, 0});
  node_pkt_counter.insert({286, 0});
  node_pkt_counter.insert({30, 0});
  node_pkt_counter.insert({287, 0});
  node_pkt_counter.insert({31, 0});
  node_pkt_counter.insert({288, 0});
  node_pkt_counter.insert({32, 0});
  node_pkt_counter.insert({289, 0});
  node_pkt_counter.insert({62, 0});
  node_pkt_counter.insert({319, 0});
  node_pkt_counter.insert({63, 0});
  node_pkt_counter.insert({320, 0});
  node_pkt_counter.insert({64, 0});
  node_pkt_counter.insert({321, 0});
  node_pkt_counter.insert({65, 0});
  node_pkt_counter.insert({322, 0});
  node_pkt_counter.insert({66, 0});
  node_pkt_counter.insert({323, 0});
  node_pkt_counter.insert({67, 0});
  node_pkt_counter.insert({324, 0});
  node_pkt_counter.insert({68, 0});
  node_pkt_counter.insert({325, 0});
  node_pkt_counter.insert({69, 0});
  node_pkt_counter.insert({326, 0});
  node_pkt_counter.insert({70, 0});
  node_pkt_counter.insert({327, 0});
  node_pkt_counter.insert({71, 0});
  node_pkt_counter.insert({328, 0});
  node_pkt_counter.insert({72, 0});
  node_pkt_counter.insert({329, 0});
  node_pkt_counter.insert({73, 0});
  node_pkt_counter.insert({330, 0});
  node_pkt_counter.insert({74, 0});
  node_pkt_counter.insert({331, 0});
  node_pkt_counter.insert({75, 0});
  node_pkt_counter.insert({332, 0});
  node_pkt_counter.insert({76, 0});
  node_pkt_counter.insert({333, 0});
  node_pkt_counter.insert({77, 0});
  node_pkt_counter.insert({334, 0});
  node_pkt_counter.insert({78, 0});
  node_pkt_counter.insert({335, 0});
  node_pkt_counter.insert({79, 0});
  node_pkt_counter.insert({336, 0});
  node_pkt_counter.insert({80, 0});
  node_pkt_counter.insert({337, 0});
  node_pkt_counter.insert({81, 0});
  node_pkt_counter.insert({338, 0});
  node_pkt_counter.insert({82, 0});
  node_pkt_counter.insert({339, 0});
  node_pkt_counter.insert({83, 0});
  node_pkt_counter.insert({340, 0});
  node_pkt_counter.insert({84, 0});
  node_pkt_counter.insert({341, 0});
  node_pkt_counter.insert({85, 0});
  node_pkt_counter.insert({342, 0});
  node_pkt_counter.insert({86, 0});
  node_pkt_counter.insert({343, 0});
  node_pkt_counter.insert({87, 0});
  node_pkt_counter.insert({344, 0});
  node_pkt_counter.insert({88, 0});
  node_pkt_counter.insert({345, 0});
  node_pkt_counter.insert({89, 0});
  node_pkt_counter.insert({346, 0});
  node_pkt_counter.insert({90, 0});
  node_pkt_counter.insert({347, 0});
  node_pkt_counter.insert({91, 0});
  node_pkt_counter.insert({348, 0});
  node_pkt_counter.insert({92, 0});
  node_pkt_counter.insert({349, 0});
  node_pkt_counter.insert({93, 0});
  node_pkt_counter.insert({350, 0});
  node_pkt_counter.insert({94, 0});
  node_pkt_counter.insert({351, 0});
  node_pkt_counter.insert({95, 0});
  node_pkt_counter.insert({352, 0});
  node_pkt_counter.insert({96, 0});
  node_pkt_counter.insert({353, 0});
  node_pkt_counter.insert({97, 0});
  node_pkt_counter.insert({354, 0});
  node_pkt_counter.insert({98, 0});
  node_pkt_counter.insert({355, 0});
  node_pkt_counter.insert({99, 0});
  node_pkt_counter.insert({356, 0});
  node_pkt_counter.insert({100, 0});
  node_pkt_counter.insert({357, 0});
  node_pkt_counter.insert({101, 0});
  node_pkt_counter.insert({358, 0});
  node_pkt_counter.insert({102, 0});
  node_pkt_counter.insert({359, 0});
  node_pkt_counter.insert({103, 0});
  node_pkt_counter.insert({360, 0});
  node_pkt_counter.insert({104, 0});
  node_pkt_counter.insert({361, 0});
  node_pkt_counter.insert({105, 0});
  node_pkt_counter.insert({362, 0});
  node_pkt_counter.insert({106, 0});
  node_pkt_counter.insert({363, 0});
  node_pkt_counter.insert({107, 0});
  node_pkt_counter.insert({364, 0});
  node_pkt_counter.insert({108, 0});
  node_pkt_counter.insert({365, 0});
  node_pkt_counter.insert({109, 0});
  node_pkt_counter.insert({366, 0});
  node_pkt_counter.insert({110, 0});
  node_pkt_counter.insert({111, 0});
  node_pkt_counter.insert({112, 0});
  node_pkt_counter.insert({113, 0});
  node_pkt_counter.insert({114, 0});
  node_pkt_counter.insert({115, 0});
  node_pkt_counter.insert({116, 0});
  node_pkt_counter.insert({117, 0});
  node_pkt_counter.insert({118, 0});
  node_pkt_counter.insert({119, 0});
  node_pkt_counter.insert({120, 0});
  node_pkt_counter.insert({121, 0});
  node_pkt_counter.insert({122, 0});
  return true;
}


int nf_process(uint16_t device, uint8_t *buffer, uint16_t packet_length, time_ns_t now) {
  // BDDNode 2
  inc_path_counter(2);
  uint8_t* hdr;
  packet_borrow_next_chunk(buffer, 14, (void**)&hdr);
  // BDDNode 3
  inc_path_counter(3);
  if (((8) == (*(uint16_t*)(uint16_t*)(hdr+12))) & ((20ULL) <= ((uint16_t)((uint32_t)((4294967282) + ((uint16_t)(packet_length & 65535))))))) {
    // BDDNode 4
    inc_path_counter(4);
    uint8_t* hdr2;
    packet_borrow_next_chunk(buffer, 20, (void**)&hdr2);
    // BDDNode 5
    inc_path_counter(5);
    if ((17) != (*(hdr2+9))) {
      // BDDNode 6
      inc_path_counter(6);
      if ((6) != (*(hdr2+9))) {
        // BDDNode 7
        inc_path_counter(7);
        packet_return_chunk(buffer, hdr2);
        // BDDNode 8
        inc_path_counter(8);
        packet_return_chunk(buffer, hdr);
        // BDDNode 9
        inc_path_counter(9);
        forwarding_stats_per_route_op[9].inc_fwd((uint32_t)((((uint32_t)(((uint32_t)(((uint16_t)(*(hdr2+16)) << 8 | (uint16_t)(*(hdr2+17)))) << 8 | (uint32_t)(*(hdr2+18)))) << 8 | (uint32_t)(*(hdr2+19)))) >> (24)));
        return (uint32_t)((((uint32_t)(((uint32_t)(((uint16_t)(*(hdr2+16)) << 8 | (uint16_t)(*(hdr2+17)))) << 8 | (uint32_t)(*(hdr2+18)))) << 8 | (uint32_t)(*(hdr2+19)))) >> (24));
      } else {
        // BDDNode 10
        inc_path_counter(10);
        if ((20ULL) <= ((uint32_t)((4294967262) + ((uint16_t)(packet_length & 65535))))) {
          // BDDNode 11
          inc_path_counter(11);
          uint8_t* hdr3;
          packet_borrow_next_chunk(buffer, 20, (void**)&hdr3);
          // BDDNode 12
          inc_path_counter(12);
          if ((0) != (device & 65535)) {
            // BDDNode 13
            inc_path_counter(13);
            if ((0) == (((uint8_t)(*(hdr3+13))) & (2))) {
              // BDDNode 14
              inc_path_counter(14);
              uint8_t key[12];
              key[0] = *(hdr2+12);
              key[1] = *(hdr2+13);
              key[2] = *(hdr2+14);
              key[3] = *(hdr2+15);
              key[4] = *(hdr2+16);
              key[5] = *(hdr2+17);
              key[6] = *(hdr2+18);
              key[7] = *(hdr2+19);
              key[8] = *(hdr3+0);
              key[9] = *(hdr3+1);
              key[10] = *(hdr3+2);
              key[11] = *(hdr3+3);
              int bf_query_estimate = bf_query(bf, key);
              // BDDNode 15
              inc_path_counter(15);
              if ((0) == (bf_query_estimate)) {
                // BDDNode 16
                inc_path_counter(16);
                uint32_t rotated = rotate_left(740644437, 5);
                // BDDNode 17
                inc_path_counter(17);
                uint32_t rotated2 = rotate_left((993352779) ^ (((uint32_t)(((uint32_t)(((uint16_t)(*(key+0)) << 8 | (uint16_t)(*(key+1)))) << 8 | (uint32_t)(*(key+2)))) << 8 | (uint32_t)(*(key+3)))), 8);
                // BDDNode 18
                inc_path_counter(18);
                uint32_t rotated3 = rotate_left(1870056090, 16);
                // BDDNode 19
                inc_path_counter(19);
                uint32_t rotated4 = rotate_left((1870056090) ^ (rotated), 13);
                // BDDNode 231
                inc_path_counter(231);
                uint32_t unrolled = (993352779) ^ (((uint32_t)(((uint32_t)(((uint16_t)(*(key+0)) << 8 | (uint16_t)(*(key+1)))) << 8 | (uint32_t)(*(key+2)))) << 8 | (uint32_t)(*(key+3))));
                // BDDNode 232
                inc_path_counter(232);
                uint32_t unrolled2 = (1564959569) + (unrolled);
                // BDDNode 20
                inc_path_counter(20);
                uint32_t rotated5 = rotate_left((rotated2) ^ (unrolled2), 7);
                // BDDNode 233
                inc_path_counter(233);
                uint32_t unrolled3 = (1870056090) ^ (rotated);
                // BDDNode 234
                inc_path_counter(234);
                uint32_t unrolled4 = (unrolled) + (unrolled3);
                // BDDNode 21
                inc_path_counter(21);
                uint32_t rotated6 = rotate_left((1564959569) + (unrolled4), 16);
                // BDDNode 235
                inc_path_counter(235);
                uint32_t unrolled5 = (1564959569) + (unrolled4);
                // BDDNode 22
                inc_path_counter(22);
                uint32_t rotated7 = rotate_left((rotated4) ^ (unrolled5), 5);
                // BDDNode 236
                inc_path_counter(236);
                uint32_t unrolled6 = (rotated2) ^ (unrolled2);
                // BDDNode 237
                inc_path_counter(237);
                uint32_t unrolled7 = (rotated3) + (unrolled6);
                // BDDNode 23
                inc_path_counter(23);
                uint32_t rotated8 = rotate_left((rotated5) ^ (unrolled7), 8);
                // BDDNode 238
                inc_path_counter(238);
                uint32_t unrolled8 = (rotated4) ^ (unrolled5);
                // BDDNode 24
                inc_path_counter(24);
                uint32_t rotated9 = rotate_left((unrolled7) + (unrolled8), 16);
                // BDDNode 239
                inc_path_counter(239);
                uint32_t unrolled9 = (unrolled7) + (unrolled8);
                // BDDNode 25
                inc_path_counter(25);
                uint32_t rotated10 = rotate_left((rotated7) ^ (unrolled9), 13);
                // BDDNode 240
                inc_path_counter(240);
                uint32_t unrolled10 = (rotated5) ^ (unrolled7);
                // BDDNode 241
                inc_path_counter(241);
                uint32_t unrolled11 = (rotated6) + (unrolled10);
                // BDDNode 26
                inc_path_counter(26);
                uint32_t rotated11 = rotate_left((rotated8) ^ (unrolled11), 7);
                // BDDNode 242
                inc_path_counter(242);
                uint32_t unrolled12 = (rotated7) ^ (unrolled9);
                // BDDNode 27
                inc_path_counter(27);
                uint32_t rotated12 = rotate_left((unrolled11) + (unrolled12), 16);
                // BDDNode 243
                inc_path_counter(243);
                uint32_t unrolled13 = (unrolled11) + (unrolled12);
                // BDDNode 28
                inc_path_counter(28);
                uint32_t rotated13 = rotate_left((rotated10) ^ (unrolled13), 5);
                // BDDNode 244
                inc_path_counter(244);
                uint32_t unrolled14 = (rotated8) ^ (unrolled11);
                // BDDNode 245
                inc_path_counter(245);
                uint32_t unrolled15 = (rotated9) + (unrolled14);
                // BDDNode 246
                inc_path_counter(246);
                uint32_t unrolled16 = (rotated11) ^ (unrolled15);
                // BDDNode 29
                inc_path_counter(29);
                uint32_t rotated14 = rotate_left((unrolled16) ^ (((uint32_t)(((uint32_t)(((uint16_t)(*(key+4)) << 8 | (uint16_t)(*(key+5)))) << 8 | (uint32_t)(*(key+6)))) << 8 | (uint32_t)(*(key+7)))), 8);
                // BDDNode 247
                inc_path_counter(247);
                uint32_t unrolled17 = (unrolled15) ^ (((uint32_t)(((uint32_t)(((uint16_t)(*(key+0)) << 8 | (uint16_t)(*(key+1)))) << 8 | (uint32_t)(*(key+2)))) << 8 | (uint32_t)(*(key+3))));
                // BDDNode 248
                inc_path_counter(248);
                uint32_t unrolled18 = (rotated10) ^ (unrolled13);
                // BDDNode 30
                inc_path_counter(30);
                uint32_t rotated15 = rotate_left((unrolled17) + (unrolled18), 16);
                // BDDNode 249
                inc_path_counter(249);
                uint32_t unrolled19 = (unrolled17) + (unrolled18);
                // BDDNode 31
                inc_path_counter(31);
                uint32_t rotated16 = rotate_left((rotated13) ^ (unrolled19), 13);
                // BDDNode 250
                inc_path_counter(250);
                uint32_t unrolled20 = (unrolled16) ^ (((uint32_t)(((uint32_t)(((uint16_t)(*(key+4)) << 8 | (uint16_t)(*(key+5)))) << 8 | (uint32_t)(*(key+6)))) << 8 | (uint32_t)(*(key+7))));
                // BDDNode 251
                inc_path_counter(251);
                uint32_t unrolled21 = (rotated12) + (unrolled20);
                // BDDNode 32
                inc_path_counter(32);
                uint32_t rotated17 = rotate_left((rotated14) ^ (unrolled21), 7);
                // BDDNode 252
                inc_path_counter(252);
                uint32_t unrolled22 = (rotated13) ^ (unrolled19);
                // BDDNode 33
                inc_path_counter(33);
                uint32_t rotated18 = rotate_left((unrolled21) + (unrolled22), 16);
                // BDDNode 253
                inc_path_counter(253);
                uint32_t unrolled23 = (unrolled21) + (unrolled22);
                // BDDNode 34
                inc_path_counter(34);
                uint32_t rotated19 = rotate_left((rotated16) ^ (unrolled23), 5);
                // BDDNode 254
                inc_path_counter(254);
                uint32_t unrolled24 = (rotated14) ^ (unrolled21);
                // BDDNode 255
                inc_path_counter(255);
                uint32_t unrolled25 = (rotated15) + (unrolled24);
                // BDDNode 35
                inc_path_counter(35);
                uint32_t rotated20 = rotate_left((rotated17) ^ (unrolled25), 8);
                // BDDNode 256
                inc_path_counter(256);
                uint32_t unrolled26 = (rotated16) ^ (unrolled23);
                // BDDNode 36
                inc_path_counter(36);
                uint32_t rotated21 = rotate_left((unrolled25) + (unrolled26), 16);
                // BDDNode 257
                inc_path_counter(257);
                uint32_t unrolled27 = (unrolled25) + (unrolled26);
                // BDDNode 37
                inc_path_counter(37);
                uint32_t rotated22 = rotate_left((rotated19) ^ (unrolled27), 13);
                // BDDNode 258
                inc_path_counter(258);
                uint32_t unrolled28 = (rotated17) ^ (unrolled25);
                // BDDNode 259
                inc_path_counter(259);
                uint32_t unrolled29 = (rotated18) + (unrolled28);
                // BDDNode 38
                inc_path_counter(38);
                uint32_t rotated23 = rotate_left((rotated20) ^ (unrolled29), 7);
                // BDDNode 260
                inc_path_counter(260);
                uint32_t unrolled30 = (rotated19) ^ (unrolled27);
                // BDDNode 39
                inc_path_counter(39);
                uint32_t rotated24 = rotate_left((unrolled29) + (unrolled30), 16);
                // BDDNode 261
                inc_path_counter(261);
                uint32_t unrolled31 = (unrolled29) + (unrolled30);
                // BDDNode 40
                inc_path_counter(40);
                uint32_t rotated25 = rotate_left((rotated22) ^ (unrolled31), 5);
                // BDDNode 262
                inc_path_counter(262);
                uint32_t unrolled32 = (rotated20) ^ (unrolled29);
                // BDDNode 263
                inc_path_counter(263);
                uint32_t unrolled33 = (rotated21) + (unrolled32);
                // BDDNode 264
                inc_path_counter(264);
                uint32_t unrolled34 = (rotated23) ^ (unrolled33);
                // BDDNode 265
                inc_path_counter(265);
                uint32_t unrolled35 = ((uint16_t)(((uint16_t)(*(key+8)) << 8 | (uint16_t)(*(key+9))))) << (16);
                // BDDNode 266
                inc_path_counter(266);
                uint32_t unrolled36 = (unrolled35) | ((uint16_t)(((uint16_t)(*(key+10)) << 8 | (uint16_t)(*(key+11)))));
                // BDDNode 41
                inc_path_counter(41);
                uint32_t rotated26 = rotate_left((unrolled34) ^ (unrolled36), 8);
                // BDDNode 267
                inc_path_counter(267);
                uint32_t unrolled37 = (unrolled33) ^ (((uint32_t)(((uint32_t)(((uint16_t)(*(key+4)) << 8 | (uint16_t)(*(key+5)))) << 8 | (uint32_t)(*(key+6)))) << 8 | (uint32_t)(*(key+7))));
                // BDDNode 268
                inc_path_counter(268);
                uint32_t unrolled38 = (rotated22) ^ (unrolled31);
                // BDDNode 42
                inc_path_counter(42);
                uint32_t rotated27 = rotate_left((unrolled37) + (unrolled38), 16);
                // BDDNode 269
                inc_path_counter(269);
                uint32_t unrolled39 = (unrolled37) + (unrolled38);
                // BDDNode 43
                inc_path_counter(43);
                uint32_t rotated28 = rotate_left((rotated25) ^ (unrolled39), 13);
                // BDDNode 270
                inc_path_counter(270);
                uint32_t unrolled40 = (unrolled34) ^ (unrolled36);
                // BDDNode 271
                inc_path_counter(271);
                uint32_t unrolled41 = (rotated24) + (unrolled40);
                // BDDNode 44
                inc_path_counter(44);
                uint32_t rotated29 = rotate_left((rotated26) ^ (unrolled41), 7);
                // BDDNode 272
                inc_path_counter(272);
                uint32_t unrolled42 = (rotated25) ^ (unrolled39);
                // BDDNode 45
                inc_path_counter(45);
                uint32_t rotated30 = rotate_left((unrolled41) + (unrolled42), 16);
                // BDDNode 273
                inc_path_counter(273);
                uint32_t unrolled43 = (unrolled41) + (unrolled42);
                // BDDNode 46
                inc_path_counter(46);
                uint32_t rotated31 = rotate_left((rotated28) ^ (unrolled43), 5);
                // BDDNode 274
                inc_path_counter(274);
                uint32_t unrolled44 = (rotated26) ^ (unrolled41);
                // BDDNode 275
                inc_path_counter(275);
                uint32_t unrolled45 = (rotated27) + (unrolled44);
                // BDDNode 47
                inc_path_counter(47);
                uint32_t rotated32 = rotate_left((rotated29) ^ (unrolled45), 8);
                // BDDNode 276
                inc_path_counter(276);
                uint32_t unrolled46 = (rotated28) ^ (unrolled43);
                // BDDNode 48
                inc_path_counter(48);
                uint32_t rotated33 = rotate_left((unrolled45) + (unrolled46), 16);
                // BDDNode 277
                inc_path_counter(277);
                uint32_t unrolled47 = (unrolled45) + (unrolled46);
                // BDDNode 49
                inc_path_counter(49);
                uint32_t rotated34 = rotate_left((rotated31) ^ (unrolled47), 13);
                // BDDNode 278
                inc_path_counter(278);
                uint32_t unrolled48 = (rotated29) ^ (unrolled45);
                // BDDNode 279
                inc_path_counter(279);
                uint32_t unrolled49 = (rotated30) + (unrolled48);
                // BDDNode 50
                inc_path_counter(50);
                uint32_t rotated35 = rotate_left((rotated32) ^ (unrolled49), 7);
                // BDDNode 280
                inc_path_counter(280);
                uint32_t unrolled50 = (rotated31) ^ (unrolled47);
                // BDDNode 51
                inc_path_counter(51);
                uint32_t rotated36 = rotate_left((unrolled49) + (unrolled50), 16);
                // BDDNode 281
                inc_path_counter(281);
                uint32_t unrolled51 = (unrolled49) + (unrolled50);
                // BDDNode 52
                inc_path_counter(52);
                uint32_t rotated37 = rotate_left((rotated34) ^ (unrolled51), 5);
                // BDDNode 282
                inc_path_counter(282);
                uint32_t unrolled52 = (rotated32) ^ (unrolled49);
                // BDDNode 283
                inc_path_counter(283);
                uint32_t unrolled53 = (rotated33) + (unrolled52);
                // BDDNode 284
                inc_path_counter(284);
                uint32_t unrolled54 = (rotated35) ^ (unrolled53);
                // BDDNode 285
                inc_path_counter(285);
                uint32_t unrolled55 = (4294967295) + (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5)))) << 8 | (uint32_t)(*(hdr3+6)))) << 8 | (uint32_t)(*(hdr3+7))));
                // BDDNode 53
                inc_path_counter(53);
                uint32_t rotated38 = rotate_left((unrolled54) ^ (unrolled55), 8);
                // BDDNode 286
                inc_path_counter(286);
                uint32_t unrolled56 = (unrolled53) ^ (unrolled36);
                // BDDNode 287
                inc_path_counter(287);
                uint32_t unrolled57 = (rotated34) ^ (unrolled51);
                // BDDNode 54
                inc_path_counter(54);
                uint32_t rotated39 = rotate_left((unrolled56) + (unrolled57), 16);
                // BDDNode 288
                inc_path_counter(288);
                uint32_t unrolled58 = (unrolled56) + (unrolled57);
                // BDDNode 55
                inc_path_counter(55);
                uint32_t rotated40 = rotate_left((rotated37) ^ (unrolled58), 13);
                // BDDNode 289
                inc_path_counter(289);
                uint32_t unrolled59 = (unrolled54) ^ (unrolled55);
                // BDDNode 290
                inc_path_counter(290);
                uint32_t unrolled60 = (rotated36) + (unrolled59);
                // BDDNode 56
                inc_path_counter(56);
                uint32_t rotated41 = rotate_left((rotated38) ^ (unrolled60), 7);
                // BDDNode 291
                inc_path_counter(291);
                uint32_t unrolled61 = (rotated37) ^ (unrolled58);
                // BDDNode 57
                inc_path_counter(57);
                uint32_t rotated42 = rotate_left((unrolled60) + (unrolled61), 16);
                // BDDNode 292
                inc_path_counter(292);
                uint32_t unrolled62 = (unrolled60) + (unrolled61);
                // BDDNode 58
                inc_path_counter(58);
                uint32_t rotated43 = rotate_left((rotated40) ^ (unrolled62), 5);
                // BDDNode 293
                inc_path_counter(293);
                uint32_t unrolled63 = (rotated38) ^ (unrolled60);
                // BDDNode 294
                inc_path_counter(294);
                uint32_t unrolled64 = (rotated39) + (unrolled63);
                // BDDNode 59
                inc_path_counter(59);
                uint32_t rotated44 = rotate_left((rotated41) ^ (unrolled64), 8);
                // BDDNode 295
                inc_path_counter(295);
                uint32_t unrolled65 = (rotated40) ^ (unrolled62);
                // BDDNode 60
                inc_path_counter(60);
                uint32_t rotated45 = rotate_left((unrolled64) + (unrolled65), 16);
                // BDDNode 296
                inc_path_counter(296);
                uint32_t unrolled66 = (unrolled64) + (unrolled65);
                // BDDNode 61
                inc_path_counter(61);
                uint32_t rotated46 = rotate_left((rotated43) ^ (unrolled66), 13);
                // BDDNode 297
                inc_path_counter(297);
                uint32_t unrolled67 = (rotated41) ^ (unrolled64);
                // BDDNode 298
                inc_path_counter(298);
                uint32_t unrolled68 = (rotated42) + (unrolled67);
                // BDDNode 62
                inc_path_counter(62);
                uint32_t rotated47 = rotate_left((rotated44) ^ (unrolled68), 7);
                // BDDNode 299
                inc_path_counter(299);
                uint32_t unrolled69 = (rotated43) ^ (unrolled66);
                // BDDNode 63
                inc_path_counter(63);
                uint32_t rotated48 = rotate_left((unrolled68) + (unrolled69), 16);
                // BDDNode 300
                inc_path_counter(300);
                uint32_t unrolled70 = (unrolled68) + (unrolled69);
                // BDDNode 64
                inc_path_counter(64);
                uint32_t rotated49 = rotate_left((rotated46) ^ (unrolled70), 5);
                // BDDNode 301
                inc_path_counter(301);
                uint32_t unrolled71 = (rotated44) ^ (unrolled68);
                // BDDNode 302
                inc_path_counter(302);
                uint32_t unrolled72 = (rotated45) + (unrolled71);
                // BDDNode 65
                inc_path_counter(65);
                uint32_t rotated50 = rotate_left((rotated47) ^ (unrolled72), 8);
                // BDDNode 303
                inc_path_counter(303);
                uint32_t unrolled73 = (unrolled72) ^ (unrolled55);
                // BDDNode 304
                inc_path_counter(304);
                uint32_t unrolled74 = (rotated46) ^ (unrolled70);
                // BDDNode 66
                inc_path_counter(66);
                uint32_t rotated51 = rotate_left((unrolled73) + (unrolled74), 16);
                // BDDNode 305
                inc_path_counter(305);
                uint32_t unrolled75 = (unrolled73) + (unrolled74);
                // BDDNode 67
                inc_path_counter(67);
                uint32_t rotated52 = rotate_left((rotated49) ^ (unrolled75), 13);
                // BDDNode 306
                inc_path_counter(306);
                uint32_t unrolled76 = (rotated47) ^ (unrolled72);
                // BDDNode 307
                inc_path_counter(307);
                uint32_t unrolled77 = (rotated48) + (unrolled76);
                // BDDNode 68
                inc_path_counter(68);
                uint32_t rotated53 = rotate_left((rotated50) ^ (unrolled77), 7);
                // BDDNode 308
                inc_path_counter(308);
                uint32_t unrolled78 = (rotated49) ^ (unrolled75);
                // BDDNode 69
                inc_path_counter(69);
                uint32_t rotated54 = rotate_left((unrolled77) + (unrolled78), 16);
                // BDDNode 309
                inc_path_counter(309);
                uint32_t unrolled79 = (unrolled77) + (unrolled78);
                // BDDNode 70
                inc_path_counter(70);
                uint32_t rotated55 = rotate_left((rotated52) ^ (unrolled79), 5);
                // BDDNode 310
                inc_path_counter(310);
                uint32_t unrolled80 = (rotated50) ^ (unrolled77);
                // BDDNode 311
                inc_path_counter(311);
                uint32_t unrolled81 = (rotated51) + (unrolled80);
                // BDDNode 71
                inc_path_counter(71);
                uint32_t rotated56 = rotate_left((rotated53) ^ (unrolled81), 8);
                // BDDNode 312
                inc_path_counter(312);
                uint32_t unrolled82 = (rotated52) ^ (unrolled79);
                // BDDNode 72
                inc_path_counter(72);
                uint32_t rotated57 = rotate_left((unrolled81) + (unrolled82), 16);
                // BDDNode 313
                inc_path_counter(313);
                uint32_t unrolled83 = (unrolled81) + (unrolled82);
                // BDDNode 73
                inc_path_counter(73);
                uint32_t rotated58 = rotate_left((rotated55) ^ (unrolled83), 13);
                // BDDNode 314
                inc_path_counter(314);
                uint32_t unrolled84 = (rotated53) ^ (unrolled81);
                // BDDNode 315
                inc_path_counter(315);
                uint32_t unrolled85 = (rotated54) + (unrolled84);
                // BDDNode 74
                inc_path_counter(74);
                uint32_t rotated59 = rotate_left((rotated56) ^ (unrolled85), 7);
                // BDDNode 316
                inc_path_counter(316);
                uint32_t unrolled86 = (rotated55) ^ (unrolled83);
                // BDDNode 75
                inc_path_counter(75);
                uint32_t rotated60 = rotate_left((unrolled85) + (unrolled86), 16);
                // BDDNode 317
                inc_path_counter(317);
                uint32_t unrolled87 = (unrolled85) + (unrolled86);
                // BDDNode 76
                inc_path_counter(76);
                uint32_t rotated61 = rotate_left((rotated58) ^ (unrolled87), 5);
                // BDDNode 318
                inc_path_counter(318);
                uint32_t unrolled88 = (rotated56) ^ (unrolled85);
                // BDDNode 319
                inc_path_counter(319);
                uint32_t unrolled89 = (rotated57) + (unrolled88);
                // BDDNode 77
                inc_path_counter(77);
                uint32_t rotated62 = rotate_left((rotated59) ^ (unrolled89), 8);
                // BDDNode 320
                inc_path_counter(320);
                uint32_t unrolled90 = (rotated58) ^ (unrolled87);
                // BDDNode 78
                inc_path_counter(78);
                uint32_t rotated63 = rotate_left((unrolled89) + (unrolled90), 16);
                // BDDNode 321
                inc_path_counter(321);
                uint32_t unrolled91 = (unrolled89) + (unrolled90);
                // BDDNode 79
                inc_path_counter(79);
                uint32_t rotated64 = rotate_left((rotated61) ^ (unrolled91), 13);
                // BDDNode 322
                inc_path_counter(322);
                uint32_t unrolled92 = (rotated59) ^ (unrolled89);
                // BDDNode 323
                inc_path_counter(323);
                uint32_t unrolled93 = (rotated60) + (unrolled92);
                // BDDNode 80
                inc_path_counter(80);
                uint32_t rotated65 = rotate_left((rotated62) ^ (unrolled93), 7);
                // BDDNode 324
                inc_path_counter(324);
                uint32_t unrolled94 = (rotated61) ^ (unrolled91);
                // BDDNode 81
                inc_path_counter(81);
                uint32_t rotated66 = rotate_left((unrolled93) + (unrolled94), 16);
                // BDDNode 325
                inc_path_counter(325);
                uint32_t unrolled95 = (unrolled93) + (unrolled94);
                // BDDNode 82
                inc_path_counter(82);
                uint32_t rotated67 = rotate_left((rotated64) ^ (unrolled95), 5);
                // BDDNode 326
                inc_path_counter(326);
                uint32_t unrolled96 = (rotated62) ^ (unrolled93);
                // BDDNode 327
                inc_path_counter(327);
                uint32_t unrolled97 = (rotated63) + (unrolled96);
                // BDDNode 83
                inc_path_counter(83);
                uint32_t rotated68 = rotate_left((rotated65) ^ (unrolled97), 8);
                // BDDNode 328
                inc_path_counter(328);
                uint32_t unrolled98 = (rotated64) ^ (unrolled95);
                // BDDNode 84
                inc_path_counter(84);
                uint32_t rotated69 = rotate_left((unrolled97) + (unrolled98), 16);
                // BDDNode 329
                inc_path_counter(329);
                uint32_t unrolled99 = (unrolled97) + (unrolled98);
                // BDDNode 85
                inc_path_counter(85);
                uint32_t rotated70 = rotate_left((rotated67) ^ (unrolled99), 13);
                // BDDNode 330
                inc_path_counter(330);
                uint32_t unrolled100 = (rotated65) ^ (unrolled97);
                // BDDNode 331
                inc_path_counter(331);
                uint32_t unrolled101 = (rotated66) + (unrolled100);
                // BDDNode 86
                inc_path_counter(86);
                uint32_t rotated71 = rotate_left((rotated68) ^ (unrolled101), 7);
                // BDDNode 332
                inc_path_counter(332);
                uint32_t unrolled102 = (rotated67) ^ (unrolled99);
                // BDDNode 87
                inc_path_counter(87);
                uint32_t rotated72 = rotate_left((unrolled101) + (unrolled102), 16);
                // BDDNode 88
                inc_path_counter(88);
                uint8_t* vector_cell = 0;
                vector_borrow(vector, 0, (void**)&vector_cell);
                uint32_t vector_value_out = *(uint32_t*)vector_cell;
                // BDDNode 89
                inc_path_counter(89);
                // BDDNode 333
                inc_path_counter(333);
                uint64_t unrolled103 = (now) >> (16ULL);
                // BDDNode 334
                inc_path_counter(334);
                uint32_t unrolled104 = (unrolled103 & 4294967295) - (vector_value_out);
                // BDDNode 335
                inc_path_counter(335);
                uint32_t unrolled105 = (unrolled104) >> (12);
                // BDDNode 336
                inc_path_counter(336);
                uint32_t unrolled106 = (4294967295) + (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr3+8)) << 8 | (uint16_t)(*(hdr3+9)))) << 8 | (uint32_t)(*(hdr3+10)))) << 8 | (uint32_t)(*(hdr3+11))));
                // BDDNode 337
                inc_path_counter(337);
                uint32_t unrolled107 = (rotated68) ^ (unrolled101);
                // BDDNode 338
                inc_path_counter(338);
                uint32_t unrolled108 = (rotated69) + (unrolled107);
                // BDDNode 339
                inc_path_counter(339);
                uint32_t unrolled109 = (unrolled101) + (unrolled102);
                // BDDNode 340
                inc_path_counter(340);
                uint32_t unrolled110 = (rotated70) ^ (unrolled109);
                // BDDNode 341
                inc_path_counter(341);
                uint32_t unrolled111 = (unrolled108) ^ (unrolled110);
                // BDDNode 342
                inc_path_counter(342);
                uint32_t unrolled112 = (unrolled111) ^ (rotated72);
                // BDDNode 343
                inc_path_counter(343);
                uint32_t unrolled113 = (rotated71) ^ (unrolled108);
                // BDDNode 344
                inc_path_counter(344);
                uint32_t unrolled114 = (unrolled112) ^ (unrolled113);
                // BDDNode 345
                inc_path_counter(345);
                uint32_t unrolled115 = (unrolled106) ^ (unrolled114);
                // BDDNode 90
                inc_path_counter(90);
                if (((unrolled105) - (unrolled115)) <= (2)) {
                  // BDDNode 91
                  inc_path_counter(91);
                  int checksum = rte_ipv4_udptcp_cksum((struct rte_ipv4_hdr*)hdr2, (void*)hdr3);
                  // BDDNode 92
                  inc_path_counter(92);
                  hdr3[4] = (uint32_t)((4294967295) + (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5)))) << 8 | (uint32_t)(*(hdr3+6)))) << 8 | (uint32_t)(*(hdr3+7))))>>24);
                  hdr3[5] = (uint32_t)((4294967295) + (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5)))) << 8 | (uint32_t)(*(hdr3+6)))) << 8 | (uint32_t)(*(hdr3+7))))>>16);
                  hdr3[6] = (uint32_t)((4294967295) + (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5)))) << 8 | (uint32_t)(*(hdr3+6)))) << 8 | (uint32_t)(*(hdr3+7))))>>8);
                  hdr3[7] = (uint32_t)((4294967295) + (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5)))) << 8 | (uint32_t)(*(hdr3+6)))) << 8 | (uint32_t)(*(hdr3+7)))));
                  hdr3[12] = 80;
                  hdr3[13] = (uint32_t)(((uint8_t)(*(hdr3+13))) | (64));
                  packet_return_chunk(buffer, hdr3);
                  // BDDNode 93
                  inc_path_counter(93);
                  hdr2[0] = 69;
                  hdr2[2] = 0;
                  hdr2[3] = 40;
                  hdr2[10] = checksum & 255;
                  hdr2[11] = (checksum>>8) & 255;
                  packet_return_chunk(buffer, hdr2);
                  // BDDNode 94
                  inc_path_counter(94);
                  packet_return_chunk(buffer, hdr);
                  // BDDNode 95
                  inc_path_counter(95);
                  forwarding_stats_per_route_op[95].inc_fwd(0);
                  return 0;
                } else {
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
                  forwarding_stats_per_route_op[99].inc_drop();
                  return DROP;
                } // ((unrolled105) - (unrolled115)) <= (2)
              } else {
                // BDDNode 100
                inc_path_counter(100);
                packet_return_chunk(buffer, hdr3);
                // BDDNode 101
                inc_path_counter(101);
                packet_return_chunk(buffer, hdr2);
                // BDDNode 102
                inc_path_counter(102);
                packet_return_chunk(buffer, hdr);
                // BDDNode 103
                inc_path_counter(103);
                forwarding_stats_per_route_op[103].inc_fwd(0);
                return 0;
              } // (0) == (bf_query_estimate)
            } else {
              // BDDNode 104
              inc_path_counter(104);
              if ((0) == (((uint8_t)(*(hdr3+13))) & (16))) {
                // BDDNode 105
                inc_path_counter(105);
                uint8_t* vector_cell2 = 0;
                vector_borrow(vector, 0, (void**)&vector_cell2);
                uint32_t vector_value_out2 = *(uint32_t*)vector_cell2;
                // BDDNode 106
                inc_path_counter(106);
                // BDDNode 107
                inc_path_counter(107);
                uint32_t rotated73 = rotate_left(740644437, 5);
                // BDDNode 108
                inc_path_counter(108);
                uint32_t rotated74 = rotate_left((993352779) ^ (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr2+12)) << 8 | (uint16_t)(*(hdr2+13)))) << 8 | (uint32_t)(*(hdr2+14)))) << 8 | (uint32_t)(*(hdr2+15)))), 8);
                // BDDNode 109
                inc_path_counter(109);
                uint32_t rotated75 = rotate_left(1870056090, 16);
                // BDDNode 110
                inc_path_counter(110);
                uint32_t rotated76 = rotate_left((1870056090) ^ (rotated73), 13);
                // BDDNode 346
                inc_path_counter(346);
                uint32_t unrolled116 = (993352779) ^ (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr2+12)) << 8 | (uint16_t)(*(hdr2+13)))) << 8 | (uint32_t)(*(hdr2+14)))) << 8 | (uint32_t)(*(hdr2+15))));
                // BDDNode 347
                inc_path_counter(347);
                uint32_t unrolled117 = (1564959569) + (unrolled116);
                // BDDNode 111
                inc_path_counter(111);
                uint32_t rotated77 = rotate_left((rotated74) ^ (unrolled117), 7);
                // BDDNode 348
                inc_path_counter(348);
                uint32_t unrolled118 = (1870056090) ^ (rotated73);
                // BDDNode 349
                inc_path_counter(349);
                uint32_t unrolled119 = (unrolled116) + (unrolled118);
                // BDDNode 112
                inc_path_counter(112);
                uint32_t rotated78 = rotate_left((1564959569) + (unrolled119), 16);
                // BDDNode 350
                inc_path_counter(350);
                uint32_t unrolled120 = (1564959569) + (unrolled119);
                // BDDNode 113
                inc_path_counter(113);
                uint32_t rotated79 = rotate_left((rotated76) ^ (unrolled120), 5);
                // BDDNode 351
                inc_path_counter(351);
                uint32_t unrolled121 = (rotated74) ^ (unrolled117);
                // BDDNode 352
                inc_path_counter(352);
                uint32_t unrolled122 = (rotated75) + (unrolled121);
                // BDDNode 114
                inc_path_counter(114);
                uint32_t rotated80 = rotate_left((rotated77) ^ (unrolled122), 8);
                // BDDNode 353
                inc_path_counter(353);
                uint32_t unrolled123 = (rotated76) ^ (unrolled120);
                // BDDNode 115
                inc_path_counter(115);
                uint32_t rotated81 = rotate_left((unrolled122) + (unrolled123), 16);
                // BDDNode 354
                inc_path_counter(354);
                uint32_t unrolled124 = (unrolled122) + (unrolled123);
                // BDDNode 116
                inc_path_counter(116);
                uint32_t rotated82 = rotate_left((rotated79) ^ (unrolled124), 13);
                // BDDNode 355
                inc_path_counter(355);
                uint32_t unrolled125 = (rotated77) ^ (unrolled122);
                // BDDNode 356
                inc_path_counter(356);
                uint32_t unrolled126 = (rotated78) + (unrolled125);
                // BDDNode 117
                inc_path_counter(117);
                uint32_t rotated83 = rotate_left((rotated80) ^ (unrolled126), 7);
                // BDDNode 357
                inc_path_counter(357);
                uint32_t unrolled127 = (rotated79) ^ (unrolled124);
                // BDDNode 118
                inc_path_counter(118);
                uint32_t rotated84 = rotate_left((unrolled126) + (unrolled127), 16);
                // BDDNode 358
                inc_path_counter(358);
                uint32_t unrolled128 = (unrolled126) + (unrolled127);
                // BDDNode 119
                inc_path_counter(119);
                uint32_t rotated85 = rotate_left((rotated82) ^ (unrolled128), 5);
                // BDDNode 359
                inc_path_counter(359);
                uint32_t unrolled129 = (rotated80) ^ (unrolled126);
                // BDDNode 360
                inc_path_counter(360);
                uint32_t unrolled130 = (rotated81) + (unrolled129);
                // BDDNode 361
                inc_path_counter(361);
                uint32_t unrolled131 = (rotated83) ^ (unrolled130);
                // BDDNode 120
                inc_path_counter(120);
                uint32_t rotated86 = rotate_left((unrolled131) ^ (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr2+16)) << 8 | (uint16_t)(*(hdr2+17)))) << 8 | (uint32_t)(*(hdr2+18)))) << 8 | (uint32_t)(*(hdr2+19)))), 8);
                // BDDNode 362
                inc_path_counter(362);
                uint32_t unrolled132 = (unrolled130) ^ (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr2+12)) << 8 | (uint16_t)(*(hdr2+13)))) << 8 | (uint32_t)(*(hdr2+14)))) << 8 | (uint32_t)(*(hdr2+15))));
                // BDDNode 363
                inc_path_counter(363);
                uint32_t unrolled133 = (rotated82) ^ (unrolled128);
                // BDDNode 121
                inc_path_counter(121);
                uint32_t rotated87 = rotate_left((unrolled132) + (unrolled133), 16);
                // BDDNode 364
                inc_path_counter(364);
                uint32_t unrolled134 = (unrolled132) + (unrolled133);
                // BDDNode 122
                inc_path_counter(122);
                uint32_t rotated88 = rotate_left((rotated85) ^ (unrolled134), 13);
                // BDDNode 365
                inc_path_counter(365);
                uint32_t unrolled135 = (unrolled131) ^ (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr2+16)) << 8 | (uint16_t)(*(hdr2+17)))) << 8 | (uint32_t)(*(hdr2+18)))) << 8 | (uint32_t)(*(hdr2+19))));
                // BDDNode 366
                inc_path_counter(366);
                uint32_t unrolled136 = (rotated84) + (unrolled135);
                // BDDNode 123
                inc_path_counter(123);
                uint32_t rotated89 = rotate_left((rotated86) ^ (unrolled136), 7);
                // BDDNode 367
                inc_path_counter(367);
                uint32_t unrolled137 = (rotated85) ^ (unrolled134);
                // BDDNode 124
                inc_path_counter(124);
                uint32_t rotated90 = rotate_left((unrolled136) + (unrolled137), 16);
                // BDDNode 368
                inc_path_counter(368);
                uint32_t unrolled138 = (unrolled136) + (unrolled137);
                // BDDNode 125
                inc_path_counter(125);
                uint32_t rotated91 = rotate_left((rotated88) ^ (unrolled138), 5);
                // BDDNode 369
                inc_path_counter(369);
                uint32_t unrolled139 = (rotated86) ^ (unrolled136);
                // BDDNode 370
                inc_path_counter(370);
                uint32_t unrolled140 = (rotated87) + (unrolled139);
                // BDDNode 126
                inc_path_counter(126);
                uint32_t rotated92 = rotate_left((rotated89) ^ (unrolled140), 8);
                // BDDNode 371
                inc_path_counter(371);
                uint32_t unrolled141 = (rotated88) ^ (unrolled138);
                // BDDNode 127
                inc_path_counter(127);
                uint32_t rotated93 = rotate_left((unrolled140) + (unrolled141), 16);
                // BDDNode 372
                inc_path_counter(372);
                uint32_t unrolled142 = (unrolled140) + (unrolled141);
                // BDDNode 128
                inc_path_counter(128);
                uint32_t rotated94 = rotate_left((rotated91) ^ (unrolled142), 13);
                // BDDNode 373
                inc_path_counter(373);
                uint32_t unrolled143 = (rotated89) ^ (unrolled140);
                // BDDNode 374
                inc_path_counter(374);
                uint32_t unrolled144 = (rotated90) + (unrolled143);
                // BDDNode 129
                inc_path_counter(129);
                uint32_t rotated95 = rotate_left((rotated92) ^ (unrolled144), 7);
                // BDDNode 375
                inc_path_counter(375);
                uint32_t unrolled145 = (rotated91) ^ (unrolled142);
                // BDDNode 130
                inc_path_counter(130);
                uint32_t rotated96 = rotate_left((unrolled144) + (unrolled145), 16);
                // BDDNode 376
                inc_path_counter(376);
                uint32_t unrolled146 = (unrolled144) + (unrolled145);
                // BDDNode 131
                inc_path_counter(131);
                uint32_t rotated97 = rotate_left((rotated94) ^ (unrolled146), 5);
                // BDDNode 377
                inc_path_counter(377);
                uint32_t unrolled147 = (rotated92) ^ (unrolled144);
                // BDDNode 378
                inc_path_counter(378);
                uint32_t unrolled148 = (rotated93) + (unrolled147);
                // BDDNode 379
                inc_path_counter(379);
                uint32_t unrolled149 = (rotated95) ^ (unrolled148);
                // BDDNode 380
                inc_path_counter(380);
                uint32_t unrolled150 = ((uint16_t)(((uint16_t)(*(hdr3+0)) << 8 | (uint16_t)(*(hdr3+1))))) << (16);
                // BDDNode 381
                inc_path_counter(381);
                uint32_t unrolled151 = (unrolled150) | ((uint16_t)(((uint16_t)(*(hdr3+2)) << 8 | (uint16_t)(*(hdr3+3)))));
                // BDDNode 132
                inc_path_counter(132);
                uint32_t rotated98 = rotate_left((unrolled149) ^ (unrolled151), 8);
                // BDDNode 382
                inc_path_counter(382);
                uint32_t unrolled152 = (unrolled148) ^ (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr2+16)) << 8 | (uint16_t)(*(hdr2+17)))) << 8 | (uint32_t)(*(hdr2+18)))) << 8 | (uint32_t)(*(hdr2+19))));
                // BDDNode 383
                inc_path_counter(383);
                uint32_t unrolled153 = (rotated94) ^ (unrolled146);
                // BDDNode 133
                inc_path_counter(133);
                uint32_t rotated99 = rotate_left((unrolled152) + (unrolled153), 16);
                // BDDNode 384
                inc_path_counter(384);
                uint32_t unrolled154 = (unrolled152) + (unrolled153);
                // BDDNode 134
                inc_path_counter(134);
                uint32_t rotated100 = rotate_left((rotated97) ^ (unrolled154), 13);
                // BDDNode 385
                inc_path_counter(385);
                uint32_t unrolled155 = (unrolled149) ^ (unrolled151);
                // BDDNode 386
                inc_path_counter(386);
                uint32_t unrolled156 = (rotated96) + (unrolled155);
                // BDDNode 135
                inc_path_counter(135);
                uint32_t rotated101 = rotate_left((rotated98) ^ (unrolled156), 7);
                // BDDNode 387
                inc_path_counter(387);
                uint32_t unrolled157 = (rotated97) ^ (unrolled154);
                // BDDNode 136
                inc_path_counter(136);
                uint32_t rotated102 = rotate_left((unrolled156) + (unrolled157), 16);
                // BDDNode 388
                inc_path_counter(388);
                uint32_t unrolled158 = (unrolled156) + (unrolled157);
                // BDDNode 137
                inc_path_counter(137);
                uint32_t rotated103 = rotate_left((rotated100) ^ (unrolled158), 5);
                // BDDNode 389
                inc_path_counter(389);
                uint32_t unrolled159 = (rotated98) ^ (unrolled156);
                // BDDNode 390
                inc_path_counter(390);
                uint32_t unrolled160 = (rotated99) + (unrolled159);
                // BDDNode 138
                inc_path_counter(138);
                uint32_t rotated104 = rotate_left((rotated101) ^ (unrolled160), 8);
                // BDDNode 391
                inc_path_counter(391);
                uint32_t unrolled161 = (rotated100) ^ (unrolled158);
                // BDDNode 139
                inc_path_counter(139);
                uint32_t rotated105 = rotate_left((unrolled160) + (unrolled161), 16);
                // BDDNode 392
                inc_path_counter(392);
                uint32_t unrolled162 = (unrolled160) + (unrolled161);
                // BDDNode 140
                inc_path_counter(140);
                uint32_t rotated106 = rotate_left((rotated103) ^ (unrolled162), 13);
                // BDDNode 393
                inc_path_counter(393);
                uint32_t unrolled163 = (rotated101) ^ (unrolled160);
                // BDDNode 394
                inc_path_counter(394);
                uint32_t unrolled164 = (rotated102) + (unrolled163);
                // BDDNode 141
                inc_path_counter(141);
                uint32_t rotated107 = rotate_left((rotated104) ^ (unrolled164), 7);
                // BDDNode 395
                inc_path_counter(395);
                uint32_t unrolled165 = (rotated103) ^ (unrolled162);
                // BDDNode 142
                inc_path_counter(142);
                uint32_t rotated108 = rotate_left((unrolled164) + (unrolled165), 16);
                // BDDNode 396
                inc_path_counter(396);
                uint32_t unrolled166 = (unrolled164) + (unrolled165);
                // BDDNode 143
                inc_path_counter(143);
                uint32_t rotated109 = rotate_left((rotated106) ^ (unrolled166), 5);
                // BDDNode 397
                inc_path_counter(397);
                uint32_t unrolled167 = (rotated104) ^ (unrolled164);
                // BDDNode 398
                inc_path_counter(398);
                uint32_t unrolled168 = (rotated105) + (unrolled167);
                // BDDNode 399
                inc_path_counter(399);
                uint32_t unrolled169 = (rotated107) ^ (unrolled168);
                // BDDNode 144
                inc_path_counter(144);
                uint32_t rotated110 = rotate_left((unrolled169) ^ (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5)))) << 8 | (uint32_t)(*(hdr3+6)))) << 8 | (uint32_t)(*(hdr3+7)))), 8);
                // BDDNode 400
                inc_path_counter(400);
                uint32_t unrolled170 = (unrolled168) ^ (unrolled151);
                // BDDNode 401
                inc_path_counter(401);
                uint32_t unrolled171 = (rotated106) ^ (unrolled166);
                // BDDNode 145
                inc_path_counter(145);
                uint32_t rotated111 = rotate_left((unrolled170) + (unrolled171), 16);
                // BDDNode 402
                inc_path_counter(402);
                uint32_t unrolled172 = (unrolled170) + (unrolled171);
                // BDDNode 146
                inc_path_counter(146);
                uint32_t rotated112 = rotate_left((rotated109) ^ (unrolled172), 13);
                // BDDNode 403
                inc_path_counter(403);
                uint32_t unrolled173 = (unrolled169) ^ (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5)))) << 8 | (uint32_t)(*(hdr3+6)))) << 8 | (uint32_t)(*(hdr3+7))));
                // BDDNode 404
                inc_path_counter(404);
                uint32_t unrolled174 = (rotated108) + (unrolled173);
                // BDDNode 147
                inc_path_counter(147);
                uint32_t rotated113 = rotate_left((rotated110) ^ (unrolled174), 7);
                // BDDNode 405
                inc_path_counter(405);
                uint32_t unrolled175 = (rotated109) ^ (unrolled172);
                // BDDNode 148
                inc_path_counter(148);
                uint32_t rotated114 = rotate_left((unrolled174) + (unrolled175), 16);
                // BDDNode 406
                inc_path_counter(406);
                uint32_t unrolled176 = (unrolled174) + (unrolled175);
                // BDDNode 149
                inc_path_counter(149);
                uint32_t rotated115 = rotate_left((rotated112) ^ (unrolled176), 5);
                // BDDNode 407
                inc_path_counter(407);
                uint32_t unrolled177 = (rotated110) ^ (unrolled174);
                // BDDNode 408
                inc_path_counter(408);
                uint32_t unrolled178 = (rotated111) + (unrolled177);
                // BDDNode 150
                inc_path_counter(150);
                uint32_t rotated116 = rotate_left((rotated113) ^ (unrolled178), 8);
                // BDDNode 409
                inc_path_counter(409);
                uint32_t unrolled179 = (rotated112) ^ (unrolled176);
                // BDDNode 151
                inc_path_counter(151);
                uint32_t rotated117 = rotate_left((unrolled178) + (unrolled179), 16);
                // BDDNode 410
                inc_path_counter(410);
                uint32_t unrolled180 = (unrolled178) + (unrolled179);
                // BDDNode 152
                inc_path_counter(152);
                uint32_t rotated118 = rotate_left((rotated115) ^ (unrolled180), 13);
                // BDDNode 411
                inc_path_counter(411);
                uint32_t unrolled181 = (rotated113) ^ (unrolled178);
                // BDDNode 412
                inc_path_counter(412);
                uint32_t unrolled182 = (rotated114) + (unrolled181);
                // BDDNode 153
                inc_path_counter(153);
                uint32_t rotated119 = rotate_left((rotated116) ^ (unrolled182), 7);
                // BDDNode 413
                inc_path_counter(413);
                uint32_t unrolled183 = (rotated115) ^ (unrolled180);
                // BDDNode 154
                inc_path_counter(154);
                uint32_t rotated120 = rotate_left((unrolled182) + (unrolled183), 16);
                // BDDNode 414
                inc_path_counter(414);
                uint32_t unrolled184 = (unrolled182) + (unrolled183);
                // BDDNode 155
                inc_path_counter(155);
                uint32_t rotated121 = rotate_left((rotated118) ^ (unrolled184), 5);
                // BDDNode 415
                inc_path_counter(415);
                uint32_t unrolled185 = (rotated116) ^ (unrolled182);
                // BDDNode 416
                inc_path_counter(416);
                uint32_t unrolled186 = (rotated117) + (unrolled185);
                // BDDNode 156
                inc_path_counter(156);
                uint32_t rotated122 = rotate_left((rotated119) ^ (unrolled186), 8);
                // BDDNode 417
                inc_path_counter(417);
                uint32_t unrolled187 = (unrolled186) ^ (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5)))) << 8 | (uint32_t)(*(hdr3+6)))) << 8 | (uint32_t)(*(hdr3+7))));
                // BDDNode 418
                inc_path_counter(418);
                uint32_t unrolled188 = (rotated118) ^ (unrolled184);
                // BDDNode 157
                inc_path_counter(157);
                uint32_t rotated123 = rotate_left((unrolled187) + (unrolled188), 16);
                // BDDNode 419
                inc_path_counter(419);
                uint32_t unrolled189 = (unrolled187) + (unrolled188);
                // BDDNode 158
                inc_path_counter(158);
                uint32_t rotated124 = rotate_left((rotated121) ^ (unrolled189), 13);
                // BDDNode 420
                inc_path_counter(420);
                uint32_t unrolled190 = (rotated119) ^ (unrolled186);
                // BDDNode 421
                inc_path_counter(421);
                uint32_t unrolled191 = (rotated120) + (unrolled190);
                // BDDNode 159
                inc_path_counter(159);
                uint32_t rotated125 = rotate_left((rotated122) ^ (unrolled191), 7);
                // BDDNode 422
                inc_path_counter(422);
                uint32_t unrolled192 = (rotated121) ^ (unrolled189);
                // BDDNode 160
                inc_path_counter(160);
                uint32_t rotated126 = rotate_left((unrolled191) + (unrolled192), 16);
                // BDDNode 423
                inc_path_counter(423);
                uint32_t unrolled193 = (unrolled191) + (unrolled192);
                // BDDNode 161
                inc_path_counter(161);
                uint32_t rotated127 = rotate_left((rotated124) ^ (unrolled193), 5);
                // BDDNode 424
                inc_path_counter(424);
                uint32_t unrolled194 = (rotated122) ^ (unrolled191);
                // BDDNode 425
                inc_path_counter(425);
                uint32_t unrolled195 = (rotated123) + (unrolled194);
                // BDDNode 162
                inc_path_counter(162);
                uint32_t rotated128 = rotate_left((rotated125) ^ (unrolled195), 8);
                // BDDNode 426
                inc_path_counter(426);
                uint32_t unrolled196 = (rotated124) ^ (unrolled193);
                // BDDNode 163
                inc_path_counter(163);
                uint32_t rotated129 = rotate_left((unrolled195) + (unrolled196), 16);
                // BDDNode 427
                inc_path_counter(427);
                uint32_t unrolled197 = (unrolled195) + (unrolled196);
                // BDDNode 164
                inc_path_counter(164);
                uint32_t rotated130 = rotate_left((rotated127) ^ (unrolled197), 13);
                // BDDNode 428
                inc_path_counter(428);
                uint32_t unrolled198 = (rotated125) ^ (unrolled195);
                // BDDNode 429
                inc_path_counter(429);
                uint32_t unrolled199 = (rotated126) + (unrolled198);
                // BDDNode 165
                inc_path_counter(165);
                uint32_t rotated131 = rotate_left((rotated128) ^ (unrolled199), 7);
                // BDDNode 430
                inc_path_counter(430);
                uint32_t unrolled200 = (rotated127) ^ (unrolled197);
                // BDDNode 166
                inc_path_counter(166);
                uint32_t rotated132 = rotate_left((unrolled199) + (unrolled200), 16);
                // BDDNode 431
                inc_path_counter(431);
                uint32_t unrolled201 = (unrolled199) + (unrolled200);
                // BDDNode 167
                inc_path_counter(167);
                uint32_t rotated133 = rotate_left((rotated130) ^ (unrolled201), 5);
                // BDDNode 432
                inc_path_counter(432);
                uint32_t unrolled202 = (rotated128) ^ (unrolled199);
                // BDDNode 433
                inc_path_counter(433);
                uint32_t unrolled203 = (rotated129) + (unrolled202);
                // BDDNode 168
                inc_path_counter(168);
                uint32_t rotated134 = rotate_left((rotated131) ^ (unrolled203), 8);
                // BDDNode 434
                inc_path_counter(434);
                uint32_t unrolled204 = (rotated130) ^ (unrolled201);
                // BDDNode 169
                inc_path_counter(169);
                uint32_t rotated135 = rotate_left((unrolled203) + (unrolled204), 16);
                // BDDNode 435
                inc_path_counter(435);
                uint32_t unrolled205 = (unrolled203) + (unrolled204);
                // BDDNode 170
                inc_path_counter(170);
                uint32_t rotated136 = rotate_left((rotated133) ^ (unrolled205), 13);
                // BDDNode 436
                inc_path_counter(436);
                uint32_t unrolled206 = (rotated131) ^ (unrolled203);
                // BDDNode 437
                inc_path_counter(437);
                uint32_t unrolled207 = (rotated132) + (unrolled206);
                // BDDNode 171
                inc_path_counter(171);
                uint32_t rotated137 = rotate_left((rotated134) ^ (unrolled207), 7);
                // BDDNode 438
                inc_path_counter(438);
                uint32_t unrolled208 = (rotated133) ^ (unrolled205);
                // BDDNode 172
                inc_path_counter(172);
                uint32_t rotated138 = rotate_left((unrolled207) + (unrolled208), 16);
                // BDDNode 439
                inc_path_counter(439);
                uint32_t unrolled209 = (unrolled207) + (unrolled208);
                // BDDNode 173
                inc_path_counter(173);
                uint32_t rotated139 = rotate_left((rotated136) ^ (unrolled209), 5);
                // BDDNode 440
                inc_path_counter(440);
                uint32_t unrolled210 = (rotated134) ^ (unrolled207);
                // BDDNode 441
                inc_path_counter(441);
                uint32_t unrolled211 = (rotated135) + (unrolled210);
                // BDDNode 174
                inc_path_counter(174);
                uint32_t rotated140 = rotate_left((rotated137) ^ (unrolled211), 8);
                // BDDNode 442
                inc_path_counter(442);
                uint32_t unrolled212 = (rotated136) ^ (unrolled209);
                // BDDNode 175
                inc_path_counter(175);
                uint32_t rotated141 = rotate_left((unrolled211) + (unrolled212), 16);
                // BDDNode 443
                inc_path_counter(443);
                uint32_t unrolled213 = (unrolled211) + (unrolled212);
                // BDDNode 176
                inc_path_counter(176);
                uint32_t rotated142 = rotate_left((rotated139) ^ (unrolled213), 13);
                // BDDNode 444
                inc_path_counter(444);
                uint32_t unrolled214 = (rotated137) ^ (unrolled211);
                // BDDNode 445
                inc_path_counter(445);
                uint32_t unrolled215 = (rotated138) + (unrolled214);
                // BDDNode 177
                inc_path_counter(177);
                uint32_t rotated143 = rotate_left((rotated140) ^ (unrolled215), 7);
                // BDDNode 446
                inc_path_counter(446);
                uint32_t unrolled216 = (rotated139) ^ (unrolled213);
                // BDDNode 178
                inc_path_counter(178);
                uint32_t rotated144 = rotate_left((unrolled215) + (unrolled216), 16);
                // BDDNode 179
                inc_path_counter(179);
                int checksum2 = rte_ipv4_udptcp_cksum((struct rte_ipv4_hdr*)hdr2, (void*)hdr3);
                // BDDNode 447
                inc_path_counter(447);
                uint64_t unrolled217 = (now) >> (16ULL);
                // BDDNode 448
                inc_path_counter(448);
                uint32_t unrolled218 = (unrolled217 & 4294967295) - (vector_value_out2);
                // BDDNode 449
                inc_path_counter(449);
                uint32_t unrolled219 = (unrolled218) >> (12);
                // BDDNode 450
                inc_path_counter(450);
                uint32_t unrolled220 = (rotated140) ^ (unrolled215);
                // BDDNode 451
                inc_path_counter(451);
                uint32_t unrolled221 = (rotated141) + (unrolled220);
                // BDDNode 452
                inc_path_counter(452);
                uint32_t unrolled222 = (unrolled215) + (unrolled216);
                // BDDNode 453
                inc_path_counter(453);
                uint32_t unrolled223 = (rotated142) ^ (unrolled222);
                // BDDNode 454
                inc_path_counter(454);
                uint32_t unrolled224 = (unrolled221) ^ (unrolled223);
                // BDDNode 455
                inc_path_counter(455);
                uint32_t unrolled225 = (unrolled224) ^ (rotated144);
                // BDDNode 456
                inc_path_counter(456);
                uint32_t unrolled226 = (rotated143) ^ (unrolled221);
                // BDDNode 457
                inc_path_counter(457);
                uint32_t unrolled227 = (unrolled225) ^ (unrolled226);
                // BDDNode 180
                inc_path_counter(180);
                hdr3[0] = *(hdr3+2);
                hdr3[1] = *(hdr3+3);
                hdr3[2] = *(hdr3+0);
                hdr3[3] = *(hdr3+1);
                hdr3[4] = (uint32_t)((unrolled219) ^ (unrolled227)>>24);
                hdr3[5] = (uint32_t)((unrolled219) ^ (unrolled227)>>16);
                hdr3[6] = (uint32_t)((unrolled219) ^ (unrolled227)>>8);
                hdr3[7] = (uint32_t)((unrolled219) ^ (unrolled227));
                hdr3[8] = (uint32_t)((1) + (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5)))) << 8 | (uint32_t)(*(hdr3+6)))) << 8 | (uint32_t)(*(hdr3+7))))>>24);
                hdr3[9] = (uint32_t)((1) + (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5)))) << 8 | (uint32_t)(*(hdr3+6)))) << 8 | (uint32_t)(*(hdr3+7))))>>16);
                hdr3[10] = (uint32_t)((1) + (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5)))) << 8 | (uint32_t)(*(hdr3+6)))) << 8 | (uint32_t)(*(hdr3+7))))>>8);
                hdr3[11] = (uint32_t)((1) + (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr3+4)) << 8 | (uint16_t)(*(hdr3+5)))) << 8 | (uint32_t)(*(hdr3+6)))) << 8 | (uint32_t)(*(hdr3+7)))));
                hdr3[12] = 80;
                hdr3[13] = (uint32_t)(((uint8_t)(*(hdr3+13))) | (18));
                packet_return_chunk(buffer, hdr3);
                // BDDNode 181
                inc_path_counter(181);
                hdr2[0] = 69;
                hdr2[2] = 0;
                hdr2[3] = 40;
                hdr2[10] = checksum2 & 255;
                hdr2[11] = (checksum2>>8) & 255;
                hdr2[12] = *(hdr2+16);
                hdr2[13] = *(hdr2+17);
                hdr2[14] = *(hdr2+18);
                hdr2[15] = *(hdr2+19);
                hdr2[16] = *(hdr2+12);
                hdr2[17] = *(hdr2+13);
                hdr2[18] = *(hdr2+14);
                hdr2[19] = *(hdr2+15);
                packet_return_chunk(buffer, hdr2);
                // BDDNode 182
                inc_path_counter(182);
                packet_return_chunk(buffer, hdr);
                // BDDNode 183
                inc_path_counter(183);
                forwarding_stats_per_route_op[183].inc_fwd(device & 65535);
                return device & 65535;
              } else {
                // BDDNode 184
                inc_path_counter(184);
                packet_return_chunk(buffer, hdr3);
                // BDDNode 185
                inc_path_counter(185);
                packet_return_chunk(buffer, hdr2);
                // BDDNode 186
                inc_path_counter(186);
                packet_return_chunk(buffer, hdr);
                // BDDNode 187
                inc_path_counter(187);
                forwarding_stats_per_route_op[187].inc_drop();
                return DROP;
              } // (0) == (((uint8_t)(*(hdr3+13))) & (16))
            } // (0) == (((uint8_t)(*(hdr3+13))) & (2))
          } else {
            // BDDNode 188
            inc_path_counter(188);
            if ((0) == (((uint8_t)(*(hdr3+13))) & (64))) {
              // BDDNode 189
              inc_path_counter(189);
              packet_return_chunk(buffer, hdr3);
              // BDDNode 190
              inc_path_counter(190);
              packet_return_chunk(buffer, hdr2);
              // BDDNode 191
              inc_path_counter(191);
              packet_return_chunk(buffer, hdr);
              // BDDNode 192
              inc_path_counter(192);
              forwarding_stats_per_route_op[192].inc_fwd((uint32_t)((((uint32_t)(((uint32_t)(((uint16_t)(*(hdr2+16)) << 8 | (uint16_t)(*(hdr2+17)))) << 8 | (uint32_t)(*(hdr2+18)))) << 8 | (uint32_t)(*(hdr2+19)))) >> (24)));
              return (uint32_t)((((uint32_t)(((uint32_t)(((uint16_t)(*(hdr2+16)) << 8 | (uint16_t)(*(hdr2+17)))) << 8 | (uint32_t)(*(hdr2+18)))) << 8 | (uint32_t)(*(hdr2+19)))) >> (24));
            } else {
              // BDDNode 193
              inc_path_counter(193);
              uint8_t key2[12];
              key2[0] = *(hdr2+12);
              key2[1] = *(hdr2+13);
              key2[2] = *(hdr2+14);
              key2[3] = *(hdr2+15);
              key2[4] = *(hdr2+16);
              key2[5] = *(hdr2+17);
              key2[6] = *(hdr2+18);
              key2[7] = *(hdr2+19);
              key2[8] = *(hdr3+0);
              key2[9] = *(hdr3+1);
              key2[10] = *(hdr3+2);
              key2[11] = *(hdr3+3);
              bf_set(bf, key2);
              // BDDNode 194
              inc_path_counter(194);
              packet_return_chunk(buffer, hdr3);
              // BDDNode 195
              inc_path_counter(195);
              packet_return_chunk(buffer, hdr2);
              // BDDNode 196
              inc_path_counter(196);
              packet_return_chunk(buffer, hdr);
              // BDDNode 197
              inc_path_counter(197);
              forwarding_stats_per_route_op[197].inc_drop();
              return DROP;
            } // (0) == (((uint8_t)(*(hdr3+13))) & (64))
          } // (0) != (device & 65535)
        } else {
          // BDDNode 198
          inc_path_counter(198);
          packet_return_chunk(buffer, hdr2);
          // BDDNode 199
          inc_path_counter(199);
          packet_return_chunk(buffer, hdr);
          // BDDNode 200
          inc_path_counter(200);
          forwarding_stats_per_route_op[200].inc_drop();
          return DROP;
        } // (20ULL) <= ((uint32_t)((4294967262) + ((uint16_t)(packet_length & 65535))))
      } // (6) != (*(hdr2+9))
    } else {
      // BDDNode 201
      inc_path_counter(201);
      if ((8ULL) <= ((uint32_t)((4294967262) + ((uint16_t)(packet_length & 65535))))) {
        // BDDNode 202
        inc_path_counter(202);
        uint8_t* hdr4;
        packet_borrow_next_chunk(buffer, 8, (void**)&hdr4);
        // BDDNode 203
        inc_path_counter(203);
        if ((0) == (device & 65535)) {
          // BDDNode 204
          inc_path_counter(204);
          if ((45845) == (*(uint16_t*)(uint16_t*)(hdr4+2))) {
            // BDDNode 205
            inc_path_counter(205);
            if (((uint32_t)((4294967254) + ((uint16_t)(packet_length & 65535)))) < (4ULL)) {
              // BDDNode 206
              inc_path_counter(206);
              packet_return_chunk(buffer, hdr4);
              // BDDNode 207
              inc_path_counter(207);
              packet_return_chunk(buffer, hdr2);
              // BDDNode 208
              inc_path_counter(208);
              packet_return_chunk(buffer, hdr);
              // BDDNode 209
              inc_path_counter(209);
              forwarding_stats_per_route_op[209].inc_fwd((uint32_t)((((uint32_t)(((uint32_t)(((uint16_t)(*(hdr2+16)) << 8 | (uint16_t)(*(hdr2+17)))) << 8 | (uint32_t)(*(hdr2+18)))) << 8 | (uint32_t)(*(hdr2+19)))) >> (24)));
              return (uint32_t)((((uint32_t)(((uint32_t)(((uint16_t)(*(hdr2+16)) << 8 | (uint16_t)(*(hdr2+17)))) << 8 | (uint32_t)(*(hdr2+18)))) << 8 | (uint32_t)(*(hdr2+19)))) >> (24));
            } else {
              // BDDNode 210
              inc_path_counter(210);
              uint8_t* hdr5;
              packet_borrow_next_chunk(buffer, 4, (void**)&hdr5);
              // BDDNode 211
              inc_path_counter(211);
              uint8_t* vector_cell3 = 0;
              vector_borrow(vector, 0, (void**)&vector_cell3);
              uint32_t vector_value_out3 = *(uint32_t*)vector_cell3;
              // BDDNode 458
              inc_path_counter(458);
              uint64_t unrolled228 = (now) >> (16ULL);
              // BDDNode 212
              inc_path_counter(212);
              *(uint32_t*)vector_cell3 = (unrolled228 & 4294967295) - (((uint32_t)(((uint32_t)(((uint16_t)(*(hdr5+0)) << 8 | (uint16_t)(*(hdr5+1)))) << 8 | (uint32_t)(*(hdr5+2)))) << 8 | (uint32_t)(*(hdr5+3))));
              // BDDNode 213
              inc_path_counter(213);
              packet_return_chunk(buffer, hdr5);
              // BDDNode 214
              inc_path_counter(214);
              packet_return_chunk(buffer, hdr4);
              // BDDNode 215
              inc_path_counter(215);
              packet_return_chunk(buffer, hdr2);
              // BDDNode 216
              inc_path_counter(216);
              packet_return_chunk(buffer, hdr);
              // BDDNode 217
              inc_path_counter(217);
              forwarding_stats_per_route_op[217].inc_drop();
              return DROP;
            } // ((uint32_t)((4294967254) + ((uint16_t)(packet_length & 65535)))) < (4ULL)
          } else {
            // BDDNode 218
            inc_path_counter(218);
            packet_return_chunk(buffer, hdr4);
            // BDDNode 219
            inc_path_counter(219);
            packet_return_chunk(buffer, hdr2);
            // BDDNode 220
            inc_path_counter(220);
            packet_return_chunk(buffer, hdr);
            // BDDNode 221
            inc_path_counter(221);
            forwarding_stats_per_route_op[221].inc_fwd((uint32_t)((((uint32_t)(((uint32_t)(((uint16_t)(*(hdr2+16)) << 8 | (uint16_t)(*(hdr2+17)))) << 8 | (uint32_t)(*(hdr2+18)))) << 8 | (uint32_t)(*(hdr2+19)))) >> (24)));
            return (uint32_t)((((uint32_t)(((uint32_t)(((uint16_t)(*(hdr2+16)) << 8 | (uint16_t)(*(hdr2+17)))) << 8 | (uint32_t)(*(hdr2+18)))) << 8 | (uint32_t)(*(hdr2+19)))) >> (24));
          } // (45845) == (*(uint16_t*)(uint16_t*)(hdr4+2))
        } else {
          // BDDNode 222
          inc_path_counter(222);
          packet_return_chunk(buffer, hdr4);
          // BDDNode 223
          inc_path_counter(223);
          packet_return_chunk(buffer, hdr2);
          // BDDNode 224
          inc_path_counter(224);
          packet_return_chunk(buffer, hdr);
          // BDDNode 225
          inc_path_counter(225);
          forwarding_stats_per_route_op[225].inc_fwd((uint32_t)((((uint32_t)(((uint32_t)(((uint16_t)(*(hdr2+16)) << 8 | (uint16_t)(*(hdr2+17)))) << 8 | (uint32_t)(*(hdr2+18)))) << 8 | (uint32_t)(*(hdr2+19)))) >> (24)));
          return (uint32_t)((((uint32_t)(((uint32_t)(((uint16_t)(*(hdr2+16)) << 8 | (uint16_t)(*(hdr2+17)))) << 8 | (uint32_t)(*(hdr2+18)))) << 8 | (uint32_t)(*(hdr2+19)))) >> (24));
        } // (0) == (device & 65535)
      } else {
        // BDDNode 226
        inc_path_counter(226);
        packet_return_chunk(buffer, hdr2);
        // BDDNode 227
        inc_path_counter(227);
        packet_return_chunk(buffer, hdr);
        // BDDNode 228
        inc_path_counter(228);
        forwarding_stats_per_route_op[228].inc_fwd((uint32_t)((((uint32_t)(((uint32_t)(((uint16_t)(*(hdr2+16)) << 8 | (uint16_t)(*(hdr2+17)))) << 8 | (uint32_t)(*(hdr2+18)))) << 8 | (uint32_t)(*(hdr2+19)))) >> (24)));
        return (uint32_t)((((uint32_t)(((uint32_t)(((uint16_t)(*(hdr2+16)) << 8 | (uint16_t)(*(hdr2+17)))) << 8 | (uint32_t)(*(hdr2+18)))) << 8 | (uint32_t)(*(hdr2+19)))) >> (24));
      } // (8ULL) <= ((uint32_t)((4294967262) + ((uint16_t)(packet_length & 65535))))
    } // (17) != (*(hdr2+9))
  } else {
    // BDDNode 229
    inc_path_counter(229);
    packet_return_chunk(buffer, hdr);
    // BDDNode 230
    inc_path_counter(230);
    forwarding_stats_per_route_op[230].inc_drop();
    return DROP;
  } // ((8) == (*(uint16_t*)(uint16_t*)(hdr+12))) & ((20ULL) <= ((uint16_t)((uint32_t)((4294967282) + ((uint16_t)(packet_length & 65535))))))
}
