#include <LibCore/TrafficGenerator.h>

#include <arpa/inet.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include <CLI/CLI.hpp>

using namespace LibCore;

using device_t    = TrafficGenerator::device_t;
using config_t    = TrafficGenerator::config_t;
using TrafficType = TrafficGenerator::TrafficType;

// Traffic for the meta4 NF: the DNS responses that teach it which client-server pairs belong to
// which watched domain, and the data those pairs then exchange. Every flow is one {client, server}
// pair with a domain, and its data is only attributed once a DNS response has announced the pair.
//
// The warmup announces every pair once. The measured stream is data packets, server to client, plus
// DNS responses at --dns-ratio: a pair freshly replaced by churn announces itself on its next turn,
// and the rest re-announce pairs already installed, as a client re-resolving after a TTL does.
// --ignored-ratio puts that share of clients inside an ignored prefix, so their responses are
// refused and their data goes unattributed; --unattributed-ratio sends that share of packets between
// pairs that are never announced at all.
//
// Domain patterns come from a file, one per line -- the same file the NF is configured with, so a
// name emitted here is one the NF is watching for. A leading "*" stands for any label, and is
// filled in with a random one.

constexpr u32 DNS_TTL           = 60;
constexpr u16 DATA_PORT         = 80;
constexpr size_t DNS_MAX_LABELS = 4;  // labels of a name the NF reads
constexpr size_t DNS_WILD_LABEL = 15; // length of a label made up for a wildcard: what the NF keeps of one

// A watched pattern, as its labels. "*" is a wildcard.
using pattern_t = std::vector<std::string>;

struct prefix_t {
  u32 base; // host order
  u32 mask; // host order
};

struct m4_flow_t {
  u32 client;      // network order
  u32 server;      // network order
  u16 client_port; // network order
  size_t domain;   // index into the patterns
  pattern_t name;  // the pattern with its wildcards filled in
  bool announced;
};

struct m4_config_t {
  std::filesystem::path domains_file;
  std::vector<std::string> ignored_clients; // <address>/<prefix length>
  double dns_ratio;
  double ignored_ratio;
  double unattributed_ratio;
  std::string resolver;
};

static std::vector<pattern_t> read_patterns(const std::filesystem::path &path) {
  std::ifstream in(path);
  if (!in) {
    panic("Could not open the domains file %s", path.c_str());
  }

  std::vector<pattern_t> patterns;
  std::string line;
  while (std::getline(in, line)) {
    const size_t first = line.find_first_not_of(" \t\r");
    if (first == std::string::npos || line[first] == '#') {
      continue;
    }
    line = line.substr(first, line.find_last_not_of(" \t\r") - first + 1);

    pattern_t pattern;
    size_t wildcards = 0;
    size_t start     = 0;
    while (start <= line.size()) {
      size_t dot = line.find('.', start);
      if (dot == std::string::npos) {
        dot = line.size();
      }
      const std::string label = line.substr(start, dot - start);
      if (label.empty() || label.size() > DNS_MAX_LABEL_LEN) {
        panic("%s: `%s` has a label that is empty or longer than %u bytes", path.c_str(), line.c_str(), DNS_MAX_LABEL_LEN);
      }
      if (label == "*") {
        // The NF only takes wildcards that lead a pattern.
        if (pattern.size() != wildcards) {
          panic("%s: `%s` has a wildcard after a spelled-out label", path.c_str(), line.c_str());
        }
        wildcards++;
      }
      pattern.push_back(label);
      start = dot + 1;
    }
    if (pattern.size() > DNS_MAX_LABELS) {
      panic("%s: `%s` has more than %zu labels", path.c_str(), line.c_str(), DNS_MAX_LABELS);
    }
    patterns.push_back(pattern);
  }

  if (patterns.empty()) {
    panic("%s names no domain", path.c_str());
  }

  return patterns;
}

