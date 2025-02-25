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
  struct option long_options[] = {{"server", required_argument, NULL, 's'},
                                  {"capacity", required_argument, NULL, 'c'},
                                  {"expire", required_argument, NULL, 't'},
                                  {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "i:c:t:", long_options, NULL)) != EOF) {
    switch (opt) {
    case 's':
      config.server_dev = nf_util_parse_int(optarg, "server", 10, '\0');
      if (config.server_dev >= rte_eth_dev_count_avail()) {
        PARSE_ERROR("Invalid server device.\n");
      }
      break;

    case 'c':
      config.capacity = nf_util_parse_int(optarg, "capacity", 10, '\0');
      if ((config.capacity & (config.capacity - 1)) != 0) {
        PARSE_ERROR("Capacity must be a power of 2.\n");
      }
      break;

    case 't':
      config.expiration_time_us = nf_util_parse_int(optarg, "expire", 10, '\0');
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
          "\t--server <device>: server device.\n"
          "\t--capacity <n>: capacity.\n"
          "\t--expire <interval>: Expiration time (us).\n");
}

void nf_config_print(void) {
  NF_INFO("\n--- KVS Config ---\n");

  NF_INFO("Server Device: %" PRIu16, config.server_dev);
  NF_INFO("Capacity: %" PRIu32, config.capacity);
  NF_INFO("Expiration time: %" PRIu32 "us", config.expiration_time_us);

  NF_INFO("\n--- --- ------ ---\n");
}
