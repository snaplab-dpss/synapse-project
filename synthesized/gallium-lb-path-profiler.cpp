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

#include <getopt.h>
#include <pcap.h>
#include <stdbool.h>
#include <unistd.h>

#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>

using json = nlohmann::json;

#define REPORT_NAME "bdd-analysis"

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

#define ARG_HELP "help"
#define ARG_DEVICE "device"
#define ARG_PCAP "pcap"
#define ARG_TOTAL_PACKETS "packets"
#define ARG_TOTAL_FLOWS "flows"
#define ARG_TOTAL_CHURN_FPM "churn"
#define ARG_TOTAL_RATE_PPS "pps"
#define ARG_PACKET_SIZES "size"
#define ARG_TRAFFIC_UNIFORM "uniform"
#define ARG_TRAFFIC_ZIPF "zipf"
#define ARG_TRAFFIC_ZIPF_PARAM "zipf-param"
#define ARG_RANDOM_SEED "seed"

#define DEFAULT_DEVICE 0
#define DEFAULT_TOTAL_PACKETS 1000000lu
#define DEFAULT_TOTAL_FLOWS 65536lu
#define DEFAULT_TOTAL_CHURN_FPM 0lu
#define DEFAULT_TOTAL_RATE_PPS 150'000'000lu
#define DEFAULT_PACKET_SIZES MIN_PKT_SIZE
#define DEFAULT_TRAFFIC_UNIFORM true
#define DEFAULT_TRAFFIC_ZIPF false
#define DEFAULT_TRAFFIC_ZIPF_PARAMETER 1.26 // From Castan [SIGCOMM'18]

