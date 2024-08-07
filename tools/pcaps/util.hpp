#pragma once

#include <arpa/inet.h>
#include <assert.h>
#include <byteswap.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <pcap.h>
#include <getopt.h>

#include <iomanip>
#include <sstream>
#include <string>
#include <functional>
#include <random>

#define MIN_THROUGHPUT_GIGABIT_PER_SEC 1   // Gbps
#define MAX_THROUGHPUT_GIGABIT_PER_SEC 100 // Gbps

#define MIN_PACKET_SIZE_BYTES 8
#define MAX_PACKET_SIZE_BYTES 1500

#define TRILLION 1000000000000LLU
#define BILLION 1000000000LLU

#define THOUSAND 1000LLU
#define MILLION 1000000LLU
#define BILLION 1000000000LLU

#define bit_to_byte(B) ((B) / 8)
#define byte_to_bit(B) ((B)*8)

#define gbps_to_bps(R) ((R)*BILLION)

typedef int64_t time_s_t;
typedef int64_t time_ms_t;
typedef int64_t time_us_t;
typedef int64_t time_ns_t;
typedef int64_t time_ps_t;

#define min_to_s(T) ((T)*60)

#define s_to_min(T) ((T) / (double)(60))
#define s_to_us(T) ((T)*MILLION)
#define s_to_ns(T) ((T)*BILLION)

#define us_to_s(T) ((double)(T) / (double)(MILLION))
#define us_to_ns(T) ((T)*THOUSAND)

#define ns_to_s(T) ((double)(T) / (double)(BILLION))
#define ns_to_us(T) ((T) / THOUSAND)
#define ns_to_ps(T) ((time_ps_t)((T)*THOUSAND))

#define ps_to_ns(T) ((time_ns_t)((double)(T) / (double)THOUSAND))

#define PARSE_ERROR(argv, format, ...)                                         \
  fprintf(stderr, format, ##__VA_ARGS__);                                      \
  exit(EXIT_FAILURE);

#define PARSER_ASSERT(cond, fmt, ...)                                          \
  if (!(cond))                                                                 \
    exit(EXIT_FAILURE, fmt, ##__VA_ARGS__);

inline std::string fmt(uint64_t n) {
  auto ss = std::stringstream();
  auto rem = n % 1000;
  n /= 1000;

  if (n > 0) {
    ss << fmt(n);
    ss << "," << std::setfill('0') << std::setw(3) << rem;
  } else {
    ss << rem;
  }

  return ss.str();
}

/* Compute checksum for count bytes starting at addr, using one's complement of
 * one's complement sum*/
static unsigned short compute_checksum(unsigned short *addr,
                                       unsigned int count) {
  unsigned long sum = 0;
  while (count > 1) {
    sum += *addr++;
    count -= 2;
  }
  // if any bytes left, pad the bytes and add
  if (count > 0) {
    sum += ((*addr) & htons(0xFF00));
  }
  // Fold sum to 16 bits: add carrier to result
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }
  // one's complement
  sum = ~sum;
  return ((unsigned short)sum);
}

/* set ip checksum of a given ip header*/
inline void compute_ip_checksum(iphdr *iphdrp) {
  iphdrp->check = 0;
  iphdrp->check = compute_checksum((unsigned short *)iphdrp, iphdrp->ihl << 2);
}

/* set tcp checksum: given IP header and UDP datagram */
inline void compute_udp_checksum(iphdr *pIph, unsigned short *ipPayload) {
  unsigned long sum = 0;
  udphdr *udphdrp = (udphdr *)(ipPayload);
  unsigned short udpLen = htons(udphdrp->len);
  // printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~udp len=%dn", udpLen);
  // add the pseudo header
  // printf("add pseudo headern");
  // the source ip
  sum += (pIph->saddr >> 16) & 0xFFFF;
  sum += (pIph->saddr) & 0xFFFF;
  // the dest ip
  sum += (pIph->daddr >> 16) & 0xFFFF;
  sum += (pIph->daddr) & 0xFFFF;
  // protocol and reserved: 17
  sum += htons(IPPROTO_UDP);
  // the length
  sum += udphdrp->len;

  // add the IP payload
  // printf("add ip payloadn");
  // initialize checksum to 0
  udphdrp->check = 0;
  while (udpLen > 1) {
    sum += *ipPayload++;
    udpLen -= 2;
  }
  // if any bytes left, pad the bytes and add
  if (udpLen > 0) {
    // printf("+++++++++++++++padding: %dn", udpLen);
    sum += ((*ipPayload) & htons(0xFF00));
  }
  // Fold sum to 16 bits: add carrier to result
  // printf("add carriern");
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }
  // printf("one's complementn");
  sum = ~sum;
  // set computation result
  udphdrp->check =
      ((unsigned short)sum == 0x0000) ? 0xFFFF : (unsigned short)sum;
}

