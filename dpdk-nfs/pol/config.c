#include "config.h"

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "nf-util.h"
#include "nf-log.h"

const uint64_t DEFAULT_RATE     = 1000000; // 1MB/s
const uint64_t DEFAULT_BURST    = 100000;  // 100kB
const uint32_t DEFAULT_CAPACITY = 128;     // IPs

#define PARSE_ERROR(format, ...)                                                                                                           \
  nf_config_usage();                                                                                                                       \
  fprintf(stderr, format, ##__VA_ARGS__);                                                                                                  \
  exit(EXIT_FAILURE);

void nf_config_init(int argc, char **argv) {
  config.fwd_rules.n = 0;

  // Set the default values
  config.rate         = DEFAULT_RATE;     // B/s
  config.burst        = DEFAULT_BURST;    // B
  config.dyn_capacity = DEFAULT_CAPACITY; // MAC addresses

  unsigned nb_devices = rte_eth_dev_count_avail();

  struct option long_options[] = {{"internal-devs", required_argument, NULL, 'd'}, {"fwd-rule", required_argument, NULL, 'f'},
                                  {"rate", required_argument, NULL, 'r'},          {"burst", required_argument, NULL, 'b'},
                                  {"capacity", required_argument, NULL, 'c'},      {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "d:f:r:b:c:", long_options, NULL)) != EOF) {
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

    case 'f': {
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

    case 'r':
      config.rate = nf_util_parse_int(optarg, "rate", 10, '\0');
      if (config.rate == 0) {
        PARSE_ERROR("Policer rate must be strictly positive.\n");
      }
      break;

    case 'b':
      config.burst = nf_util_parse_int(optarg, "burst", 10, '\0');
      if (config.burst == 0) {
        PARSE_ERROR("Policer burst size must be strictly positive.\n");
      }
      break;

    case 'c':
      config.dyn_capacity = nf_util_parse_int(optarg, "capacity", 10, '\0');
      if (config.dyn_capacity <= 0) {
        PARSE_ERROR("Flow table size must be strictly positive.\n");
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
          "\t--rate <rate>: policer rate in bytes/s,"
          " default: %" PRIu64 ".\n"
          "\t--burst <size>: policer burst size in bytes,"
          " default: %" PRIu64 ".\n"
          "\t--capacity <n>: policer table capacity,"
          " default: %" PRIu32 ".\n",
          DEFAULT_RATE, DEFAULT_BURST, DEFAULT_CAPACITY);
}

void nf_config_print(void) {
  NF_INFO("\n--- Policer Config ---\n");

  NF_INFO("Internals devs:");
  for (size_t i = 0; i < config.internal_devs.n; i++) {
    NF_INFO("\t%" PRIu16, config.internal_devs.devices[i]);
  }

  NF_INFO("Forwarding rules:");
  for (size_t i = 0; i < config.fwd_rules.n; i++) {
    NF_INFO("\t%" PRIu16 " -> %" PRIu16, config.fwd_rules.src_dev[i], config.fwd_rules.dst_dev[i]);
  }

  NF_INFO("Rate: %" PRIu64, config.rate);
  NF_INFO("Burst: %" PRIu64, config.burst);
  NF_INFO("Capacity: %" PRIu16, config.dyn_capacity);

  NF_INFO("\n--- ------ ------ ---\n");
}
