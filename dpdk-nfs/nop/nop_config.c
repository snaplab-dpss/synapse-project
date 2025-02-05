#include "nop_config.h"
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
  uint16_t nb_devices = rte_eth_dev_count_avail();

  struct option long_options[] = {{NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "", long_options, NULL)) != EOF) {
    unsigned device;
    switch (opt) {
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
          "[DPDK EAL options] --\n");
}

void nf_config_print(void) {
  NF_INFO("\n--- NOP Config ---\n");

  NF_INFO("\n--- --- ------ ---\n");
}
