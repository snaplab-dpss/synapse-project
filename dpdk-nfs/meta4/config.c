#include "config.h"

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "nf-parse.h"
#include "nf-util.h"
#include "nf-log.h"

// Reduces a dotted pattern to the key a name on the wire produces, and to how much of that key the
// pattern spells out. Labels go in reversed, top-level first, so the wildcards -- which only ever
// lead a pattern -- land at the end of the key, past where the prefix reaches.
bool domain_name_from_str(const char *str, struct dns_name *name, uint32_t *prefix_bits) {
  memset(name, 0, sizeof(struct dns_name));

  struct dns_label spelled[DNS_MAX_LABELS];
  size_t labels    = 0;
  size_t wildcards = 0;
  size_t i         = 0;

  while (str[i] != '\0') {
    if (labels >= DNS_MAX_LABELS) {
      return false;
    }

    size_t start = i;
    while (str[i] != '\0' && str[i] != '.') {
      i++;
    }

    size_t len = i - start;
    if (len == 0) {
      return false;
    }

    if (len == 1 && str[start] == '*') {
      // A wildcard stands for any label, and only leads: one that follows a spelled-out label
      // would need the key to say which slots were skipped, which it deliberately does not.
      if (labels != wildcards) {
        return false;
      }
      wildcards++;
      memset(&spelled[labels], 0, sizeof(struct dns_label));
    } else {
      memset(&spelled[labels], 0, sizeof(struct dns_label));
      spelled[labels].len = (uint8_t)len;
      memcpy(spelled[labels].bytes, &str[start], len < DNS_LABEL_BYTES ? len : DNS_LABEL_BYTES);
    }

    labels++;

    if (str[i] == '.') {
      i++;
    }
  }

  if (labels == 0) {
    return false;
  }

  for (size_t l = 0; l < labels; l++) {
    name->label[labels - 1 - l] = spelled[l]; // top-level label first
  }

  name->labels = (uint8_t)labels;
  *prefix_bits = DNS_NAME_PREFIX_BITS(labels - wildcards);

  return true;
}

const uint32_t DEFAULT_DRT_CAPACITY        = 65536;
const uint64_t DEFAULT_DRT_EXPIRATION_TIME = 1000000; // 1 s, in us
const uint32_t DEFAULT_NUM_KNOWN_DOMAINS   = 2048;

