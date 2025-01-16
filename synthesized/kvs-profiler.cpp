#ifdef __cplusplus
extern "C" {
#endif
#include <lib/verified/map.h>
#include <lib/verified/vector.h>
#include <lib/verified/double-chain.h>
#include <lib/verified/cht.h>
#include <lib/verified/cms.h>
#include <lib/verified/token-bucket.h>

#include <lib/verified/hash.h>
#include <lib/verified/expirator.h>
#include <lib/verified/packet-io.h>
#include <lib/verified/tcpudp_hdr.h>
#include <lib/verified/vigor-time.h>
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
#include <stdbool.h>
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
  std::string report_fname;
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

PcapReader warmup_reader;
PcapReader reader;
std::unordered_map<int, MapStats> stats_per_map;
uint64_t *path_profiler_counter_ptr;
uint64_t path_profiler_counter_sz;
time_ns_t elapsed_time;

void inc_path_counter(int i) {
  if (warmup) {
    return;
  }

  path_profiler_counter_ptr[i]++;
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

  report["counters"] = json::object();
  for (size_t i = 0; i < path_profiler_counter_sz; i++) {
    report["counters"][std::to_string(i)] = path_profiler_counter_ptr[i];
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
uint64_t path_profiler_counter[78];


bool nf_init() {
  if (!map_allocate(8192, 12, &map)) {
    return false;
  }
  if (!vector_allocate(12, 8192, &vector)) {
    return false;
  }
  if (!vector_allocate(128, 8192, &vector2)) {
    return false;
  }
  if (!dchain_allocate(8192, &dchain)) {
    return false;
  }
  memset((void*)path_profiler_counter, 0, sizeof(path_profiler_counter));
  path_profiler_counter_ptr = path_profiler_counter;
  path_profiler_counter_sz = 78;
  stats_per_map[1073912440].init(48);
  stats_per_map[1073912440].init(20);
  stats_per_map[1073912440].init(14);
  return true;
}


int nf_process(uint16_t device, uint8_t *buffer, uint16_t packet_length, time_ns_t now) {
  // Node 0
  inc_path_counter(0);
  int freed_flows = expire_items_single_map(dchain, vector, map, (-1000000000LL) + (now));
  // Node 1
  inc_path_counter(1);
  uint8_t* hdr;
  packet_borrow_next_chunk(buffer, 14, (void**)&hdr);
  // Node 2
  inc_path_counter(2);
  if (((8) == (*(uint16_t*)(uint16_t*)(hdr+12))) & ((20) <= ((uint16_t)((uint32_t)((4294967282LL) + ((uint16_t)(packet_length & 65535))))))) {
    // Node 3
    inc_path_counter(3);
    uint8_t* hdr2;
    packet_borrow_next_chunk(buffer, 20, (void**)&hdr2);
    // Node 4
    inc_path_counter(4);
    if (((17) == (*(hdr2+9))) & ((8) <= ((uint32_t)((4294967262LL) + ((uint16_t)(packet_length & 65535)))))) {
      // Node 5
      inc_path_counter(5);
      uint8_t* hdr3;
      packet_borrow_next_chunk(buffer, 8, (void**)&hdr3);
      // Node 6
      inc_path_counter(6);
      if (((40450) == (*(uint16_t*)(uint16_t*)(hdr3+2))) & ((142) <= ((uint16_t)((uint32_t)((4294967254LL) + ((uint16_t)(packet_length & 65535))))))) {
        // Node 7
        inc_path_counter(7);
        uint8_t* hdr4;
        packet_borrow_next_chunk(buffer, 142, (void**)&hdr4);
        // Node 8
        inc_path_counter(8);
        if ((0) != (device & 65535)) {
          // Node 9
          inc_path_counter(9);
          packet_return_chunk(buffer, hdr4);
          // Node 10
          inc_path_counter(10);
          packet_return_chunk(buffer, hdr3);
          // Node 11
          inc_path_counter(11);
          packet_return_chunk(buffer, hdr2);
          // Node 12
          inc_path_counter(12);
          packet_return_chunk(buffer, hdr);
          // Node 13
          inc_path_counter(13);
          return 0;
        } else {
          // Node 14
          inc_path_counter(14);
          uint8_t key[12];
          memcpy((void*)key, (void*)(hdr4+1), 12);
          int value;
          int map_hit = map_get(map, key, &value);
          stats_per_map[1073912440].update(14, key, 12, now);
          // Node 15
          inc_path_counter(15);
          if ((0) == (map_hit)) {
            // Node 16
            inc_path_counter(16);
            if ((1) == (*(hdr4+0))) {
              // Node 17
              inc_path_counter(17);
              int index;
              int out_of_space = !dchain_allocate_new_index(dchain, &index, now);
              // Node 18
              inc_path_counter(18);
              if ((0) == ((uint8_t)((uint32_t)(((uint8_t)((bool)((0) != (out_of_space)))) & ((0) == (freed_flows)))))) {
                // Node 19
                inc_path_counter(19);
                uint8_t* vector_value_out = 0;
                vector_borrow(vector, index, (void**)&vector_value_out);
                // Node 20
                inc_path_counter(20);
                memcpy((void*)vector_value_out, (void*)key, 12);
                map_put(map, vector_value_out, index);
                stats_per_map[1073912440].update(20, vector_value_out, 12, now);
                // Node 21
                inc_path_counter(21);
                // Node 22
                inc_path_counter(22);
                uint8_t* vector_value_out2 = 0;
                vector_borrow(vector2, index, (void**)&vector_value_out2);
                // Node 23
                inc_path_counter(23);
                memcpy((void*)vector_value_out2, (void*)(hdr4+13), 128);
                // Node 24
                inc_path_counter(24);
                packet_return_chunk(buffer, hdr4);
                // Node 25
                inc_path_counter(25);
                hdr3[0] = *(hdr3+2);
                hdr3[1] = *(hdr3+3);
                hdr3[2] = *(hdr3+0);
                hdr3[3] = *(hdr3+1);
                packet_return_chunk(buffer, hdr3);
                // Node 26
                inc_path_counter(26);
                hdr2[12] = *(hdr2+16);
                hdr2[13] = *(hdr2+17);
                hdr2[14] = *(hdr2+18);
                hdr2[15] = *(hdr2+19);
                hdr2[16] = *(hdr2+12);
                hdr2[17] = *(hdr2+13);
                hdr2[18] = *(hdr2+14);
                hdr2[19] = *(hdr2+15);
                packet_return_chunk(buffer, hdr2);
                // Node 27
                inc_path_counter(27);
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
                // Node 28
                inc_path_counter(28);
                return 0;
              } else {
                // Node 29
                inc_path_counter(29);
                packet_return_chunk(buffer, hdr4);
                // Node 30
                inc_path_counter(30);
                packet_return_chunk(buffer, hdr3);
                // Node 31
                inc_path_counter(31);
                packet_return_chunk(buffer, hdr2);
                // Node 32
                inc_path_counter(32);
                packet_return_chunk(buffer, hdr);
                // Node 33
                inc_path_counter(33);
                return 1;
              } // (0) == ((uint8_t)((uint32_t)(((uint8_t)((bool)((0) != (out_of_space)))) & ((0) == (freed_flows)))))
            } else {
              // Node 34
              inc_path_counter(34);
              packet_return_chunk(buffer, hdr4);
              // Node 35
              inc_path_counter(35);
              packet_return_chunk(buffer, hdr3);
              // Node 36
              inc_path_counter(36);
              packet_return_chunk(buffer, hdr2);
              // Node 37
              inc_path_counter(37);
              packet_return_chunk(buffer, hdr);
              // Node 38
              inc_path_counter(38);
              return 1;
            } // (1) == (*(hdr4+0))
          } else {
            // Node 39
            inc_path_counter(39);
            dchain_rejuvenate_index(dchain, value, now);
            // Node 40
            inc_path_counter(40);
            if ((1) != (*(hdr4+0))) {
              // Node 41
              inc_path_counter(41);
              if ((0) != (*(hdr4+0))) {
                // Node 42
                inc_path_counter(42);
                if ((2) != (*(hdr4+0))) {
                  // Node 43
                  inc_path_counter(43);
                  packet_return_chunk(buffer, hdr4);
                  // Node 44
                  inc_path_counter(44);
                  hdr3[0] = *(hdr3+2);
                  hdr3[1] = *(hdr3+3);
                  hdr3[2] = *(hdr3+0);
                  hdr3[3] = *(hdr3+1);
                  packet_return_chunk(buffer, hdr3);
                  // Node 45
                  inc_path_counter(45);
                  hdr2[12] = *(hdr2+16);
                  hdr2[13] = *(hdr2+17);
                  hdr2[14] = *(hdr2+18);
                  hdr2[15] = *(hdr2+19);
                  hdr2[16] = *(hdr2+12);
                  hdr2[17] = *(hdr2+13);
                  hdr2[18] = *(hdr2+14);
                  hdr2[19] = *(hdr2+15);
                  packet_return_chunk(buffer, hdr2);
                  // Node 46
                  inc_path_counter(46);
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
                  // Node 47
                  inc_path_counter(47);
                  return 0;
                } else {
                  // Node 48
                  inc_path_counter(48);
                  void* trash;
                  map_erase(map, key, &trash);
                  stats_per_map[1073912440].update(48, key, 12, now);
                  // Node 49
                  inc_path_counter(49);
                  dchain_free_index(dchain, value);
                  // Node 50
                  inc_path_counter(50);
                  packet_return_chunk(buffer, hdr4);
                  // Node 51
                  inc_path_counter(51);
                  hdr3[0] = *(hdr3+2);
                  hdr3[1] = *(hdr3+3);
                  hdr3[2] = *(hdr3+0);
                  hdr3[3] = *(hdr3+1);
                  packet_return_chunk(buffer, hdr3);
                  // Node 52
                  inc_path_counter(52);
                  hdr2[12] = *(hdr2+16);
                  hdr2[13] = *(hdr2+17);
                  hdr2[14] = *(hdr2+18);
                  hdr2[15] = *(hdr2+19);
                  hdr2[16] = *(hdr2+12);
                  hdr2[17] = *(hdr2+13);
                  hdr2[18] = *(hdr2+14);
                  hdr2[19] = *(hdr2+15);
                  packet_return_chunk(buffer, hdr2);
                  // Node 53
                  inc_path_counter(53);
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
                  // Node 54
                  inc_path_counter(54);
                  return 0;
                } // (2) != (*(hdr4+0))
              } else {
                // Node 55
                inc_path_counter(55);
                uint8_t* vector_value_out3 = 0;
                vector_borrow(vector2, value, (void**)&vector_value_out3);
                // Node 56
                inc_path_counter(56);
                // Node 57
                inc_path_counter(57);
                hdr4[13] = *(vector_value_out3+0);
                hdr4[14] = *(vector_value_out3+1);
                hdr4[15] = *(vector_value_out3+2);
                hdr4[16] = *(vector_value_out3+3);
                hdr4[17] = *(vector_value_out3+4);
                hdr4[18] = *(vector_value_out3+5);
                hdr4[19] = *(vector_value_out3+6);
                hdr4[20] = *(vector_value_out3+7);
                hdr4[21] = *(vector_value_out3+8);
                hdr4[22] = *(vector_value_out3+9);
                hdr4[23] = *(vector_value_out3+10);
                hdr4[24] = *(vector_value_out3+11);
                hdr4[25] = *(vector_value_out3+12);
                hdr4[26] = *(vector_value_out3+13);
                hdr4[27] = *(vector_value_out3+14);
                hdr4[28] = *(vector_value_out3+15);
                hdr4[29] = *(vector_value_out3+16);
                hdr4[30] = *(vector_value_out3+17);
                hdr4[31] = *(vector_value_out3+18);
                hdr4[32] = *(vector_value_out3+19);
                hdr4[33] = *(vector_value_out3+20);
                hdr4[34] = *(vector_value_out3+21);
                hdr4[35] = *(vector_value_out3+22);
                hdr4[36] = *(vector_value_out3+23);
                hdr4[37] = *(vector_value_out3+24);
                hdr4[38] = *(vector_value_out3+25);
                hdr4[39] = *(vector_value_out3+26);
                hdr4[40] = *(vector_value_out3+27);
                hdr4[41] = *(vector_value_out3+28);
                hdr4[42] = *(vector_value_out3+29);
                hdr4[43] = *(vector_value_out3+30);
                hdr4[44] = *(vector_value_out3+31);
                hdr4[45] = *(vector_value_out3+32);
                hdr4[46] = *(vector_value_out3+33);
                hdr4[47] = *(vector_value_out3+34);
                hdr4[48] = *(vector_value_out3+35);
                hdr4[49] = *(vector_value_out3+36);
                hdr4[50] = *(vector_value_out3+37);
                hdr4[51] = *(vector_value_out3+38);
                hdr4[52] = *(vector_value_out3+39);
                hdr4[53] = *(vector_value_out3+40);
                hdr4[54] = *(vector_value_out3+41);
                hdr4[55] = *(vector_value_out3+42);
                hdr4[56] = *(vector_value_out3+43);
                hdr4[57] = *(vector_value_out3+44);
                hdr4[58] = *(vector_value_out3+45);
                hdr4[59] = *(vector_value_out3+46);
                hdr4[60] = *(vector_value_out3+47);
                hdr4[61] = *(vector_value_out3+48);
                hdr4[62] = *(vector_value_out3+49);
                hdr4[63] = *(vector_value_out3+50);
                hdr4[64] = *(vector_value_out3+51);
                hdr4[65] = *(vector_value_out3+52);
                hdr4[66] = *(vector_value_out3+53);
                hdr4[67] = *(vector_value_out3+54);
                hdr4[68] = *(vector_value_out3+55);
                hdr4[69] = *(vector_value_out3+56);
                hdr4[70] = *(vector_value_out3+57);
                hdr4[71] = *(vector_value_out3+58);
                hdr4[72] = *(vector_value_out3+59);
                hdr4[73] = *(vector_value_out3+60);
                hdr4[74] = *(vector_value_out3+61);
                hdr4[75] = *(vector_value_out3+62);
                hdr4[76] = *(vector_value_out3+63);
                hdr4[77] = *(vector_value_out3+64);
                hdr4[78] = *(vector_value_out3+65);
                hdr4[79] = *(vector_value_out3+66);
                hdr4[80] = *(vector_value_out3+67);
                hdr4[81] = *(vector_value_out3+68);
                hdr4[82] = *(vector_value_out3+69);
                hdr4[83] = *(vector_value_out3+70);
                hdr4[84] = *(vector_value_out3+71);
                hdr4[85] = *(vector_value_out3+72);
                hdr4[86] = *(vector_value_out3+73);
                hdr4[87] = *(vector_value_out3+74);
                hdr4[88] = *(vector_value_out3+75);
                hdr4[89] = *(vector_value_out3+76);
                hdr4[90] = *(vector_value_out3+77);
                hdr4[91] = *(vector_value_out3+78);
                hdr4[92] = *(vector_value_out3+79);
                hdr4[93] = *(vector_value_out3+80);
                hdr4[94] = *(vector_value_out3+81);
                hdr4[95] = *(vector_value_out3+82);
                hdr4[96] = *(vector_value_out3+83);
                hdr4[97] = *(vector_value_out3+84);
                hdr4[98] = *(vector_value_out3+85);
                hdr4[99] = *(vector_value_out3+86);
                hdr4[100] = *(vector_value_out3+87);
                hdr4[101] = *(vector_value_out3+88);
                hdr4[102] = *(vector_value_out3+89);
                hdr4[103] = *(vector_value_out3+90);
                hdr4[104] = *(vector_value_out3+91);
                hdr4[105] = *(vector_value_out3+92);
                hdr4[106] = *(vector_value_out3+93);
                hdr4[107] = *(vector_value_out3+94);
                hdr4[108] = *(vector_value_out3+95);
                hdr4[109] = *(vector_value_out3+96);
                hdr4[110] = *(vector_value_out3+97);
                hdr4[111] = *(vector_value_out3+98);
                hdr4[112] = *(vector_value_out3+99);
                hdr4[113] = *(vector_value_out3+100);
                hdr4[114] = *(vector_value_out3+101);
                hdr4[115] = *(vector_value_out3+102);
                hdr4[116] = *(vector_value_out3+103);
                hdr4[117] = *(vector_value_out3+104);
                hdr4[118] = *(vector_value_out3+105);
                hdr4[119] = *(vector_value_out3+106);
                hdr4[120] = *(vector_value_out3+107);
                hdr4[121] = *(vector_value_out3+108);
                hdr4[122] = *(vector_value_out3+109);
                hdr4[123] = *(vector_value_out3+110);
                hdr4[124] = *(vector_value_out3+111);
                hdr4[125] = *(vector_value_out3+112);
                hdr4[126] = *(vector_value_out3+113);
                hdr4[127] = *(vector_value_out3+114);
                hdr4[128] = *(vector_value_out3+115);
                hdr4[129] = *(vector_value_out3+116);
                hdr4[130] = *(vector_value_out3+117);
                hdr4[131] = *(vector_value_out3+118);
                hdr4[132] = *(vector_value_out3+119);
                hdr4[133] = *(vector_value_out3+120);
                hdr4[134] = *(vector_value_out3+121);
                hdr4[135] = *(vector_value_out3+122);
                hdr4[136] = *(vector_value_out3+123);
                hdr4[137] = *(vector_value_out3+124);
                hdr4[138] = *(vector_value_out3+125);
                hdr4[139] = *(vector_value_out3+126);
                hdr4[140] = *(vector_value_out3+127);
                packet_return_chunk(buffer, hdr4);
                // Node 58
                inc_path_counter(58);
                hdr3[0] = *(hdr3+2);
                hdr3[1] = *(hdr3+3);
                hdr3[2] = *(hdr3+0);
                hdr3[3] = *(hdr3+1);
                packet_return_chunk(buffer, hdr3);
                // Node 59
                inc_path_counter(59);
                hdr2[12] = *(hdr2+16);
                hdr2[13] = *(hdr2+17);
                hdr2[14] = *(hdr2+18);
                hdr2[15] = *(hdr2+19);
                hdr2[16] = *(hdr2+12);
                hdr2[17] = *(hdr2+13);
                hdr2[18] = *(hdr2+14);
                hdr2[19] = *(hdr2+15);
                packet_return_chunk(buffer, hdr2);
                // Node 60
                inc_path_counter(60);
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
                // Node 61
                inc_path_counter(61);
                return 0;
              } // (0) != (*(hdr4+0))
            } else {
              // Node 62
              inc_path_counter(62);
              uint8_t* vector_value_out4 = 0;
              vector_borrow(vector2, value, (void**)&vector_value_out4);
              // Node 63
              inc_path_counter(63);
              memcpy((void*)vector_value_out4, (void*)(hdr4+13), 128);
              // Node 64
              inc_path_counter(64);
              packet_return_chunk(buffer, hdr4);
              // Node 65
              inc_path_counter(65);
              hdr3[0] = *(hdr3+2);
              hdr3[1] = *(hdr3+3);
              hdr3[2] = *(hdr3+0);
              hdr3[3] = *(hdr3+1);
              packet_return_chunk(buffer, hdr3);
              // Node 66
              inc_path_counter(66);
              hdr2[12] = *(hdr2+16);
              hdr2[13] = *(hdr2+17);
              hdr2[14] = *(hdr2+18);
              hdr2[15] = *(hdr2+19);
              hdr2[16] = *(hdr2+12);
              hdr2[17] = *(hdr2+13);
              hdr2[18] = *(hdr2+14);
              hdr2[19] = *(hdr2+15);
              packet_return_chunk(buffer, hdr2);
              // Node 67
              inc_path_counter(67);
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
              // Node 68
              inc_path_counter(68);
              return 0;
            } // (1) != (*(hdr4+0))
          } // (0) == (map_hit)
        } // (0) != (device & 65535)
      } else {
        // Node 69
        inc_path_counter(69);
        packet_return_chunk(buffer, hdr3);
        // Node 70
        inc_path_counter(70);
        packet_return_chunk(buffer, hdr2);
        // Node 71
        inc_path_counter(71);
        packet_return_chunk(buffer, hdr);
        // Node 72
        inc_path_counter(72);
        return DROP;
      } // ((40450) == (*(uint16_t*)(uint16_t*)(hdr3+2))) & ((142) <= ((uint16_t)((uint32_t)((4294967254LL) + ((uint16_t)(packet_length & 65535))))))
    } else {
      // Node 73
      inc_path_counter(73);
      packet_return_chunk(buffer, hdr2);
      // Node 74
      inc_path_counter(74);
      packet_return_chunk(buffer, hdr);
      // Node 75
      inc_path_counter(75);
      return DROP;
    } // ((17) == (*(hdr2+9))) & ((8) <= ((uint32_t)((4294967262LL) + ((uint16_t)(packet_length & 65535)))))
  } else {
    // Node 76
    inc_path_counter(76);
    packet_return_chunk(buffer, hdr);
    // Node 77
    inc_path_counter(77);
    return DROP;
  } // ((8) == (*(uint16_t*)(uint16_t*)(hdr+12))) & ((20) <= ((uint16_t)((uint32_t)((4294967282LL) + ((uint16_t)(packet_length & 65535))))))
}
