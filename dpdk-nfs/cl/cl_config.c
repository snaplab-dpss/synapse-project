#include "cl_config.h"

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "nf-util.h"
#include "nf-log.h"

const uint16_t DEFAULT_LAN                     = 1;
const uint16_t DEFAULT_WAN                     = 0;
const uint32_t DEFAULT_MAX_FLOWS               = 65536;
const uint16_t DEFAULT_MAX_CLIENTS             = 60;
const uint64_t DEFAULT_EXPIRATION_TIME         = 1000000; // 1s
const uint32_t DEFAULT_SKETCH_HEIGHT           = 4;
const uint32_t DEFAULT_SKETCH_WIDTH            = 1024;
const uint64_t DEFAULT_SKETCH_CLEANUP_INTERVAL = 10000000; // 10s

#define PARSE_ERROR(format, ...)                                                                                                           \
  nf_config_usage();                                                                                                                       \
  fprintf(stderr, format, ##__VA_ARGS__);                                                                                                  \
  exit(EXIT_FAILURE);

void nf_config_init(int argc, char **argv) {
  // Set the default values
  config.lan_device              = DEFAULT_LAN;
  config.wan_device              = DEFAULT_WAN;
  config.max_flows               = DEFAULT_MAX_FLOWS;
  config.max_clients             = DEFAULT_MAX_CLIENTS;
  config.expiration_time         = DEFAULT_EXPIRATION_TIME;
  config.sketch_height           = DEFAULT_SKETCH_HEIGHT;
  config.sketch_width            = DEFAULT_SKETCH_WIDTH;
  config.sketch_cleanup_interval = DEFAULT_SKETCH_CLEANUP_INTERVAL;

  unsigned nb_devices = rte_eth_dev_count_avail();

  struct option long_options[] = {{"lan", required_argument, NULL, 'l'},
                                  {"wan", required_argument, NULL, 'w'},
                                  {"max-flows", required_argument, NULL, 'f'},
                                  {"max-clients", required_argument, NULL, 'c'},
                                  {"expire", required_argument, NULL, 't'},
                                  {"sketch-height", required_argument, NULL, 'h'},
                                  {"sketch-width", required_argument, NULL, 'W'},
                                  {"sketch-cleanup-interval", required_argument, NULL, 'C'},
                                  {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "l:w:f:c:t:h:W:C:", long_options, NULL)) != EOF) {
    switch (opt) {
    case 'l':
      config.lan_device = nf_util_parse_int(optarg, "lan", 10, '\0');
      if (config.lan_device >= nb_devices) {
        PARSE_ERROR("Invalid LAN device.\n");
      }
      break;

    case 'w':
      config.wan_device = nf_util_parse_int(optarg, "wan", 10, '\0');
      if (config.wan_device >= nb_devices) {
        PARSE_ERROR("Invalid WAN device.\n");
      }
      break;

    case 'f':
      config.max_flows = nf_util_parse_int(optarg, "max-flows", 10, '\0');
      if ((config.max_flows & (config.max_flows - 1)) != 0) {
        PARSE_ERROR("Maximum number of flows must be a power of 2.\n");
      }
      break;

    case 'c':
      config.max_clients = nf_util_parse_int(optarg, "max-clients", 10, '\0');
      break;

    case 't':
      config.expiration_time = nf_util_parse_int(optarg, "expire", 10, '\0');
      break;

    case 'h':
      config.sketch_height = nf_util_parse_int(optarg, "sketch-height", 10, '\0');
      break;

    case 'W':
      config.sketch_width = nf_util_parse_int(optarg, "sketch-width", 10, '\0');
      break;

    case 'C':
      config.sketch_cleanup_interval = nf_util_parse_int(optarg, "sketch-cleanup-interval", 10, '\0');
      break;

    default:
      PARSE_ERROR("Unknown option %c", opt);
    }
  }

  // Reset getopt
  optind = 1;
}

void nf_config_usage(void) {
  NF_INFO("Usage:\n"
          "[DPDK EAL options] --\n"
          "\t--lan <device>: LAN device,"
          " default: %" PRIu16 ".\n"
          "\t--wan <device>: WAN device,"
          " default: %" PRIu16 ".\n"
          "\t--max-flows <max-flows>: maximum number of flows,"
          " default: %" PRIu32 ".\n"
          "\t--max-clients <max-clients>: maximum allowed number of clients,"
          " default: %" PRIu16 ".\n"
          "\t--expire <time>: expiration time (us),"
          " default: %" PRIu64 ".\n"
          "\t--sketch-height <height>: CMS height,"
          " default: %" PRIu32 ".\n"
          "\t--sketch-width <width>: CMS width,"
          " default: %" PRIu32 ".\n"
          "\t--sketch-cleanup-interval <interval>: CMS cleanup interval (us),"
          " default: %" PRIu64 ".\n",
          DEFAULT_LAN, DEFAULT_WAN, DEFAULT_MAX_FLOWS, DEFAULT_MAX_CLIENTS, DEFAULT_EXPIRATION_TIME, DEFAULT_SKETCH_HEIGHT,
          DEFAULT_SKETCH_WIDTH, DEFAULT_SKETCH_CLEANUP_INTERVAL);
}

void nf_config_print(void) {
  NF_INFO("\n--- Connection Limiter Config ---\n");

  NF_INFO("LAN Device: %" PRIu16, config.lan_device);
  NF_INFO("WAN Device: %" PRIu16, config.wan_device);
  NF_INFO("Max flows: %" PRIu32, config.max_flows);
  NF_INFO("Max clients: %" PRIu16, config.max_clients);
  NF_INFO("Expiration time: %" PRIu64, config.expiration_time);
  NF_INFO("CMS sketch height: %" PRIu32, config.sketch_height);
  NF_INFO("CMS sketch width: %" PRIu32, config.sketch_width);
  NF_INFO("CMS sketch cleanup interval: %" PRIu64, config.sketch_cleanup_interval);

  NF_INFO("\n--- ------ ------ ---\n");
}
