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

#include <pcap.h>
#include <stdbool.h>
#include <unistd.h>

#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <vector>

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

#define DROP ((uint16_t)-1)
#define FLOOD ((uint16_t)-2)

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
  std::string nf_name;
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

    rewind(dev);
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
  NF_INFO("Usage: %s [[--warmup] dev0:pcap0] [[--warmup] dev1:pcap1] ...\n",
          argv[0]);
}

void nf_config_print(void) {
  NF_INFO("----- Config -----");
  for (const auto &dev_pcap : config.pcaps) {
    NF_INFO("device: %u, pcap: %s warmup: %d", dev_pcap.device,
            dev_pcap.pcap.filename().c_str(), dev_pcap.warmup);
  }
  NF_INFO("--- ---------- ---");
}

std::string nf_name_from_executable(const char *argv0) {
  std::filesystem::path nf_name = std::string(argv0);
  return nf_name.filename().stem().string();
}

void nf_config_init(int argc, char **argv) {
  if (argc < 2) {
    PARSE_ERROR(argv, "Insufficient arguments.\n");
  }

  config.nf_name = nf_name_from_executable(argv[0]);
  bool incoming_warmup = false;

  // split the arguments into device and pcap pairs joined by a :
  for (int i = 1; i < argc; i++) {
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

  Stats() : stats() {}

  void update(const void *key, uint32_t len) {
    key_t k((uint8_t *)key, len);
    stats[k]++;
  }
};

struct MapStats {
  std::unordered_map<int, Stats> stats_per_map_op;

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
  report["meta"]["total_packets"] = reader.get_total_packets();
  report["meta"]["total_bytes"] = reader.get_total_bytes();
  report["meta"]["avg_pkt_size"] =
      reader.get_total_bytes() / reader.get_total_packets();

  report["map_stats"] = json::array();
  for (const auto &[map_op, stats] : map_stats.stats_per_map_op) {
    json map_op_stats_json;
    map_op_stats_json["node"] = map_op;
    map_op_stats_json["packets_per_flow"] = json::array();
    map_op_stats_json["total_flows"] = stats.stats.size();

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

    map_op_stats_json["total_packets"] = total_packets;
    map_op_stats_json["avg_pkts_per_flow"] = total_packets / stats.stats.size();

    report["map_stats"].push_back(map_op_stats_json);
  }

  std::stringstream report_fname_ss;

  report_fname_ss << config.nf_name;
  for (const auto &dev_pcap : config.pcaps) {
    report_fname_ss << "-dev-" << dev_pcap.device;
    report_fname_ss << "-pcap-" << dev_pcap.pcap.filename().stem().string();
    report_fname_ss << (dev_pcap.warmup ? "-warmup" : "");
  }
  report_fname_ss << ".json";

  std::filesystem::path report_fname = report_fname_ss.str();

  std::ofstream os = std::ofstream(report_fname_ss.str());
  os << report.dump(2);
  os.flush();
  os.close();

  NF_INFO("Generated report %s", report_fname.c_str());
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

struct Map* map;
struct Vector* vector;
struct Vector* vector_1;
struct DoubleChain* dchain;
uint64_t path_profiler_counter[78];

bool nf_init() {

  if (map_allocate(65536u, 16u, &map) == 0) {
    return 0;
  }


  if (vector_allocate(16u, 65536u, &vector) == 0) {
    return 0;
  }


  if (vector_allocate(128u, 65536u, &vector_1) == 0) {
    return 0;
  }


  if (dchain_allocate(65536u, &dchain) == 0) {
    return 0;
  }

  path_profiler_counter[0] = 0;
  path_profiler_counter[1] = 0;
  path_profiler_counter[2] = 0;
  path_profiler_counter[3] = 0;
  path_profiler_counter[4] = 0;
  path_profiler_counter[5] = 0;
  path_profiler_counter[6] = 0;
  path_profiler_counter[7] = 0;
  path_profiler_counter[8] = 0;
  path_profiler_counter[9] = 0;
  path_profiler_counter[10] = 0;
  path_profiler_counter[11] = 0;
  path_profiler_counter[12] = 0;
  path_profiler_counter[13] = 0;
  path_profiler_counter[14] = 0;
  path_profiler_counter[15] = 0;
  path_profiler_counter[16] = 0;
  path_profiler_counter[17] = 0;
  path_profiler_counter[18] = 0;
  path_profiler_counter[19] = 0;
  path_profiler_counter[20] = 0;
  path_profiler_counter[21] = 0;
  path_profiler_counter[22] = 0;
  path_profiler_counter[23] = 0;
  path_profiler_counter[24] = 0;
  path_profiler_counter[25] = 0;
  path_profiler_counter[26] = 0;
  path_profiler_counter[27] = 0;
  path_profiler_counter[28] = 0;
  path_profiler_counter[29] = 0;
  path_profiler_counter[30] = 0;
  path_profiler_counter[31] = 0;
  path_profiler_counter[32] = 0;
  path_profiler_counter[33] = 0;
  path_profiler_counter[34] = 0;
  path_profiler_counter[35] = 0;
  path_profiler_counter[36] = 0;
  path_profiler_counter[37] = 0;
  path_profiler_counter[38] = 0;
  path_profiler_counter[39] = 0;
  path_profiler_counter[40] = 0;
  path_profiler_counter[41] = 0;
  path_profiler_counter[42] = 0;
  path_profiler_counter[43] = 0;
  path_profiler_counter[44] = 0;
  path_profiler_counter[45] = 0;
  path_profiler_counter[46] = 0;
  path_profiler_counter[47] = 0;
  path_profiler_counter[48] = 0;
  path_profiler_counter[49] = 0;
  path_profiler_counter[50] = 0;
  path_profiler_counter[51] = 0;
  path_profiler_counter[52] = 0;
  path_profiler_counter[53] = 0;
  path_profiler_counter[54] = 0;
  path_profiler_counter[55] = 0;
  path_profiler_counter[56] = 0;
  path_profiler_counter[57] = 0;
  path_profiler_counter[58] = 0;
  path_profiler_counter[59] = 0;
  path_profiler_counter[60] = 0;
  path_profiler_counter[61] = 0;
  path_profiler_counter[62] = 0;
  path_profiler_counter[63] = 0;
  path_profiler_counter[64] = 0;
  path_profiler_counter[65] = 0;
  path_profiler_counter[66] = 0;
  path_profiler_counter[67] = 0;
  path_profiler_counter[68] = 0;
  path_profiler_counter[69] = 0;
  path_profiler_counter[70] = 0;
  path_profiler_counter[71] = 0;
  path_profiler_counter[72] = 0;
  path_profiler_counter[73] = 0;
  path_profiler_counter[74] = 0;
  path_profiler_counter[75] = 0;
  path_profiler_counter[76] = 0;
  path_profiler_counter[77] = 0;
  path_profiler_counter_ptr = path_profiler_counter;
  path_profiler_counter_sz = 78u;
  return 1;
}

int nf_process(uint16_t device, uint8_t* packet, uint16_t packet_length, int64_t now) {
  uint16_t dst_device;
  inc_path_counter(0);
  struct rte_ether_hdr* ether_header_0 = (struct rte_ether_hdr*)(packet);
  inc_path_counter(1);

  // 14
  // 21
  // 28
  // 35
  // 42
  // 56
  // 61
  // 68
  // 72
  // 75
  if ((8u == (ether_header_0->ether_type)) & (20ul <= (4294967282u + packet_length))) {
    inc_path_counter(2);
    struct rte_ipv4_hdr* ipv4_header_0 = (struct rte_ipv4_hdr*)(packet + 14u);
    inc_path_counter(3);

    // 14
    // 21
    // 28
    // 35
    // 42
    // 56
    // 61
    // 68
    // 72
    if ((17u == (ipv4_header_0->next_proto_id)) & ((4294967262u + packet_length) >= 8ul)) {
      inc_path_counter(4);
      struct rte_udp_hdr* udp_header_0 = (struct rte_udp_hdr*)(packet + (14u + 20u));
      inc_path_counter(5);

      // 14
      // 21
      // 28
      // 35
      // 42
      // 56
      // 61
      // 68
      if ((40450u == (udp_header_0->dst_port)) & (146ul <= (4294967254u + packet_length))) {
        inc_path_counter(6);
        uint8_t* app_header_0 = (uint8_t*)(packet + ((14u + 20u) + 8u));
        inc_path_counter(7);

        // 14
        // 21
        // 28
        // 35
        // 42
        if (1u != (app_header_0[0ul])) {
          inc_path_counter(8);

          // 14
          // 21
          // 28
          if (0u != (app_header_0[0ul])) {
            inc_path_counter(9);

            // 14
            if (2u != (app_header_0[0ul])) {
              inc_path_counter(10);
              app_header_0[0u] = 1u;
              inc_path_counter(11);
              memcpy(((uint8_t*)(udp_header_0)) + 0ul, (void*)(&udp_header_0->dst_port), 2ul);
              memcpy(((uint8_t*)(udp_header_0)) + 2ul, (void*)(&udp_header_0->src_port), 2ul);
              inc_path_counter(12);
              memcpy(((uint8_t*)(ipv4_header_0)) + 0ul, (void*)(&ipv4_header_0->dst_addr), 4ul);
              memcpy(((uint8_t*)(ipv4_header_0)) + 4ul, (void*)(&ipv4_header_0->src_addr), 4ul);
              inc_path_counter(13);
              memcpy(((uint8_t*)(ether_header_0)) + 0ul, (void*)(&ether_header_0->s_addr.addr_bytes[0ul]), 6ul);
              memcpy(((uint8_t*)(ether_header_0)) + 6ul, (void*)(&ether_header_0->d_addr.addr_bytes[0ul]), 6ul);
              inc_path_counter(14);
              dst_device = 0;
              return dst_device;
            }

            // 21
            // 28
            else {
              inc_path_counter(15);
              uint8_t map_key[16];
              map_key[0u] = app_header_0[1ul];
              map_key[1u] = app_header_0[2ul];
              map_key[2u] = app_header_0[3ul];
              map_key[3u] = app_header_0[4ul];
              map_key[4u] = app_header_0[5ul];
              map_key[5u] = app_header_0[6ul];
              map_key[6u] = app_header_0[7ul];
              map_key[7u] = app_header_0[8ul];
              map_key[8u] = app_header_0[9ul];
              map_key[9u] = app_header_0[10ul];
              map_key[10u] = app_header_0[11ul];
              map_key[11u] = app_header_0[12ul];
              map_key[12u] = app_header_0[13ul];
              map_key[13u] = app_header_0[14ul];
              map_key[14u] = app_header_0[15ul];
              map_key[15u] = app_header_0[16ul];
              int map_value_out;
              map_stats.update(15, map_key, 16);
              int map_has_this_key = map_get(map, map_key, &map_value_out);
              inc_path_counter(16);

              // 21
              if (0u == map_has_this_key) {
                inc_path_counter(17);
                app_header_0[0u] = 1u;
                inc_path_counter(18);
                memcpy(((uint8_t*)(udp_header_0)) + 0ul, (void*)(&udp_header_0->dst_port), 2ul);
                memcpy(((uint8_t*)(udp_header_0)) + 2ul, (void*)(&udp_header_0->src_port), 2ul);
                inc_path_counter(19);
                memcpy(((uint8_t*)(ipv4_header_0)) + 0ul, (void*)(&ipv4_header_0->dst_addr), 4ul);
                memcpy(((uint8_t*)(ipv4_header_0)) + 4ul, (void*)(&ipv4_header_0->src_addr), 4ul);
                inc_path_counter(20);
                memcpy(((uint8_t*)(ether_header_0)) + 0ul, (void*)(&ether_header_0->s_addr.addr_bytes[0ul]), 6ul);
                memcpy(((uint8_t*)(ether_header_0)) + 6ul, (void*)(&ether_header_0->d_addr.addr_bytes[0ul]), 6ul);
                inc_path_counter(21);
                dst_device = 0;
                return dst_device;
              }

              // 28
              else {
                inc_path_counter(22);
                dchain_free_index(dchain, map_value_out);
                inc_path_counter(23);
                uint8_t map_key_1[16];
                map_key_1[0u] = app_header_0[1ul];
                map_key_1[1u] = app_header_0[2ul];
                map_key_1[2u] = app_header_0[3ul];
                map_key_1[3u] = app_header_0[4ul];
                map_key_1[4u] = app_header_0[5ul];
                map_key_1[5u] = app_header_0[6ul];
                map_key_1[6u] = app_header_0[7ul];
                map_key_1[7u] = app_header_0[8ul];
                map_key_1[8u] = app_header_0[9ul];
                map_key_1[9u] = app_header_0[10ul];
                map_key_1[10u] = app_header_0[11ul];
                map_key_1[11u] = app_header_0[12ul];
                map_key_1[12u] = app_header_0[13ul];
                map_key_1[13u] = app_header_0[14ul];
                map_key_1[14u] = app_header_0[15ul];
                map_key_1[15u] = app_header_0[16ul];
                uint8_t trash[16];
                map_stats.update(23, map_key_1, 16);
                map_erase(map, &map_key_1, (void**)(&trash));
                inc_path_counter(24);
                app_header_0[0u] = 0u;
                inc_path_counter(25);
                memcpy(((uint8_t*)(udp_header_0)) + 0ul, (void*)(&udp_header_0->dst_port), 2ul);
                memcpy(((uint8_t*)(udp_header_0)) + 2ul, (void*)(&udp_header_0->src_port), 2ul);
                inc_path_counter(26);
                memcpy(((uint8_t*)(ipv4_header_0)) + 0ul, (void*)(&ipv4_header_0->dst_addr), 4ul);
                memcpy(((uint8_t*)(ipv4_header_0)) + 4ul, (void*)(&ipv4_header_0->src_addr), 4ul);
                inc_path_counter(27);
                memcpy(((uint8_t*)(ether_header_0)) + 0ul, (void*)(&ether_header_0->s_addr.addr_bytes[0ul]), 6ul);
                memcpy(((uint8_t*)(ether_header_0)) + 6ul, (void*)(&ether_header_0->d_addr.addr_bytes[0ul]), 6ul);
                inc_path_counter(28);
                dst_device = 0;
                return dst_device;
              } // !(0u == map_has_this_key)

            } // !(2u != (app_header_0[0ul]))

          }

          // 35
          // 42
          else {
            inc_path_counter(29);
            uint8_t map_key[16];
            map_key[0u] = app_header_0[1ul];
            map_key[1u] = app_header_0[2ul];
            map_key[2u] = app_header_0[3ul];
            map_key[3u] = app_header_0[4ul];
            map_key[4u] = app_header_0[5ul];
            map_key[5u] = app_header_0[6ul];
            map_key[6u] = app_header_0[7ul];
            map_key[7u] = app_header_0[8ul];
            map_key[8u] = app_header_0[9ul];
            map_key[9u] = app_header_0[10ul];
            map_key[10u] = app_header_0[11ul];
            map_key[11u] = app_header_0[12ul];
            map_key[12u] = app_header_0[13ul];
            map_key[13u] = app_header_0[14ul];
            map_key[14u] = app_header_0[15ul];
            map_key[15u] = app_header_0[16ul];
            int map_value_out;
            map_stats.update(29, map_key, 16);
            int map_has_this_key = map_get(map, map_key, &map_value_out);
            inc_path_counter(30);

            // 35
            if (0u == map_has_this_key) {
              inc_path_counter(31);
              app_header_0[0u] = 1u;
              inc_path_counter(32);
              memcpy(((uint8_t*)(udp_header_0)) + 0ul, (void*)(&udp_header_0->dst_port), 2ul);
              memcpy(((uint8_t*)(udp_header_0)) + 2ul, (void*)(&udp_header_0->src_port), 2ul);
              inc_path_counter(33);
              memcpy(((uint8_t*)(ipv4_header_0)) + 0ul, (void*)(&ipv4_header_0->dst_addr), 4ul);
              memcpy(((uint8_t*)(ipv4_header_0)) + 4ul, (void*)(&ipv4_header_0->src_addr), 4ul);
              inc_path_counter(34);
              memcpy(((uint8_t*)(ether_header_0)) + 0ul, (void*)(&ether_header_0->s_addr.addr_bytes[0ul]), 6ul);
              memcpy(((uint8_t*)(ether_header_0)) + 6ul, (void*)(&ether_header_0->d_addr.addr_bytes[0ul]), 6ul);
              inc_path_counter(35);
              dst_device = 0;
              return dst_device;
            }

            // 42
            else {
              inc_path_counter(36);
              uint8_t* vector_value_out = 0u;
              vector_borrow(vector_1, map_value_out, (void**)(&vector_value_out));
              inc_path_counter(37);
              vector_return(vector_1, map_value_out, vector_value_out);
              inc_path_counter(38);
              memcpy(app_header_0 + 0ul, vector_value_out, 128ul);
              app_header_0[0u] = 0u;
              inc_path_counter(39);
              memcpy(((uint8_t*)(udp_header_0)) + 0ul, (void*)(&udp_header_0->dst_port), 2ul);
              memcpy(((uint8_t*)(udp_header_0)) + 2ul, (void*)(&udp_header_0->src_port), 2ul);
              inc_path_counter(40);
              memcpy(((uint8_t*)(ipv4_header_0)) + 0ul, (void*)(&ipv4_header_0->dst_addr), 4ul);
              memcpy(((uint8_t*)(ipv4_header_0)) + 4ul, (void*)(&ipv4_header_0->src_addr), 4ul);
              inc_path_counter(41);
              memcpy(((uint8_t*)(ether_header_0)) + 0ul, (void*)(&ether_header_0->s_addr.addr_bytes[0ul]), 6ul);
              memcpy(((uint8_t*)(ether_header_0)) + 6ul, (void*)(&ether_header_0->d_addr.addr_bytes[0ul]), 6ul);
              inc_path_counter(42);
              dst_device = 0;
              return dst_device;
            } // !(0u == map_has_this_key)

          } // !(0u != (app_header_0[0ul]))

        }

        // 56
        // 61
        // 68
        else {
          inc_path_counter(43);
          uint8_t map_key[16];
          map_key[0u] = app_header_0[1ul];
          map_key[1u] = app_header_0[2ul];
          map_key[2u] = app_header_0[3ul];
          map_key[3u] = app_header_0[4ul];
          map_key[4u] = app_header_0[5ul];
          map_key[5u] = app_header_0[6ul];
          map_key[6u] = app_header_0[7ul];
          map_key[7u] = app_header_0[8ul];
          map_key[8u] = app_header_0[9ul];
          map_key[9u] = app_header_0[10ul];
          map_key[10u] = app_header_0[11ul];
          map_key[11u] = app_header_0[12ul];
          map_key[12u] = app_header_0[13ul];
          map_key[13u] = app_header_0[14ul];
          map_key[14u] = app_header_0[15ul];
          map_key[15u] = app_header_0[16ul];
          int map_value_out;
          map_stats.update(43, map_key, 16);
          int map_has_this_key = map_get(map, map_key, &map_value_out);
          inc_path_counter(44);

          // 56
          // 61
          if (0u == map_has_this_key) {
            inc_path_counter(45);
            int32_t new_index;
            int out_of_space_1 = !dchain_allocate_new_index(dchain, &new_index, now);
            inc_path_counter(46);

            // 56
            if (false == (out_of_space_1)) {
              inc_path_counter(47);
              uint8_t* vector_value_out = 0u;
              vector_borrow(vector, new_index, (void**)(&vector_value_out));
              memcpy(vector_value_out + 0ul, (void*)(&app_header_0[1ul]), 16ul);
              inc_path_counter(48);
              map_stats.update(48, vector_value_out, 16);
              map_put(map, vector_value_out, new_index);
              inc_path_counter(49);
              vector_return(vector, new_index, vector_value_out);
              inc_path_counter(50);
              uint8_t* vector_value_out_1 = 0u;
              vector_borrow(vector_1, new_index, (void**)(&vector_value_out_1));
              memcpy(vector_value_out_1 + 0ul, (void*)(&app_header_0[17ul]), 128ul);
              inc_path_counter(51);
              vector_return(vector_1, new_index, vector_value_out_1);
              inc_path_counter(52);
              app_header_0[0u] = 0u;
              inc_path_counter(53);
              memcpy(((uint8_t*)(udp_header_0)) + 0ul, (void*)(&udp_header_0->dst_port), 2ul);
              memcpy(((uint8_t*)(udp_header_0)) + 2ul, (void*)(&udp_header_0->src_port), 2ul);
              inc_path_counter(54);
              memcpy(((uint8_t*)(ipv4_header_0)) + 0ul, (void*)(&ipv4_header_0->dst_addr), 4ul);
              memcpy(((uint8_t*)(ipv4_header_0)) + 4ul, (void*)(&ipv4_header_0->src_addr), 4ul);
              inc_path_counter(55);
              memcpy(((uint8_t*)(ether_header_0)) + 0ul, (void*)(&ether_header_0->s_addr.addr_bytes[0ul]), 6ul);
              memcpy(((uint8_t*)(ether_header_0)) + 6ul, (void*)(&ether_header_0->d_addr.addr_bytes[0ul]), 6ul);
              inc_path_counter(56);
              dst_device = 0;
              return dst_device;
            }

            // 61
            else {
              inc_path_counter(57);
              app_header_0[0u] = 1u;
              inc_path_counter(58);
              memcpy(((uint8_t*)(udp_header_0)) + 0ul, (void*)(&udp_header_0->dst_port), 2ul);
              memcpy(((uint8_t*)(udp_header_0)) + 2ul, (void*)(&udp_header_0->src_port), 2ul);
              inc_path_counter(59);
              memcpy(((uint8_t*)(ipv4_header_0)) + 0ul, (void*)(&ipv4_header_0->dst_addr), 4ul);
              memcpy(((uint8_t*)(ipv4_header_0)) + 4ul, (void*)(&ipv4_header_0->src_addr), 4ul);
              inc_path_counter(60);
              memcpy(((uint8_t*)(ether_header_0)) + 0ul, (void*)(&ether_header_0->s_addr.addr_bytes[0ul]), 6ul);
              memcpy(((uint8_t*)(ether_header_0)) + 6ul, (void*)(&ether_header_0->d_addr.addr_bytes[0ul]), 6ul);
              inc_path_counter(61);
              dst_device = 0;
              return dst_device;
            } // !(false == (out_of_space_1))

          }

          // 68
          else {
            inc_path_counter(62);
            uint8_t* vector_value_out = 0u;
            vector_borrow(vector_1, map_value_out, (void**)(&vector_value_out));
            memcpy(vector_value_out + 0ul, (void*)(&app_header_0[17ul]), 128ul);
            inc_path_counter(63);
            vector_return(vector_1, map_value_out, vector_value_out);
            inc_path_counter(64);
            app_header_0[0u] = 0u;
            inc_path_counter(65);
            memcpy(((uint8_t*)(udp_header_0)) + 0ul, (void*)(&udp_header_0->dst_port), 2ul);
            memcpy(((uint8_t*)(udp_header_0)) + 2ul, (void*)(&udp_header_0->src_port), 2ul);
            inc_path_counter(66);
            memcpy(((uint8_t*)(ipv4_header_0)) + 0ul, (void*)(&ipv4_header_0->dst_addr), 4ul);
            memcpy(((uint8_t*)(ipv4_header_0)) + 4ul, (void*)(&ipv4_header_0->src_addr), 4ul);
            inc_path_counter(67);
            memcpy(((uint8_t*)(ether_header_0)) + 0ul, (void*)(&ether_header_0->s_addr.addr_bytes[0ul]), 6ul);
            memcpy(((uint8_t*)(ether_header_0)) + 6ul, (void*)(&ether_header_0->d_addr.addr_bytes[0ul]), 6ul);
            inc_path_counter(68);
            dst_device = 0;
            return dst_device;
          } // !(0u == map_has_this_key)

        } // !(1u != (app_header_0[0ul]))

      }

      // 72
      else {
        inc_path_counter(69);
        inc_path_counter(70);
        inc_path_counter(71);
        inc_path_counter(72);
        dst_device = DROP;
        return dst_device;
      } // !((40450u == (udp_header_0->dst_port)) & (146ul <= (4294967254u + packet_length)))

    }

    // 75
    else {
      inc_path_counter(73);
      inc_path_counter(74);
      inc_path_counter(75);
      dst_device = DROP;
      return dst_device;
    } // !((17u == (ipv4_header_0->next_proto_id)) & ((4294967262u + packet_length) >= 8ul))

  }

  // 77
  else {
    inc_path_counter(76);
    inc_path_counter(77);
    dst_device = DROP;
    return dst_device;
  } // !((8u == (ether_header_0->ether_type)) & (20ul <= (4294967282u + packet_length)))

}
