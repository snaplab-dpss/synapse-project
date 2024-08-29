#include "kvstore_config.h"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

#include "nf-util.h"
#include "nf-log.h"
#include "nf-parse.h"

#define PARSE_ERROR(format, ...)                                               \
  nf_config_usage();                                                           \
  fprintf(stderr, format, ##__VA_ARGS__);                                      \
  exit(EXIT_FAILURE);

const uint16_t DEFAULT_INTERNAL = 1;
const uint16_t DEFAULT_EXTERNAL = 0;

void nf_config_init(int argc, char **argv) {
  // Set the default values
  config.internal_device = DEFAULT_INTERNAL;
  config.external_device = DEFAULT_EXTERNAL;

  uint16_t nb_devices = rte_eth_dev_count_avail();

  struct option long_options[] = {{"int", required_argument, NULL, 'i'},
                                  {"ext", required_argument, NULL, 'e'},
                                  {"max-entries", required_argument, NULL, 'f'},
                                  {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "i:e:f:", long_options, NULL)) != EOF) {
    unsigned device;
    switch (opt) {
    case 'i':
      config.internal_device = nf_util_parse_int(optarg, "int", 10, '\0');
      if (config.internal_device < 0 || config.internal_device >= nb_devices) {
        PARSE_ERROR("Invalid internal device.\n");
      }
      break;
    case 'e':
      config.external_device = nf_util_parse_int(optarg, "ext", 10, '\0');
      if (config.external_device < 0 || config.external_device >= nb_devices) {
        PARSE_ERROR("Invalid external device.\n");
      }
      break;
    case 'f':
      config.max_entries = nf_util_parse_int(optarg, "max-entries", 10, '\0');
      if (config.max_entries <= 0) {
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
          "\t--int <device>: internal device, default: %" PRIu16 ".\n"
          "\t--ext <device>: external device, default: %" PRIu16 ".\n"
          "\t--max-entries <n>: flow table capacity.\n",
          DEFAULT_INTERNAL, DEFAULT_EXTERNAL);
}

void nf_config_print(void) {
  NF_INFO("\n--- KVS Config ---\n");

  NF_INFO("Internal Device: %" PRIu16, config.internal_device);
  NF_INFO("External Device: %" PRIu16, config.external_device);
  NF_INFO("Max entries: %" PRIu32, config.max_entries);

  NF_INFO("\n--- --- ------ ---\n");
}
