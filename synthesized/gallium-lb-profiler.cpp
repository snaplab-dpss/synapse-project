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
  NF_INFO("Usage: %s [dev0:pcap0] [dev1:pcap1] ...\n", argv[0]);
}

void nf_config_print(void) {
  NF_INFO("----- Config -----");
  for (const auto &dev_pcap : config.pcaps) {
    NF_INFO("device: %u, pcap: %s", dev_pcap.device,
            dev_pcap.pcap.filename().c_str());
  }
  NF_INFO("--- ---------- ---");
}

std::string nf_name_from_executable(const char *argv0) {
  std::filesystem::path nf_name = std::string(argv0);
  return nf_name.filename().stem();
}

void nf_config_init(int argc, char **argv) {
  if (argc < 2) {
    PARSE_ERROR(argv, "Insufficient arguments.\n");
  }

  config.nf_name = nf_name_from_executable(argv[0]);

  // split the arguments into device and pcap pairs joined by a :
  for (int i = 1; i < argc; i++) {
    char *arg = argv[i];
    char *device_str = strtok(arg, ":");
    char *pcap_str = strtok(NULL, ":");

    if (!device_str || !pcap_str) {
      nf_config_usage(argv);
      PARSE_ERROR(argv, "Invalid argument format: %s\n", arg);
    }

    dev_pcap_t dev_pcap;
    dev_pcap.device = nf_util_parse_int(device_str, "device", 10, '\0');
    dev_pcap.pcap = pcap_str;

    config.pcaps.push_back(dev_pcap);
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

PcapReader reader;
MapStats map_stats;
uint64_t *path_profiler_counter_ptr;
uint64_t path_profiler_counter_sz;
time_ns_t elapsed_time;

void generate_report() {
  json report;

  report["config"] = json();
  report["config"]["pcaps"] = json::array();
  for (const auto &dev_pcap : config.pcaps) {
    json dev_pcap_elem;
    dev_pcap_elem["device"] = dev_pcap.device;
    dev_pcap_elem["pcap"] = dev_pcap.pcap.filename().stem().string();
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

  reader.setup(config.pcaps);

  uint16_t dev;
  pkt_t pkt;

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
struct Vector* vector_2;
uint64_t path_profiler_counter[90];

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


  if (vector_allocate(4u, 64u, &vector_2) == 0) {
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
  path_profiler_counter_ptr = path_profiler_counter;
  path_profiler_counter_sz = 90u;
  return 1;
}

int nf_process(uint16_t device, uint8_t* packet, uint16_t packet_length, int64_t now) {
  uint16_t dst_device;
  path_profiler_counter[0] = (path_profiler_counter[0]) + 1;
  int number_of_freed_flows = expire_items_single_map(dchain, vector, map, now - 300000000000ul);
  path_profiler_counter[1] = (path_profiler_counter[1]) + 1;

  // 2
  if (0u != device) {
    path_profiler_counter[2] = (path_profiler_counter[2]) + 1;
    dst_device = 0;
    return dst_device;
  }

  // 25
  // 29
  // 37
  // 42
  // 53
  // 72
  // 76
  // 84
  // 87
  // 89
  else {
    path_profiler_counter[3] = (path_profiler_counter[3]) + 1;
    struct rte_ether_hdr* ether_header_0 = (struct rte_ether_hdr*)(packet);
    path_profiler_counter[4] = (path_profiler_counter[4]) + 1;

    // 25
    // 29
    // 37
    // 42
    // 53
    // 72
    // 76
    // 84
    // 87
    if ((8u == (ether_header_0->ether_type)) & (20ul <= (4294967282u + packet_length))) {
      path_profiler_counter[5] = (path_profiler_counter[5]) + 1;
      struct rte_ipv4_hdr* ipv4_header_0 = (struct rte_ipv4_hdr*)(packet + 14u);
      path_profiler_counter[6] = (path_profiler_counter[6]) + 1;

      // 25
      // 29
      // 37
      // 42
      // 53
      if ((6u == (ipv4_header_0->next_proto_id)) & ((4294967262u + packet_length) >= 20ul)) {
        path_profiler_counter[7] = (path_profiler_counter[7]) + 1;
        struct rte_tcp_hdr* tcp_header_0 = (struct rte_tcp_hdr*)(packet + (14u + 20u));
        path_profiler_counter[8] = (path_profiler_counter[8]) + 1;
        uint8_t map_key[13];
        map_key[0u] = ipv4_header_0->src_addr & 0xff;
        map_key[1u] = (ipv4_header_0->src_addr >> 8) & 0xff;
        map_key[2u] = (ipv4_header_0->src_addr >> 16) & 0xff;
        map_key[3u] = (ipv4_header_0->src_addr >> 24) & 0xff;
        map_key[4u] = ipv4_header_0->dst_addr & 0xff;
        map_key[5u] = (ipv4_header_0->dst_addr >> 8) & 0xff;
        map_key[6u] = (ipv4_header_0->dst_addr >> 16) & 0xff;
        map_key[7u] = (ipv4_header_0->dst_addr >> 24) & 0xff;
        map_key[8u] = tcp_header_0->src_port & 0xff;
        map_key[9u] = (tcp_header_0->src_port >> 8) & 0xff;
        map_key[10u] = tcp_header_0->dst_port & 0xff;
        map_key[11u] = (tcp_header_0->dst_port >> 8) & 0xff;
        map_key[12u] = ipv4_header_0->next_proto_id;
        int map_value_out;
        map_stats.update(8, map_key, 13);
        int map_has_this_key = map_get(map, map_key, &map_value_out);
        path_profiler_counter[9] = (path_profiler_counter[9]) + 1;

        // 25
        // 29
        // 37
        if (0u == ((tcp_header_0->tcp_flags) & 5u)) {
          path_profiler_counter[10] = (path_profiler_counter[10]) + 1;

          // 25
          // 29
          if (0u == map_has_this_key) {
            path_profiler_counter[11] = (path_profiler_counter[11]) + 1;
            int32_t new_index;
            int out_of_space_1 = !dchain_allocate_new_index(dchain, &new_index, now);
            path_profiler_counter[12] = (path_profiler_counter[12]) + 1;

            // 25
            if (false == ((out_of_space_1) & (0u == number_of_freed_flows))) {
              path_profiler_counter[13] = (path_profiler_counter[13]) + 1;
              int hash = hash_obj(map_key, 13u);
              path_profiler_counter[14] = (path_profiler_counter[14]) + 1;
              uint8_t* vector_value_out = 0u;
              vector_borrow(vector, new_index, (void**)(&vector_value_out));
              memcpy(vector_value_out + 0ul, (void*)(&ipv4_header_0->src_addr), 8ul);
              memcpy(vector_value_out + 8ul, (void*)(&tcp_header_0->src_port), 4ul);
              memcpy(vector_value_out + 12ul, (void*)(&ipv4_header_0->next_proto_id), 1ul);
              path_profiler_counter[15] = (path_profiler_counter[15]) + 1;
              uint8_t* vector_value_out_1 = 0u;
              vector_borrow(vector_2, hash % 64u, (void**)(&vector_value_out_1));
              path_profiler_counter[16] = (path_profiler_counter[16]) + 1;
              uint8_t* vector_value_out_2 = 0u;
              vector_borrow(vector_1, new_index, (void**)(&vector_value_out_2));
              memcpy(vector_value_out_2 + 0ul, vector_value_out_1, 4ul);
              path_profiler_counter[17] = (path_profiler_counter[17]) + 1;
              map_stats.update(17, vector_value_out, 13);
              map_put(map, vector_value_out, new_index);
              path_profiler_counter[18] = (path_profiler_counter[18]) + 1;
              vector_return(vector, new_index, vector_value_out);
              path_profiler_counter[19] = (path_profiler_counter[19]) + 1;
              vector_return(vector_2, hash % 64u, vector_value_out_1);
              path_profiler_counter[20] = (path_profiler_counter[20]) + 1;
              vector_return(vector_1, new_index, vector_value_out_2);
              path_profiler_counter[21] = (path_profiler_counter[21]) + 1;
              int checksum = rte_ipv4_udptcp_cksum(ipv4_header_0, tcp_header_0);
              path_profiler_counter[22] = (path_profiler_counter[22]) + 1;
              path_profiler_counter[23] = (path_profiler_counter[23]) + 1;
              memcpy(((uint8_t*)(ipv4_header_0)) + 0ul, (void*)(&checksum), 2ul);
              memcpy(((uint8_t*)(ipv4_header_0)) + 2ul, (void*)(&ipv4_header_0->src_addr), 4ul);
              memcpy(((uint8_t*)(ipv4_header_0)) + 6ul, vector_value_out_1, 4ul);
              path_profiler_counter[24] = (path_profiler_counter[24]) + 1;
              path_profiler_counter[25] = (path_profiler_counter[25]) + 1;
              dst_device = 1;
              return dst_device;
            }

            // 29
            else {
              path_profiler_counter[26] = (path_profiler_counter[26]) + 1;
              path_profiler_counter[27] = (path_profiler_counter[27]) + 1;
              path_profiler_counter[28] = (path_profiler_counter[28]) + 1;
              path_profiler_counter[29] = (path_profiler_counter[29]) + 1;
              dst_device = DROP;
              return dst_device;
            } // !(false == ((out_of_space_1) & (0u == number_of_freed_flows)))

          }

          // 37
          else {
            path_profiler_counter[30] = (path_profiler_counter[30]) + 1;
            dchain_rejuvenate_index(dchain, map_value_out, now);
            path_profiler_counter[31] = (path_profiler_counter[31]) + 1;
            uint8_t* vector_value_out = 0u;
            vector_borrow(vector_1, map_value_out, (void**)(&vector_value_out));
            path_profiler_counter[32] = (path_profiler_counter[32]) + 1;
            vector_return(vector_1, map_value_out, vector_value_out);
            path_profiler_counter[33] = (path_profiler_counter[33]) + 1;
            int checksum = rte_ipv4_udptcp_cksum(ipv4_header_0, tcp_header_0);
            path_profiler_counter[34] = (path_profiler_counter[34]) + 1;
            path_profiler_counter[35] = (path_profiler_counter[35]) + 1;
            memcpy(((uint8_t*)(ipv4_header_0)) + 0ul, (void*)(&checksum), 2ul);
            memcpy(((uint8_t*)(ipv4_header_0)) + 2ul, (void*)(&ipv4_header_0->src_addr), 4ul);
            memcpy(((uint8_t*)(ipv4_header_0)) + 6ul, vector_value_out, 4ul);
            path_profiler_counter[36] = (path_profiler_counter[36]) + 1;
            path_profiler_counter[37] = (path_profiler_counter[37]) + 1;
            dst_device = 1;
            return dst_device;
          } // !(0u == map_has_this_key)

        }

        // 42
        // 53
        else {
          path_profiler_counter[38] = (path_profiler_counter[38]) + 1;

          // 42
          if (0u == map_has_this_key) {
            path_profiler_counter[39] = (path_profiler_counter[39]) + 1;
            path_profiler_counter[40] = (path_profiler_counter[40]) + 1;
            path_profiler_counter[41] = (path_profiler_counter[41]) + 1;
            path_profiler_counter[42] = (path_profiler_counter[42]) + 1;
            dst_device = DROP;
            return dst_device;
          }

          // 53
          else {
            path_profiler_counter[43] = (path_profiler_counter[43]) + 1;
            uint8_t* vector_value_out = 0u;
            vector_borrow(vector_1, map_value_out, (void**)(&vector_value_out));
            path_profiler_counter[44] = (path_profiler_counter[44]) + 1;
            vector_return(vector_1, map_value_out, vector_value_out);
            path_profiler_counter[45] = (path_profiler_counter[45]) + 1;
            dchain_free_index(dchain, map_value_out);
            path_profiler_counter[46] = (path_profiler_counter[46]) + 1;
            uint8_t* vector_value_out_1 = 0u;
            vector_borrow(vector, map_value_out, (void**)(&vector_value_out_1));
            path_profiler_counter[47] = (path_profiler_counter[47]) + 1;
            uint8_t map_key_1[13];
            map_key_1[0u] = vector_value_out_1[0ul];
            map_key_1[1u] = vector_value_out_1[1ul];
            map_key_1[2u] = vector_value_out_1[2ul];
            map_key_1[3u] = vector_value_out_1[3ul];
            map_key_1[4u] = vector_value_out_1[4ul];
            map_key_1[5u] = vector_value_out_1[5ul];
            map_key_1[6u] = vector_value_out_1[6ul];
            map_key_1[7u] = vector_value_out_1[7ul];
            map_key_1[8u] = vector_value_out_1[8ul];
            map_key_1[9u] = vector_value_out_1[9ul];
            map_key_1[10u] = vector_value_out_1[10ul];
            map_key_1[11u] = vector_value_out_1[11ul];
            map_key_1[12u] = vector_value_out_1[12ul];
            uint8_t trash[13];
            map_stats.update(47, map_key_1, 13);
            map_erase(map, &map_key_1, (void**)(&trash));
            path_profiler_counter[48] = (path_profiler_counter[48]) + 1;
            vector_return(vector, map_value_out, vector_value_out_1);
            path_profiler_counter[49] = (path_profiler_counter[49]) + 1;
            int checksum = rte_ipv4_udptcp_cksum(ipv4_header_0, tcp_header_0);
            path_profiler_counter[50] = (path_profiler_counter[50]) + 1;
            path_profiler_counter[51] = (path_profiler_counter[51]) + 1;
            memcpy(((uint8_t*)(ipv4_header_0)) + 0ul, (void*)(&checksum), 2ul);
            memcpy(((uint8_t*)(ipv4_header_0)) + 2ul, (void*)(&ipv4_header_0->src_addr), 4ul);
            memcpy(((uint8_t*)(ipv4_header_0)) + 6ul, vector_value_out, 4ul);
            path_profiler_counter[52] = (path_profiler_counter[52]) + 1;
            path_profiler_counter[53] = (path_profiler_counter[53]) + 1;
            dst_device = 1;
            return dst_device;
          } // !(0u == map_has_this_key)

        } // !(0u == ((tcp_header_0->tcp_flags) & 5u))

      }

      // 72
      // 76
      // 84
      // 87
      else {
        path_profiler_counter[54] = (path_profiler_counter[54]) + 1;

        // 72
        // 76
        // 84
        if ((17u == (ipv4_header_0->next_proto_id)) & ((4294967262u + packet_length) >= 8ul)) {
          path_profiler_counter[55] = (path_profiler_counter[55]) + 1;
          struct rte_udp_hdr* udp_header_0 = (struct rte_udp_hdr*)(packet + (14u + 20u));
          path_profiler_counter[56] = (path_profiler_counter[56]) + 1;
          uint8_t map_key[13];
          map_key[0u] = ipv4_header_0->src_addr & 0xff;
          map_key[1u] = (ipv4_header_0->src_addr >> 8) & 0xff;
          map_key[2u] = (ipv4_header_0->src_addr >> 16) & 0xff;
          map_key[3u] = (ipv4_header_0->src_addr >> 24) & 0xff;
          map_key[4u] = ipv4_header_0->dst_addr & 0xff;
          map_key[5u] = (ipv4_header_0->dst_addr >> 8) & 0xff;
          map_key[6u] = (ipv4_header_0->dst_addr >> 16) & 0xff;
          map_key[7u] = (ipv4_header_0->dst_addr >> 24) & 0xff;
          map_key[8u] = udp_header_0->src_port & 0xff;
          map_key[9u] = (udp_header_0->src_port >> 8) & 0xff;
          map_key[10u] = udp_header_0->dst_port & 0xff;
          map_key[11u] = (udp_header_0->dst_port >> 8) & 0xff;
          map_key[12u] = ipv4_header_0->next_proto_id;
          int map_value_out;
          map_stats.update(56, map_key, 13);
          int map_has_this_key = map_get(map, map_key, &map_value_out);
          path_profiler_counter[57] = (path_profiler_counter[57]) + 1;

          // 72
          // 76
          if (0u == map_has_this_key) {
            path_profiler_counter[58] = (path_profiler_counter[58]) + 1;
            int32_t new_index;
            int out_of_space_1 = !dchain_allocate_new_index(dchain, &new_index, now);
            path_profiler_counter[59] = (path_profiler_counter[59]) + 1;

            // 72
            if (false == ((out_of_space_1) & (0u == number_of_freed_flows))) {
              path_profiler_counter[60] = (path_profiler_counter[60]) + 1;
              int hash = hash_obj(map_key, 13u);
              path_profiler_counter[61] = (path_profiler_counter[61]) + 1;
              uint8_t* vector_value_out = 0u;
              vector_borrow(vector, new_index, (void**)(&vector_value_out));
              memcpy(vector_value_out + 0ul, (void*)(&ipv4_header_0->src_addr), 8ul);
              memcpy(vector_value_out + 8ul, (void*)(&udp_header_0->src_port), 4ul);
              memcpy(vector_value_out + 12ul, (void*)(&ipv4_header_0->next_proto_id), 1ul);
              path_profiler_counter[62] = (path_profiler_counter[62]) + 1;
              uint8_t* vector_value_out_1 = 0u;
              vector_borrow(vector_2, hash % 64u, (void**)(&vector_value_out_1));
              path_profiler_counter[63] = (path_profiler_counter[63]) + 1;
              uint8_t* vector_value_out_2 = 0u;
              vector_borrow(vector_1, new_index, (void**)(&vector_value_out_2));
              memcpy(vector_value_out_2 + 0ul, vector_value_out_1, 4ul);
              path_profiler_counter[64] = (path_profiler_counter[64]) + 1;
              map_stats.update(64, vector_value_out, 13);
              map_put(map, vector_value_out, new_index);
              path_profiler_counter[65] = (path_profiler_counter[65]) + 1;
              vector_return(vector, new_index, vector_value_out);
              path_profiler_counter[66] = (path_profiler_counter[66]) + 1;
              vector_return(vector_2, hash % 64u, vector_value_out_1);
              path_profiler_counter[67] = (path_profiler_counter[67]) + 1;
              vector_return(vector_1, new_index, vector_value_out_2);
              path_profiler_counter[68] = (path_profiler_counter[68]) + 1;
              int checksum = rte_ipv4_udptcp_cksum(ipv4_header_0, udp_header_0);
              path_profiler_counter[69] = (path_profiler_counter[69]) + 1;
              path_profiler_counter[70] = (path_profiler_counter[70]) + 1;
              memcpy(((uint8_t*)(ipv4_header_0)) + 0ul, (void*)(&checksum), 2ul);
              memcpy(((uint8_t*)(ipv4_header_0)) + 2ul, (void*)(&ipv4_header_0->src_addr), 4ul);
              memcpy(((uint8_t*)(ipv4_header_0)) + 6ul, vector_value_out_1, 4ul);
              path_profiler_counter[71] = (path_profiler_counter[71]) + 1;
              path_profiler_counter[72] = (path_profiler_counter[72]) + 1;
              dst_device = 1;
              return dst_device;
            }

            // 76
            else {
              path_profiler_counter[73] = (path_profiler_counter[73]) + 1;
              path_profiler_counter[74] = (path_profiler_counter[74]) + 1;
              path_profiler_counter[75] = (path_profiler_counter[75]) + 1;
              path_profiler_counter[76] = (path_profiler_counter[76]) + 1;
              dst_device = DROP;
              return dst_device;
            } // !(false == ((out_of_space_1) & (0u == number_of_freed_flows)))

          }

          // 84
          else {
            path_profiler_counter[77] = (path_profiler_counter[77]) + 1;
            dchain_rejuvenate_index(dchain, map_value_out, now);
            path_profiler_counter[78] = (path_profiler_counter[78]) + 1;
            uint8_t* vector_value_out = 0u;
            vector_borrow(vector_1, map_value_out, (void**)(&vector_value_out));
            path_profiler_counter[79] = (path_profiler_counter[79]) + 1;
            vector_return(vector_1, map_value_out, vector_value_out);
            path_profiler_counter[80] = (path_profiler_counter[80]) + 1;
            int checksum = rte_ipv4_udptcp_cksum(ipv4_header_0, udp_header_0);
            path_profiler_counter[81] = (path_profiler_counter[81]) + 1;
            path_profiler_counter[82] = (path_profiler_counter[82]) + 1;
            memcpy(((uint8_t*)(ipv4_header_0)) + 0ul, (void*)(&checksum), 2ul);
            memcpy(((uint8_t*)(ipv4_header_0)) + 2ul, (void*)(&ipv4_header_0->src_addr), 4ul);
            memcpy(((uint8_t*)(ipv4_header_0)) + 6ul, vector_value_out, 4ul);
            path_profiler_counter[83] = (path_profiler_counter[83]) + 1;
            path_profiler_counter[84] = (path_profiler_counter[84]) + 1;
            dst_device = 1;
            return dst_device;
          } // !(0u == map_has_this_key)

        }

        // 87
        else {
          path_profiler_counter[85] = (path_profiler_counter[85]) + 1;
          path_profiler_counter[86] = (path_profiler_counter[86]) + 1;
          path_profiler_counter[87] = (path_profiler_counter[87]) + 1;
          dst_device = DROP;
          return dst_device;
        } // !((17u == (ipv4_header_0->next_proto_id)) & ((4294967262u + packet_length) >= 8ul))

      } // !((6u == (ipv4_header_0->next_proto_id)) & ((4294967262u + packet_length) >= 20ul))

    }

    // 89
    else {
      path_profiler_counter[88] = (path_profiler_counter[88]) + 1;
      path_profiler_counter[89] = (path_profiler_counter[89]) + 1;
      dst_device = DROP;
      return dst_device;
    } // !((8u == (ether_header_0->ether_type)) & (20ul <= (4294967282u + packet_length)))

  } // !(0u != device)

}
