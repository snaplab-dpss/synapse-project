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

void nf_config_init(int argc, char **argv) {}

void nf_config_usage(void) {
  NF_INFO("Usage:\n"
          "[DPDK EAL options]\n");
}

void nf_config_print(void) {
  NF_INFO("\n--- Echo Config ---\n");

  NF_INFO("\n--- --- ------ ---\n");
}
