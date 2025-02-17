#include "config.h"

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "nf-util.h"
#include "nf-log.h"

const uint32_t DEFAULT_MAX_FLOWS               = 65536;
const uint16_t DEFAULT_MAX_CLIENTS             = 60;
const uint64_t DEFAULT_EXPIRATION_TIME         = 1000000; // 1s
const uint32_t DEFAULT_SKETCH_HEIGHT           = 4;
const uint32_t DEFAULT_SKETCH_WIDTH            = 1024;
const uint64_t DEFAULT_SKETCH_CLEANUP_INTERVAL = 10000000; // 10s

#define PARSE_ERROR(format, ...)                                                                                                           \
  nf_config_usage();                                                                                                                       \
  fprintf(stderr, format, ##__VA_ARGS__);                                                                                                  \
  exit(EXIT_FAILURE);

void nf_config_init(int argc, char **argv) {
  config.fwd_rules.n = 0;

  // Set the default values
  config.max_flows               = DEFAULT_MAX_FLOWS;
  config.max_clients             = DEFAULT_MAX_CLIENTS;
  config.expiration_time         = DEFAULT_EXPIRATION_TIME;
  config.sketch_height           = DEFAULT_SKETCH_HEIGHT;
  config.sketch_width            = DEFAULT_SKETCH_WIDTH;
  config.sketch_cleanup_interval = DEFAULT_SKETCH_CLEANUP_INTERVAL;

  unsigned nb_devices = rte_eth_dev_count_avail();

  struct option long_options[] = {{"internal-devs", required_argument, NULL, 'd'},
                                  {"fwd-rule", required_argument, NULL, 'r'},
                                  {"max-flows", required_argument, NULL, 'f'},
                                  {"max-clients", required_argument, NULL, 'c'},
                                  {"expire", required_argument, NULL, 't'},
                                  {"sketch-height", required_argument, NULL, 'h'},
                                  {"sketch-width", required_argument, NULL, 'W'},
                                  {"sketch-cleanup-interval", required_argument, NULL, 'C'},
                                  {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "d:r:f:c:t:h:W:C:", long_options, NULL)) != EOF) {
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

    case 'f':
      config.max_flows = nf_util_parse_int(optarg, "max-flows", 10, '\0');
      if ((config.max_flows & (config.max_flows - 1)) != 0) {
        PARSE_ERROR("Maximum number of flows must be a power of 2.\n");
      }
      break;

    case 'c':
      config.max_clients = nf_util_parse_int(optarg, "max-clients", 10, '\0');
      break;

    case 't':
      config.expiration_time = nf_util_parse_int(optarg, "expire", 10, '\0');
      break;

    case 'h':
      config.sketch_height = nf_util_parse_int(optarg, "sketch-height", 10, '\0');
      break;

    case 'W':
      config.sketch_width = nf_util_parse_int(optarg, "sketch-width", 10, '\0');
      break;

    case 'C':
      config.sketch_cleanup_interval = nf_util_parse_int(optarg, "sketch-cleanup-interval", 10, '\0');
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
          "\t--max-flows <max-flows>: maximum number of flows,"
          " default: %" PRIu32 ".\n"
          "\t--max-clients <max-clients>: maximum allowed number of clients,"
          " default: %" PRIu16 ".\n"
          "\t--expire <time>: expiration time (us),"
          " default: %" PRIu64 ".\n"
          "\t--sketch-height <height>: CMS height,"
          " default: %" PRIu32 ".\n"
          "\t--sketch-width <width>: CMS width,"
          " default: %" PRIu32 ".\n"
          "\t--sketch-cleanup-interval <interval>: CMS cleanup interval (us),"
          " default: %" PRIu64 ".\n",
          DEFAULT_MAX_FLOWS, DEFAULT_MAX_CLIENTS, DEFAULT_EXPIRATION_TIME, DEFAULT_SKETCH_HEIGHT, DEFAULT_SKETCH_WIDTH,
          DEFAULT_SKETCH_CLEANUP_INTERVAL);
}

void nf_config_print(void) {
  NF_INFO("\n--- Connection Limiter Config ---\n");

  NF_INFO("Internals devs:");
  for (size_t i = 0; i < config.internal_devs.n; i++) {
    NF_INFO("\t%" PRIu16, config.internal_devs.devices[i]);
  }

  NF_INFO("Forwarding rules:");
  for (size_t i = 0; i < config.fwd_rules.n; i++) {
    NF_INFO("\t%" PRIu16 " -> %" PRIu16, config.fwd_rules.src_dev[i], config.fwd_rules.dst_dev[i]);
  }

  NF_INFO("Max flows: %" PRIu32, config.max_flows);
  NF_INFO("Max clients: %" PRIu16, config.max_clients);
  NF_INFO("Expiration time: %" PRIu64, config.expiration_time);
  NF_INFO("CMS sketch height: %" PRIu32, config.sketch_height);
  NF_INFO("CMS sketch width: %" PRIu32, config.sketch_width);
  NF_INFO("CMS sketch cleanup interval: %" PRIu64, config.sketch_cleanup_interval);

  NF_INFO("\n--- ------ ------ ---\n");
}
