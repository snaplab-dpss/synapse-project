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
      nf_parse_etheraddr(DEFAULT_DST_MAC, &eth_hdr->dst_addr);
      nf_parse_etheraddr(DEFAULT_SRC_MAC, &eth_hdr->src_addr);
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

struct TokenBucket *tb;
struct Vector *vector;
struct Vector *vector2;


bool nf_init() {
  tb_allocate(65536, 17179869184ULLull, 131072ull, 4, &tb);
  if (!tb_allocation_succeeded) {
    return false;
  }
  int vector_alloc_success = vector_allocate(4, 32, &vector);
  if (!vector_alloc_success) {
    return false;
  }
  int vector_alloc_success2 = vector_allocate(2, 32, &vector2);
  if (!vector_alloc_success2) {
    return false;
  }
  uint8_t* vector_value_out = 0;
  vector_borrow(vector, 0, (void**)&vector_value_out);
  *(uint32_t*)vector_value_out = 1;
  uint8_t* vector_value_out2 = 0;
  vector_borrow(vector2, 0, (void**)&vector_value_out2);
  *(uint16_t*)vector_value_out2 = 1;
  uint8_t* vector_value_out3 = 0;
  vector_borrow(vector, 1, (void**)&vector_value_out3);
  *(uint32_t*)vector_value_out3 = 0;
  uint8_t* vector_value_out4 = 0;
  vector_borrow(vector2, 1, (void**)&vector_value_out4);
  *(uint16_t*)vector_value_out4 = 0;
  uint8_t* vector_value_out5 = 0;
  vector_borrow(vector, 2, (void**)&vector_value_out5);
  *(uint32_t*)vector_value_out5 = 1;
  uint8_t* vector_value_out6 = 0;
  vector_borrow(vector2, 2, (void**)&vector_value_out6);
  *(uint16_t*)vector_value_out6 = 3;
  uint8_t* vector_value_out7 = 0;
  vector_borrow(vector, 3, (void**)&vector_value_out7);
  *(uint32_t*)vector_value_out7 = 0;
  uint8_t* vector_value_out8 = 0;
  vector_borrow(vector2, 3, (void**)&vector_value_out8);
  *(uint16_t*)vector_value_out8 = 2;
  uint8_t* vector_value_out9 = 0;
  vector_borrow(vector, 4, (void**)&vector_value_out9);
  *(uint32_t*)vector_value_out9 = 1;
  uint8_t* vector_value_out10 = 0;
  vector_borrow(vector2, 4, (void**)&vector_value_out10);
  *(uint16_t*)vector_value_out10 = 5;
  uint8_t* vector_value_out11 = 0;
  vector_borrow(vector, 5, (void**)&vector_value_out11);
  *(uint32_t*)vector_value_out11 = 0;
  uint8_t* vector_value_out12 = 0;
  vector_borrow(vector2, 5, (void**)&vector_value_out12);
  *(uint16_t*)vector_value_out12 = 4;
  uint8_t* vector_value_out13 = 0;
  vector_borrow(vector, 6, (void**)&vector_value_out13);
  *(uint32_t*)vector_value_out13 = 1;
  uint8_t* vector_value_out14 = 0;
  vector_borrow(vector2, 6, (void**)&vector_value_out14);
  *(uint16_t*)vector_value_out14 = 7;
  uint8_t* vector_value_out15 = 0;
  vector_borrow(vector, 7, (void**)&vector_value_out15);
  *(uint32_t*)vector_value_out15 = 0;
  uint8_t* vector_value_out16 = 0;
  vector_borrow(vector2, 7, (void**)&vector_value_out16);
  *(uint16_t*)vector_value_out16 = 6;
  uint8_t* vector_value_out17 = 0;
  vector_borrow(vector, 8, (void**)&vector_value_out17);
  *(uint32_t*)vector_value_out17 = 1;
  uint8_t* vector_value_out18 = 0;
  vector_borrow(vector2, 8, (void**)&vector_value_out18);
  *(uint16_t*)vector_value_out18 = 9;
  uint8_t* vector_value_out19 = 0;
  vector_borrow(vector, 9, (void**)&vector_value_out19);
  *(uint32_t*)vector_value_out19 = 0;
  uint8_t* vector_value_out20 = 0;
  vector_borrow(vector2, 9, (void**)&vector_value_out20);
  *(uint16_t*)vector_value_out20 = 8;
  uint8_t* vector_value_out21 = 0;
  vector_borrow(vector, 10, (void**)&vector_value_out21);
  *(uint32_t*)vector_value_out21 = 1;
  uint8_t* vector_value_out22 = 0;
  vector_borrow(vector2, 10, (void**)&vector_value_out22);
  *(uint16_t*)vector_value_out22 = 11;
  uint8_t* vector_value_out23 = 0;
  vector_borrow(vector, 11, (void**)&vector_value_out23);
  *(uint32_t*)vector_value_out23 = 0;
  uint8_t* vector_value_out24 = 0;
  vector_borrow(vector2, 11, (void**)&vector_value_out24);
  *(uint16_t*)vector_value_out24 = 10;
  uint8_t* vector_value_out25 = 0;
  vector_borrow(vector, 12, (void**)&vector_value_out25);
  *(uint32_t*)vector_value_out25 = 1;
  uint8_t* vector_value_out26 = 0;
  vector_borrow(vector2, 12, (void**)&vector_value_out26);
  *(uint16_t*)vector_value_out26 = 13;
  uint8_t* vector_value_out27 = 0;
  vector_borrow(vector, 13, (void**)&vector_value_out27);
  *(uint32_t*)vector_value_out27 = 0;
  uint8_t* vector_value_out28 = 0;
  vector_borrow(vector2, 13, (void**)&vector_value_out28);
  *(uint16_t*)vector_value_out28 = 12;
  uint8_t* vector_value_out29 = 0;
  vector_borrow(vector, 14, (void**)&vector_value_out29);
  *(uint32_t*)vector_value_out29 = 1;
  uint8_t* vector_value_out30 = 0;
  vector_borrow(vector2, 14, (void**)&vector_value_out30);
  *(uint16_t*)vector_value_out30 = 15;
  uint8_t* vector_value_out31 = 0;
  vector_borrow(vector, 15, (void**)&vector_value_out31);
  *(uint32_t*)vector_value_out31 = 0;
  uint8_t* vector_value_out32 = 0;
  vector_borrow(vector2, 15, (void**)&vector_value_out32);
  *(uint16_t*)vector_value_out32 = 14;
  uint8_t* vector_value_out33 = 0;
  vector_borrow(vector, 16, (void**)&vector_value_out33);
  *(uint32_t*)vector_value_out33 = 1;
  uint8_t* vector_value_out34 = 0;
  vector_borrow(vector2, 16, (void**)&vector_value_out34);
  *(uint16_t*)vector_value_out34 = 17;
  uint8_t* vector_value_out35 = 0;
  vector_borrow(vector, 17, (void**)&vector_value_out35);
  *(uint32_t*)vector_value_out35 = 0;
  uint8_t* vector_value_out36 = 0;
  vector_borrow(vector2, 17, (void**)&vector_value_out36);
  *(uint16_t*)vector_value_out36 = 16;
  uint8_t* vector_value_out37 = 0;
  vector_borrow(vector, 18, (void**)&vector_value_out37);
  *(uint32_t*)vector_value_out37 = 1;
  uint8_t* vector_value_out38 = 0;
  vector_borrow(vector2, 18, (void**)&vector_value_out38);
  *(uint16_t*)vector_value_out38 = 19;
  uint8_t* vector_value_out39 = 0;
  vector_borrow(vector, 19, (void**)&vector_value_out39);
  *(uint32_t*)vector_value_out39 = 0;
  uint8_t* vector_value_out40 = 0;
  vector_borrow(vector2, 19, (void**)&vector_value_out40);
  *(uint16_t*)vector_value_out40 = 18;
  uint8_t* vector_value_out41 = 0;
  vector_borrow(vector, 20, (void**)&vector_value_out41);
  *(uint32_t*)vector_value_out41 = 1;
  uint8_t* vector_value_out42 = 0;
  vector_borrow(vector2, 20, (void**)&vector_value_out42);
  *(uint16_t*)vector_value_out42 = 21;
  uint8_t* vector_value_out43 = 0;
  vector_borrow(vector, 21, (void**)&vector_value_out43);
  *(uint32_t*)vector_value_out43 = 0;
  uint8_t* vector_value_out44 = 0;
  vector_borrow(vector2, 21, (void**)&vector_value_out44);
  *(uint16_t*)vector_value_out44 = 20;
  uint8_t* vector_value_out45 = 0;
  vector_borrow(vector, 22, (void**)&vector_value_out45);
  *(uint32_t*)vector_value_out45 = 1;
  uint8_t* vector_value_out46 = 0;
  vector_borrow(vector2, 22, (void**)&vector_value_out46);
  *(uint16_t*)vector_value_out46 = 23;
  uint8_t* vector_value_out47 = 0;
  vector_borrow(vector, 23, (void**)&vector_value_out47);
  *(uint32_t*)vector_value_out47 = 0;
  uint8_t* vector_value_out48 = 0;
  vector_borrow(vector2, 23, (void**)&vector_value_out48);
  *(uint16_t*)vector_value_out48 = 22;
  uint8_t* vector_value_out49 = 0;
  vector_borrow(vector, 24, (void**)&vector_value_out49);
  *(uint32_t*)vector_value_out49 = 1;
  uint8_t* vector_value_out50 = 0;
  vector_borrow(vector2, 24, (void**)&vector_value_out50);
  *(uint16_t*)vector_value_out50 = 25;
  uint8_t* vector_value_out51 = 0;
  vector_borrow(vector, 25, (void**)&vector_value_out51);
  *(uint32_t*)vector_value_out51 = 0;
  uint8_t* vector_value_out52 = 0;
  vector_borrow(vector2, 25, (void**)&vector_value_out52);
  *(uint16_t*)vector_value_out52 = 24;
  uint8_t* vector_value_out53 = 0;
  vector_borrow(vector, 26, (void**)&vector_value_out53);
  *(uint32_t*)vector_value_out53 = 1;
  uint8_t* vector_value_out54 = 0;
  vector_borrow(vector2, 26, (void**)&vector_value_out54);
  *(uint16_t*)vector_value_out54 = 27;
  uint8_t* vector_value_out55 = 0;
  vector_borrow(vector, 27, (void**)&vector_value_out55);
  *(uint32_t*)vector_value_out55 = 0;
  uint8_t* vector_value_out56 = 0;
  vector_borrow(vector2, 27, (void**)&vector_value_out56);
  *(uint16_t*)vector_value_out56 = 26;
  uint8_t* vector_value_out57 = 0;
  vector_borrow(vector, 28, (void**)&vector_value_out57);
  *(uint32_t*)vector_value_out57 = 1;
  uint8_t* vector_value_out58 = 0;
  vector_borrow(vector2, 28, (void**)&vector_value_out58);
  *(uint16_t*)vector_value_out58 = 29;
  uint8_t* vector_value_out59 = 0;
  vector_borrow(vector, 29, (void**)&vector_value_out59);
  *(uint32_t*)vector_value_out59 = 0;
  uint8_t* vector_value_out60 = 0;
  vector_borrow(vector2, 29, (void**)&vector_value_out60);
  *(uint16_t*)vector_value_out60 = 28;
  uint8_t* vector_value_out61 = 0;
  vector_borrow(vector, 30, (void**)&vector_value_out61);
  *(uint32_t*)vector_value_out61 = 1;
  uint8_t* vector_value_out62 = 0;
  vector_borrow(vector2, 30, (void**)&vector_value_out62);
  *(uint16_t*)vector_value_out62 = 31;
  uint8_t* vector_value_out63 = 0;
  vector_borrow(vector, 31, (void**)&vector_value_out63);
  *(uint32_t*)vector_value_out63 = 0;
  uint8_t* vector_value_out64 = 0;
  vector_borrow(vector2, 31, (void**)&vector_value_out64);
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
  forwarding_stats_per_route_op.insert({175, PortStats{}});
  forwarding_stats_per_route_op.insert({168, PortStats{}});
  forwarding_stats_per_route_op.insert({167, PortStats{}});
  forwarding_stats_per_route_op.insert({174, PortStats{}});
  forwarding_stats_per_route_op.insert({161, PortStats{}});
  forwarding_stats_per_route_op.insert({156, PortStats{}});
  forwarding_stats_per_route_op.insert({177, PortStats{}});
  forwarding_stats_per_route_op.insert({151, PortStats{}});
  forwarding_stats_per_route_op.insert({150, PortStats{}});
  forwarding_stats_per_route_op.insert({144, PortStats{}});
  node_pkt_counter.insert({177, 0});
  node_pkt_counter.insert({176, 0});
  node_pkt_counter.insert({175, 0});
  node_pkt_counter.insert({174, 0});
  node_pkt_counter.insert({173, 0});
  node_pkt_counter.insert({172, 0});
  node_pkt_counter.insert({171, 0});
  node_pkt_counter.insert({170, 0});
  node_pkt_counter.insert({169, 0});
  node_pkt_counter.insert({168, 0});
  node_pkt_counter.insert({167, 0});
  node_pkt_counter.insert({166, 0});
  node_pkt_counter.insert({165, 0});
  node_pkt_counter.insert({164, 0});
  node_pkt_counter.insert({163, 0});
  node_pkt_counter.insert({162, 0});
  node_pkt_counter.insert({161, 0});
  node_pkt_counter.insert({160, 0});
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
  node_pkt_counter.insert({144, 0});
  node_pkt_counter.insert({145, 0});
  node_pkt_counter.insert({146, 0});
  node_pkt_counter.insert({147, 0});
  node_pkt_counter.insert({148, 0});
  node_pkt_counter.insert({149, 0});
  node_pkt_counter.insert({150, 0});
  node_pkt_counter.insert({151, 0});
  node_pkt_counter.insert({152, 0});
  node_pkt_counter.insert({153, 0});
  node_pkt_counter.insert({154, 0});
  node_pkt_counter.insert({155, 0});
  node_pkt_counter.insert({156, 0});
  node_pkt_counter.insert({157, 0});
  node_pkt_counter.insert({158, 0});
  node_pkt_counter.insert({159, 0});
  return true;
}


