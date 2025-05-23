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

class PcapReader {
private:
  std::unordered_map<uint16_t, pcap_t *> pcaps;
  std::unordered_map<uint16_t, bool> assume_ip;
  std::unordered_map<uint16_t, long> pcaps_start;
  std::unordered_map<uint16_t, pkt_t> pending_pkts_per_dev;
  int64_t last_ts;
  std::unordered_set<uint16_t> last_devs;
  
  // Meta
  uint64_t total_packets;
  uint64_t total_bytes;
  uint64_t processed_packets;
  int last_percentage_report;

public:
  PcapReader() {}

  uint64_t get_total_packets() { return total_packets; }
  uint64_t get_total_bytes() { return total_bytes; }

  void setup(const std::vector<dev_pcap_t> &_pcaps) {
    last_ts                = -1;
    total_packets          = 0;
    total_bytes            = 0;
    processed_packets      = 0;
    last_percentage_report = -1;

    for (const auto &dev_pcap : _pcaps) {
      char errbuf[PCAP_ERRBUF_SIZE];
      pcap_t *pcap = pcap_open_offline(dev_pcap.pcap.c_str(), errbuf);

      if (pcap == NULL) {
        rte_exit(EXIT_FAILURE, "pcap_open_offline() failed: %s\n", errbuf);
      }

      int link_hdr_type = pcap_datalink(pcap);

      switch (link_hdr_type) {
      case DLT_EN10MB:
        // Normal ethernet, as expected.
        assume_ip[dev_pcap.device] = false;
        break;
      case DLT_RAW:
        // Contains raw IP packets.
        assume_ip[dev_pcap.device] = true;
        break;
      default: {
        fprintf(stderr, "Unknown header type (%d)", link_hdr_type);
        exit(1);
      }
      }

      pcaps[dev_pcap.device] = pcap;

      FILE *pcap_fptr = pcap_file(pcap);
      assert(pcap_fptr && "Invalid pcap file pointer");
      pcaps_start[dev_pcap.device] = ftell(pcap_fptr);

      accumulate_stats(dev_pcap.device);

      pkt_t pkt;
      if (read(dev_pcap.device, pkt)) {
        pending_pkts_per_dev[dev_pcap.device] = pkt;
      }
    }
  }

