#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "nf-util.h"
#include "nf-log.h"

#include "lib/util/compute.h"

#define PARSE_ERROR(format, ...)                                                                                                                     \
  nf_config_usage();                                                                                                                                 \
  fprintf(stderr, format, ##__VA_ARGS__);                                                                                                            \
  exit(EXIT_FAILURE);

void nf_config_init(int argc, char **argv) {
  uint16_t nb_devices = rte_eth_dev_count_avail();

  struct option long_options[] = {{"max-backends", required_argument, NULL, 'b'},
                                  {"cht-height", required_argument, NULL, 'h'},
                                  {"management-dev", required_argument, NULL, 'm'},
                                  {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "b:h:m:", long_options, NULL)) != EOF) {
    switch (opt) {
    case 'b':
      config.max_backends = nf_util_parse_int(optarg, "max_backends", 10, '\0');
      if (config.max_backends <= 0) {
        PARSE_ERROR("Backend capacity must be strictly positive.\n");
      }
      break;

    case 'h':
      config.cht_height = nf_util_parse_int(optarg, "cht-height", 10, '\0');
      if (config.cht_height <= 0) {
        PARSE_ERROR("CHT height must be strictly positive.\n");
      }

      if (!is_prime(config.cht_height)) {
        PARSE_ERROR("CHT height must be a prime number.\n");
      }

      break;

    case 'm':
      config.management_dev = nf_util_parse_int(optarg, "management-dev", 10, '\0');
      if (config.management_dev >= nb_devices) {
        PARSE_ERROR("Management device does not exist.\n");
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
          "\t--max-backends <n>: backend table capacity.\n"
          "\t--cht-height <n>: consistent hashing table height: bigger <n> generates more smooth distribution.\n"
          "\t--management-dev <device>: set device to be the external one.\n");
}

void nf_config_print(void) {
  NF_INFO("\n--- LoadBalancer Config ---\n");

  NF_INFO("Management device: %" PRIu16, config.management_dev);
  NF_INFO("CHT height: %" PRIu32, config.cht_height);
  NF_INFO("Max backends: %" PRIu32, config.max_backends);

  NF_INFO("\n--- --- ------ ---\n");
}
