#include "config.h"
#include "nf-parse.h"

#include <getopt.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#include "nf.h"
#include "nf-util.h"
#include "nf-log.h"

#define PARSE_ERROR(format, ...)                                                                                                                     \
  nf_config_usage();                                                                                                                                 \
  fprintf(stderr, format, ##__VA_ARGS__);                                                                                                            \
  exit(EXIT_FAILURE);

#define DEFAULT_NUM_ESTIMATORS 64
#define DEFAULT_SCALING 20

// Width in bits of the hash: its top log(m) bits pick the estimator, the rest
// feed the trailing-zero rank.
#define HASH_BITS 32
#define LN2 0.6931471805599453
// HyperLogLog bias-correction constant (Flajolet et al.): alpha_m = 0.7213 / (1 + 1.079/m).
#define ALPHA(m) (0.7213 / (1.0 + 1.079 / (m)))
// Linear counting replaces the estimate when it falls below 2.5 * m.
#define LC_THRESHOLD(m) (5 * (m) / 2)

void nf_config_init(int argc, char **argv) {
  config.num_estimators = DEFAULT_NUM_ESTIMATORS;
  config.scaling        = DEFAULT_SCALING;

  struct option long_options[] = {
      {"estimators", required_argument, NULL, 'e'},
      {"scaling", required_argument, NULL, 's'},
      {NULL, 0, NULL, 0},
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "e:s:", long_options, NULL)) != EOF) {
    switch (opt) {
    case 'e':
      config.num_estimators = nf_util_parse_int(optarg, "estimators", 10, '\0');
      break;
    case 's':
      config.scaling = nf_util_parse_int(optarg, "scaling", 10, '\0');
      break;
    default:
      PARSE_ERROR("Unknown option.\n");
    }
  }

  uint32_t log_num_estimators = 0;
  for (uint32_t n = config.num_estimators; n > 1; n >>= 1) {
    log_num_estimators++;
  }

  config.log_num_estimators = log_num_estimators;
  config.hash_mask          = (1u << (HASH_BITS - log_num_estimators)) - 1;
  config.offset             = config.num_estimators << config.scaling; // m * 2^scaling

  // Numerator of the HLL estimate alpha * m^2 / sum(2^-rank), kept in fixed point.
  config.magnify_factor = (uint32_t)(ALPHA(config.num_estimators) * config.num_estimators * config.num_estimators * (double)(1u << config.scaling));

  config.lc_offset    = (uint32_t)(config.num_estimators * config.log_num_estimators * LN2); // m * ln(m)
  config.lc_threshold = LC_THRESHOLD(config.num_estimators);

  optind = 1;
}

void nf_config_usage(void) {
  NF_INFO("Usage:\n"
          "[DPDK EAL options] --\n"
          "\t--estimators <n>: number of HyperLogLog estimators (power of two).\n"
          "\t--scaling <s>: fixed-point scaling for inverse probabilities.\n");
}

void nf_config_print(void) {
  NF_INFO("\n--- HyperLogLog Config ---\n");
  NF_INFO("Estimators: %" PRIu32, config.num_estimators);
  NF_INFO("Scaling: %" PRIu32, config.scaling);
  NF_INFO("\n--- --- ------ ---\n");
}