int nf_process(uint16_t device, uint8_t *buffer, uint16_t packet_length, time_ns_t now) {
  // Node 131
  inc_path_counter(131);
  tb_expire(tb, now);
  // Node 132
  inc_path_counter(132);
  uint8_t* hdr;
  packet_borrow_next_chunk(buffer, 14, (void**)&hdr);
  // Node 133
  inc_path_counter(133);
  if (((8) == (*(uint16_t*)(uint16_t*)(hdr+12))) & ((20) <= ((uint16_t)((uint32_t)((4294967282LL) + ((uint16_t)(packet_length & 65535))))))) {
    // Node 134
    inc_path_counter(134);
    uint8_t* hdr2;
    packet_borrow_next_chunk(buffer, 20, (void**)&hdr2);
    // Node 135
    inc_path_counter(135);
    uint8_t* vector_value_out65 = 0;
    vector_borrow(vector, (uint16_t)(device & 65535), (void**)&vector_value_out65);
    // Node 136
    inc_path_counter(136);
    // Node 137
    inc_path_counter(137);
    if ((0) == (*(uint32_t*)vector_value_out65)) {
      // Node 138
      inc_path_counter(138);
      uint8_t key[4];
      uint32_t hdr2_slice = *(uint32_t*)(hdr2+16);
      *(uint32_t*)key = hdr2_slice;
      int index;
      int is_tracing = tb_is_tracing(tb, key, &index);
      // Node 139
      inc_path_counter(139);
      if ((0) == (is_tracing)) {
        // Node 140
        inc_path_counter(140);
        int index2;
        int successfuly_tracing = tb_trace(tb, key, packet_length & 65535, now, &index2);
        // Node 141
        inc_path_counter(141);
        if ((0) == (successfuly_tracing)) {
          // Node 142
          inc_path_counter(142);
          packet_return_chunk(buffer, hdr2);
          // Node 143
          inc_path_counter(143);
          packet_return_chunk(buffer, hdr);
          // Node 144
          inc_path_counter(144);
          forwarding_stats_per_route_op[144].inc_drop();
          return DROP;
        } else {
          // Node 145
          inc_path_counter(145);
          uint8_t* vector_value_out66 = 0;
          vector_borrow(vector2, (uint16_t)(device & 65535), (void**)&vector_value_out66);
          // Node 146
          inc_path_counter(146);
          // Node 147
          inc_path_counter(147);
          packet_return_chunk(buffer, hdr2);
          // Node 148
          inc_path_counter(148);
          packet_return_chunk(buffer, hdr);
          // Node 149
          inc_path_counter(149);
          if ((65535) != (*(uint16_t*)vector_value_out66)) {
            // Node 150
            inc_path_counter(150);
            forwarding_stats_per_route_op[150].inc_fwd(*(uint16_t*)vector_value_out66);
            return *(uint16_t*)vector_value_out66;
          } else {
            // Node 151
            inc_path_counter(151);
            forwarding_stats_per_route_op[151].inc_drop();
            return DROP;
          } // (65535) != (*(uint16_t*)vector_value_out66)
        } // (0) == (successfuly_tracing)
      } else {
        // Node 152
        inc_path_counter(152);
        int pass = tb_update_and_check(tb, index, packet_length & 65535, now);
        // Node 153
        inc_path_counter(153);
        if ((0) == (pass)) {
          // Node 154
          inc_path_counter(154);
          packet_return_chunk(buffer, hdr2);
          // Node 155
          inc_path_counter(155);
          packet_return_chunk(buffer, hdr);
          // Node 156
          inc_path_counter(156);
          forwarding_stats_per_route_op[156].inc_drop();
          return DROP;
        } else {
          // Node 157
          inc_path_counter(157);
          int index3;
          int successfuly_tracing2 = tb_trace(tb, key, packet_length & 65535, now, &index3);
          // Node 158
          inc_path_counter(158);
          if ((0) == (successfuly_tracing2)) {
            // Node 159
            inc_path_counter(159);
            packet_return_chunk(buffer, hdr2);
            // Node 160
            inc_path_counter(160);
            packet_return_chunk(buffer, hdr);
            // Node 161
            inc_path_counter(161);
            forwarding_stats_per_route_op[161].inc_drop();
            return DROP;
          } else {
            // Node 162
            inc_path_counter(162);
            uint8_t* vector_value_out67 = 0;
            vector_borrow(vector2, (uint16_t)(device & 65535), (void**)&vector_value_out67);
            // Node 163
            inc_path_counter(163);
            // Node 164
            inc_path_counter(164);
            packet_return_chunk(buffer, hdr2);
            // Node 165
            inc_path_counter(165);
            packet_return_chunk(buffer, hdr);
            // Node 166
            inc_path_counter(166);
            if ((65535) != (*(uint16_t*)vector_value_out67)) {
              // Node 167
              inc_path_counter(167);
              forwarding_stats_per_route_op[167].inc_fwd(*(uint16_t*)vector_value_out67);
              return *(uint16_t*)vector_value_out67;
            } else {
              // Node 168
              inc_path_counter(168);
              forwarding_stats_per_route_op[168].inc_drop();
              return DROP;
            } // (65535) != (*(uint16_t*)vector_value_out67)
          } // (0) == (successfuly_tracing2)
        } // (0) == (pass)
      } // (0) == (is_tracing)
    } else {
      // Node 169
      inc_path_counter(169);
      uint8_t* vector_value_out68 = 0;
      vector_borrow(vector2, (uint16_t)(device & 65535), (void**)&vector_value_out68);
      // Node 170
      inc_path_counter(170);
      // Node 171
      inc_path_counter(171);
      packet_return_chunk(buffer, hdr2);
      // Node 172
      inc_path_counter(172);
      packet_return_chunk(buffer, hdr);
      // Node 173
      inc_path_counter(173);
      if ((65535) != (*(uint16_t*)vector_value_out68)) {
        // Node 174
        inc_path_counter(174);
        forwarding_stats_per_route_op[174].inc_fwd(*(uint16_t*)vector_value_out68);
        return *(uint16_t*)vector_value_out68;
      } else {
        // Node 175
        inc_path_counter(175);
        forwarding_stats_per_route_op[175].inc_drop();
        return DROP;
      } // (65535) != (*(uint16_t*)vector_value_out68)
    } // (0) == (*(uint32_t*)vector_value_out65)
  } else {
    // Node 176
    inc_path_counter(176);
    packet_return_chunk(buffer, hdr);
    // Node 177
    inc_path_counter(177);
    forwarding_stats_per_route_op[177].inc_drop();
    return DROP;
  } // ((8) == (*(uint16_t*)(uint16_t*)(hdr+12))) & ((20) <= ((uint16_t)((uint32_t)((4294967282LL) + ((uint16_t)(packet_length & 65535))))))
}
