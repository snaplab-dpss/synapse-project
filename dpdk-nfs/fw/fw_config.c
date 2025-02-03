#include "fw_config.h"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

#include "nf-util.h"
#include "nf-log.h"
#include "nf-parse.h"

#define PARSE_ERROR(format, ...)                                                                                                           \
  nf_config_usage();                                                                                                                       \
  fprintf(stderr, format, ##__VA_ARGS__);                                                                                                  \
  exit(EXIT_FAILURE);

void nf_config_init(int argc, char **argv) {
  struct option long_options[] = {{"fwd-table", required_argument, NULL, 'd'},
                                  {"expire", required_argument, NULL, 't'},
                                  {"max-flows", required_argument, NULL, 'f'},
                                  {NULL, 0, NULL, 0}};
  config.devices_cfg_fname[0]  = '\0'; // no static filtering configuration

  int opt;
  while ((opt = getopt_long(argc, argv, "d:t:f:", long_options, NULL)) != EOF) {
    unsigned device;
    switch (opt) {
    case 'd':
      strncpy(config.devices_cfg_fname, optarg, CONFIG_FNAME_LEN - 1);
      config.devices_cfg_fname[CONFIG_FNAME_LEN - 1] = '\0';
      break;

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
          "\t--fwd-table <file>: file containing the forwarding table.\n");
}

void nf_config_print(void) {
  NF_INFO("\n--- FW Config ---\n");

  NF_INFO("Expiration time: %" PRIu32 "us", config.expiration_time);
  NF_INFO("Max flows: %" PRIu32, config.max_flows);
  NF_INFO("Fwd table: %s", config.devices_cfg_fname);

  NF_INFO("\n--- --- ------ ---\n");
}
