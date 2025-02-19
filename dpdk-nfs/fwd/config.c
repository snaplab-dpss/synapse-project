#include "config.h"
#include "nf-parse.h"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

#include "nf.h"
#include "nf-util.h"
#include "nf-log.h"

#define PARSE_ERROR(format, ...)                                                                                                           \
  nf_config_usage();                                                                                                                       \
  fprintf(stderr, format, ##__VA_ARGS__);                                                                                                  \
  exit(EXIT_FAILURE);

void nf_config_init(int argc, char **argv) {
  config.fwd_rules.n = 0;

  struct option long_options[] = {{"fwd-rule", required_argument, NULL, 'r'}, {NULL, 0, NULL, 0}};
  int opt;
  while ((opt = getopt_long(argc, argv, "", long_options, NULL)) != EOF) {
    switch (opt) {
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
          "\t--fwd-rule <src,dst>: set forwarding rule.\n");
}

void nf_config_print(void) {
  NF_INFO("\n--- Fowarder Config ---\n");

  NF_INFO("Forwarding rules:");
  for (size_t i = 0; i < config.fwd_rules.n; i++) {
    NF_INFO("\t%" PRIu16 " -> %" PRIu16, config.fwd_rules.src_dev[i], config.fwd_rules.dst_dev[i]);
  }

  NF_INFO("\n--- --- ------ ---\n");
}
