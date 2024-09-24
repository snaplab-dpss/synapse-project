#ifdef __cplusplus
extern "C" {
#endif
#include <lib/verified/cht.h>
#include <lib/verified/double-chain.h>
#include <lib/verified/map.h>
#include <lib/verified/vector.h>
#include <lib/unverified/sketch.h>
#include <lib/unverified/hash.h>
#include <lib/unverified/expirator.h>

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

#define NF_INFO(text, ...)                                                     \
  printf(text "\n", ##__VA_ARGS__);                                            \
  fflush(stdout);

#ifdef ENABLE_LOG
#define NF_DEBUG(text, ...)                                                    \
  fprintf(stderr, "DEBUG: " text "\n", ##__VA_ARGS__);                         \
  fflush(stderr);
#else // ENABLE_LOG
#define NF_DEBUG(...)
#endif // ENABLE_LOG

#define DROP ((uint16_t) - 1)
#define FLOOD ((uint16_t) - 2)

#define MIN_PKT_SIZE 64   // With CRC
#define MAX_PKT_SIZE 1518 // With CRC

#define DEFAULT_SRC_MAC "90:e2:ba:8e:4f:6c"
#define DEFAULT_DST_MAC "90:e2:ba:8e:4f:6d"

#define PARSE_ERROR(argv, format, ...)                                         \
  nf_config_usage(argv);                                                       \
  fprintf(stderr, format, ##__VA_ARGS__);                                      \
  exit(EXIT_FAILURE);

#define PARSER_ASSERT(cond, fmt, ...)                                          \
  if (!(cond))                                                                 \
    rte_exit(EXIT_FAILURE, fmt, ##__VA_ARGS__);

bool nf_init(void);
int nf_process(uint16_t device, uint8_t *buffer, uint16_t packet_length,
               time_ns_t now);

uintmax_t nf_util_parse_int(const char *str, const char *name, int base,
                            char next) {
  char *temp;
  intmax_t result = strtoimax(str, &temp, base);

  // There's also a weird failure case with overflows, but let's not care
  if (temp == str || *temp != next) {
    rte_exit(EXIT_FAILURE, "Error while parsing '%s': %s\n", name, str);
  }

  return result;
}

bool nf_parse_etheraddr(const char *str, struct rte_ether_addr *addr) {
  return sscanf(str, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX",
                addr->addr_bytes + 0, addr->addr_bytes + 1,
                addr->addr_bytes + 2, addr->addr_bytes + 3,
                addr->addr_bytes + 4, addr->addr_bytes + 5) == 6;
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
  const u_char *data;
  const struct pcap_pkthdr *header;
};

class PcapReader {
private:
  std::unordered_map<uint16_t, pcap_t *> pcaps;
  std::unordered_map<uint16_t, bool> assume_ip;
  std::unordered_map<uint16_t, long> pcaps_start;
  std::unordered_map<uint16_t, pkt_t> pending_packets_per_dev;

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
    total_packets = 0;
    total_bytes = 0;
    processed_packets = 0;
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
        pending_packets_per_dev[dev_pcap.device] = pkt;
      }
    }
  }

  bool get_next_packet(uint16_t &dev, pkt_t &pkt) {
    bool set = false;

    for (const auto &dev_pkt : pending_packets_per_dev) {
      if (!set || dev_pkt.second.ts < pkt.ts) {
        dev = dev_pkt.first;
        pkt = dev_pkt.second;
        set = true;
      }
    }

    if (set) {
      pkt_t new_pkt;
      if (read(dev, new_pkt)) {
        pending_packets_per_dev[dev] = new_pkt;
      } else {
        pending_packets_per_dev.erase(dev);
      }
    }

    update_and_show_progress();

    return set;
  }

private:
  bool read(uint16_t dev, pkt_t &pkt) {
    pcap_t *pd = pcaps[dev];

    const u_char *data;
    struct pcap_pkthdr *hdr;

    if (pcap_next_ex(pd, &hdr, &data) != 1) {
      rewind(dev);
      return false;
    }

    u_char *pkt_data = pkt.data;

    if (assume_ip[dev]) {
      struct rte_ether_hdr *eth_hdr = (struct rte_ether_hdr *)pkt_data;
      nf_parse_etheraddr(DEFAULT_DST_MAC, &eth_hdr->d_addr);
      nf_parse_etheraddr(DEFAULT_SRC_MAC, &eth_hdr->s_addr);
      eth_hdr->ether_type = rte_bswap16(RTE_ETHER_TYPE_IPV4);
      pkt_data += sizeof(struct rte_ether_hdr);
    }

    memcpy(pkt_data, data, hdr->len);
    pkt.len = hdr->len;
    pkt.ts = hdr->ts.tv_sec * 1e9 + hdr->ts.tv_usec * 1e3;

    return true;
  }

  // WARNING: this does not work on windows!
  // https://winpcap-users.winpcap.narkive.com/scCKD3x2/packet-random-access-using-file-seek
  void rewind(uint16_t dev) {
    pcap_t *pd = pcaps[dev];
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

      uint16_t len =
          rte_bswap16(ip_hdr->total_length) + sizeof(struct rte_ether_hdr);

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

void nf_log_pkt(time_ns_t time, uint16_t device, uint8_t *packet,
                uint16_t packet_length) {
  struct rte_ether_hdr *rte_ether_header = (struct rte_ether_hdr *)(packet);
  struct rte_ipv4_hdr *rte_ipv4_header =
      (struct rte_ipv4_hdr *)(packet + sizeof(struct rte_ether_hdr));
  struct tcpudp_hdr *tcpudp_header =
      (struct tcpudp_hdr *)(packet + sizeof(struct rte_ether_hdr) +
                            sizeof(struct rte_ipv4_hdr));

  NF_DEBUG("[%lu:%u] %u.%u.%u.%u:%u -> %u.%u.%u.%u:%u", time, device,
           (rte_ipv4_header->src_addr >> 0) & 0xff,
           (rte_ipv4_header->src_addr >> 8) & 0xff,
           (rte_ipv4_header->src_addr >> 16) & 0xff,
           (rte_ipv4_header->src_addr >> 24) & 0xff,
           rte_bswap16(tcpudp_header->src_port),
           (rte_ipv4_header->dst_addr >> 0) & 0xff,
           (rte_ipv4_header->dst_addr >> 8) & 0xff,
           (rte_ipv4_header->dst_addr >> 16) & 0xff,
           (rte_ipv4_header->dst_addr >> 24) & 0xff,
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
    NF_INFO("device: %u | pcap: %s | warmup: %s", dev_pcap.device,
            dev_pcap.pcap.filename().c_str(), dev_pcap.warmup ? "yes" : "no");
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
    char *pcap_str = strtok(NULL, ":");

    if (!device_str || !pcap_str) {
      nf_config_usage(argv);
      PARSE_ERROR(argv, "Invalid argument format: %s\n", arg);
    }

    dev_pcap_t dev_pcap;
    dev_pcap.device = nf_util_parse_int(device_str, "device", 10, '\0');
    dev_pcap.pcap = pcap_str;
    dev_pcap.warmup = incoming_warmup;

    config.pcaps.push_back(dev_pcap);

    incoming_warmup = false;
  }

  nf_config_print();
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

    bool operator==(const key_t &other) const {
      return len == other.len && memcmp(data, other.data, len) == 0;
    }

    ~key_t() { delete[] data; }
  };

  struct KeyHasher {
    std::size_t operator()(const key_t &key) const {
      return hash_obj((void *)key.data, key.len);
    }
  };

  std::unordered_map<key_t, uint64_t, KeyHasher> stats;
  std::unordered_map<uint32_t, std::unordered_set<uint32_t>> mask_to_crc32;

  Stats() : stats() {
    uint32_t mask = 0;
    while (1) {
      mask = (mask << 1) | 1;
      mask_to_crc32[mask] = {};
      if (mask == 0xffffffff) {
        break;
      }
    }
  }

  void update(const void *key, uint32_t len) {
    key_t k((uint8_t *)key, len);
    stats[k]++;

    uint32_t crc32 = rte_hash_crc(k.data, k.len, 0xffffffff);
    for (auto &[mask, hashes] : mask_to_crc32) {
      hashes.insert(crc32 & mask);
    }
  }
};

struct MapStats {
  std::unordered_map<int, Stats> stats_per_map_op;

  void init(int op) { stats_per_map_op[op] = Stats(); }

  void update(int op, const void *key, uint32_t len) {
    stats_per_map_op[op].update(key, len);
  }
};

PcapReader warmup_reader;
PcapReader reader;
MapStats map_stats;
uint64_t *path_profiler_counter_ptr;
uint64_t path_profiler_counter_sz;
time_ns_t elapsed_time;
bool warmup;

void inc_path_counter(int i) {
  if (warmup) {
    return;
  }

  path_profiler_counter_ptr[i]++;
}

void generate_report() {
  json report;

  report["config"] = json();
  report["config"]["pcaps"] = json::array();
  for (const auto &dev_pcap : config.pcaps) {
    json dev_pcap_elem;
    dev_pcap_elem["device"] = dev_pcap.device;
    dev_pcap_elem["pcap"] = dev_pcap.pcap.filename().stem().string();
    dev_pcap_elem["warmup"] = dev_pcap.warmup;
    report["config"]["pcaps"].push_back(dev_pcap_elem);
  }

  report["counters"] = json();
  for (unsigned i = 0; i < path_profiler_counter_sz; i++) {
    report["counters"][std::to_string(i)] = path_profiler_counter_ptr[i];
  }

  report["meta"] = json();
  report["meta"]["elapsed"] = elapsed_time;
  report["meta"]["packets"] = reader.get_total_packets();
  report["meta"]["bytes"] = reader.get_total_bytes();
  report["meta"]["avg_pkt_size"] =
      reader.get_total_bytes() / reader.get_total_packets();

  report["map_stats"] = json::array();
  for (const auto &[map_op, stats] : map_stats.stats_per_map_op) {
    json map_op_stats_json;
    map_op_stats_json["node"] = map_op;
    map_op_stats_json["packets_per_flow"] = json::array();
    map_op_stats_json["flows"] = stats.stats.size();

    map_op_stats_json["crc32_hashes_per_mask"] = json();
    for (const auto &[mask, crc32_hashes] : stats.mask_to_crc32) {
      map_op_stats_json["crc32_hashes_per_mask"][std::to_string(mask)] =
          crc32_hashes.size();
    }

    uint64_t total_packets = 0;
    std::vector<uint64_t> packets_per_flow;
    for (const auto &map_key_stats : stats.stats) {
      packets_per_flow.push_back(map_key_stats.second);
      total_packets += map_key_stats.second;
    }

    std::sort(packets_per_flow.begin(), packets_per_flow.end(),
              std::greater<>());

    for (uint64_t packets : packets_per_flow) {
      map_op_stats_json["packets_per_flow"].push_back(packets);
    }

    map_op_stats_json["packets"] = total_packets;
    map_op_stats_json["avg_pkts_per_flow"] =
        stats.stats.empty() ? 0 : total_packets / stats.stats.size();

    report["map_stats"].push_back(map_op_stats_json);
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
  time_ns_t last_time = 0;

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
uint64_t path_profiler_counter[47];


bool nf_init() {
  if (!map_allocate(65536, 13, &map)) {
    return false;
  }
  if (!vector_allocate(13, 65536, &vector)) {
    return false;
  }
  if (!vector_allocate(4, 65536, &vector2)) {
    return false;
  }
  if (!dchain_allocate(65536, &dchain)) {
    return false;
  }
  memset((void*)path_profiler_counter, 0, sizeof(path_profiler_counter));
  path_profiler_counter_ptr = path_profiler_counter;
  path_profiler_counter_sz = 47;
  map_stats.init(25);
  map_stats.init(20);
  map_stats.init(7);
  return true;
}


int nf_process(uint16_t device, uint8_t *buffer, uint16_t packet_length, time_ns_t now) {
  // Node 0
  inc_path_counter(0);
  int freed_flows = expire_items_single_map(dchain, vector, map, (-1000000000) + (now));
  // Node 1
  inc_path_counter(1);
  uint8_t* hdr;
  packet_borrow_next_chunk(buffer, 14, (void**)&hdr);
  // Node 2
  inc_path_counter(2);
  if (((8) == (*(uint16_t*)(hdr+12))) & ((20) <= ((uint16_t)((uint32_t)((-14) + ((uint16_t)(packet_length & 65535))))))) {
    // Node 3
    inc_path_counter(3);
    uint8_t* hdr2;
    packet_borrow_next_chunk(buffer, 20, (void**)&hdr2);
    // Node 4
    inc_path_counter(4);
    if ((((6) == (*(hdr2+9))) | ((17) == (*(hdr2+9)))) & (((uint32_t)((-34) + ((uint16_t)(packet_length & 65535)))) >= (4))) {
      // Node 5
      inc_path_counter(5);
      uint8_t* hdr3;
      packet_borrow_next_chunk(buffer, 4, (void**)&hdr3);
      // Node 6
      inc_path_counter(6);
      if ((0) != (device & 65535)) {
        // Node 7
        inc_path_counter(7);
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
        map_stats.update(7, key, 13);
        // Node 8
        inc_path_counter(8);
        if ((0) == (map_hit)) {
          // Node 9
          inc_path_counter(9);
          packet_return_chunk(buffer, hdr3);
          // Node 10
          inc_path_counter(10);
          packet_return_chunk(buffer, hdr2);
          // Node 11
          inc_path_counter(11);
          packet_return_chunk(buffer, hdr);
          // Node 12
          inc_path_counter(12);
          return DROP;
        } else {
          // Node 13
          inc_path_counter(13);
          uint8_t* vector_value_out = 0;
          vector_borrow(vector2, value, (void**)&vector_value_out);
          // Node 14
          inc_path_counter(14);
          // Node 15
          inc_path_counter(15);
          dchain_rejuvenate_index(dchain, value, now);
          // Node 16
          inc_path_counter(16);
          packet_return_chunk(buffer, hdr3);
          // Node 17
          inc_path_counter(17);
          packet_return_chunk(buffer, hdr2);
          // Node 18
          inc_path_counter(18);
          hdr[0] = 1;
          hdr[1] = 35;
          hdr[2] = 69;
          hdr[3] = 103;
          hdr[4] = 137;
          hdr[5] = 0;
          hdr[6] = 0;
          hdr[7] = 0;
          hdr[8] = 0;
          hdr[9] = 0;
          hdr[10] = 0;
          hdr[11] = 0;
          packet_return_chunk(buffer, hdr);
          // Node 19
          inc_path_counter(19);
          return 0;
        } // (0) == (map_hit)
      } else {
        // Node 20
        inc_path_counter(20);
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
        map_stats.update(20, key2, 13);
        // Node 21
        inc_path_counter(21);
        if ((0) == (map_hit2)) {
          // Node 22
          inc_path_counter(22);
          int index;
          int out_of_space = !dchain_allocate_new_index(dchain, &index, now);
          // Node 23
          inc_path_counter(23);
          if ((0) == ((uint8_t)((uint32_t)(((uint8_t)((bool)((0) != (out_of_space)))) & ((0) == (freed_flows)))))) {
            // Node 24
            inc_path_counter(24);
            uint8_t* vector_value_out2 = 0;
            vector_borrow(vector, index, (void**)&vector_value_out2);
            // Node 25
            inc_path_counter(25);
            memcpy((void*)vector_value_out2, (void*)key2, 13);
            map_put(map, vector_value_out2, index);
            map_stats.update(25, vector_value_out2, 13);
            // Node 26
            inc_path_counter(26);
            memcpy((void*)vector_value_out2, (void*)key2, 13);
            // Node 27
            inc_path_counter(27);
            uint8_t* vector_value_out3 = 0;
            vector_borrow(vector2, index, (void**)&vector_value_out3);
            // Node 28
            inc_path_counter(28);
            vector_value_out3[0] = 0;
            vector_value_out3[1] = 0;
            vector_value_out3[2] = 0;
            vector_value_out3[3] = 0;
            // Node 29
            inc_path_counter(29);
            packet_return_chunk(buffer, hdr3);
            // Node 30
            inc_path_counter(30);
            packet_return_chunk(buffer, hdr2);
            // Node 31
            inc_path_counter(31);
            hdr[0] = 1;
            hdr[1] = 35;
            hdr[2] = 69;
            hdr[3] = 103;
            hdr[4] = 137;
            hdr[5] = 1;
            hdr[6] = 0;
            hdr[7] = 0;
            hdr[8] = 0;
            hdr[9] = 0;
            hdr[10] = 0;
            hdr[11] = 0;
            packet_return_chunk(buffer, hdr);
            // Node 32
            inc_path_counter(32);
            return 1;
          } else {
            // Node 33
            inc_path_counter(33);
            packet_return_chunk(buffer, hdr3);
            // Node 34
            inc_path_counter(34);
            packet_return_chunk(buffer, hdr2);
            // Node 35
            inc_path_counter(35);
            hdr[0] = 1;
            hdr[1] = 35;
            hdr[2] = 69;
            hdr[3] = 103;
            hdr[4] = 137;
            hdr[5] = 1;
            hdr[6] = 0;
            hdr[7] = 0;
            hdr[8] = 0;
            hdr[9] = 0;
            hdr[10] = 0;
            hdr[11] = 0;
            packet_return_chunk(buffer, hdr);
            // Node 36
            inc_path_counter(36);
            return 1;
          } // (0) == ((uint8_t)((uint32_t)(((uint8_t)((bool)((0) != (out_of_space)))) & ((0) == (freed_flows)))))
        } else {
          // Node 37
          inc_path_counter(37);
          dchain_rejuvenate_index(dchain, value2, now);
          // Node 38
          inc_path_counter(38);
          packet_return_chunk(buffer, hdr3);
          // Node 39
          inc_path_counter(39);
          packet_return_chunk(buffer, hdr2);
          // Node 40
          inc_path_counter(40);
          hdr[0] = 1;
          hdr[1] = 35;
          hdr[2] = 69;
          hdr[3] = 103;
          hdr[4] = 137;
          hdr[5] = 1;
          hdr[6] = 0;
          hdr[7] = 0;
          hdr[8] = 0;
          hdr[9] = 0;
          hdr[10] = 0;
          hdr[11] = 0;
          packet_return_chunk(buffer, hdr);
          // Node 41
          inc_path_counter(41);
          return 1;
        } // (0) == (map_hit2)
      } // (0) != (device & 65535)
    } else {
      // Node 42
      inc_path_counter(42);
      packet_return_chunk(buffer, hdr2);
      // Node 43
      inc_path_counter(43);
      packet_return_chunk(buffer, hdr);
      // Node 44
      inc_path_counter(44);
      return DROP;
    } // (((6) == (*(hdr2+9))) | ((17) == (*(hdr2+9)))) & (((uint32_t)((-34) + ((uint16_t)(packet_length & 65535)))) >= (4))
  } else {
    // Node 45
    inc_path_counter(45);
    packet_return_chunk(buffer, hdr);
    // Node 46
    inc_path_counter(46);
    return DROP;
  } // ((8) == (*(uint16_t*)(hdr+12))) & ((20) <= ((uint16_t)((uint32_t)((-14) + ((uint16_t)(packet_length & 65535))))))
}