  bool get_next_packet(uint16_t &dev, pkt_t &pkt) {
    int64_t ts = -1;
    for (const auto& [pending_dev, pending_pkt] : pending_pkts_per_dev) {
      if (ts == -1 || pending_pkt.ts < ts) {
        ts = pending_pkt.ts;
      }
    }

    if (ts == -1) {
      return false;
    }
    
    bool chosen = false;
    for (const auto& [pending_dev, pending_pkt] : pending_pkts_per_dev) {
      if (pending_pkt.ts != ts) {
        continue;
      }

      if (chosen && last_devs.find(pending_dev) != last_devs.end()) {
        continue;
      }

      dev = pending_dev;
      pkt = pending_pkt;

      chosen = true;
    }

    if (last_ts != ts || last_devs.size() == pending_pkts_per_dev.size()) {
      last_devs.clear();
    }

    last_ts = ts;
    last_devs.insert(dev);

    update_and_show_progress();

    pkt_t new_pkt;
    if (read(dev, new_pkt)) {
      pending_pkts_per_dev[dev] = new_pkt;
    } else {
      pending_pkts_per_dev.erase(dev);
    }

    return true;
  }

private:
  bool read(uint16_t dev, pkt_t &pkt) {
    pcap_t *pd = pcaps[dev];

    const uint8_t *data;
    struct pcap_pkthdr *hdr;

    if (pcap_next_ex(pd, &hdr, &data) != 1) {
      rewind(dev);
      return false;
    }

    uint8_t *pkt_data = pkt.data;

    pkt.len = hdr->len;

    if (assume_ip[dev]) {
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
  void rewind(uint16_t dev) {
    pcap_t *pd      = pcaps[dev];
    long pcap_start = pcaps_start[dev];
    FILE *pcap_fptr = pcap_file(pd);
    fseek(pcap_fptr, pcap_start, SEEK_SET);
  }

  void accumulate_stats(uint16_t dev) {
    pcap_t *pd = pcaps[dev];
    pkt_t pkt;
    while (read(dev, pkt)) {
      total_packets++;
      total_bytes += pkt.len + CRC_SIZE_BYTES;
    }
  }

  void update_and_show_progress() {
    processed_packets++;
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

    if (!warmup) {
      stats_per_node.at(op).update(key, len);
      epochs.back().stats.update(key, len);
    }

    if (!epochs.empty()) {
      epochs.back().end = now;
    }
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

PcapReader warmup_reader;
PcapReader reader;
std::unordered_map<int, MapStats> stats_per_map;
std::unordered_map<int, PortStats> forwarding_stats_per_route_op;
std::unordered_map<uint64_t, uint64_t> node_pkt_counter;
time_ns_t elapsed_time;

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
  report["meta"]["pkts"]    = reader.get_total_packets();
  report["meta"]["bytes"]   = reader.get_total_bytes();

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

  warmup_reader.setup(warmup_pcaps);
  reader.setup(pcaps);

  uint16_t dev;
  pkt_t pkt;

  // First process warmup packets
  warmup = true;
  while (warmup_reader.get_next_packet(dev, pkt)) {
    nf_process(dev, pkt.data, pkt.len, pkt.ts);
  }
  warmup = false;

  // Generate the first packet manually to record the starting time
  bool success = reader.get_next_packet(dev, pkt);
  assert(success && "Failed to generate the first packet");

  time_ns_t start_time = pkt.ts;
  time_ns_t last_time  = 0;

  do {
    // Ignore destination device, we don't forward anywhere
    nf_process(dev, pkt.data, pkt.len, pkt.ts);
    last_time = pkt.ts;
  } while (reader.get_next_packet(dev, pkt));

  elapsed_time = last_time - start_time;

  NF_INFO("Elapsed virtual time: %lf s", (double)elapsed_time / 1e9);
}

int main(int argc, char **argv) {
  nf_config_init(argc, argv);
  worker_main();
  generate_report();
  return 0;
}

struct Map *map;
struct Vector *vector;
struct Vector *vector2;
struct DoubleChain *dchain;


bool nf_init() {
  int map_allocation_succeeded = map_allocate(8192, 4, &map);
  if (!map_allocation_succeeded) {
    return false;
  }
  int vector_alloc_success = vector_allocate(4, 8192, &vector);
  if (!vector_alloc_success) {
    return false;
  }
  int vector_alloc_success2 = vector_allocate(4, 8192, &vector2);
  if (!vector_alloc_success2) {
    return false;
  }
  int is_dchain_allocated = dchain_allocate(8192, &dchain);
  if (!is_dchain_allocated) {
    return false;
  }
  ports.push_back(0);
  ports.push_back(1);
  ports.push_back(2);
  ports.push_back(3);
  ports.push_back(4);
  ports.push_back(5);
  ports.push_back(6);
  ports.push_back(7);
  ports.push_back(8);
  ports.push_back(9);
  ports.push_back(10);
  ports.push_back(11);
  ports.push_back(12);
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
  ports.push_back(29);
  ports.push_back(30);
  ports.push_back(31);
  stats_per_map[1073923160].init(24);
  stats_per_map[1073923160].init(13);
  forwarding_stats_per_route_op.insert({73, PortStats{}});
  forwarding_stats_per_route_op.insert({68, PortStats{}});
  forwarding_stats_per_route_op.insert({64, PortStats{}});
  forwarding_stats_per_route_op.insert({59, PortStats{}});
  forwarding_stats_per_route_op.insert({52, PortStats{}});
  forwarding_stats_per_route_op.insert({37, PortStats{}});
  forwarding_stats_per_route_op.insert({71, PortStats{}});
  forwarding_stats_per_route_op.insert({45, PortStats{}});
  forwarding_stats_per_route_op.insert({32, PortStats{}});
  forwarding_stats_per_route_op.insert({22, PortStats{}});
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
  node_pkt_counter.insert({32, 0});
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
  node_pkt_counter.insert({16, 0});
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
  node_pkt_counter.insert({62, 0});
  return true;
}


int nf_process(uint16_t device, uint8_t *buffer, uint16_t packet_length, time_ns_t now) {
  // Node 4
  inc_path_counter(4);
  int freed_flows = profiler_expire_items_single_map(dchain, vector, map, now);
  // Node 5
  inc_path_counter(5);
  uint8_t* hdr;
  packet_borrow_next_chunk(buffer, 14, (void**)&hdr);
  // Node 6
  inc_path_counter(6);
  if (((8) == (*(uint16_t*)(uint16_t*)(hdr+12))) & ((20) <= ((uint16_t)((uint32_t)((4294967282LL) + ((uint16_t)(packet_length & 65535))))))) {
    // Node 7
    inc_path_counter(7);
    uint8_t* hdr2;
    packet_borrow_next_chunk(buffer, 20, (void**)&hdr2);
    // Node 8
    inc_path_counter(8);
    if (((17) == (*(hdr2+9))) & ((8) <= ((uint32_t)((4294967262LL) + ((uint16_t)(packet_length & 65535)))))) {
      // Node 9
      inc_path_counter(9);
      uint8_t* hdr3;
      packet_borrow_next_chunk(buffer, 8, (void**)&hdr3);
      // Node 10
      inc_path_counter(10);
      if ((((40450) == (*(uint16_t*)(uint16_t*)(hdr3+0))) | ((40450) == (*(uint16_t*)(uint16_t*)(hdr3+2)))) & ((12) <= ((uint16_t)((uint32_t)((4294967254LL) + ((uint16_t)(packet_length & 65535))))))) {
        // Node 11
        inc_path_counter(11);
        uint8_t* hdr4;
        packet_borrow_next_chunk(buffer, 12, (void**)&hdr4);
        // Node 12
        inc_path_counter(12);
        if ((0) != (device & 65535)) {
          // Node 13
          inc_path_counter(13);
          uint8_t key[4];
          uint32_t hdr4_slice = *(uint32_t*)(hdr4+1);
          *(uint32_t*)key = hdr4_slice;
          int value;
          int map_hit = map_get(map, key, &value);
          stats_per_map[1073923160].update(13, key, 4, now);
          // Node 14
          inc_path_counter(14);
          if ((0) == (map_hit)) {
            // Node 15
            inc_path_counter(15);
            if ((1) == (*(hdr4+0))) {
              // Node 16
              inc_path_counter(16);
              int index;
              int not_out_of_space = dchain_allocate_new_index(dchain, &index, now);
              // Node 17
              inc_path_counter(17);
              if ((0) == (not_out_of_space)) {
                // Node 18
                inc_path_counter(18);
                hdr4[10] = device & 255;
                hdr4[11] = (device>>8) & 255;
                packet_return_chunk(buffer, hdr4);
                // Node 19
                inc_path_counter(19);
                packet_return_chunk(buffer, hdr3);
                // Node 20
                inc_path_counter(20);
                packet_return_chunk(buffer, hdr2);
                // Node 21
                inc_path_counter(21);
                packet_return_chunk(buffer, hdr);
                // Node 22
                inc_path_counter(22);
                forwarding_stats_per_route_op[22].inc_fwd(0);
                return 0;
              } else {
                // Node 23
                inc_path_counter(23);
                uint8_t* vector_value_out = 0;
                vector_borrow(vector, index, (void**)&vector_value_out);
                // Node 24
                inc_path_counter(24);
                *(uint32_t*)vector_value_out = hdr4_slice;
                map_put(map, vector_value_out, index);
                stats_per_map[1073923160].update(24, vector_value_out, 4, now);
                // Node 25
                inc_path_counter(25);
                // Node 26
                inc_path_counter(26);
                uint8_t* vector_value_out2 = 0;
                vector_borrow(vector2, index, (void**)&vector_value_out2);
                // Node 27
                inc_path_counter(27);
                uint32_t hdr4_slice2 = *(uint32_t*)(hdr4+5);
                memcpy((void*)vector_value_out2, (void*)&hdr4_slice2, 4);
                // Node 28
                inc_path_counter(28);
                hdr4[9] = 1;
                packet_return_chunk(buffer, hdr4);
                // Node 29
                inc_path_counter(29);
                hdr3[0] = *(hdr3+2);
                hdr3[1] = *(hdr3+3);
                hdr3[2] = *(hdr3+0);
                hdr3[3] = *(hdr3+1);
                packet_return_chunk(buffer, hdr3);
                // Node 30
                inc_path_counter(30);
                hdr2[12] = *(hdr2+16);
                hdr2[13] = *(hdr2+17);
                hdr2[14] = *(hdr2+18);
                hdr2[15] = *(hdr2+19);
                hdr2[16] = *(hdr2+12);
                hdr2[17] = *(hdr2+13);
                hdr2[18] = *(hdr2+14);
                hdr2[19] = *(hdr2+15);
                packet_return_chunk(buffer, hdr2);
                // Node 31
                inc_path_counter(31);
                hdr[0] = *(hdr+6);
                hdr[1] = *(hdr+7);
                hdr[2] = *(hdr+8);
                hdr[3] = *(hdr+9);
                hdr[4] = *(hdr+10);
                hdr[5] = *(hdr+11);
                hdr[6] = *(hdr+0);
                hdr[7] = *(hdr+1);
                hdr[8] = *(hdr+2);
                hdr[9] = *(hdr+3);
                hdr[10] = *(hdr+4);
                hdr[11] = *(hdr+5);
                packet_return_chunk(buffer, hdr);
                // Node 32
                inc_path_counter(32);
                forwarding_stats_per_route_op[32].inc_fwd(device & 65535);
                return device & 65535;
              } // (0) == (not_out_of_space)
            } else {
              // Node 33
              inc_path_counter(33);
              hdr4[10] = device & 255;
              hdr4[11] = (device>>8) & 255;
              packet_return_chunk(buffer, hdr4);
              // Node 34
              inc_path_counter(34);
              packet_return_chunk(buffer, hdr3);
              // Node 35
              inc_path_counter(35);
              packet_return_chunk(buffer, hdr2);
              // Node 36
              inc_path_counter(36);
              packet_return_chunk(buffer, hdr);
              // Node 37
              inc_path_counter(37);
              forwarding_stats_per_route_op[37].inc_fwd(0);
              return 0;
            } // (1) == (*(hdr4+0))
          } else {
            // Node 38
            inc_path_counter(38);
            dchain_rejuvenate_index(dchain, value, now);
            // Node 39
            inc_path_counter(39);
            if ((1) != (*(hdr4+0))) {
              // Node 40
              inc_path_counter(40);
              if ((0) != (*(hdr4+0))) {
                // Node 41
                inc_path_counter(41);
                hdr4[9] = 1;
                packet_return_chunk(buffer, hdr4);
                // Node 42
                inc_path_counter(42);
                hdr3[0] = *(hdr3+2);
                hdr3[1] = *(hdr3+3);
                hdr3[2] = *(hdr3+0);
                hdr3[3] = *(hdr3+1);
                packet_return_chunk(buffer, hdr3);
                // Node 43
                inc_path_counter(43);
                hdr2[12] = *(hdr2+16);
                hdr2[13] = *(hdr2+17);
                hdr2[14] = *(hdr2+18);
                hdr2[15] = *(hdr2+19);
                hdr2[16] = *(hdr2+12);
                hdr2[17] = *(hdr2+13);
                hdr2[18] = *(hdr2+14);
                hdr2[19] = *(hdr2+15);
                packet_return_chunk(buffer, hdr2);
                // Node 44
                inc_path_counter(44);
                hdr[0] = *(hdr+6);
                hdr[1] = *(hdr+7);
                hdr[2] = *(hdr+8);
                hdr[3] = *(hdr+9);
                hdr[4] = *(hdr+10);
                hdr[5] = *(hdr+11);
                hdr[6] = *(hdr+0);
                hdr[7] = *(hdr+1);
                hdr[8] = *(hdr+2);
                hdr[9] = *(hdr+3);
                hdr[10] = *(hdr+4);
                hdr[11] = *(hdr+5);
                packet_return_chunk(buffer, hdr);
                // Node 45
                inc_path_counter(45);
                forwarding_stats_per_route_op[45].inc_fwd(device & 65535);
                return device & 65535;
              } else {
                // Node 46
                inc_path_counter(46);
                uint8_t* vector_value_out3 = 0;
                vector_borrow(vector2, value, (void**)&vector_value_out3);
                // Node 47
                inc_path_counter(47);
                // Node 48
                inc_path_counter(48);
                hdr4[5] = *(vector_value_out3+0);
                hdr4[6] = *(vector_value_out3+1);
                hdr4[7] = *(vector_value_out3+2);
                hdr4[8] = *(vector_value_out3+3);
                hdr4[9] = 1;
                packet_return_chunk(buffer, hdr4);
                // Node 49
                inc_path_counter(49);
                hdr3[0] = *(hdr3+2);
                hdr3[1] = *(hdr3+3);
                hdr3[2] = *(hdr3+0);
                hdr3[3] = *(hdr3+1);
                packet_return_chunk(buffer, hdr3);
                // Node 50
                inc_path_counter(50);
                hdr2[12] = *(hdr2+16);
                hdr2[13] = *(hdr2+17);
                hdr2[14] = *(hdr2+18);
                hdr2[15] = *(hdr2+19);
                hdr2[16] = *(hdr2+12);
                hdr2[17] = *(hdr2+13);
                hdr2[18] = *(hdr2+14);
                hdr2[19] = *(hdr2+15);
                packet_return_chunk(buffer, hdr2);
                // Node 51
                inc_path_counter(51);
                hdr[0] = *(hdr+6);
                hdr[1] = *(hdr+7);
                hdr[2] = *(hdr+8);
                hdr[3] = *(hdr+9);
                hdr[4] = *(hdr+10);
                hdr[5] = *(hdr+11);
                hdr[6] = *(hdr+0);
                hdr[7] = *(hdr+1);
                hdr[8] = *(hdr+2);
                hdr[9] = *(hdr+3);
                hdr[10] = *(hdr+4);
                hdr[11] = *(hdr+5);
                packet_return_chunk(buffer, hdr);
                // Node 52
                inc_path_counter(52);
                forwarding_stats_per_route_op[52].inc_fwd(device & 65535);
                return device & 65535;
              } // (0) != (*(hdr4+0))
            } else {
              // Node 53
              inc_path_counter(53);
              uint8_t* vector_value_out4 = 0;
              vector_borrow(vector2, value, (void**)&vector_value_out4);
              // Node 54
              inc_path_counter(54);
              uint32_t hdr4_slice3 = *(uint32_t*)(hdr4+5);
              memcpy((void*)vector_value_out4, (void*)&hdr4_slice3, 4);
              // Node 55
              inc_path_counter(55);
              hdr4[9] = 1;
              packet_return_chunk(buffer, hdr4);
              // Node 56
              inc_path_counter(56);
              hdr3[0] = *(hdr3+2);
              hdr3[1] = *(hdr3+3);
              hdr3[2] = *(hdr3+0);
              hdr3[3] = *(hdr3+1);
              packet_return_chunk(buffer, hdr3);
              // Node 57
              inc_path_counter(57);
              hdr2[12] = *(hdr2+16);
              hdr2[13] = *(hdr2+17);
              hdr2[14] = *(hdr2+18);
              hdr2[15] = *(hdr2+19);
              hdr2[16] = *(hdr2+12);
              hdr2[17] = *(hdr2+13);
              hdr2[18] = *(hdr2+14);
              hdr2[19] = *(hdr2+15);
              packet_return_chunk(buffer, hdr2);
              // Node 58
              inc_path_counter(58);
              hdr[0] = *(hdr+6);
              hdr[1] = *(hdr+7);
              hdr[2] = *(hdr+8);
              hdr[3] = *(hdr+9);
              hdr[4] = *(hdr+10);
              hdr[5] = *(hdr+11);
              hdr[6] = *(hdr+0);
              hdr[7] = *(hdr+1);
              hdr[8] = *(hdr+2);
              hdr[9] = *(hdr+3);
              hdr[10] = *(hdr+4);
              hdr[11] = *(hdr+5);
              packet_return_chunk(buffer, hdr);
              // Node 59
              inc_path_counter(59);
              forwarding_stats_per_route_op[59].inc_fwd(device & 65535);
              return device & 65535;
            } // (1) != (*(hdr4+0))
          } // (0) == (map_hit)
        } else {
          // Node 60
          inc_path_counter(60);
          packet_return_chunk(buffer, hdr4);
          // Node 61
          inc_path_counter(61);
          packet_return_chunk(buffer, hdr3);
          // Node 62
          inc_path_counter(62);
          packet_return_chunk(buffer, hdr2);
          // Node 63
          inc_path_counter(63);
          packet_return_chunk(buffer, hdr);
          // Node 64
          inc_path_counter(64);
          forwarding_stats_per_route_op[64].inc_fwd(*(uint16_t*)(uint16_t*)(hdr4+10));
          return *(uint16_t*)(uint16_t*)(hdr4+10);
        } // (0) != (device & 65535)
      } else {
        // Node 65
        inc_path_counter(65);
        packet_return_chunk(buffer, hdr3);
        // Node 66
        inc_path_counter(66);
        packet_return_chunk(buffer, hdr2);
        // Node 67
        inc_path_counter(67);
        packet_return_chunk(buffer, hdr);
        // Node 68
        inc_path_counter(68);
        forwarding_stats_per_route_op[68].inc_drop();
        return DROP;
      } // (((40450) == (*(uint16_t*)(uint16_t*)(hdr3+0))) | ((40450) == (*(uint16_t*)(uint16_t*)(hdr3+2)))) & ((12) <= ((uint16_t)((uint32_t)((4294967254LL) + ((uint16_t)(packet_length & 65535))))))
    } else {
      // Node 69
      inc_path_counter(69);
      packet_return_chunk(buffer, hdr2);
      // Node 70
      inc_path_counter(70);
      packet_return_chunk(buffer, hdr);
      // Node 71
      inc_path_counter(71);
      forwarding_stats_per_route_op[71].inc_drop();
      return DROP;
    } // ((17) == (*(hdr2+9))) & ((8) <= ((uint32_t)((4294967262LL) + ((uint16_t)(packet_length & 65535)))))
  } else {
    // Node 72
    inc_path_counter(72);
    packet_return_chunk(buffer, hdr);
    // Node 73
    inc_path_counter(73);
    forwarding_stats_per_route_op[73].inc_drop();
    return DROP;
  } // ((8) == (*(uint16_t*)(uint16_t*)(hdr+12))) & ((20) <= ((uint16_t)((uint32_t)((4294967282LL) + ((uint16_t)(packet_length & 65535))))))
}
