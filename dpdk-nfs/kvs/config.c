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
  config.lpm_rules.n = 0;

  struct option long_options[] = {{"lpm-entry", required_argument, NULL, 'e'},  {"server", required_argument, NULL, 'i'},
                                  {"lpm-config", required_argument, NULL, 'l'}, {"capacity", required_argument, NULL, 'c'},
                                  {"expire", required_argument, NULL, 't'},     {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "e:i:l:c:t:", long_options, NULL)) != EOF) {
    switch (opt) {
    case 'e': {
      uint16_t nb_devices = rte_eth_dev_count_avail();

      uint32_t subnet;
      uint8_t a, b, c, d;
      uint8_t mask;
      uint16_t dst_dev;

      if (sscanf(optarg, "%hhu.%hhu.%hhu.%hhu/%hhu,%hu", &a, &b, &c, &d, &mask, &dst_dev) != 6) {
        PARSE_ERROR("lpm rule: invalid format\n");
      }

      subnet = ((uint32_t)a << 0) | ((uint32_t)b << 8) | ((uint32_t)c << 16) | ((uint32_t)d << 24);

      if (dst_dev >= nb_devices) {
        PARSE_ERROR("lpm rule: dst device %u >= nb_devices (%u)\n", dst_dev, nb_devices);
      }

      if (mask > 32) {
        PARSE_ERROR("lpm rule: invalid mask %u\n", mask);
      }

      config.lpm_rules.n++;
      config.lpm_rules.subnet  = (uint32_t *)realloc(config.lpm_rules.subnet, config.lpm_rules.n * sizeof(uint32_t));
      config.lpm_rules.mask    = (uint8_t *)realloc(config.lpm_rules.mask, config.lpm_rules.n * sizeof(uint8_t));
      config.lpm_rules.dst_dev = (uint16_t *)realloc(config.lpm_rules.dst_dev, config.lpm_rules.n * sizeof(uint16_t));

      config.lpm_rules.subnet[config.lpm_rules.n - 1]  = subnet;
      config.lpm_rules.mask[config.lpm_rules.n - 1]    = mask;
      config.lpm_rules.dst_dev[config.lpm_rules.n - 1] = dst_dev;
    } break;

    case 'i':
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
          "\t--lpm-entry <ipv4_addr/mask,dst_dev>: add LPM forwarding rule.\n"
          "\t--capacity <n>: capacity.\n"
          "\t--expire <interval>: Expiration time (us).\n");
}

void nf_config_print(void) {
  NF_INFO("\n--- KVS Config ---\n");

  NF_INFO("LPM entries:");
  for (size_t i = 0; i < config.lpm_rules.n; i++) {
    NF_INFO("\t%s/%hhu -> %" PRIu16, nf_rte_ipv4_to_str(config.lpm_rules.subnet[i]), config.lpm_rules.mask[i], config.lpm_rules.dst_dev[i]);
  }

  NF_INFO("Server Device: %" PRIu16, config.server_dev);
  NF_INFO("Capacity: %" PRIu32, config.capacity);
  NF_INFO("Expiration time: %" PRIu32 "us", config.expiration_time_us);

  NF_INFO("\n--- --- ------ ---\n");
}