inline long get_file_size(const char *fname) {
  assert(fname);
  FILE *fp = fopen(fname, "r");

  // checking if the file exist or not
  if (fp == NULL) {
    fprintf(stderr, "File %s not found\n", fname);
    exit(1);
  }

  fseek(fp, 0L, SEEK_END);
  auto res = ftell(fp);
  fclose(fp);

  return res;
}

inline bool parse_etheraddr(const char *str, struct ether_addr *addr) {
  return sscanf(str, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX",
                addr->ether_addr_octet + 0, addr->ether_addr_octet + 1,
                addr->ether_addr_octet + 2, addr->ether_addr_octet + 3,
                addr->ether_addr_octet + 4, addr->ether_addr_octet + 5) == 6;
}

inline bool parse_ipv4addr(const char *str, uint32_t *addr) {
  uint8_t a, b, c, d;
  if (sscanf(str, "%hhu.%hhu.%hhu.%hhu", &a, &b, &c, &d) == 4) {
    *addr = ((uint32_t)a << 0) | ((uint32_t)b << 8) | ((uint32_t)c << 16) |
            ((uint32_t)d << 24);
    return true;
  }
  return false;
}

inline uintmax_t parse_int(const char *str, const char *name, int base,
                           char next) {
  char *temp;
  intmax_t result = strtoimax(str, &temp, base);

  // There's also a weird failure case with overflows, but let's not care
  if (temp == str || *temp != next) {
    PARSE_ERROR("Error while parsing '%s': %s\n", name, str);
  }

  return result;
}

class RandomEngine {
private:
  // What a mouth full...
  // This is the product of the bind expression below.
  typedef std::_Bind<std::uniform_int_distribution<uint64_t>(std::mt19937)>
      Generator;

  unsigned rand_seed;
  std::mt19937 gen;
  std::uniform_int_distribution<uint64_t> random_dist;
  Generator generator;

public:
  RandomEngine(unsigned _rand_seed, int _min, int _max)
      : rand_seed(_rand_seed), gen(rand_seed), random_dist(_min, _max),
        generator(std::bind(random_dist, gen)) {}

  RandomEngine(unsigned _rand_seed)
      : rand_seed(_rand_seed), gen(rand_seed), random_dist(0, UINT64_MAX),
        generator(std::bind(random_dist, gen)) {}

  RandomEngine(const RandomEngine &) = delete;
  RandomEngine(RandomEngine &&) = delete;

  RandomEngine &operator=(const RandomEngine &) = delete;

  uint64_t generate() { return generator(); }
  unsigned get_seed() const { return rand_seed; }
};

class RandomRealEngine {
private:
  // What a mouth full...
  // This is the product of the bind expression below.
  typedef std::_Bind<std::uniform_real_distribution<>(std::mt19937)> Generator;

  unsigned rand_seed;
  std::mt19937 gen;
  std::uniform_real_distribution<> random_dist;
  Generator generator;

public:
  RandomRealEngine(unsigned _rand_seed, double _min, double _max)
      : rand_seed(_rand_seed), gen(rand_seed), random_dist(_min, _max),
        generator(std::bind(random_dist, gen)) {}