#define PARSE_ERROR(argv, format, ...)                                         \
  nf_config_usage(argv);                                                       \
  fprintf(stderr, format, ##__VA_ARGS__);                                      \
  exit(EXIT_FAILURE);

#define PARSER_ASSERT(cond, fmt, ...)                                          \
  if (!(cond))                                                                 \
    rte_exit(EXIT_FAILURE, fmt, ##__VA_ARGS__);

enum {
  /* long options mapped to short options: first long only option value must
   * be >= 256, so that it does not conflict with short options.
   */
  ARG_HELP_NUM = 256,
  ARG_DEVICE_NUM,
  ARG_PCAP_NUM,
  ARG_TOTAL_PACKETS_NUM,
  ARG_TOTAL_FLOWS_NUM,
  ARG_TOTAL_CHURN_FPM_NUM,
  ARG_TOTAL_RATE_PPS_NUM,
  ARG_PACKET_SIZES_NUM,
  ARG_TRAFFIC_UNIFORM_NUM,
  ARG_TRAFFIC_ZIPF_NUM,
  ARG_TRAFFIC_ZIPF_PARAM_NUM,
  ARG_RANDOM_SEED_NUM,
};

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

struct flow_t {
  uint32_t src_ip;
  uint32_t dst_ip;
  uint16_t src_port;
  uint16_t dst_port;
};

flow_t generate_random_flow() {
  flow_t flow;
  flow.src_ip = rte_rand();
  flow.dst_ip = rte_rand();
  flow.src_port = rte_rand();
  flow.dst_port = rte_rand();
  return flow;
}

struct pkt_t {
  uint8_t data[MAX_PKT_SIZE];
  uint32_t len;
  time_ns_t ts;
};

struct config_t {
  std::string nf_name;
  uint16_t device;
  const char *pcap;
  uint64_t total_packets;
  uint64_t total_flows;
  uint64_t churn_fpm;
  uint64_t rate_pps;
  uint16_t packet_sizes;
  bool traffic_uniform;
  bool traffic_zipf;
  float traffic_zipf_param;
} config;

class PacketGenerator {
private:
  config_t config;

  pcap_t *pcap;
  long pcap_start;

  uint64_t total_packets;
  uint64_t generated_packets;
  time_ns_t last_time;
  time_ns_t churn_alarm;
  time_ns_t churn_alarm_delta;
  std::vector<flow_t> flows;
  int last_percentage_report;

public:
  PacketGenerator(const config_t &_config)
      : config(_config), pcap(NULL), total_packets(config.total_packets),
        generated_packets(0), last_time(0),
        churn_alarm_delta(
            config.churn_fpm == 0 ? 0 : ((1e9 * 60) / config.churn_fpm)),
        churn_alarm(0), last_percentage_report(-1) {
    if (!config.pcap) {
      return;
    }

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap = pcap_open_offline(config.pcap, errbuf);

    if (pcap == NULL) {
      rte_exit(EXIT_FAILURE, "pcap_open_offline() failed: %s\n", errbuf);
    }

    // Get the total number of packets inside the pcap file first.
    FILE *pcap_fptr = pcap_file(pcap);
    assert(pcap_fptr && "Invalid pcap file pointer");
    pcap_start = ftell(pcap_fptr);

    struct pcap_pkthdr *hdr;
    const u_char *pcap_pkt;
    int success;
    total_packets = 0;
    while ((success = pcap_next_ex(pcap, &hdr, &pcap_pkt)) == 1) {
      total_packets++;
    }

    // Then rewind.
    fseek(pcap_fptr, pcap_start, SEEK_SET);
  }

  bool generate(pkt_t &pkt) {
    return pcap ? generate_with_pcap(pkt) : generate_without_pcap(pkt);
  }

private:
  bool generate_with_pcap(pkt_t &pkt) {
    if (generated_packets >= total_packets) {
      return false;
    }

    struct pcap_pkthdr *hdr;
    const u_char *pcap_pkt;

    int success = pcap_next_ex(pcap, &hdr, &pcap_pkt);
    assert(success >= 0 && "Error reading pcap file");

    memcpy(pkt.data, pcap_pkt, hdr->len);
    pkt.len = hdr->len;
    pkt.ts = hdr->ts.tv_sec * 1e9 + hdr->ts.tv_usec * 1e3;

    update_and_show_progress();
    return true;
  }

  bool generate_without_pcap(pkt_t &pkt) {
    if (generated_packets >= total_packets) {
      return false;
    }

    const flow_t &flow = get_next_flow();

    // Generate a packet
    pkt.len = config.packet_sizes;
    pkt.ts = last_time + 1e9 / config.rate_pps;

    last_time = pkt.ts;

    struct rte_ether_hdr *eth_hdr = (struct rte_ether_hdr *)pkt.data;
    struct rte_ipv4_hdr *ip_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);
    struct rte_udp_hdr *udp_hdr = (struct rte_udp_hdr *)(ip_hdr + 1);

    eth_hdr->ether_type = rte_bswap16(RTE_ETHER_TYPE_IPV4);

    ip_hdr->version_ihl = RTE_IPV4_VHL_DEF;
    ip_hdr->total_length = rte_bswap16(pkt.len - sizeof(struct rte_ether_hdr));
    ip_hdr->next_proto_id = IPPROTO_UDP;
    ip_hdr->src_addr = flow.src_ip;
    ip_hdr->dst_addr = flow.dst_ip;

    udp_hdr->src_port = flow.src_port;
    udp_hdr->dst_port = flow.dst_port;

    update_and_show_progress();
    return true;
  }

  void update_and_show_progress() {
    generated_packets++;
    int percentage = 100 * generated_packets / (double)total_packets;

    if (percentage == last_percentage_report) {
      return;
    }

    last_percentage_report = percentage;
    printf("\rProcessing packets %lu/%lu (%d%%) ...", generated_packets,
           total_packets, percentage);

    if (generated_packets == total_packets) {
      printf("\n");
    }

    fflush(stdout);
  }

  const flow_t &get_next_flow() {
    if (config.traffic_uniform) {
      if (generated_packets < config.total_flows) {
        flow_t random_flow = generate_random_flow();
        flows.push_back(random_flow);
        return flows[generated_packets];
      }

      int curr_flow_i = generated_packets % config.total_flows;

      if (churn_alarm_delta > 0 && last_time >= churn_alarm) {
        flows[curr_flow_i] = generate_random_flow();
        churn_alarm = last_time + churn_alarm_delta;
      }

      return flows[curr_flow_i];
    }

    assert(false && "Zipf traffic not implemented");
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
  NF_INFO(
      "Usage: %s\n"
      "\t [--" ARG_DEVICE " <dev> (default=%u)]\n"
      "\t [--" ARG_PCAP " <pcap>]\n"
      "\t [--" ARG_TOTAL_PACKETS " <#packets> (default=%lu)]\n"
      "\t [--" ARG_TOTAL_FLOWS " <#flows> (default=%lu)]\n"
      "\t [--" ARG_TOTAL_CHURN_FPM " <fpm> (default=%lu)]\n"
      "\t [--" ARG_TOTAL_RATE_PPS " <pps> (default=%lu)]\n"
      "\t [--" ARG_PACKET_SIZES " <bytes> (default=%u)]\n"
      "\t [--" ARG_TRAFFIC_UNIFORM " (default=%s)]\n"
      "\t [--" ARG_TRAFFIC_ZIPF " (default=%s)]\n"
      "\t [--" ARG_TRAFFIC_ZIPF_PARAM " <param> (default=%f)]\n"
      "\t [--" ARG_RANDOM_SEED " <seed> (default set by DPDK)]\n"
      "\t [--" ARG_HELP "]\n"
      "\n"
      "Argument descriptions:\n"
      "\t " ARG_DEVICE ": networking device to be analyzed\n"
      "\t " ARG_PCAP ": traffic trace to analyze (using this argument makes "
      "the program ignore all the other ones, as those are extracted "
      "directly from the pcap)\n"
      "\t " ARG_TOTAL_PACKETS ": total number of packets to generate\n"
      "\t " ARG_TOTAL_FLOWS ": total number of flows to generate\n"
      "\t " ARG_TOTAL_CHURN_FPM ": flow churn (fpm)\n"
      "\t " ARG_TOTAL_RATE_PPS ": packet rate (pps)\n"
      "\t " ARG_PACKET_SIZES ": packet sizes (bytes)\n"
      "\t " ARG_RANDOM_SEED ": random seed\n"
      "\t " ARG_HELP ": show this menu\n",
      argv[0], DEFAULT_DEVICE, DEFAULT_TOTAL_PACKETS, DEFAULT_TOTAL_FLOWS,
      DEFAULT_TOTAL_CHURN_FPM, DEFAULT_TOTAL_RATE_PPS, DEFAULT_PACKET_SIZES,
      DEFAULT_TRAFFIC_UNIFORM ? "true" : "false",
      DEFAULT_TRAFFIC_ZIPF ? "true" : "false", DEFAULT_TRAFFIC_ZIPF_PARAMETER);
}

void nf_config_print(void) {
  NF_INFO("----- Config -----");
  NF_INFO("device:     %u", config.device);
  if (config.pcap) {
    NF_INFO("pcap:       %s", config.pcap);
  } else {
    NF_INFO("#packets:   %lu", config.total_packets);
    NF_INFO("#flows:     %lu", config.total_flows);
    NF_INFO("churn:      %lu fpm", config.churn_fpm);
    NF_INFO("rate:       %lu pps", config.rate_pps);
    NF_INFO("pkt sizes:  %u bytes", config.packet_sizes);
    if (config.traffic_uniform) {
      NF_INFO("traffic:    uniform");
    } else if (config.traffic_zipf) {
      NF_INFO("traffic:    zipf");
      NF_INFO("zipf param: %f", config.traffic_zipf_param);
    }
  }
  NF_INFO("--- ---------- ---");
}

std::string nf_name_from_executable(const char *argv0) {
  std::string nf_name = argv0;
  size_t last_slash = nf_name.find_last_of('/');
  if (last_slash != std::string::npos) {
    nf_name = nf_name.substr(last_slash + 1);
  }
  return nf_name;
}

void nf_config_init(int argc, char **argv) {
  config.nf_name = nf_name_from_executable(argv[0]);
  config.device = DEFAULT_DEVICE;
  config.pcap = NULL;
  config.total_packets = DEFAULT_TOTAL_PACKETS;
  config.total_flows = DEFAULT_TOTAL_FLOWS;
  config.churn_fpm = DEFAULT_TOTAL_CHURN_FPM;
  config.rate_pps = DEFAULT_TOTAL_RATE_PPS;
  config.packet_sizes = DEFAULT_PACKET_SIZES;
  config.traffic_uniform = DEFAULT_TRAFFIC_UNIFORM;
  config.traffic_zipf = DEFAULT_TRAFFIC_ZIPF;
  config.traffic_zipf_param = DEFAULT_TRAFFIC_ZIPF_PARAMETER;

  const char short_options[] = "";

  struct option long_options[] = {
      {ARG_DEVICE, required_argument, NULL, ARG_DEVICE_NUM},
      {ARG_PCAP, required_argument, NULL, ARG_PCAP_NUM},
      {ARG_TOTAL_PACKETS, required_argument, NULL, ARG_TOTAL_PACKETS_NUM},
      {ARG_TOTAL_FLOWS, required_argument, NULL, ARG_TOTAL_FLOWS_NUM},
      {ARG_TOTAL_CHURN_FPM, required_argument, NULL, ARG_TOTAL_CHURN_FPM_NUM},
      {ARG_TOTAL_RATE_PPS, required_argument, NULL, ARG_TOTAL_RATE_PPS_NUM},
      {ARG_PACKET_SIZES, required_argument, NULL, ARG_PACKET_SIZES_NUM},
      {ARG_TRAFFIC_UNIFORM, no_argument, NULL, ARG_TRAFFIC_UNIFORM_NUM},
      {ARG_TRAFFIC_ZIPF, no_argument, NULL, ARG_TRAFFIC_ZIPF_NUM},
      {ARG_TRAFFIC_ZIPF_PARAM, required_argument, NULL,
       ARG_TRAFFIC_ZIPF_PARAM_NUM},
      {ARG_RANDOM_SEED, required_argument, NULL, ARG_RANDOM_SEED_NUM},
      {ARG_HELP, no_argument, NULL, ARG_HELP_NUM},
      {NULL, 0, NULL, 0}};

  int opt;

  while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) !=
         EOF) {
    switch (opt) {
    case ARG_HELP_NUM: {
      nf_config_usage(argv);
      exit(EXIT_SUCCESS);
    } break;
    case ARG_DEVICE_NUM: {
      config.device = nf_util_parse_int(optarg, "device", 10, '\0');
    } break;
    case ARG_PCAP_NUM: {
      config.pcap = optarg;
    } break;
    case ARG_TOTAL_PACKETS_NUM: {
      config.total_packets = nf_util_parse_int(optarg, "packets", 10, '\0');
    } break;
    case ARG_TOTAL_FLOWS_NUM: {
      config.total_flows = nf_util_parse_int(optarg, "flows", 10, '\0');
    } break;
    case ARG_TOTAL_CHURN_FPM_NUM: {
      config.churn_fpm = nf_util_parse_int(optarg, "churn", 10, '\0');
    } break;
    case ARG_TOTAL_RATE_PPS_NUM: {
      config.rate_pps = nf_util_parse_int(optarg, "pps", 10, '\0');
    } break;
    case ARG_PACKET_SIZES_NUM: {
      config.packet_sizes = nf_util_parse_int(optarg, "size", 10, '\0');
      PARSER_ASSERT(config.packet_sizes >= MIN_PKT_SIZE &&
                        config.packet_sizes <= MAX_PKT_SIZE,
                    "Packet size must be in the interval [%u-%" PRIu16
                    "] (requested %u).\n",
                    MIN_PKT_SIZE, MAX_PKT_SIZE, config.packet_sizes);
    } break;
    case ARG_TRAFFIC_UNIFORM_NUM: {
      config.traffic_uniform = true;
      config.traffic_zipf = false;
    } break;
    case ARG_TRAFFIC_ZIPF_NUM: {
      config.traffic_uniform = false;
      config.traffic_zipf = true;
    } break;
    case ARG_TRAFFIC_ZIPF_PARAM_NUM: {
      config.traffic_zipf_param = strtof(optarg, NULL);
    } break;
    case ARG_RANDOM_SEED_NUM: {
      uint32_t seed = nf_util_parse_int(optarg, "seed", 10, '\0');
      rte_srand(seed);
    } break;
    default:
      PARSE_ERROR(argv, "Unknown option.\n");
    }
  }

  nf_config_print();
}