static prefix_t parse_prefix(const std::string &str) {
  const size_t slash = str.find('/');
  if (slash == std::string::npos) {
    panic("ignore-client: `%s` is not <address>/<prefix length>", str.c_str());
  }

  const in_addr_t address = inet_addr(str.substr(0, slash).c_str());
  if (address == INADDR_NONE) {
    panic("ignore-client: `%s` is not an IPv4 address", str.substr(0, slash).c_str());
  }

  const int len = std::stoi(str.substr(slash + 1));
  if (len < 0 || len > 32) {
    panic("ignore-client: prefix length %d is not between 0 and 32", len);
  }

  const u32 mask = len == 0 ? 0 : (~0u << (32 - len));
  return {ntohl(address) & mask, mask};
}

class Meta4TrafficGenerator : public TrafficGenerator {
private:
  const m4_config_t m4_config;
  const std::vector<pattern_t> patterns;
  const std::vector<prefix_t> ignored;
  const u32 resolver; // network order

  std::vector<m4_flow_t> flows;
  std::vector<m4_flow_t> unattributed; // pairs no response ever announces

  std::mt19937 rng;
  std::uniform_real_distribution<double> unif;

  std::string random_label() {
    static constexpr char alphabet[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::string label(DNS_WILD_LABEL, 'a');
    for (char &c : label) {
      c = alphabet[rng() % (sizeof(alphabet) - 1)];
    }
    return label;
  }

  u32 inside(const prefix_t &prefix) { return htonl(prefix.base | (static_cast<u32>(rng()) & ~prefix.mask)); }

  m4_flow_t new_flow() {
    m4_flow_t flow;
    flow.client      = random_addr();
    flow.server      = random_addr();
    flow.client_port = random_port();
    flow.domain      = rng() % patterns.size();
    flow.announced   = false;

    if (!ignored.empty() && unif(rng) < m4_config.ignored_ratio) {
      flow.client = inside(ignored[rng() % ignored.size()]);
    }

    flow.name = patterns[flow.domain];
    for (std::string &label : flow.name) {
      if (label == "*") {
        label = random_label();
      }
    }

    return flow;
  }

  // The response a resolver sends the client: its question is the pair's name, its one answer the
  // address of the pair's server.
  pkt_t dns_response(const m4_flow_t &flow) {
    pkt_t pkt = template_packet;

    pkt.ip_hdr.src_addr  = resolver;
    pkt.ip_hdr.dst_addr  = flow.client;
    pkt.udp_hdr.src_port = htons(DNS_PORT);
    pkt.udp_hdr.dst_port = flow.client_port;

    u8 *p = pkt.payload;

    const dns_hdr_t hdr = {htons(0x1234), htons(DNS_FLAGS_RESPONSE), htons(1), htons(1), 0, 0};
    std::memcpy(p, &hdr, sizeof(hdr));
    p += sizeof(hdr);

    for (const std::string &label : flow.name) {
      *p++ = static_cast<u8>(label.size());
      std::memcpy(p, label.data(), label.size());
      p += label.size();
    }
    *p++ = 0;

    const u16 qtype  = htons(DNS_TYPE_A);
    const u16 qclass = htons(DNS_CLASS_IN);
    std::memcpy(p, &qtype, sizeof(qtype));
    p += sizeof(qtype);
    std::memcpy(p, &qclass, sizeof(qclass));
    p += sizeof(qclass);

    const dns_rr_hdr_t record = {htons(DNS_NAME_POINTER), htons(DNS_TYPE_A), htons(DNS_CLASS_IN), htonl(DNS_TTL), htons(sizeof(flow.server))};
    std::memcpy(p, &record, sizeof(record));
    p += sizeof(record);
    std::memcpy(p, &flow.server, sizeof(flow.server));
    p += sizeof(flow.server);

    const bytes_t payload   = p - pkt.payload;
    pkt.udp_hdr.len         = htons(sizeof(udp_hdr_t) + payload);
    pkt.ip_hdr.total_length = htons(sizeof(ipv4_hdr_t) + sizeof(udp_hdr_t) + payload);

    return pkt;
  }

  pkt_t data_packet(const m4_flow_t &flow) {
    pkt_t pkt            = template_packet;
    pkt.ip_hdr.src_addr  = flow.server;
    pkt.ip_hdr.dst_addr  = flow.client;
    pkt.udp_hdr.src_port = htons(DATA_PORT);
    pkt.udp_hdr.dst_port = flow.client_port;
    return pkt;
  }

  static bool is_dns(const pkt_t &pkt) { return pkt.udp_hdr.src_port == htons(DNS_PORT); }

public:
  Meta4TrafficGenerator(const config_t &_config, const m4_config_t &_m4_config, const std::vector<pattern_t> &_patterns,
                        const std::vector<prefix_t> &_ignored, u32 _resolver)
      : TrafficGenerator("meta4", _config, true), m4_config(_m4_config), patterns(_patterns), ignored(_ignored), resolver(_resolver),
        rng(_config.random_seed), unif(0.0, 1.0) {
    flows.reserve(config.total_flows);
    for (size_t i = 0; i < config.total_flows; i++) {
      flows.push_back(new_flow());
    }

    if (m4_config.unattributed_ratio > 0) {
      const size_t n = std::max<size_t>(1, static_cast<size_t>(m4_config.unattributed_ratio * config.total_flows));
      unattributed.reserve(n);
      for (size_t i = 0; i < n; i++) {
        unattributed.push_back(new_flow());
      }
    }
  }

  virtual bytes_t get_hdrs_len() const override { return sizeof(ether_hdr_t) + sizeof(ipv4_hdr_t) + sizeof(udp_hdr_t); }

  // A DNS response is captured whole, since the NF reads all of it; a data packet is headers only,
  // padded to the configured size like everyone else's.
  virtual bytes_t get_pkt_hdrs_len(const pkt_t &pkt) const override {
    return is_dns(pkt) ? sizeof(ether_hdr_t) + ntohs(pkt.ip_hdr.total_length) : get_hdrs_len();
  }

  virtual bytes_t get_pkt_len(const pkt_t &pkt) const override {
    return is_dns(pkt) ? sizeof(ether_hdr_t) + ntohs(pkt.ip_hdr.total_length) : config.packet_size_without_crc;
  }

  virtual void random_swap_flow(flow_idx_t flow_idx) override {
    assert(flow_idx < flows.size());
    flows[flow_idx] = new_flow();
  }

  virtual pkt_t build_warmup_packet(device_t dev, flow_idx_t flow_idx) override {
    m4_flow_t &flow = flows[flow_idx];
    flow.announced  = true;
    return dns_response(flow);
  }

  virtual std::optional<pkt_t> build_packet(device_t dev, flow_idx_t flow_idx) override {
    m4_flow_t &flow = flows[flow_idx];

    // A pair churn just brought in has to be announced before any of its data means anything.
    if (!flow.announced) {
      flow.announced = true;
      return dns_response(flow);
    }

    const double r = unif(rng);
    if (r < m4_config.dns_ratio) {
      return dns_response(flow);
    }
    if (!unattributed.empty() && r < m4_config.dns_ratio + m4_config.unattributed_ratio) {
      return data_packet(unattributed[rng() % unattributed.size()]);
    }
    return data_packet(flow);
  }

  virtual std::optional<device_t> get_response_dev(device_t dev, flow_idx_t flow_idx) const override { return {}; }
};

int main(int argc, char *argv[]) {
  CLI::App app{"Traffic generator for the meta4 nf."};

  config_t config;
  m4_config_t m4_config;
  bytes_t packet_size;

  app.add_option("--out", config.out_dir, "Output directory.")->default_val(TrafficGenerator::DEFAULT_OUTPUT_DIR);
  app.add_option("--packets", config.total_packets, "Total packets.")->default_val(TrafficGenerator::DEFAULT_TOTAL_PACKETS);
  app.add_option("--flows", config.total_flows, "Total flows (client-server pairs).")->default_val(TrafficGenerator::DEFAULT_TOTAL_FLOWS);
  app.add_option("--rate", config.rate, "Rate (bps).")->default_val(TrafficGenerator::DEFAULT_RATE);
  app.add_option("--packet-size", packet_size, "Data packet size (bytes).")->default_val(TrafficGenerator::DEFAULT_PACKET_SIZE);
  app.add_option("--churn", config.churn, "Total churn (fpm).")->default_val(TrafficGenerator::DEFAULT_TOTAL_CHURN_FPM);
  app.add_option("--traffic", config.traffic_type, "Traffic distribution.")
      ->default_val(TrafficGenerator::DEFAULT_TRAFFIC_TYPE)
      ->transform(CLI::CheckedTransformer(
          std::unordered_map<std::string, TrafficType>{
              {"uniform", TrafficType::Uniform},
              {"zipf", TrafficType::Zipf},
          },
          CLI::ignore_case));
  app.add_option("--zipf-param", config.zipf_param, "Zipf parameter.")->default_val(TrafficGenerator::DEFAULT_ZIPF_PARAM);
  app.add_option("--devs", config.devices, "Devices.")->required();
  app.add_option("--domains", m4_config.domains_file, "File naming the watched domains, one per line, as the NF is configured with.")->required();
  app.add_option("--ignore-client", m4_config.ignored_clients, "Client prefix the NF ignores (<address>/<length>).")
      ->default_val(std::vector<std::string>{"10.9.0.0/24"});
  app.add_option("--dns-ratio", m4_config.dns_ratio, "Fraction of packets that are DNS responses.")->default_val(0.0014);
  app.add_option("--ignored-ratio", m4_config.ignored_ratio, "Fraction of pairs whose client is in an ignored prefix.")->default_val(0.0);
  app.add_option("--unattributed-ratio", m4_config.unattributed_ratio, "Fraction of packets between pairs no response ever announces.")
      ->default_val(0.0);
  app.add_option("--resolver", m4_config.resolver, "Address the DNS responses come from.")->default_val("8.8.8.8");
  app.add_option("--seed", config.random_seed, "Random seed.")->default_val(std::random_device()());
  app.add_flag("--dry-run", config.dry_run, "Print out the configuration values without generating the pcaps.")->default_val(false);

  CLI11_PARSE(app, argc, argv);

  config.warmup_devices          = config.devices;
  config.packet_size_without_crc = std::max(packet_size, MIN_PKT_SIZE_BYTES) - CRC_SIZE_BYTES;

  if (m4_config.dns_ratio + m4_config.unattributed_ratio > 1.0) {
    panic("dns-ratio and unattributed-ratio add up to more than 1");
  }

  const in_addr_t resolver = inet_addr(m4_config.resolver.c_str());
  if (resolver == INADDR_NONE) {
    panic("resolver: `%s` is not an IPv4 address", m4_config.resolver.c_str());
  }

  std::vector<prefix_t> ignored;
  for (const std::string &prefix : m4_config.ignored_clients) {
    ignored.push_back(parse_prefix(prefix));
  }

  srand(config.random_seed);

  const std::vector<pattern_t> patterns = read_patterns(m4_config.domains_file);

  config.print();
  std::cout << "domains:      " << patterns.size() << " from " << m4_config.domains_file << "\n";
  std::cout << "ignored:      ";
  for (const std::string &prefix : m4_config.ignored_clients) {
    std::cout << prefix << " ";
  }
  std::cout << "\n";
  std::cout << "dns:          " << m4_config.dns_ratio << "\n";
  std::cout << "ignored:      " << m4_config.ignored_ratio << "\n";
  std::cout << "unattributed: " << m4_config.unattributed_ratio << "\n";
  std::cout << "resolver:     " << m4_config.resolver << "\n";
  if (config.dry_run) {
    return 0;
  }

  Meta4TrafficGenerator generator(config, m4_config, patterns, ignored, resolver);

  generator.generate_warmup();
  generator.generate();

  return 0;
}
