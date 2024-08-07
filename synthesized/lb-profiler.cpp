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
    for (const auto &map_key_stats : stats.stats) {
      map_op_stats_json["packets_per_flow"].push_back(map_key_stats.second);
      total_packets += map_key_stats.second;
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
  std::ofstream(report_fname) << report.dump(2);

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
struct DoubleChain* dchain;
struct Vector* vector_1;
struct Map* map_1;
struct Vector* vector_2;
struct Vector* vector_3;
struct DoubleChain* dchain_1;
struct Vector* vector_4;
uint64_t path_profiler_counter[132];

bool nf_init() {

  if (map_allocate(65536u, 13u, &map) == 0) {
    return 0;
  }


  if (vector_allocate(13u, 65536u, &vector) == 0) {
    return 0;
  }


  if (dchain_allocate(65536u, &dchain) == 0) {
    return 0;
  }


  if (vector_allocate(4u, 65536u, &vector_1) == 0) {
    return 0;
  }


  if (map_allocate(32u, 4u, &map_1) == 0) {
    return 0;
  }


  if (vector_allocate(4u, 32u, &vector_2) == 0) {
    return 0;
  }


  if (vector_allocate(12u, 32u, &vector_3) == 0) {
    return 0;
  }


  if (dchain_allocate(32u, &dchain_1) == 0) {
    return 0;
  }


  if (vector_allocate(4u, 3104u, &vector_4) == 0) {
    return 0;
  }


  if (cht_fill_cht(vector_4, 97u, 32u) == 0) {
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
  path_profiler_counter[78] = 0;
  path_profiler_counter[79] = 0;
  path_profiler_counter[80] = 0;
  path_profiler_counter[81] = 0;
  path_profiler_counter[82] = 0;
  path_profiler_counter[83] = 0;
  path_profiler_counter[84] = 0;
  path_profiler_counter[85] = 0;
  path_profiler_counter[86] = 0;
  path_profiler_counter[87] = 0;
  path_profiler_counter[88] = 0;
  path_profiler_counter[89] = 0;
  path_profiler_counter[90] = 0;
  path_profiler_counter[91] = 0;
  path_profiler_counter[92] = 0;
  path_profiler_counter[93] = 0;
  path_profiler_counter[94] = 0;
  path_profiler_counter[95] = 0;
  path_profiler_counter[96] = 0;
  path_profiler_counter[97] = 0;
  path_profiler_counter[98] = 0;
  path_profiler_counter[99] = 0;
  path_profiler_counter[100] = 0;
  path_profiler_counter[101] = 0;
  path_profiler_counter[102] = 0;
  path_profiler_counter[103] = 0;
  path_profiler_counter[104] = 0;
  path_profiler_counter[105] = 0;
  path_profiler_counter[106] = 0;
  path_profiler_counter[107] = 0;
  path_profiler_counter[108] = 0;
  path_profiler_counter[109] = 0;
  path_profiler_counter[110] = 0;
  path_profiler_counter[111] = 0;
  path_profiler_counter[112] = 0;
  path_profiler_counter[113] = 0;
  path_profiler_counter[114] = 0;
  path_profiler_counter[115] = 0;
  path_profiler_counter[116] = 0;
  path_profiler_counter[117] = 0;
  path_profiler_counter[118] = 0;
  path_profiler_counter[119] = 0;
  path_profiler_counter[120] = 0;
  path_profiler_counter[121] = 0;
  path_profiler_counter[122] = 0;
  path_profiler_counter[123] = 0;
  path_profiler_counter[124] = 0;
  path_profiler_counter[125] = 0;
  path_profiler_counter[126] = 0;
  path_profiler_counter[127] = 0;
  path_profiler_counter[128] = 0;
  path_profiler_counter[129] = 0;
  path_profiler_counter[130] = 0;
  path_profiler_counter[131] = 0;
  path_profiler_counter_ptr = path_profiler_counter;
  path_profiler_counter_sz = 132u;
  return 1;
}

int nf_process(uint16_t device, uint8_t* packet, uint16_t packet_length, int64_t now) {
  uint16_t dst_device;
  inc_path_counter(0);
  int number_of_freed_flows = expire_items_single_map(dchain, vector, map, now - 1000000000ul);
  inc_path_counter(1);
  int number_of_freed_flows_1 = expire_items_single_map(dchain_1, vector_2, map_1, now - 100000000000ul);
  inc_path_counter(2);
  struct rte_ether_hdr* ether_header_0 = (struct rte_ether_hdr*)(packet);
  inc_path_counter(3);

  // 17
  // 32
  // 33
  // 41
  // 42
  // 55
  // 56
  // 67
  // 81
  // 82
  // 95
  // 99
  // 104
  // 117
  // 121
  // 126
  // 129
  if ((8u == (ether_header_0->ether_type)) & (20ul <= (4294967282u + packet_length))) {
    inc_path_counter(4);
    struct rte_ipv4_hdr* ipv4_header_0 = (struct rte_ipv4_hdr*)(packet + 14u);
    inc_path_counter(5);

    // 17
    // 32
    // 33
    // 41
    // 42
    // 55
    // 56
    // 67
    // 81
    // 82
    // 95
    // 99
    // 104
    // 117
    // 121
    // 126
    if (((6u == (ipv4_header_0->next_proto_id)) | (17u == (ipv4_header_0->next_proto_id))) & ((4294967262u + packet_length) >= 4ul)) {
      inc_path_counter(6);
      struct tcpudp_hdr* tcpudp_header_0 = (struct tcpudp_hdr*)(packet + (14u + 20u));
      inc_path_counter(7);

      // 17
      // 32
      // 33
      // 41
      // 42
      // 55
      // 56
      // 67
      // 81
      // 82
      // 95
      // 99
      // 104
      if (0u != device) {
        inc_path_counter(8);

        // 17
        // 32
        // 33
        // 41
        // 42
        // 55
        // 56
        // 67
        // 81
        // 82
        if (1u != device) {
          inc_path_counter(9);
          uint8_t map_key[13];
          map_key[0u] = ipv4_header_0->src_addr & 0xff;
          map_key[1u] = (ipv4_header_0->src_addr >> 8) & 0xff;
          map_key[2u] = (ipv4_header_0->src_addr >> 16) & 0xff;
          map_key[3u] = (ipv4_header_0->src_addr >> 24) & 0xff;
          map_key[4u] = ipv4_header_0->dst_addr & 0xff;
          map_key[5u] = (ipv4_header_0->dst_addr >> 8) & 0xff;
          map_key[6u] = (ipv4_header_0->dst_addr >> 16) & 0xff;
          map_key[7u] = (ipv4_header_0->dst_addr >> 24) & 0xff;
          map_key[8u] = tcpudp_header_0->src_port & 0xff;
          map_key[9u] = (tcpudp_header_0->src_port >> 8) & 0xff;
          map_key[10u] = tcpudp_header_0->dst_port & 0xff;
          map_key[11u] = (tcpudp_header_0->dst_port >> 8) & 0xff;
          map_key[12u] = ipv4_header_0->next_proto_id;
          int map_value_out;
          map_stats.update(9, map_key, 13);
          int map_has_this_key = map_get(map, map_key, &map_value_out);
          inc_path_counter(10);

          // 17
          // 32
          // 33
          // 41
          // 42
          if (0u == map_has_this_key) {
            inc_path_counter(11);
            int hash = hash_obj(map_key, 13u);
            inc_path_counter(12);
            int32_t chosen_backend = 0u;
            int32_t prefered_backend_found = cht_find_preferred_available_backend(hash, vector_4, dchain_1, 97u, 32u, &chosen_backend);
            inc_path_counter(13);

            // 17
            if (0u == prefered_backend_found) {
              inc_path_counter(14);
              inc_path_counter(15);
              inc_path_counter(16);
              inc_path_counter(17);
              dst_device = 2;
              return dst_device;
            }

            // 32
            // 33
            // 41
            // 42
            else {
              inc_path_counter(18);
              int32_t new_index;
              int out_of_space_2 = !dchain_allocate_new_index(dchain, &new_index, now);
              inc_path_counter(19);

              // 32
              // 33
              if (false == ((out_of_space_2) & (0u == number_of_freed_flows))) {
                inc_path_counter(20);
                uint8_t* vector_value_out = 0u;
                vector_borrow(vector, new_index, (void**)(&vector_value_out));
                memcpy(vector_value_out + 0ul, (void*)(&ipv4_header_0->src_addr), 8ul);
                memcpy(vector_value_out + 8ul, tcpudp_header_0, 4ul);
                memcpy(vector_value_out + 12ul, (void*)(&ipv4_header_0->next_proto_id), 1ul);
                inc_path_counter(21);
                uint8_t* vector_value_out_1 = 0u;
                vector_borrow(vector_1, new_index, (void**)(&vector_value_out_1));
                memcpy(vector_value_out_1 + 0ul, (void*)(&chosen_backend), 4ul);
                inc_path_counter(22);
                vector_return(vector_1, new_index, vector_value_out_1);
                inc_path_counter(23);
                map_stats.update(23, vector_value_out, 13);
                map_put(map, vector_value_out, new_index);
                inc_path_counter(24);
                vector_return(vector, new_index, vector_value_out);
                inc_path_counter(25);
                uint8_t* vector_value_out_2 = 0u;
                vector_borrow(vector_3, chosen_backend, (void**)(&vector_value_out_2));
                inc_path_counter(26);
                vector_return(vector_3, chosen_backend, vector_value_out_2);
                inc_path_counter(27);
                int checksum = rte_ipv4_udptcp_cksum(ipv4_header_0, tcpudp_header_0);
                inc_path_counter(28);
                inc_path_counter(29);
                memcpy(((uint8_t*)(ipv4_header_0)) + 0ul, (void*)(&checksum), 2ul);
                memcpy(((uint8_t*)(ipv4_header_0)) + 2ul, (void*)(&ipv4_header_0->src_addr), 4ul);
                memcpy(((uint8_t*)(ipv4_header_0)) + 6ul, (void*)(&vector_value_out_2[8ul]), 4ul);
                inc_path_counter(30);
                memcpy(((uint8_t*)(ether_header_0)) + 0ul, (void*)(&vector_value_out_2[2ul]), 6ul);
                *(uint8_t*)(ether_header_0) = 171u;
                *(uint8_t*)(ether_header_0) = 171u;
                *(uint8_t*)(ether_header_0) = 171u;
                *(uint8_t*)(ether_header_0) = 171u;
                *(uint8_t*)(ether_header_0) = 171u;
                *(uint8_t*)(ether_header_0) = 171u;
                inc_path_counter(31);

                // 32
                if (0u != (vector_value_out_2[0ul])) {
                  inc_path_counter(32);
                  dst_device = 1;
                  return dst_device;
                }

                // 33
                else {
                  inc_path_counter(33);
                  dst_device = 0;
                  return dst_device;
                } // !(0u != (vector_value_out_2[0ul]))

              }

              // 41
              // 42
              else {
                inc_path_counter(34);
                uint8_t* vector_value_out = 0u;
                vector_borrow(vector_3, chosen_backend, (void**)(&vector_value_out));
                inc_path_counter(35);
                vector_return(vector_3, chosen_backend, vector_value_out);
                inc_path_counter(36);
                int checksum = rte_ipv4_udptcp_cksum(ipv4_header_0, tcpudp_header_0);
                inc_path_counter(37);
                inc_path_counter(38);
                memcpy(((uint8_t*)(ipv4_header_0)) + 0ul, (void*)(&checksum), 2ul);
                memcpy(((uint8_t*)(ipv4_header_0)) + 2ul, (void*)(&ipv4_header_0->src_addr), 4ul);
                memcpy(((uint8_t*)(ipv4_header_0)) + 6ul, (void*)(&vector_value_out[8ul]), 4ul);
                inc_path_counter(39);
                memcpy(((uint8_t*)(ether_header_0)) + 0ul, (void*)(&vector_value_out[2ul]), 6ul);
                *(uint8_t*)(ether_header_0) = 171u;
                *(uint8_t*)(ether_header_0) = 171u;
                *(uint8_t*)(ether_header_0) = 171u;
                *(uint8_t*)(ether_header_0) = 171u;
                *(uint8_t*)(ether_header_0) = 171u;
                *(uint8_t*)(ether_header_0) = 171u;
                inc_path_counter(40);

                // 41
                if (0u != (vector_value_out[0ul])) {
                  inc_path_counter(41);
                  dst_device = 1;
                  return dst_device;
                }

                // 42
                else {
                  inc_path_counter(42);
                  dst_device = 0;
                  return dst_device;
                } // !(0u != (vector_value_out[0ul]))

              } // !(false == ((out_of_space_2) & (0u == number_of_freed_flows)))

            } // !(0u == prefered_backend_found)

          }

          // 55
          // 56
          // 67
          // 81
          // 82
          else {
            inc_path_counter(43);
            uint8_t* vector_value_out = 0u;
            vector_borrow(vector_1, map_value_out, (void**)(&vector_value_out));
            inc_path_counter(44);
            vector_return(vector_1, map_value_out, vector_value_out);
            inc_path_counter(45);
            int32_t dchain_is_index_allocated = dchain_is_index_allocated(dchain_1, *(int*)(vector_value_out));
            inc_path_counter(46);

            // 55
            // 56
            if (0u != dchain_is_index_allocated) {
              inc_path_counter(47);
              dchain_rejuvenate_index(dchain, map_value_out, now);
              inc_path_counter(48);
              uint8_t* vector_value_out_1 = 0u;
              vector_borrow(vector_3, *(int*)(vector_value_out), (void**)(&vector_value_out_1));
              inc_path_counter(49);
              vector_return(vector_3, *(int*)(vector_value_out), vector_value_out_1);
              inc_path_counter(50);
              int checksum = rte_ipv4_udptcp_cksum(ipv4_header_0, tcpudp_header_0);
              inc_path_counter(51);
              inc_path_counter(52);
              memcpy(((uint8_t*)(ipv4_header_0)) + 0ul, (void*)(&checksum), 2ul);
              memcpy(((uint8_t*)(ipv4_header_0)) + 2ul, (void*)(&ipv4_header_0->src_addr), 4ul);
              memcpy(((uint8_t*)(ipv4_header_0)) + 6ul, (void*)(&vector_value_out_1[8ul]), 4ul);
              inc_path_counter(53);
              memcpy(((uint8_t*)(ether_header_0)) + 0ul, (void*)(&vector_value_out_1[2ul]), 6ul);
              *(uint8_t*)(ether_header_0) = 171u;
              *(uint8_t*)(ether_header_0) = 171u;
              *(uint8_t*)(ether_header_0) = 171u;
              *(uint8_t*)(ether_header_0) = 171u;
              *(uint8_t*)(ether_header_0) = 171u;
              *(uint8_t*)(ether_header_0) = 171u;
              inc_path_counter(54);

              // 55
              if (0u != (vector_value_out_1[0ul])) {
                inc_path_counter(55);
                dst_device = 1;
                return dst_device;
              }

              // 56
              else {
                inc_path_counter(56);
                dst_device = 0;
                return dst_device;
              } // !(0u != (vector_value_out_1[0ul]))

            }

            // 67
            // 81
            // 82
            else {
              inc_path_counter(57);
              uint8_t* vector_value_out_1 = 0u;
              vector_borrow(vector, map_value_out, (void**)(&vector_value_out_1));
              inc_path_counter(58);
              uint8_t map_key_1[13];
              map_key_1[0u] = ipv4_header_0->src_addr & 0xff;
              map_key_1[1u] = (ipv4_header_0->src_addr >> 8) & 0xff;
              map_key_1[2u] = (ipv4_header_0->src_addr >> 16) & 0xff;
              map_key_1[3u] = (ipv4_header_0->src_addr >> 24) & 0xff;
              map_key_1[4u] = ipv4_header_0->dst_addr & 0xff;
              map_key_1[5u] = (ipv4_header_0->dst_addr >> 8) & 0xff;
              map_key_1[6u] = (ipv4_header_0->dst_addr >> 16) & 0xff;
              map_key_1[7u] = (ipv4_header_0->dst_addr >> 24) & 0xff;
              map_key_1[8u] = tcpudp_header_0->src_port & 0xff;
              map_key_1[9u] = (tcpudp_header_0->src_port >> 8) & 0xff;
              map_key_1[10u] = tcpudp_header_0->dst_port & 0xff;
              map_key_1[11u] = (tcpudp_header_0->dst_port >> 8) & 0xff;
              map_key_1[12u] = ipv4_header_0->next_proto_id;
              uint8_t trash[13];
              map_stats.update(58, map_key_1, 13);
              map_erase(map, &map_key_1, (void**)(&trash));
              inc_path_counter(59);
              dchain_free_index(dchain, map_value_out);
              inc_path_counter(60);
              vector_return(vector, map_value_out, vector_value_out_1);
              inc_path_counter(61);
              int hash = hash_obj(map_key, 13u);
              inc_path_counter(62);
              int32_t chosen_backend = 0u;
              int32_t prefered_backend_found = cht_find_preferred_available_backend(hash, vector_4, dchain_1, 97u, 32u, &chosen_backend);
              inc_path_counter(63);

              // 67
              if (0u == prefered_backend_found) {
                inc_path_counter(64);
                inc_path_counter(65);
                inc_path_counter(66);
                inc_path_counter(67);
                dst_device = 2;
                return dst_device;
              }

              // 81
              // 82
              else {
                inc_path_counter(68);
                int32_t new_index;
                dchain_allocate_new_index(dchain, &new_index, now);
                inc_path_counter(69);
                uint8_t* vector_value_out_2 = 0u;
                vector_borrow(vector, new_index, (void**)(&vector_value_out_2));
                memcpy(vector_value_out_2 + 0ul, (void*)(&ipv4_header_0->src_addr), 8ul);
                memcpy(vector_value_out_2 + 8ul, tcpudp_header_0, 4ul);
                memcpy(vector_value_out_2 + 12ul, (void*)(&ipv4_header_0->next_proto_id), 1ul);
                inc_path_counter(70);
                uint8_t* vector_value_out_3 = 0u;
                vector_borrow(vector_1, new_index, (void**)(&vector_value_out_3));
                memcpy(vector_value_out_3 + 0ul, (void*)(&chosen_backend), 4ul);
                inc_path_counter(71);
                vector_return(vector_1, new_index, vector_value_out_3);
                inc_path_counter(72);
                map_stats.update(72, vector_value_out_2, 13);
                map_put(map, vector_value_out_2, new_index);
                inc_path_counter(73);
                vector_return(vector, new_index, vector_value_out_2);
                inc_path_counter(74);
                uint8_t* vector_value_out_4 = 0u;
                vector_borrow(vector_3, chosen_backend, (void**)(&vector_value_out_4));
                inc_path_counter(75);
                vector_return(vector_3, chosen_backend, vector_value_out_4);
                inc_path_counter(76);
                int checksum = rte_ipv4_udptcp_cksum(ipv4_header_0, tcpudp_header_0);
                inc_path_counter(77);
                inc_path_counter(78);
                memcpy(((uint8_t*)(ipv4_header_0)) + 0ul, (void*)(&checksum), 2ul);
                memcpy(((uint8_t*)(ipv4_header_0)) + 2ul, (void*)(&ipv4_header_0->src_addr), 4ul);
                memcpy(((uint8_t*)(ipv4_header_0)) + 6ul, (void*)(&vector_value_out_4[8ul]), 4ul);
                inc_path_counter(79);
                memcpy(((uint8_t*)(ether_header_0)) + 0ul, (void*)(&vector_value_out_4[2ul]), 6ul);
                *(uint8_t*)(ether_header_0) = 171u;
                *(uint8_t*)(ether_header_0) = 171u;
                *(uint8_t*)(ether_header_0) = 171u;
                *(uint8_t*)(ether_header_0) = 171u;
                *(uint8_t*)(ether_header_0) = 171u;
                *(uint8_t*)(ether_header_0) = 171u;
                inc_path_counter(80);

                // 81
                if (0u != (vector_value_out_4[0ul])) {
                  inc_path_counter(81);
                  dst_device = 1;
                  return dst_device;
                }

                // 82
                else {
                  inc_path_counter(82);
                  dst_device = 0;
                  return dst_device;
                } // !(0u != (vector_value_out_4[0ul]))

              } // !(0u == prefered_backend_found)

            } // !(0u != dchain_is_index_allocated)

          } // !(0u == map_has_this_key)

        }

        // 95
        // 99
        // 104
        else {
          inc_path_counter(83);
          uint8_t map_key[4];
          map_key[0u] = ipv4_header_0->src_addr & 0xff;
          map_key[1u] = (ipv4_header_0->src_addr >> 8) & 0xff;
          map_key[2u] = (ipv4_header_0->src_addr >> 16) & 0xff;
          map_key[3u] = (ipv4_header_0->src_addr >> 24) & 0xff;
          int map_value_out;
          map_stats.update(83, map_key, 4);
          int map_has_this_key = map_get(map_1, map_key, &map_value_out);
          inc_path_counter(84);

          // 95
          // 99
          if (0u == map_has_this_key) {
            inc_path_counter(85);
            int32_t new_index_1;
            int out_of_space_3 = !dchain_allocate_new_index(dchain_1, &new_index_1, now);
            inc_path_counter(86);

            // 95
            if (false == ((out_of_space_3) & (0u == number_of_freed_flows_1))) {
              inc_path_counter(87);
              uint8_t* vector_value_out = 0u;
              vector_borrow(vector_3, new_index_1, (void**)(&vector_value_out));
              vector_value_out[0u] = 1u;
              vector_value_out[1u] = 0u;
              memcpy(vector_value_out + 2ul, (void*)(&ether_header_0->s_addr.addr_bytes[0ul]), 6ul);
              memcpy(vector_value_out + 8ul, (void*)(&ipv4_header_0->src_addr), 4ul);
              inc_path_counter(88);
              vector_return(vector_3, new_index_1, vector_value_out);
              inc_path_counter(89);
              uint8_t* vector_value_out_1 = 0u;
              vector_borrow(vector_2, new_index_1, (void**)(&vector_value_out_1));
              memcpy(vector_value_out_1 + 0ul, (void*)(&ipv4_header_0->src_addr), 4ul);
              inc_path_counter(90);
              map_stats.update(90, vector_value_out_1, 4);
              map_put(map_1, vector_value_out_1, new_index_1);
              inc_path_counter(91);
              vector_return(vector_2, new_index_1, vector_value_out_1);
              inc_path_counter(92);
              inc_path_counter(93);
              inc_path_counter(94);
              inc_path_counter(95);
              dst_device = DROP;
              return dst_device;
            }

            // 99
            else {
              inc_path_counter(96);
              inc_path_counter(97);
              inc_path_counter(98);
              inc_path_counter(99);
              dst_device = DROP;
              return dst_device;
            } // !(false == ((out_of_space_3) & (0u == number_of_freed_flows_1)))

          }

          // 104
          else {
            inc_path_counter(100);
            dchain_rejuvenate_index(dchain_1, map_value_out, now);
            inc_path_counter(101);
            inc_path_counter(102);
            inc_path_counter(103);
            inc_path_counter(104);
            dst_device = DROP;
            return dst_device;
          } // !(0u == map_has_this_key)

        } // !(1u != device)

      }

      // 117
      // 121
      // 126
      else {
        inc_path_counter(105);
        uint8_t map_key[4];
        map_key[0u] = ipv4_header_0->src_addr & 0xff;
        map_key[1u] = (ipv4_header_0->src_addr >> 8) & 0xff;
        map_key[2u] = (ipv4_header_0->src_addr >> 16) & 0xff;
        map_key[3u] = (ipv4_header_0->src_addr >> 24) & 0xff;
        int map_value_out;
        map_stats.update(105, map_key, 4);
        int map_has_this_key = map_get(map_1, map_key, &map_value_out);
        inc_path_counter(106);

        // 117
        // 121
        if (0u == map_has_this_key) {
          inc_path_counter(107);
          int32_t new_index_1;
          int out_of_space_3 = !dchain_allocate_new_index(dchain_1, &new_index_1, now);
          inc_path_counter(108);

          // 117
          if (false == ((out_of_space_3) & (0u == number_of_freed_flows_1))) {
            inc_path_counter(109);
            uint8_t* vector_value_out = 0u;
            vector_borrow(vector_3, new_index_1, (void**)(&vector_value_out));
            vector_value_out[0u] = 0u;
            vector_value_out[1u] = 0u;
            memcpy(vector_value_out + 2ul, (void*)(&ether_header_0->s_addr.addr_bytes[0ul]), 6ul);
            memcpy(vector_value_out + 8ul, (void*)(&ipv4_header_0->src_addr), 4ul);
            inc_path_counter(110);
            vector_return(vector_3, new_index_1, vector_value_out);
            inc_path_counter(111);
            uint8_t* vector_value_out_1 = 0u;
            vector_borrow(vector_2, new_index_1, (void**)(&vector_value_out_1));
            memcpy(vector_value_out_1 + 0ul, (void*)(&ipv4_header_0->src_addr), 4ul);
            inc_path_counter(112);
            map_stats.update(112, vector_value_out_1, 4);
            map_put(map_1, vector_value_out_1, new_index_1);
            inc_path_counter(113);
            vector_return(vector_2, new_index_1, vector_value_out_1);
            inc_path_counter(114);
            inc_path_counter(115);
            inc_path_counter(116);
            inc_path_counter(117);
            dst_device = DROP;
            return dst_device;
          }

          // 121
          else {
            inc_path_counter(118);
            inc_path_counter(119);
            inc_path_counter(120);
            inc_path_counter(121);
            dst_device = DROP;
            return dst_device;
          } // !(false == ((out_of_space_3) & (0u == number_of_freed_flows_1)))

        }

        // 126
        else {
          inc_path_counter(122);
          dchain_rejuvenate_index(dchain_1, map_value_out, now);
          inc_path_counter(123);
          inc_path_counter(124);
          inc_path_counter(125);
          inc_path_counter(126);
          dst_device = DROP;
          return dst_device;
        } // !(0u == map_has_this_key)

      } // !(0u != device)

    }

    // 129
    else {
      inc_path_counter(127);
      inc_path_counter(128);
      inc_path_counter(129);
      dst_device = DROP;
      return dst_device;
    } // !(((6u == (ipv4_header_0->next_proto_id)) | (17u == (ipv4_header_0->next_proto_id))) & ((4294967262u + packet_length) >= 4ul))

  }

  // 131
  else {
    inc_path_counter(130);
    inc_path_counter(131);
    dst_device = DROP;
    return dst_device;
  } // !((8u == (ether_header_0->ether_type)) & (20ul <= (4294967282u + packet_length)))

}
