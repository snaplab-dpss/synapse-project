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

const uint16_t DEFAULT_INTERNAL = 1;
const uint16_t DEFAULT_EXTERNAL = 0;
const uint32_t DEFAULT_CAPACITY = 65536;
const uint32_t DEFAULT_SAMPLE_SIZE = 100;
const uint32_t DEFAULT_SKETCH_HEIGHT = 4;
const uint32_t DEFAULT_SKETCH_WIDTH = 1024;
const uint64_t DEFAULT_SKETCH_CLEANUP_INTERVAL = 10000000; // 10s
const uint64_t DEFAULT_HH_THRESHOLD = 10000;

void nf_config_init(int argc, char **argv) {
  // Set the default values
  config.internal_device = DEFAULT_INTERNAL;
  config.external_device = DEFAULT_EXTERNAL;
  config.capacity = DEFAULT_CAPACITY;
  config.sample_size = DEFAULT_SAMPLE_SIZE;
  config.sketch_height = DEFAULT_SKETCH_HEIGHT;
  config.sketch_width = DEFAULT_SKETCH_WIDTH;
  config.sketch_cleanup_interval = DEFAULT_SKETCH_CLEANUP_INTERVAL;
  config.hh_threshold = DEFAULT_HH_THRESHOLD;

  uint16_t nb_devices = rte_eth_dev_count_avail();

  struct option long_options[] = {
      {"int", required_argument, NULL, 'i'},
      {"ext", required_argument, NULL, 'e'},
      {"capacity", required_argument, NULL, 'c'},
      {"sample-size", required_argument, NULL, 's'},
      {"sketch-height", required_argument, NULL, 'h'},
      {"sketch-width", required_argument, NULL, 'W'},
      {"sketch-cleanup-interval", required_argument, NULL, 'C'},
      {"hh-threshold", required_argument, NULL, 'T'},
      {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "i:e:c:s:h:W:C:T:", long_options,
                            NULL)) != EOF) {
    unsigned device;
    switch (opt) {
    case 'i':
      config.internal_device = nf_util_parse_int(optarg, "int", 10, '\0');
      if (config.internal_device >= nb_devices) {
        PARSE_ERROR("Invalid internal device.\n");
      }
      break;

    case 'e':
      config.external_device = nf_util_parse_int(optarg, "ext", 10, '\0');
      if (config.external_device >= nb_devices) {
        PARSE_ERROR("Invalid external device.\n");
      }
      break;

    case 'c':
      config.capacity = nf_util_parse_int(optarg, "capacity", 10, '\0');
      if ((config.capacity & (config.capacity - 1)) != 0) {
        PARSE_ERROR("Capacity must be a power of 2.\n");
      }
      break;

    case 's':
      config.sample_size = nf_util_parse_int(optarg, "sample-size", 10, '\0');
      if (config.sample_size >= config.capacity) {
        PARSE_ERROR("Sample size must be less than capacity.\n");
      }
      break;

    case 'h':
      config.sketch_height =
          nf_util_parse_int(optarg, "sketch-height", 10, '\0');
      break;

    case 'W':
      config.sketch_width = nf_util_parse_int(optarg, "sketch-width", 10, '\0');
      break;

    case 'C':
      config.sketch_cleanup_interval =
          nf_util_parse_int(optarg, "sketch-cleanup-interval", 10, '\0');
      break;

    case 'T':
      config.hh_threshold = nf_util_parse_int(optarg, "hh-threshold", 10, '\0');
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
          "\t--int <device>: internal device,"
          " default: %" PRIu16 ".\n"
          "\t--ext <device>: external device,"
          " default: %" PRIu16 ".\n"
          "\t--capacity <n>: capacity,"
          " default: %" PRIu32 ".\n"
          "\t--sample-size <n>: sample size,"
          " default: %" PRIu32 ".\n"
          "\t--sketch-height <height>: CMS height,"
          " default: %" PRIu32 ".\n"
          "\t--sketch-width <width>: CMS width,"
          " default: %" PRIu32 ".\n"
          "\t--sketch-cleanup-interval <interval>: CMS cleanup interval (us),"
          " default: %" PRIu64 ".\n"
          "\t--hh-threshold <threshold>: heavy hitter threshold,"
          " default: %" PRIu64 ".\n",
          DEFAULT_INTERNAL, DEFAULT_EXTERNAL, DEFAULT_CAPACITY,
          DEFAULT_SAMPLE_SIZE, DEFAULT_SKETCH_HEIGHT, DEFAULT_SKETCH_WIDTH,
          DEFAULT_SKETCH_CLEANUP_INTERVAL, DEFAULT_HH_THRESHOLD);
}

void nf_config_print(void) {
  NF_INFO("\n--- KVS Config ---\n");

  NF_INFO("Internal Device: %" PRIu16, config.internal_device);
  NF_INFO("External Device: %" PRIu16, config.external_device);
  NF_INFO("Capacity: %" PRIu32, config.capacity);
  NF_INFO("Sample Size: %" PRIu32, config.sample_size);
  NF_INFO("Sketch Height: %" PRIu32, config.sketch_height);
  NF_INFO("Sketch Width: %" PRIu32, config.sketch_width);
  NF_INFO("Sketch Cleanup Interval: %" PRIu64, config.sketch_cleanup_interval);
  NF_INFO("Heavy Hitter Threshold: %" PRIu64, config.hh_threshold);

  NF_INFO("\n--- --- ------ ---\n");
}