uint64_t *path_profiler_counter_ptr;
uint64_t path_profiler_counter_sz;
time_ns_t elapsed_time;

void generate_report() {
  json report;

  report["config"] = json();
  report["config"]["device"] = config.device;
  if (config.pcap) {
    report["config"]["pcap"] = config.pcap;
  } else {
    report["config"]["total_packets"] = config.total_packets;
    report["config"]["total_flows"] = config.total_flows;
    report["config"]["churn_fpm"] = config.churn_fpm;
    report["config"]["rate_pps"] = config.rate_pps;
    report["config"]["packet_sizes"] = config.packet_sizes;
    report["config"]["traffic_uniform"] = config.traffic_uniform;
    report["config"]["traffic_zipf"] = config.traffic_zipf;
    report["config"]["traffic_zipf_param"] = config.traffic_zipf_param;
  }

  report["elapsed"] = elapsed_time;

  report["counters"] = json();
  for (unsigned i = 0; i < path_profiler_counter_sz; i++) {
    report["counters"][std::to_string(i)] = path_profiler_counter_ptr[i];
  }

  std::stringstream report_fname_ss;
  report_fname_ss << config.nf_name;
  report_fname_ss << "-" << REPORT_NAME;
  report_fname_ss << "-dev-" << config.device;
  if (config.pcap) {
    report_fname_ss << "-pcap-" << config.pcap;
  } else {
    report_fname_ss << "-packets-" << config.total_packets;
    report_fname_ss << "-flows-" << config.total_flows;
    report_fname_ss << "-churn-" << config.churn_fpm;
    report_fname_ss << "-rate-" << config.rate_pps;
    report_fname_ss << "-size-" << config.packet_sizes;
    if (config.traffic_uniform) {
      report_fname_ss << "-uniform";
    } else if (config.traffic_zipf) {
      report_fname_ss << "-zipf-param-" << config.traffic_zipf_param;
    }
  }
  report_fname_ss << ".json";

  auto report_fname = report_fname_ss.str();
  std::ofstream(report_fname) << report.dump(2);

  NF_INFO("Generated report %s", report_fname.c_str());
}