#define PARSE_ERROR(format, ...)                                                                                                                     \
  nf_config_usage();                                                                                                                                 \
  fprintf(stderr, format, ##__VA_ARGS__);                                                                                                            \
  exit(EXIT_FAILURE);

void nf_config_init(int argc, char **argv) {
  config.drt_capacity        = DEFAULT_DRT_CAPACITY;
  config.drt_expiration_time = DEFAULT_DRT_EXPIRATION_TIME;
  config.num_known_domains   = DEFAULT_NUM_KNOWN_DOMAINS;
  config.domains.n           = 0;
  config.domains.names       = NULL;
  config.domains.prefix_bits = NULL;
  config.ignored_clients.n   = 0;
  config.ignored_clients.prefix    = NULL;
  config.ignored_clients.prefixlen = NULL;

  struct option long_options[] = {{"drt-capacity", required_argument, NULL, 'c'},
                                  {"drt-expire", required_argument, NULL, 't'},
                                  {"known-domains", required_argument, NULL, 'd'},
                                  {"domain", required_argument, NULL, 'D'},
                                  {"ignore-client", required_argument, NULL, 'i'},
                                  {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "c:t:d:D:i:", long_options, NULL)) != EOF) {
    switch (opt) {
    case 'c':
      config.drt_capacity = nf_util_parse_int(optarg, "drt-capacity", 10, '\0');
      if (config.drt_capacity == 0) {
        PARSE_ERROR("drt-capacity must be strictly positive\n");
      }
      break;

    case 't':
      config.drt_expiration_time = nf_util_parse_int(optarg, "drt-expire", 10, '\0');
      if (config.drt_expiration_time == 0) {
        PARSE_ERROR("drt-expire must be strictly positive\n");
      }
      break;

    case 'd':
      config.num_known_domains = nf_util_parse_int(optarg, "known-domains", 10, '\0');
      if (config.num_known_domains == 0) {
        PARSE_ERROR("known-domains must be strictly positive\n");
      }
      break;

    case 'D': {
      struct dns_name name;
      uint32_t prefix_bits;
      if (!domain_name_from_str(optarg, &name, &prefix_bits)) {
        PARSE_ERROR("domain: `%s` is not a name of at most %d labels, wildcards leading\n", optarg, DNS_MAX_LABELS);
      }

      config.domains.names       = (struct dns_name *)realloc(config.domains.names, (config.domains.n + 1) * sizeof(struct dns_name));
      config.domains.prefix_bits = (uint32_t *)realloc(config.domains.prefix_bits, (config.domains.n + 1) * sizeof(uint32_t));
      if (config.domains.names == NULL || config.domains.prefix_bits == NULL) {
        PARSE_ERROR("domain: out of memory\n");
      }

      config.domains.names[config.domains.n]       = name;
      config.domains.prefix_bits[config.domains.n] = prefix_bits;
      config.domains.n++;
    } break;

    case 'i': {
      char *slash = strchr(optarg, '/');
      if (slash == NULL) {
        PARSE_ERROR("ignore-client: `%s` is not <address>/<prefix length>\n", optarg);
      }

      *slash = '\0';
      uint32_t prefix;
      if (!nf_parse_ipv4addr(optarg, &prefix)) {
        PARSE_ERROR("ignore-client: `%s` is not an IPv4 address\n", optarg);
      }

      uintmax_t prefixlen = nf_util_parse_int(slash + 1, "ignore-client prefix length", 10, '\0');
      if (prefixlen > 32) {
        PARSE_ERROR("ignore-client: prefix length %ju is above 32\n", prefixlen);
      }

      size_t n = config.ignored_clients.n;
      config.ignored_clients.prefix    = (uint32_t *)realloc(config.ignored_clients.prefix, (n + 1) * sizeof(uint32_t));
      config.ignored_clients.prefixlen = (uint8_t *)realloc(config.ignored_clients.prefixlen, (n + 1) * sizeof(uint8_t));
      if (config.ignored_clients.prefix == NULL || config.ignored_clients.prefixlen == NULL) {
        PARSE_ERROR("ignore-client: out of memory\n");
      }

      config.ignored_clients.prefix[n]    = prefix;
      config.ignored_clients.prefixlen[n] = (uint8_t)prefixlen;
      config.ignored_clients.n            = n + 1;
    } break;

    default:
      PARSE_ERROR("unknown option `%c`\n", opt);
    }
  }
}

void nf_config_usage(void) {
  NF_INFO("Usage:\n"
          "[DPDK EAL options] --\n"
          "\t--drt-capacity <n>: entries in the DNS Response Table.\n"
          "\t--drt-expire <us>: DNS Response Table entry timeout, in microseconds.\n"
          "\t--known-domains <n>: size of the watched-domain list.\n"
          "\t--domain <name>: watch this domain name; repeat for more.\n"
          "\t--ignore-client <addr>/<len>: leave this client prefix out of the accounting.\n");
}

void nf_config_print(void) {
  NF_INFO("\n--- Meta4 Config ---\n");
  NF_INFO("DRT capacity:   %" PRIu32, config.drt_capacity);
  NF_INFO("DRT expiration: %" PRIu64 " us", config.drt_expiration_time);
  NF_INFO("Known domains:  %" PRIu32, config.num_known_domains);
  NF_INFO("Watching:       %zu name(s)", config.domains.n);
  NF_INFO("Ignoring:       %zu client prefix(es)", config.ignored_clients.n);
  NF_INFO("\n--- --- ------ ---\n");
}
