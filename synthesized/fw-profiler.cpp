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

#define DROP ((uint16_t)-1)
#define FLOOD ((uint16_t)-2)

#define MIN_PKT_SIZE 64   // With CRC
#define MAX_PKT_SIZE 1518 // With CRC

#define DEFAULT_SRC_MAC "90:e2:ba:8e:4f:6c"
#define DEFAULT_DST_MAC "90:e2:ba:8e:4f:6d"

#define EPOCH_DURATION_NS 1'000'000'000 // 1 second

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
  uint8_t data[MAX_PKT_SIZE];
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
    bool set = false;

    for (const auto &dev_pkt : pending_pkts_per_dev) {
      if (!set || dev_pkt.second.ts < pkt.ts) {
        dev = dev_pkt.first;
        pkt = dev_pkt.second;
        set = true;
      }
    }

    if (set) {
      pkt_t new_pkt;
      if (read(dev, new_pkt)) {
        pending_pkts_per_dev[dev] = new_pkt;
      } else {
        pending_pkts_per_dev.erase(dev);
      }
    }

    update_and_show_progress();

    return set;
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

    if (assume_ip[dev]) {
      struct rte_ether_hdr *eth_hdr = (struct rte_ether_hdr *)pkt_data;
      nf_parse_etheraddr(DEFAULT_DST_MAC, &eth_hdr->d_addr);
      nf_parse_etheraddr(DEFAULT_SRC_MAC, &eth_hdr->s_addr);
      eth_hdr->ether_type = rte_bswap16(RTE_ETHER_TYPE_IPV4);
      pkt_data += sizeof(struct rte_ether_hdr);
    }

    memcpy(pkt_data, data, hdr->len);
    pkt.len = hdr->len;
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
      uint8_t *data = pkt.data;

      if (!assume_ip[dev]) {
        struct rte_ether_hdr *eth_hdr = (struct rte_ether_hdr *)data;
        data += sizeof(struct rte_ether_hdr);
        if (eth_hdr->ether_type != rte_bswap16(RTE_ETHER_TYPE_IPV4)) {
          total_bytes += pkt.len;
          continue;
        }
      }

      struct rte_ipv4_hdr *ip_hdr = (struct rte_ipv4_hdr *)(data);

      uint16_t len = rte_bswap16(ip_hdr->total_length) + sizeof(struct rte_ether_hdr);

      total_bytes += len;
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

  MapStats() : epoch_duration(EPOCH_DURATION_NS) {}

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
struct DoubleChain *dchain;
struct Vector *vector2;
struct Vector *vector3;


