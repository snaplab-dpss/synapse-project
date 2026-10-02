#include "hhh_config.h"

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "nf-util.h"
#include "nf-log.h"
#include "lib/util/math.h"

const uint32_t DEFAULT_STAGES   = 6;
const uint32_t DEFAULT_WIDTH    = 1024;
const uint64_t DEFAULT_INTERVAL = 1000000; // 1 s

#define PARSE_ERROR(format, ...)                                                                                                                     \
  nf_config_usage();                                                                                                                                 \
  fprintf(stderr, format, ##__VA_ARGS__);                                                                                                            \
  exit(EXIT_FAILURE);

void nf_config_init(int argc, char **argv) {
  config.fwd_rules.n = 0;

  // Set the default values
  config.stages   = DEFAULT_STAGES;
  config.width    = DEFAULT_WIDTH;
  config.interval = DEFAULT_INTERVAL;

  unsigned nb_devices = rte_eth_dev_count_avail();

  struct option long_options[] = {{"internal-devs", required_argument, NULL, 'd'}, {"fwd-rule", required_argument, NULL, 'f'},
                                  {"stages", required_argument, NULL, 's'},        {"width", required_argument, NULL, 'w'},
                                  {"interval", required_argument, NULL, 'i'},      {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "d:f:s:w:i:", long_options, NULL)) != EOF) {
    switch (opt) {
    case 'd': {
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

      config.fwd_rules.src_dev[config.fwd_rules.n - 1] = src_dev;
      config.fwd_rules.dst_dev[config.fwd_rules.n - 1] = dst_dev;
    } break;

    case 's':
      config.stages = nf_util_parse_int(optarg, "stages", 10, '\0');
      if (config.stages == 0 || config.stages > CRC32_BANK_SIZE) {
        PARSE_ERROR("Stages must be between 1 and %d.\n", CRC32_BANK_SIZE);
      }
      break;

    case 'w':
      config.width = nf_util_parse_int(optarg, "width", 10, '\0');
      if (config.width == 0 || (config.width & (config.width - 1)) != 0) {
        PARSE_ERROR("Width must be a power of two.\n");
      }
      break;

    case 'i':
      config.interval = nf_util_parse_int(optarg, "interval", 10, '\0');
      if (config.interval == 0) {
        PARSE_ERROR("Interval must be strictly positive.\n");
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
          "\t--internal-devs <dev1,dev2,...>: set devices to be internal (LAN); the others are WAN, whose sources are monitored.\n"
          "\t--fwd-rule <src,dst>: set forwarding rule.\n"
          "\t--stages <n>: stages of the /24 HashPipe,"
          " default: %" PRIu32 ".\n"
          "\t--width <slots>: slots per HashPipe stage (a power of two),"
          " default: %" PRIu32 ".\n"
          "\t--interval <us>: counting interval,"
          " default: %" PRIu64 ".\n",
          DEFAULT_STAGES, DEFAULT_WIDTH, DEFAULT_INTERVAL);
}

void nf_config_print(void) {
  NF_INFO("\n--- Hierarchical Heavy Hitter Config ---\n");

  NF_INFO("Internals devs:");
  for (size_t i = 0; i < config.internal_devs.n; i++) {
    NF_INFO("\t%" PRIu16, config.internal_devs.devices[i]);
  }

  NF_INFO("Forwarding rules:");
  for (size_t i = 0; i < config.fwd_rules.n; i++) {
    NF_INFO("\t%" PRIu16 " -> %" PRIu16, config.fwd_rules.src_dev[i], config.fwd_rules.dst_dev[i]);
  }
  NF_INFO("Stages: %" PRIu32, config.stages);
  NF_INFO("Width: %" PRIu32, config.width);
  NF_INFO("Interval: %" PRIu64 " us", config.interval);

  NF_INFO("\n--- ------ ------ ---\n");
}
