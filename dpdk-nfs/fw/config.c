#include "config.h"
#include "nf-parse.h"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

#include "nf-util.h"
#include "nf-log.h"

#define PARSE_ERROR(format, ...)                                                                                                           \
  nf_config_usage();                                                                                                                       \
  fprintf(stderr, format, ##__VA_ARGS__);                                                                                                  \
  exit(EXIT_FAILURE);

void nf_config_init(int argc, char **argv) {
  struct option long_options[] = {{"expire", required_argument, NULL, 't'},
                                  {"max-flows", required_argument, NULL, 'f'},
                                  {"wan", required_argument, NULL, 'w'},
                                  {NULL, 0, NULL, 0}};
  int opt;
  while ((opt = getopt_long(argc, argv, "t:f:w:", long_options, NULL)) != EOF) {
    unsigned device;
    switch (opt) {
    case 't':
      config.expiration_time = nf_util_parse_int(optarg, "exp-time", 10, '\0');
      if (config.expiration_time == 0) {
        PARSE_ERROR("Expiration time must be strictly positive.\n");
      }
      break;

    case 'f':
      config.max_flows = nf_util_parse_int(optarg, "max-flows", 10, '\0');
      if (config.max_flows <= 0) {
        PARSE_ERROR("Flow table size must be strictly positive.\n");
      }
      break;

    case 'w':
      config.wan_device = nf_util_parse_int(optarg, "wan-dev", 10, '\0');
      if (config.wan_device >= rte_eth_dev_count_avail()) {
        PARSE_ERROR("WAN device does not exist.\n");
      }
      break;

    default:
      PARSE_ERROR("Unknown option.\n");
      break;
    }
  }

  // Reset getopt
  optind = 1;
}

void nf_config_usage(void) {
  NF_INFO("Usage:\n"
          "[DPDK EAL options] --\n"
          "\t--expire <time>: flow expiration time (us).\n"
          "\t--max-flows <n>: flow table capacity.\n"
          "\t--wan <device>: set device to be the external one.\n");
}

void nf_config_print(void) {
  NF_INFO("\n--- FW Config ---\n");

  NF_INFO("WAN device: %" PRIu16, config.wan_device);
  NF_INFO("Expiration time: %" PRIu32 "us", config.expiration_time);
  NF_INFO("Max flows: %" PRIu32, config.max_flows);

  NF_INFO("\n--- --- ------ ---\n");
}