  RandomRealEngine(unsigned _rand_seed)
      : rand_seed(_rand_seed), gen(rand_seed), random_dist(0, UINT64_MAX),
        generator(std::bind(random_dist, gen)) {}

  RandomRealEngine(const RandomRealEngine &) = delete;
  RandomRealEngine(RandomRealEngine &&) = delete;

  RandomRealEngine &operator=(const RandomRealEngine &) = delete;

  double generate() { return generator(); }
  unsigned get_seed() const { return rand_seed; }
};

class PcapWriter {
private:
  std::string output_fname;
  pcap_t *pd;
  pcap_dumper_t *pdumper;
  bool assume_ip;

public:
  PcapWriter(const std::string &_output_fname, bool _assume_ip)
      : output_fname(_output_fname), pd(NULL), pdumper(NULL),
        assume_ip(_assume_ip) {
    if (assume_ip) {
      pd = pcap_open_dead(DLT_RAW, 65535 /* snaplen */);
    } else {
      pd = pcap_open_dead(DLT_EN10MB, 65535 /* snaplen */);
    }

    pdumper = pcap_dump_open(pd, _output_fname.c_str());

    if (pdumper == nullptr) {
      fprintf(stderr, "Unable to write to file %s\n", _output_fname.c_str());
      exit(1);
    }
  }

  const std::string &get_output_fname() const { return output_fname; }

  PcapWriter(const std::string &_output_fname)
      : PcapWriter(_output_fname, false) {}

  void write(const u_char *pkt, uint16_t len, time_ns_t ts) {
    time_s_t sec = ts / 1'000'000'000;
    time_us_t usec = (ts % 1'000'000'000) / 1'000;
    pcap_pkthdr pcap_hdr{{sec, usec}, len, len};
    pcap_dump((u_char *)pdumper, &pcap_hdr, pkt);
  }

  ~PcapWriter() {
    if (pd)
      pcap_close(pd);
    if (pdumper)
      pcap_dump_close(pdumper);
  }
};

class RandomZipfEngine {
private:
  RandomRealEngine rand;
  double zipf_param;
  uint64_t range;

public:
  RandomZipfEngine(unsigned _random_seed, double _zipf_param, uint64_t _range)
      : rand(_random_seed, 0, 1), zipf_param(_zipf_param), range(_range) {}

  // From Castan [SIGCOMM'18]
  // Source:
  // https://github.com/nal-epfl/castan/blob/master/scripts/pcap_tools/create_zipfian_distribution_pcap.py
  uint64_t generate() {
    double probability = rand.generate();
    assert(probability >= 0 && probability <= 1);

    double p = probability;
    uint64_t N = range + 1;
    double s = zipf_param;
    double tolerance = 0.01;
    double x = (double)N / 2.0;

    double D = p * (12.0 * (pow(N, 1.0 - s) - 1) / (1.0 - s) + 6.0 -
                    6.0 * pow(N, -s) + s - pow(N, -1.0 - s) * s);

    while (true) {
      double m = pow(x, -2 - s);
      double mx = m * x;
      double mxx = mx * x;
      double mxxx = mxx * x;

      double a = 12.0 * (mxxx - 1) / (1.0 - s) + 6.0 * (1.0 - mxx) +
                 (s - (mx * s)) - D;
      double b = 12.0 * mxx + 6.0 * (s * mx) + (m * s * (s + 1.0));
      double newx = std::max(1.0, x - a / b);

      if (std::abs(newx - x) <= tolerance) {
        int i = newx - 1;
        assert(i >= 0 && i < range);
        return i;
      }

      x = newx;
    }
  }
};

in_addr_t random_addr() {
  std::stringstream ss;
  ss << "10.";
  ss << rand() % 256 << ".";
  ss << rand() % 256 << ".";
  ss << rand() % 256;
  return inet_addr(ss.str().c_str());
}

in_port_t random_port() { return rand() % 65536; }