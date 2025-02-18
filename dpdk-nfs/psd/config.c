#include "config.h"

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "nf-util.h"
#include "nf-log.h"
#include "lib/util/compute.h"

const uint32_t DEFAULT_CAPACITY        = 65536;
const uint32_t DEFAULT_MAX_PORTS       = 60;
const uint32_t DEFAULT_EXPIRATION_TIME = 1000000; // 1s

#define PARSE_ERROR(format, ...)                                                                                                           \
  nf_config_usage();                                                                                                                       \
  fprintf(stderr, format, ##__VA_ARGS__);                                                                                                  \
  exit(EXIT_FAILURE);

void nf_config_init(int argc, char **argv) {
  config.fwd_rules.n = 0;

  // Set the default values
  config.capacity        = DEFAULT_CAPACITY;
  config.max_ports       = DEFAULT_MAX_PORTS;
  config.expiration_time = DEFAULT_EXPIRATION_TIME;

  unsigned nb_devices = rte_eth_dev_count_avail();

  struct option long_options[] = {{"internal-devs", required_argument, NULL, 'd'}, {"fwd-rule", required_argument, NULL, 'r'},
                                  {"capacity", required_argument, NULL, 'c'},      {"max-ports", required_argument, NULL, 'p'},
                                  {"expire", required_argument, NULL, 't'},        {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "d:r:c:p:t:", long_options, NULL)) != EOF) {
    switch (opt) {
    case 'd': {
      uint16_t nb_devices        = rte_eth_dev_count_avail();
      struct int_list_t int_list = nf_util_parse_int_list(optarg, "internal devs", 10, ',');

      config.internal_devs.n       = int_list.n;
      config.internal_devs.devices = (uint16_t *)malloc(int_list.n * sizeof(uint16_t));
      for (int i = 0; i < int_list.n; i++) {
        if (int_list.list[i] >= nb_devices) {
          PARSE_ERROR("internal devs: device %lu >= nb_devices (%u)\n", int_list.list[i], nb_devices);
        }
        config.internal_devs.devices[i] = int_list.list[i];
      }
    } break;

    case 'r': {
      uint16_t nb_devices        = rte_eth_dev_count_avail();
      struct int_list_t int_list = nf_util_parse_int_list(optarg, "fwd rule", 10, ',');

      if (int_list.n != 2) {
        PARSE_ERROR("fwd rule: expected 2 devices, got %lu\n", int_list.n);
      }

      config.fwd_rules.n++;
      config.fwd_rules.src_dev = (uint16_t *)realloc(config.fwd_rules.src_dev, config.fwd_rules.n * sizeof(uint16_t));
      config.fwd_rules.dst_dev = (uint16_t *)realloc(config.fwd_rules.dst_dev, config.fwd_rules.n * sizeof(uint16_t));

      uint16_t src_dev = int_list.list[0];
      uint16_t dst_dev = int_list.list[1];

      if (src_dev >= nb_devices) {
        PARSE_ERROR("fwd rule: src device %u >= nb_devices (%u)\n", src_dev, nb_devices);
      }

      if (dst_dev >= nb_devices) {
        PARSE_ERROR("fwd rule: dst device %u >= nb_devices (%u)\n", dst_dev, nb_devices);
      }

      config.fwd_rules.src_dev[config.fwd_rules.n - 1] = int_list.list[0];
      config.fwd_rules.dst_dev[config.fwd_rules.n - 1] = int_list.list[1];
    } break;

    case 'c':
      config.capacity = nf_util_parse_int(optarg, "capacity", 10, '\0');
      if (config.capacity <= 0) {
        PARSE_ERROR("Capacity must be strictly positive.\n");
      }
      if (!is_power_of_two(config.capacity)) {
        PARSE_ERROR("Capacity must be a power of 2.\n");
      }
      break;

    case 'p':
      config.max_ports = nf_util_parse_int(optarg, "max-ports", 10, '\0');
      if (config.max_ports <= 0) {
        PARSE_ERROR("Maximum number of ports must be strictly positive.\n");
      }
      if (!is_power_of_two(config.max_ports)) {
        PARSE_ERROR("Maximum number of ports must be a power of 2.\n");
      }
      break;

    case 't':
      config.expiration_time = nf_util_parse_int(optarg, "expire", 10, '\0');
      if (config.expiration_time <= 0) {
        PARSE_ERROR("Expiration time must be strictly positive.\n");
      }
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
          "\t--internal-devs <dev1,dev2,...>: set devices to be internal.\n"
          "\t--fwd-rule <src,dst>: set forwarding rule.\n"
          "\t--capacity <capacity>: maximum number of concurrent sources,"
          " default: %" PRIu32 ".\n"
          "\t--max-ports <max-ports>: maximum allowed number of touched ports,"
          " default: %" PRIu32 ".\n"
          "\t--expire <time>: source expiration time (us).\n"
          " default: %" PRIu32 ".\n",
          DEFAULT_CAPACITY, DEFAULT_MAX_PORTS, DEFAULT_EXPIRATION_TIME);
}

void nf_config_print(void) {
  NF_INFO("\n--- Port Scanner Detector Config ---\n");

  NF_INFO("Internals devs:");
  for (size_t i = 0; i < config.internal_devs.n; i++) {
    NF_INFO("\t%" PRIu16, config.internal_devs.devices[i]);
  }

  NF_INFO("Forwarding rules:");
  for (size_t i = 0; i < config.fwd_rules.n; i++) {
    NF_INFO("\t%" PRIu16 " -> %" PRIu16, config.fwd_rules.src_dev[i], config.fwd_rules.dst_dev[i]);
  }

  NF_INFO("Capacity: %" PRIu32, config.capacity);
  NF_INFO("Max ports: %" PRIu32, config.max_ports);
  NF_INFO("Expiration time: %" PRIu32, config.expiration_time);

  NF_INFO("\n--- ------ ------ ---\n");
}