bool nf_init() {
  int map_allocation_succeeded = map_allocate(65536, 13, &map);
  if (!map_allocation_succeeded) {
    return false;
  }
  int vector_alloc_success = vector_allocate(13, 65536, &vector);
  if (!vector_alloc_success) {
    return false;
  }
  int is_dchain_allocated = dchain_allocate(65536, &dchain);
  if (!is_dchain_allocated) {
    return false;
  }
  int vector_alloc_success2 = vector_allocate(4, 32, &vector2);
  if (!vector_alloc_success2) {
    return false;
  }
  int vector_alloc_success3 = vector_allocate(2, 32, &vector3);
  if (!vector_alloc_success3) {
    return false;
  }
  uint8_t* vector_value_out = 0;
  vector_borrow(vector2, 0, (void**)&vector_value_out);
  *(uint32_t*)vector_value_out = 1;
  uint8_t* vector_value_out2 = 0;
  vector_borrow(vector3, 0, (void**)&vector_value_out2);
  *(uint16_t*)vector_value_out2 = 1;
  uint8_t* vector_value_out3 = 0;
  vector_borrow(vector2, 1, (void**)&vector_value_out3);
  *(uint32_t*)vector_value_out3 = 0;
  uint8_t* vector_value_out4 = 0;
  vector_borrow(vector3, 1, (void**)&vector_value_out4);
  *(uint16_t*)vector_value_out4 = 0;
  uint8_t* vector_value_out5 = 0;
  vector_borrow(vector2, 2, (void**)&vector_value_out5);
  *(uint32_t*)vector_value_out5 = 1;
  uint8_t* vector_value_out6 = 0;
  vector_borrow(vector3, 2, (void**)&vector_value_out6);
  *(uint16_t*)vector_value_out6 = 3;
  uint8_t* vector_value_out7 = 0;
  vector_borrow(vector2, 3, (void**)&vector_value_out7);
  *(uint32_t*)vector_value_out7 = 0;
  uint8_t* vector_value_out8 = 0;
  vector_borrow(vector3, 3, (void**)&vector_value_out8);
  *(uint16_t*)vector_value_out8 = 2;
  uint8_t* vector_value_out9 = 0;
  vector_borrow(vector2, 4, (void**)&vector_value_out9);
  *(uint32_t*)vector_value_out9 = 1;
  uint8_t* vector_value_out10 = 0;
  vector_borrow(vector3, 4, (void**)&vector_value_out10);
  *(uint16_t*)vector_value_out10 = 5;
  uint8_t* vector_value_out11 = 0;
  vector_borrow(vector2, 5, (void**)&vector_value_out11);
  *(uint32_t*)vector_value_out11 = 0;
  uint8_t* vector_value_out12 = 0;
  vector_borrow(vector3, 5, (void**)&vector_value_out12);
  *(uint16_t*)vector_value_out12 = 4;
  uint8_t* vector_value_out13 = 0;
  vector_borrow(vector2, 6, (void**)&vector_value_out13);
  *(uint32_t*)vector_value_out13 = 1;
  uint8_t* vector_value_out14 = 0;
  vector_borrow(vector3, 6, (void**)&vector_value_out14);
  *(uint16_t*)vector_value_out14 = 7;
  uint8_t* vector_value_out15 = 0;
  vector_borrow(vector2, 7, (void**)&vector_value_out15);
  *(uint32_t*)vector_value_out15 = 0;
  uint8_t* vector_value_out16 = 0;
  vector_borrow(vector3, 7, (void**)&vector_value_out16);
  *(uint16_t*)vector_value_out16 = 6;
  uint8_t* vector_value_out17 = 0;
  vector_borrow(vector2, 8, (void**)&vector_value_out17);
  *(uint32_t*)vector_value_out17 = 1;
  uint8_t* vector_value_out18 = 0;
  vector_borrow(vector3, 8, (void**)&vector_value_out18);
  *(uint16_t*)vector_value_out18 = 9;
  uint8_t* vector_value_out19 = 0;
  vector_borrow(vector2, 9, (void**)&vector_value_out19);
  *(uint32_t*)vector_value_out19 = 0;
  uint8_t* vector_value_out20 = 0;
  vector_borrow(vector3, 9, (void**)&vector_value_out20);
  *(uint16_t*)vector_value_out20 = 8;
  uint8_t* vector_value_out21 = 0;
  vector_borrow(vector2, 10, (void**)&vector_value_out21);
  *(uint32_t*)vector_value_out21 = 1;
  uint8_t* vector_value_out22 = 0;
  vector_borrow(vector3, 10, (void**)&vector_value_out22);
  *(uint16_t*)vector_value_out22 = 11;
  uint8_t* vector_value_out23 = 0;
  vector_borrow(vector2, 11, (void**)&vector_value_out23);
  *(uint32_t*)vector_value_out23 = 0;
  uint8_t* vector_value_out24 = 0;
  vector_borrow(vector3, 11, (void**)&vector_value_out24);
  *(uint16_t*)vector_value_out24 = 10;
  uint8_t* vector_value_out25 = 0;
  vector_borrow(vector2, 12, (void**)&vector_value_out25);
  *(uint32_t*)vector_value_out25 = 1;
  uint8_t* vector_value_out26 = 0;
  vector_borrow(vector3, 12, (void**)&vector_value_out26);
  *(uint16_t*)vector_value_out26 = 13;
  uint8_t* vector_value_out27 = 0;
  vector_borrow(vector2, 13, (void**)&vector_value_out27);
  *(uint32_t*)vector_value_out27 = 0;
  uint8_t* vector_value_out28 = 0;
  vector_borrow(vector3, 13, (void**)&vector_value_out28);
  *(uint16_t*)vector_value_out28 = 12;
  uint8_t* vector_value_out29 = 0;
  vector_borrow(vector2, 14, (void**)&vector_value_out29);
  *(uint32_t*)vector_value_out29 = 1;
  uint8_t* vector_value_out30 = 0;
  vector_borrow(vector3, 14, (void**)&vector_value_out30);
  *(uint16_t*)vector_value_out30 = 15;
  uint8_t* vector_value_out31 = 0;
  vector_borrow(vector2, 15, (void**)&vector_value_out31);
  *(uint32_t*)vector_value_out31 = 0;
  uint8_t* vector_value_out32 = 0;
  vector_borrow(vector3, 15, (void**)&vector_value_out32);
  *(uint16_t*)vector_value_out32 = 14;
  uint8_t* vector_value_out33 = 0;
  vector_borrow(vector2, 16, (void**)&vector_value_out33);
  *(uint32_t*)vector_value_out33 = 1;
  uint8_t* vector_value_out34 = 0;
  vector_borrow(vector3, 16, (void**)&vector_value_out34);
  *(uint16_t*)vector_value_out34 = 17;
  uint8_t* vector_value_out35 = 0;
  vector_borrow(vector2, 17, (void**)&vector_value_out35);
  *(uint32_t*)vector_value_out35 = 0;
  uint8_t* vector_value_out36 = 0;
  vector_borrow(vector3, 17, (void**)&vector_value_out36);
  *(uint16_t*)vector_value_out36 = 16;
  uint8_t* vector_value_out37 = 0;
  vector_borrow(vector2, 18, (void**)&vector_value_out37);
  *(uint32_t*)vector_value_out37 = 1;
  uint8_t* vector_value_out38 = 0;
  vector_borrow(vector3, 18, (void**)&vector_value_out38);
  *(uint16_t*)vector_value_out38 = 19;
  uint8_t* vector_value_out39 = 0;
  vector_borrow(vector2, 19, (void**)&vector_value_out39);
  *(uint32_t*)vector_value_out39 = 0;
  uint8_t* vector_value_out40 = 0;
  vector_borrow(vector3, 19, (void**)&vector_value_out40);
  *(uint16_t*)vector_value_out40 = 18;
  uint8_t* vector_value_out41 = 0;
  vector_borrow(vector2, 20, (void**)&vector_value_out41);
  *(uint32_t*)vector_value_out41 = 1;
  uint8_t* vector_value_out42 = 0;
  vector_borrow(vector3, 20, (void**)&vector_value_out42);
  *(uint16_t*)vector_value_out42 = 21;
  uint8_t* vector_value_out43 = 0;
  vector_borrow(vector2, 21, (void**)&vector_value_out43);
  *(uint32_t*)vector_value_out43 = 0;
  uint8_t* vector_value_out44 = 0;
  vector_borrow(vector3, 21, (void**)&vector_value_out44);
  *(uint16_t*)vector_value_out44 = 20;
  uint8_t* vector_value_out45 = 0;
  vector_borrow(vector2, 22, (void**)&vector_value_out45);
  *(uint32_t*)vector_value_out45 = 1;
  uint8_t* vector_value_out46 = 0;
  vector_borrow(vector3, 22, (void**)&vector_value_out46);
  *(uint16_t*)vector_value_out46 = 23;
  uint8_t* vector_value_out47 = 0;
  vector_borrow(vector2, 23, (void**)&vector_value_out47);
  *(uint32_t*)vector_value_out47 = 0;
  uint8_t* vector_value_out48 = 0;
  vector_borrow(vector3, 23, (void**)&vector_value_out48);
  *(uint16_t*)vector_value_out48 = 22;
  uint8_t* vector_value_out49 = 0;
  vector_borrow(vector2, 24, (void**)&vector_value_out49);
  *(uint32_t*)vector_value_out49 = 1;
  uint8_t* vector_value_out50 = 0;
  vector_borrow(vector3, 24, (void**)&vector_value_out50);
  *(uint16_t*)vector_value_out50 = 25;
  uint8_t* vector_value_out51 = 0;
  vector_borrow(vector2, 25, (void**)&vector_value_out51);
  *(uint32_t*)vector_value_out51 = 0;
  uint8_t* vector_value_out52 = 0;
  vector_borrow(vector3, 25, (void**)&vector_value_out52);
  *(uint16_t*)vector_value_out52 = 24;
  uint8_t* vector_value_out53 = 0;
  vector_borrow(vector2, 26, (void**)&vector_value_out53);
  *(uint32_t*)vector_value_out53 = 1;
  uint8_t* vector_value_out54 = 0;
  vector_borrow(vector3, 26, (void**)&vector_value_out54);
  *(uint16_t*)vector_value_out54 = 27;
  uint8_t* vector_value_out55 = 0;
  vector_borrow(vector2, 27, (void**)&vector_value_out55);
  *(uint32_t*)vector_value_out55 = 0;
  uint8_t* vector_value_out56 = 0;
  vector_borrow(vector3, 27, (void**)&vector_value_out56);
  *(uint16_t*)vector_value_out56 = 26;
  uint8_t* vector_value_out57 = 0;
  vector_borrow(vector2, 28, (void**)&vector_value_out57);
  *(uint32_t*)vector_value_out57 = 1;
  uint8_t* vector_value_out58 = 0;
  vector_borrow(vector3, 28, (void**)&vector_value_out58);
  *(uint16_t*)vector_value_out58 = 29;
  uint8_t* vector_value_out59 = 0;
  vector_borrow(vector2, 29, (void**)&vector_value_out59);
  *(uint32_t*)vector_value_out59 = 0;
  uint8_t* vector_value_out60 = 0;
  vector_borrow(vector3, 29, (void**)&vector_value_out60);
  *(uint16_t*)vector_value_out60 = 28;
  uint8_t* vector_value_out61 = 0;
  vector_borrow(vector2, 30, (void**)&vector_value_out61);
  *(uint32_t*)vector_value_out61 = 1;
  uint8_t* vector_value_out62 = 0;
  vector_borrow(vector3, 30, (void**)&vector_value_out62);
  *(uint16_t*)vector_value_out62 = 31;
  uint8_t* vector_value_out63 = 0;
  vector_borrow(vector2, 31, (void**)&vector_value_out63);
  *(uint32_t*)vector_value_out63 = 0;
  uint8_t* vector_value_out64 = 0;
  vector_borrow(vector3, 31, (void**)&vector_value_out64);
  *(uint16_t*)vector_value_out64 = 30;
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
  stats_per_map[1074043808].init(170);
  stats_per_map[1074043808].init(157);
  stats_per_map[1074043808].init(142);
  forwarding_stats_per_route_op.insert({188, PortStats{}});
  forwarding_stats_per_route_op.insert({187, PortStats{}});
  forwarding_stats_per_route_op.insert({179, PortStats{}});
  forwarding_stats_per_route_op.insert({191, PortStats{}});
  forwarding_stats_per_route_op.insert({178, PortStats{}});
  forwarding_stats_per_route_op.insert({193, PortStats{}});
  forwarding_stats_per_route_op.insert({167, PortStats{}});
  forwarding_stats_per_route_op.insert({156, PortStats{}});
  forwarding_stats_per_route_op.insert({168, PortStats{}});
  forwarding_stats_per_route_op.insert({155, PortStats{}});
  forwarding_stats_per_route_op.insert({147, PortStats{}});
  node_pkt_counter.insert({193, 0});
  node_pkt_counter.insert({192, 0});
  node_pkt_counter.insert({161, 0});
  node_pkt_counter.insert({160, 0});
  node_pkt_counter.insert({159, 0});
  node_pkt_counter.insert({158, 0});
  node_pkt_counter.insert({157, 0});
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
  node_pkt_counter.insert({133, 0});
  node_pkt_counter.insert({134, 0});
  node_pkt_counter.insert({135, 0});
  node_pkt_counter.insert({136, 0});
  node_pkt_counter.insert({137, 0});
  node_pkt_counter.insert({138, 0});
  node_pkt_counter.insert({139, 0});
  node_pkt_counter.insert({140, 0});
  node_pkt_counter.insert({141, 0});
  node_pkt_counter.insert({142, 0});
  node_pkt_counter.insert({143, 0});
  node_pkt_counter.insert({144, 0});
  node_pkt_counter.insert({145, 0});
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
  node_pkt_counter.insert({173, 0});
  node_pkt_counter.insert({174, 0});
  node_pkt_counter.insert({175, 0});
  node_pkt_counter.insert({176, 0});
  node_pkt_counter.insert({177, 0});
  node_pkt_counter.insert({178, 0});
  node_pkt_counter.insert({179, 0});
  node_pkt_counter.insert({180, 0});
  node_pkt_counter.insert({181, 0});
  node_pkt_counter.insert({182, 0});
  node_pkt_counter.insert({183, 0});
  node_pkt_counter.insert({184, 0});
  node_pkt_counter.insert({185, 0});
  node_pkt_counter.insert({186, 0});
  node_pkt_counter.insert({187, 0});
  node_pkt_counter.insert({188, 0});
  node_pkt_counter.insert({189, 0});
  node_pkt_counter.insert({190, 0});
  node_pkt_counter.insert({191, 0});
  return true;
}


