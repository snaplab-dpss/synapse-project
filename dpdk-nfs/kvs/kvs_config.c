#include "kvs_config.h"

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

void nf_config_init(int argc, char **argv) {
  uint16_t nb_devices = rte_eth_dev_count_avail();

  struct option long_options[] = {{"server", required_argument, NULL, 'i'},
                                  {"client", required_argument, NULL, 'e'},
                                  {"capacity", required_argument, NULL, 'c'},
                                  {"expire", required_argument, NULL, 't'},
                                  {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "i:e:c:t:", long_options, NULL)) !=
         EOF) {
    unsigned device;
    switch (opt) {
    case 'i':
      config.server_dev = nf_util_parse_int(optarg, "server", 10, '\0');
      if (config.server_dev >= nb_devices) {
        PARSE_ERROR("Invalid server device.\n");
      }
      break;

    case 'e':
      config.client_dev = nf_util_parse_int(optarg, "client", 10, '\0');
      if (config.client_dev >= nb_devices) {
        PARSE_ERROR("Invalid client device.\n");
      }
      break;

    case 'c':
      config.capacity = nf_util_parse_int(optarg, "capacity", 10, '\0');
      if ((config.capacity & (config.capacity - 1)) != 0) {
        PARSE_ERROR("Capacity must be a power of 2.\n");
      }
      break;

    case 't':
      config.expiration_time = nf_util_parse_int(optarg, "expire", 10, '\0');
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
          "\t--client <device>: client device.\n"
          "\t--server <device>: server device.\n"
          "\t--capacity <n>: capacity.\n"
          "\t--expire <interval>: Expiration time (us).\n");
}

void nf_config_print(void) {
  NF_INFO("\n--- KVS Config ---\n");

  NF_INFO("Client Device: %" PRIu16, config.client_dev);
  NF_INFO("Server Device: %" PRIu16, config.server_dev);
  NF_INFO("Capacity: %" PRIu32, config.capacity);
  NF_INFO("Expiration time: %" PRIu64, config.expiration_time);

  NF_INFO("\n--- --- ------ ---\n");
}
