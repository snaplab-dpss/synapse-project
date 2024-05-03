#include "kvstore_config.h"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

#include "nf-util.h"
#include "nf-log.h"
#include "nf-parse.h"

#define PARSE_ERROR(format, ...)          \
  nf_config_usage();                      \
  fprintf(stderr, format, ##__VA_ARGS__); \
  exit(EXIT_FAILURE);

void nf_config_init(int argc, char **argv) {
  uint16_t nb_devices = rte_eth_dev_count_avail();

  struct option long_options[] = {{"max-entries", required_argument, NULL, 'f'},
                                  {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "f:", long_options, NULL)) != EOF) {
    unsigned device;
    switch (opt) {
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
  NF_INFO(
      "Usage:\n"
      "[DPDK EAL options] --\n"
      "\t--max-entries <n>: flow table capacity.\n");
}

void nf_config_print(void) {
  NF_INFO("\n--- KVStore Config ---\n");

  NF_INFO("Max entries: %" PRIu32, config.max_entries);

  NF_INFO("\n--- --- ------ ---\n");
}