int nf_process(uint16_t device, uint8_t *buffer, uint16_t packet_length, time_ns_t now) {
  // Node 133
  inc_path_counter(133);
  int freed_flows = expire_items_single_map(dchain, vector, map, (-1000000000LL) + (now));
  // Node 134
  inc_path_counter(134);
  uint8_t* hdr;
  packet_borrow_next_chunk(buffer, 14, (void**)&hdr);
  // Node 135
  inc_path_counter(135);
  if (((8) == (*(uint16_t*)(uint16_t*)(hdr+12))) & ((20) <= ((uint16_t)((uint32_t)((4294967282LL) + ((uint16_t)(packet_length & 65535))))))) {
    // Node 136
    inc_path_counter(136);
    uint8_t* hdr2;
    packet_borrow_next_chunk(buffer, 20, (void**)&hdr2);
    // Node 137
    inc_path_counter(137);
    if ((((6) == (*(hdr2+9))) | ((17) == (*(hdr2+9)))) & ((4) <= ((uint32_t)((4294967262LL) + ((uint16_t)(packet_length & 65535)))))) {
      // Node 138
      inc_path_counter(138);
      uint8_t* hdr3;
      packet_borrow_next_chunk(buffer, 4, (void**)&hdr3);
      // Node 139
      inc_path_counter(139);
      uint8_t* vector_value_out65 = 0;
      vector_borrow(vector2, (uint16_t)(device & 65535), (void**)&vector_value_out65);
      // Node 140
      inc_path_counter(140);
      // Node 141
      inc_path_counter(141);
      if ((0) == ((uint8_t)((bool)((0) != (*(uint32_t*)vector_value_out65))))) {
        // Node 142
        inc_path_counter(142);
        uint8_t key[13];
        key[0] = *(hdr3+2);
        key[1] = *(hdr3+3);
        key[2] = *(hdr3+0);
        key[3] = *(hdr3+1);
        key[4] = *(hdr2+16);
        key[5] = *(hdr2+17);
        key[6] = *(hdr2+18);
        key[7] = *(hdr2+19);
        key[8] = *(hdr2+12);
        key[9] = *(hdr2+13);
        key[10] = *(hdr2+14);
        key[11] = *(hdr2+15);
        key[12] = *(hdr2+9);
        int value;
        int map_hit = map_get(map, key, &value);
        stats_per_map[1074043808].update(142, key, 13, now);
        // Node 143
        inc_path_counter(143);
        if ((0) == (map_hit)) {
          // Node 144
          inc_path_counter(144);
          packet_return_chunk(buffer, hdr3);
          // Node 145
          inc_path_counter(145);
          packet_return_chunk(buffer, hdr2);
          // Node 146
          inc_path_counter(146);
          packet_return_chunk(buffer, hdr);
          // Node 147
          inc_path_counter(147);
          forwarding_stats_per_route_op[147].inc_drop();
          return DROP;
        } else {
          // Node 148
          inc_path_counter(148);
          dchain_rejuvenate_index(dchain, value, now);
          // Node 149
          inc_path_counter(149);
          uint8_t* vector_value_out66 = 0;
          vector_borrow(vector3, (uint16_t)(device & 65535), (void**)&vector_value_out66);
          // Node 150
          inc_path_counter(150);
          // Node 151
          inc_path_counter(151);
          packet_return_chunk(buffer, hdr3);
          // Node 152
          inc_path_counter(152);
          packet_return_chunk(buffer, hdr2);
          // Node 153
          inc_path_counter(153);
          packet_return_chunk(buffer, hdr);
          // Node 154
          inc_path_counter(154);
          if ((65535) != ((uint32_t)((uint16_t)(*(uint16_t*)vector_value_out66)))) {
            // Node 155
            inc_path_counter(155);
            forwarding_stats_per_route_op[155].inc_fwd((uint32_t)((uint16_t)(*(uint16_t*)vector_value_out66)));
            return (uint32_t)((uint16_t)(*(uint16_t*)vector_value_out66));
          } else {
            // Node 156
            inc_path_counter(156);
            forwarding_stats_per_route_op[156].inc_drop();
            return DROP;
          } // (65535) != ((uint32_t)((uint16_t)(*(uint16_t*)vector_value_out66)))
        } // (0) == (map_hit)
      } else {
        // Node 157
        inc_path_counter(157);
        uint8_t key2[13];
        key2[0] = *(hdr3+0);
        key2[1] = *(hdr3+1);
        key2[2] = *(hdr3+2);
        key2[3] = *(hdr3+3);
        key2[4] = *(hdr2+12);
        key2[5] = *(hdr2+13);
        key2[6] = *(hdr2+14);
        key2[7] = *(hdr2+15);
        key2[8] = *(hdr2+16);
        key2[9] = *(hdr2+17);
        key2[10] = *(hdr2+18);
        key2[11] = *(hdr2+19);
        key2[12] = *(hdr2+9);
        int value2;
        int map_hit2 = map_get(map, key2, &value2);
        stats_per_map[1074043808].update(157, key2, 13, now);
        // Node 158
        inc_path_counter(158);
        if ((0) == (map_hit2)) {
          // Node 159
          inc_path_counter(159);
          int index;
          int not_out_of_space = dchain_allocate_new_index(dchain, &index, now);
          // Node 160
          inc_path_counter(160);
          if ((0) == (not_out_of_space)) {
            // Node 161
            inc_path_counter(161);
            uint8_t* vector_value_out67 = 0;
            vector_borrow(vector3, (uint16_t)(device & 65535), (void**)&vector_value_out67);
            // Node 162
            inc_path_counter(162);
            // Node 163
            inc_path_counter(163);
            packet_return_chunk(buffer, hdr3);
            // Node 164
            inc_path_counter(164);
            packet_return_chunk(buffer, hdr2);
            // Node 165
            inc_path_counter(165);
            packet_return_chunk(buffer, hdr);
            // Node 166
            inc_path_counter(166);
            if ((65535) != ((uint32_t)((uint16_t)(*(uint16_t*)vector_value_out67)))) {
              // Node 167
              inc_path_counter(167);
              forwarding_stats_per_route_op[167].inc_fwd((uint32_t)((uint16_t)(*(uint16_t*)vector_value_out67)));
              return (uint32_t)((uint16_t)(*(uint16_t*)vector_value_out67));
            } else {
              // Node 168
              inc_path_counter(168);
              forwarding_stats_per_route_op[168].inc_drop();
              return DROP;
            } // (65535) != ((uint32_t)((uint16_t)(*(uint16_t*)vector_value_out67)))
          } else {
            // Node 169
            inc_path_counter(169);
            uint8_t* vector_value_out68 = 0;
            vector_borrow(vector, index, (void**)&vector_value_out68);
            // Node 170
            inc_path_counter(170);
            memcpy((void*)vector_value_out68, (void*)key2, 13);
            map_put(map, vector_value_out68, index);
            stats_per_map[1074043808].update(170, vector_value_out68, 13, now);
            // Node 171
            inc_path_counter(171);
            // Node 172
            inc_path_counter(172);
            uint8_t* vector_value_out69 = 0;
            vector_borrow(vector3, (uint16_t)(device & 65535), (void**)&vector_value_out69);
            // Node 173
            inc_path_counter(173);
            // Node 174
            inc_path_counter(174);
            packet_return_chunk(buffer, hdr3);
            // Node 175
            inc_path_counter(175);
            packet_return_chunk(buffer, hdr2);
            // Node 176
            inc_path_counter(176);
            packet_return_chunk(buffer, hdr);
            // Node 177
            inc_path_counter(177);
            if ((65535) != ((uint32_t)((uint16_t)(*(uint16_t*)vector_value_out69)))) {
              // Node 178
              inc_path_counter(178);
              forwarding_stats_per_route_op[178].inc_fwd((uint32_t)((uint16_t)(*(uint16_t*)vector_value_out69)));
              return (uint32_t)((uint16_t)(*(uint16_t*)vector_value_out69));
            } else {
              // Node 179
              inc_path_counter(179);
              forwarding_stats_per_route_op[179].inc_drop();
              return DROP;
            } // (65535) != ((uint32_t)((uint16_t)(*(uint16_t*)vector_value_out69)))
          } // (0) == (not_out_of_space)
        } else {
          // Node 180
          inc_path_counter(180);
          dchain_rejuvenate_index(dchain, value2, now);
          // Node 181
          inc_path_counter(181);
          uint8_t* vector_value_out70 = 0;
          vector_borrow(vector3, (uint16_t)(device & 65535), (void**)&vector_value_out70);
          // Node 182
          inc_path_counter(182);
          // Node 183
          inc_path_counter(183);
          packet_return_chunk(buffer, hdr3);
          // Node 184
          inc_path_counter(184);
          packet_return_chunk(buffer, hdr2);
          // Node 185
          inc_path_counter(185);
          packet_return_chunk(buffer, hdr);
          // Node 186
          inc_path_counter(186);
          if ((65535) != ((uint32_t)((uint16_t)(*(uint16_t*)vector_value_out70)))) {
            // Node 187
            inc_path_counter(187);
            forwarding_stats_per_route_op[187].inc_fwd((uint32_t)((uint16_t)(*(uint16_t*)vector_value_out70)));
            return (uint32_t)((uint16_t)(*(uint16_t*)vector_value_out70));
          } else {
            // Node 188
            inc_path_counter(188);
            forwarding_stats_per_route_op[188].inc_drop();
            return DROP;
          } // (65535) != ((uint32_t)((uint16_t)(*(uint16_t*)vector_value_out70)))
        } // (0) == (map_hit2)
      } // (0) == ((uint8_t)((bool)((0) != (*(uint32_t*)vector_value_out65))))
    } else {
      // Node 189
      inc_path_counter(189);
      packet_return_chunk(buffer, hdr2);
      // Node 190
      inc_path_counter(190);
      packet_return_chunk(buffer, hdr);
      // Node 191
      inc_path_counter(191);
      forwarding_stats_per_route_op[191].inc_drop();
      return DROP;
    } // (((6) == (*(hdr2+9))) | ((17) == (*(hdr2+9)))) & ((4) <= ((uint32_t)((4294967262LL) + ((uint16_t)(packet_length & 65535)))))
  } else {
    // Node 192
    inc_path_counter(192);
    packet_return_chunk(buffer, hdr);
    // Node 193
    inc_path_counter(193);
    forwarding_stats_per_route_op[193].inc_drop();
    return DROP;
  } // ((8) == (*(uint16_t*)(uint16_t*)(hdr+12))) & ((20) <= ((uint16_t)((uint32_t)((4294967282LL) + ((uint16_t)(packet_length & 65535))))))
}
