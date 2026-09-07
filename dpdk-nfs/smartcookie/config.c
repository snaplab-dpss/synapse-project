#include "config.h"

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "nf-util.h"
#include "nf-log.h"
#include "halfsiphash.h"

const uint16_t DEFAULT_SERVER_DEV          = 0;
const uint32_t DEFAULT_SIP_KEY_0           = 0x33323130; // upstream's default key
const uint32_t DEFAULT_SIP_KEY_1           = 0x42413938;
const uint32_t DEFAULT_BLOOM_FILTER_HEIGHT = 2;
const uint32_t DEFAULT_BLOOM_FILTER_WIDTH  = 1048576; // 2^20 bits per array, the paper's size (see tofino/smartcookie/README.md)

#define PARSE_ERROR(format, ...)                                                                                                                     \
  nf_config_usage();                                                                                                                                 \
  fprintf(stderr, format, ##__VA_ARGS__);                                                                                                            \
  exit(EXIT_FAILURE);

void nf_config_init(int argc, char **argv) {
  config.server_dev          = DEFAULT_SERVER_DEV;
  config.sip_key_0           = DEFAULT_SIP_KEY_0;
  config.sip_key_1           = DEFAULT_SIP_KEY_1;
  config.bloom_filter_height = DEFAULT_BLOOM_FILTER_HEIGHT;
  config.bloom_filter_width  = DEFAULT_BLOOM_FILTER_WIDTH;

  unsigned nb_devices = rte_eth_dev_count_avail();

  struct option long_options[] = {{"server-dev", required_argument, NULL, 's'}, {"sip-key0", required_argument, NULL, 'k'},
                                  {"sip-key1", required_argument, NULL, 'K'},   {"bf-height", required_argument, NULL, 'h'},
                                  {"bf-width", required_argument, NULL, 'w'},   {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "s:k:K:h:w:", long_options, NULL)) != EOF) {
    switch (opt) {
    case 's':
      config.server_dev = nf_util_parse_int(optarg, "server-dev", 10, '\0');
      if (config.server_dev >= nb_devices) {
        PARSE_ERROR("server-dev: device %u >= nb_devices (%u)\n", config.server_dev, nb_devices);
      }
      break;

    case 'k':
      config.sip_key_0 = nf_util_parse_int(optarg, "sip-key0", 10, '\0');
      break;

    case 'K':
      config.sip_key_1 = nf_util_parse_int(optarg, "sip-key1", 10, '\0');
      break;

    case 'h':
      config.bloom_filter_height = nf_util_parse_int(optarg, "bf-height", 10, '\0');
      break;

    case 'w':
      config.bloom_filter_width = nf_util_parse_int(optarg, "bf-width", 10, '\0');
      break;

    default:
      PARSE_ERROR("Unknown option %c", opt);
    }
  }

  halfsiphash_init(config.sip_init, config.sip_key_0, config.sip_key_1);

  // Reset getopt
  optind = 1;
}

void nf_config_usage(void) {
  NF_INFO("Usage:\n"
          "[DPDK EAL options] --\n"
          "\t--server-dev <dev>: device facing the protected server (all others face clients),"
          " default: %" PRIu16 ".\n"
          "\t--sip-key0 <key>: first HalfSipHash key word (decimal),"
          " default: %" PRIu32 ".\n"
          "\t--sip-key1 <key>: second HalfSipHash key word (decimal),"
          " default: %" PRIu32 ".\n"
          "\t--bf-height <height>: Bloom Filter height,"
          " default: %" PRIu32 ".\n"
          "\t--bf-width <width>: Bloom Filter width,"
          " default: %" PRIu32 ".\n",
          DEFAULT_SERVER_DEV, DEFAULT_SIP_KEY_0, DEFAULT_SIP_KEY_1, DEFAULT_BLOOM_FILTER_HEIGHT, DEFAULT_BLOOM_FILTER_WIDTH);
}

void nf_config_print(void) {
  NF_INFO("\n--- SmartCookie Config ---\n");

  NF_INFO("Server dev: %" PRIu16, config.server_dev);
  NF_INFO("SipHash key: 0x%08" PRIx32 " 0x%08" PRIx32, config.sip_key_0, config.sip_key_1);
  NF_INFO("Bloom Filter height: %" PRIu32, config.bloom_filter_height);
  NF_INFO("Bloom Filter width: %" PRIu32, config.bloom_filter_width);

  NF_INFO("\n--- ------ ------ ---\n");
}