// Main worker method (for now used on a single thread...)
static void worker_main() {
  if (!nf_init()) {
    rte_exit(EXIT_FAILURE, "Error initializing NF");
  }

  PacketGenerator pkt_gen(config);
  pkt_t pkt;

  // Generate the first packet manually to record the starting time
  bool success = pkt_gen.generate(pkt);
  assert(success && "Failed to generate the first packet");

  time_ns_t start_time = pkt.ts;
  time_ns_t last_time = 0;

  do {
    // Ignore destination device, we don't forward anywhere
    nf_process(config.device, pkt.data, pkt.len, pkt.ts);
    last_time = pkt.ts;
  } while (pkt_gen.generate(pkt));

  elapsed_time = last_time - start_time;
  NF_INFO("Elapsed time: %lf s", (double)elapsed_time / 1e9);
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
            int32_t new_index_1;
            int out_of_space_1 = !dchain_allocate_new_index(dchain, &new_index_1, now);
            path_profiler_counter[12] = (path_profiler_counter[12]) + 1;

            // 25
            if (false == ((out_of_space_1) & (0u == number_of_freed_flows))) {
              path_profiler_counter[13] = (path_profiler_counter[13]) + 1;
              int hash = hash_obj(map_key, 13u);
              path_profiler_counter[14] = (path_profiler_counter[14]) + 1;
              uint8_t* vector_value_out = 0u;
              vector_borrow(vector, new_index_1, (void**)(&vector_value_out));
              memcpy(vector_value_out + 0ul, (void*)(&ipv4_header_0->src_addr), 8ul);
              memcpy(vector_value_out + 8ul, (void*)(&tcp_header_0->src_port), 4ul);
              memcpy(vector_value_out + 12ul, (void*)(&ipv4_header_0->next_proto_id), 1ul);
              path_profiler_counter[15] = (path_profiler_counter[15]) + 1;
              uint8_t* vector_value_out_1 = 0u;
              vector_borrow(vector_2, hash % 64u, (void**)(&vector_value_out_1));
              path_profiler_counter[16] = (path_profiler_counter[16]) + 1;
              uint8_t* vector_value_out_2 = 0u;
              vector_borrow(vector_1, new_index_1, (void**)(&vector_value_out_2));
              memcpy(vector_value_out_2 + 0ul, vector_value_out_1, 4ul);
              path_profiler_counter[17] = (path_profiler_counter[17]) + 1;
              map_put(map, vector_value_out, new_index_1);
              path_profiler_counter[18] = (path_profiler_counter[18]) + 1;
              vector_return(vector, new_index_1, vector_value_out);
              path_profiler_counter[19] = (path_profiler_counter[19]) + 1;
              vector_return(vector_2, hash % 64u, vector_value_out_1);
              path_profiler_counter[20] = (path_profiler_counter[20]) + 1;
              vector_return(vector_1, new_index_1, vector_value_out_2);
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
          int map_has_this_key = map_get(map, map_key, &map_value_out);
          path_profiler_counter[57] = (path_profiler_counter[57]) + 1;

          // 72
          // 76
          if (0u == map_has_this_key) {
            path_profiler_counter[58] = (path_profiler_counter[58]) + 1;
            int32_t new_index_1;
            int out_of_space_1 = !dchain_allocate_new_index(dchain, &new_index_1, now);
            path_profiler_counter[59] = (path_profiler_counter[59]) + 1;

            // 72
            if (false == ((out_of_space_1) & (0u == number_of_freed_flows))) {
              path_profiler_counter[60] = (path_profiler_counter[60]) + 1;
              int hash = hash_obj(map_key, 13u);
              path_profiler_counter[61] = (path_profiler_counter[61]) + 1;
              uint8_t* vector_value_out = 0u;
              vector_borrow(vector, new_index_1, (void**)(&vector_value_out));
              memcpy(vector_value_out + 0ul, (void*)(&ipv4_header_0->src_addr), 8ul);
              memcpy(vector_value_out + 8ul, (void*)(&udp_header_0->src_port), 4ul);
              memcpy(vector_value_out + 12ul, (void*)(&ipv4_header_0->next_proto_id), 1ul);
              path_profiler_counter[62] = (path_profiler_counter[62]) + 1;
              uint8_t* vector_value_out_1 = 0u;
              vector_borrow(vector_2, hash % 64u, (void**)(&vector_value_out_1));
              path_profiler_counter[63] = (path_profiler_counter[63]) + 1;
              uint8_t* vector_value_out_2 = 0u;
              vector_borrow(vector_1, new_index_1, (void**)(&vector_value_out_2));
              memcpy(vector_value_out_2 + 0ul, vector_value_out_1, 4ul);
              path_profiler_counter[64] = (path_profiler_counter[64]) + 1;
              map_put(map, vector_value_out, new_index_1);
              path_profiler_counter[65] = (path_profiler_counter[65]) + 1;
              vector_return(vector, new_index_1, vector_value_out);
              path_profiler_counter[66] = (path_profiler_counter[66]) + 1;
              vector_return(vector_2, hash % 64u, vector_value_out_1);
              path_profiler_counter[67] = (path_profiler_counter[67]) + 1;
              vector_return(vector_1, new_index_1, vector_value_out_2);
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
