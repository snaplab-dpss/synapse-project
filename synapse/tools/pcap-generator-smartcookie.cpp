#include <LibCore/TrafficGenerator.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <random>
#include <vector>

#include <CLI/CLI.hpp>

using namespace LibCore;

using device_t    = TrafficGenerator::device_t;
using config_t    = TrafficGenerator::config_t;
using TrafficType = TrafficGenerator::TrafficType;

// Traffic for the SmartCookie switch agent: a mix of a SYN flood and the paths of a working
// deployment. The server sits on `server_dev` (the NF's default is device 0); every other
// device is a client. Each flow is one of:
//   attack      - the client only ever sends SYNs (the flood; answered with SYN-ACK cookies)
//   connected   - the server confirms it once (client->server 4-tuple tagged with ECE, which
//                 sets the bloom filter), then the client's data hits the bloom filter and the
//                 server's data is routed to the client
//   unconfirmed - the client sends data the server never confirmed: bloom miss, cookie
//                 verification fails, dropped
// The server also sends a time-sync UDP datagram (port 5555) about once per second.
//
// A connected flow completes the handshake the way a real client does: a SYN, then an ACK
// carrying the cookie the switch would have minted for that SYN, which the agent verifies and
// admits. Without that ACK no capture ever exercises the accepted-cookie path, the verification
// looks like pure overhead, and a profile built from the capture rewards handing the whole check
// to the controller.

constexpr u16 TIMESYNC_PORT             = 5555;
constexpr time_ns_t TIMESYNC_PERIOD_NS  = 1'000'000'000;
constexpr u8 TCP_SYN                    = 0x02;
constexpr u8 TCP_ACK                    = 0x10;
constexpr u8 TCP_ECE                    = 0x40;
constexpr u8 TCP_DATA_OFFSET_NO_OPTIONS = 5 << 4;

// The cookie, as dpdk-nfs/smartcookie computes it: HalfSipHash-2-4 over the flow and the client's
// sequence number, xored with the current epoch. Kept here rather than included because the NF's
// header is DPDK-flavoured; the constants, the key and the missing finalization step (the switch
// agent omits v[2] ^= 0xff) all follow dpdk-nfs/smartcookie/halfsiphash.h, which is the source of
// truth.
constexpr u32 SIP_KEY_0 = 0x33323130;
constexpr u32 SIP_KEY_1 = 0x42413938;

static inline u32 rotl32(u32 x, unsigned n) { return n == 0 ? x : ((x << n) | (x >> (32 - n))); }

static inline void sipround(u32 v[4]) {
  v[0] += v[1];
  v[2] += v[3];
  v[1] = rotl32(v[1], 5);
  v[3] = rotl32(v[3], 8);
  v[1] ^= v[0];
  v[3] ^= v[2];
  v[0] = rotl32(v[0], 16);
  v[2] += v[1];
  v[0] += v[3];
  v[1] = rotl32(v[1], 13);
  v[3] = rotl32(v[3], 7);
  v[1] ^= v[2];
  v[3] ^= v[0];
  v[2] = rotl32(v[2], 16);
}

static u32 halfsiphash(u32 w0, u32 w1, u32 w2, u32 w3) {
  u32 v[4]       = {SIP_KEY_0 ^ 0x70736575u, SIP_KEY_1 ^ 0x6e646f6du, SIP_KEY_0 ^ 0x6e657261u, SIP_KEY_1 ^ 0x79746573u};
  const u32 m[4] = {w0, w1, w2, w3};
  for (int i = 0; i < 4; i++) {
    v[3] ^= m[i];
    sipround(v);
    sipround(v);
    v[0] ^= m[i];
  }
  for (int i = 0; i < 4; i++) {
    sipround(v);
  }
  return v[0] ^ v[1] ^ v[2] ^ v[3];
}

enum class FlowClass { Attack, Connected, Unconfirmed };

struct sc_flow_t {
  flow_t flow;
  FlowClass cls;
  u32 isn;
  bool confirmed;   // The server has tagged the flow with ECE, so the bloom filter holds it.
  bool syn_sent;    // The client has opened with a SYN, so a cookie exists to return.
  bool cookie_sent; // The client has returned that cookie and been admitted.
};

struct sc_config_t {
  device_t server_dev;
  std::vector<device_t> client_devs;
  double attack_ratio;
  double unconfirmed_ratio;
};

class SmartCookieTrafficGenerator : public TrafficGenerator {
private:
  const sc_config_t sc_config;
  std::vector<device_t> client_devs;
  std::vector<sc_flow_t> flows;
  std::mt19937 rng;
  time_ns_t next_timesync;

  sc_flow_t new_flow() {
    std::uniform_real_distribution<double> unif(0.0, 1.0);
    const double r = unif(rng);
    FlowClass cls  = FlowClass::Connected;
    if (r < sc_config.attack_ratio) {
      cls = FlowClass::Attack;
    } else if (r < sc_config.attack_ratio + sc_config.unconfirmed_ratio) {
      cls = FlowClass::Unconfirmed;
    }

    // The switch routes server->client traffic by the first octet of the client's IP (the
    // expert's naive_routing), so client IPs start with a client device number, as the
    // testbed assigns them.
    flow_t flow               = random_flow();
    const device_t client_dev = client_devs[rng() % client_devs.size()];
    flow.five_tuple.src_ip    = (flow.five_tuple.src_ip & 0xffffff00) | client_dev;

    return {flow, cls, static_cast<u32>(rng()), false, false, false};
  }

  // Writes a TCP header over the template's UDP header + payload bytes.
  pkt_t tcp_packet(const flow_t &flow, bool client_to_server, u32 seq, u32 ack, u8 flags) {
    pkt_t pkt                = template_packet;
    pkt.ip_hdr.next_proto_id = IPPROTO_TCP;
    pkt.ip_hdr.src_addr      = client_to_server ? flow.five_tuple.src_ip : flow.five_tuple.dst_ip;
    pkt.ip_hdr.dst_addr      = client_to_server ? flow.five_tuple.dst_ip : flow.five_tuple.src_ip;
    tcp_hdr_t *tcp           = reinterpret_cast<tcp_hdr_t *>(&pkt.udp_hdr);
    std::memset(tcp, 0, sizeof(tcp_hdr_t));
    tcp->src_port  = client_to_server ? flow.five_tuple.src_port : flow.five_tuple.dst_port;
    tcp->dst_port  = client_to_server ? flow.five_tuple.dst_port : flow.five_tuple.src_port;
    tcp->sent_seq  = htonl(seq);
    tcp->recv_ack  = htonl(ack);
    tcp->data_off  = TCP_DATA_OFFSET_NO_OPTIONS;
    tcp->tcp_flags = flags;
    tcp->rx_win    = htons(0xffff);
    return pkt;
  }

  // The epoch the agent is in: (ticks(now) - delta) >> 12, with ticks of 2^16 ns. The time-sync
  // datagrams below carry this generator's own clock, so the agent's delta settles at zero and
  // the epoch is just the clock shifted. An epoch lasts 2^28 ns and the agent accepts the
  // current one and the two before it, so a client answering within about a second is admitted.
  u32 cookie_epoch() const { return static_cast<u32>(current_time >> 28); }

  // What the agent's SYN-ACK would carry for this flow's SYN. The agent hashes the addresses,
  // the ports and the sequence number in host order, so undo the header's byte order here.
  u32 cookie_for(const flow_t &flow, u32 seq) const {
    const u32 ports = (static_cast<u32>(ntohs(flow.five_tuple.src_port)) << 16) | ntohs(flow.five_tuple.dst_port);
    return cookie_epoch() ^ halfsiphash(ntohl(flow.five_tuple.src_ip), ntohl(flow.five_tuple.dst_ip), ports, seq);
  }

  pkt_t timesync_packet() {
    pkt_t pkt            = template_packet;
    pkt.udp_hdr.dst_port = htons(TIMESYNC_PORT);
    const u32 ticks      = static_cast<u32>(current_time >> 16);
    const u32 ticks_be   = htonl(ticks);
    std::memcpy(pkt.payload, &ticks_be, sizeof(ticks_be));
    return pkt;
  }

public:
  SmartCookieTrafficGenerator(const config_t &_config, const sc_config_t &_sc_config)
      : TrafficGenerator("smartcookie", _config, true), sc_config(_sc_config), rng(_config.random_seed), next_timesync(0) {
    client_devs = sc_config.client_devs;
    if (client_devs.empty()) {
      for (const device_t dev : config.devices) {
        if (dev != sc_config.server_dev) {
          client_devs.push_back(dev);
        }
      }
    }
    assert(!client_devs.empty() && "No client devices");

    flows.reserve(config.total_flows);
    for (size_t i = 0; i < config.total_flows; i++) {
      flows.push_back(new_flow());
    }
  }

  virtual bytes_t get_hdrs_len() const override { return sizeof(ether_hdr_t) + sizeof(ipv4_hdr_t) + sizeof(tcp_hdr_t); }

  virtual void random_swap_flow(flow_idx_t flow_idx) override {
    assert(flow_idx < flows.size());
    flows[flow_idx] = new_flow();
  }

  virtual std::optional<pkt_t> build_packet(device_t dev, flow_idx_t flow_idx) override {
    sc_flow_t &f = flows[flow_idx];

    if (dev == sc_config.server_dev) {
      if (current_time >= next_timesync) {
        next_timesync = current_time + TIMESYNC_PERIOD_NS;
        return timesync_packet();
      }
      if (f.cls != FlowClass::Connected) {
        return {};
      }
      if (!f.cookie_sent) {
        // The server has not seen this flow yet: the switch only passes it on once the client
        // has returned a valid cookie. Confirming earlier would put the flow in the bloom filter
        // before its ACK arrives, and the ACK would take the bloom-hit path instead of the
        // verification the capture is meant to exercise.
        return {};
      }
      if (!f.confirmed) {
        f.confirmed = true;
        return tcp_packet(f.flow, true, f.isn + 1, 0, TCP_ACK | TCP_ECE);
      }
      return tcp_packet(f.flow, false, static_cast<u32>(rng()), f.isn + 1, TCP_ACK);
    }

    switch (f.cls) {
    case FlowClass::Attack:
      return tcp_packet(f.flow, true, f.isn, 0, TCP_SYN);
    case FlowClass::Connected:
      // The handshake a real client completes: the SYN the agent answers with a cookie, then the
      // ACK returning it (ack = cookie + 1, seq = the SYN's + 1), which the agent verifies and
      // passes to the server. Afterwards the flow is in the bloom filter and the data flows.
      if (!f.syn_sent) {
        f.syn_sent = true;
        return tcp_packet(f.flow, true, f.isn, 0, TCP_SYN);
      }
      if (!f.cookie_sent) {
        f.cookie_sent = true;
        return tcp_packet(f.flow, true, f.isn + 1, cookie_for(f.flow, f.isn) + 1, TCP_ACK);
      }
      return tcp_packet(f.flow, true, f.isn + 1, static_cast<u32>(rng()), TCP_ACK);
    case FlowClass::Unconfirmed:
      // A cookie that does not verify: the spoofed case the agent has to reject.
      return tcp_packet(f.flow, true, f.isn + 1, static_cast<u32>(rng()), TCP_ACK);
    }

    return {};
  }

  virtual std::optional<device_t> get_response_dev(device_t dev, flow_idx_t flow_idx) const override { return {}; }
};

int main(int argc, char *argv[]) {
  CLI::App app{"Traffic generator for the smartcookie nf."};

  config_t config;
  sc_config_t sc_config;
  bytes_t packet_size;

  app.add_option("--out", config.out_dir, "Output directory.")->default_val(TrafficGenerator::DEFAULT_OUTPUT_DIR);
  app.add_option("--packets", config.total_packets, "Total packets.")->default_val(TrafficGenerator::DEFAULT_TOTAL_PACKETS);
  app.add_option("--flows", config.total_flows, "Total flows.")->default_val(TrafficGenerator::DEFAULT_TOTAL_FLOWS);
  app.add_option("--rate", config.rate, "Rate (bps).")->default_val(TrafficGenerator::DEFAULT_RATE);
  app.add_option("--packet-size", packet_size, "Packet size (bytes).")->default_val(TrafficGenerator::DEFAULT_PACKET_SIZE);
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
  app.add_option("--devs", config.devices, "Devices (the server device plus the client devices).")->required();
  app.add_option("--server-dev", sc_config.server_dev, "Device the server is attached to.")->default_val(0);
  app.add_option("--client-devs", sc_config.client_devs, "Devices client IPs are numbered after (default: every device but the server's).");
  app.add_option("--attack-ratio", sc_config.attack_ratio, "Fraction of flows that are SYN floods.")->default_val(0.4);
  app.add_option("--unconfirmed-ratio", sc_config.unconfirmed_ratio, "Fraction of flows whose data the server never confirmed.")->default_val(0.1);
  app.add_option("--seed", config.random_seed, "Random seed.")->default_val(std::random_device()());
  app.add_flag("--dry-run", config.dry_run, "Print out the configuration values without generating the pcaps.")->default_val(false);

  CLI11_PARSE(app, argc, argv);

  config.warmup_devices          = config.devices;
  config.packet_size_without_crc = std::max(packet_size, MIN_PKT_SIZE_BYTES) - CRC_SIZE_BYTES;

  srand(config.random_seed);

  config.print();
  std::cout << "server dev:  " << sc_config.server_dev << "\n";
  std::cout << "attack:      " << sc_config.attack_ratio << "\n";
  std::cout << "unconfirmed: " << sc_config.unconfirmed_ratio << "\n";
  if (config.dry_run) {
    return 0;
  }

  SmartCookieTrafficGenerator generator(config, sc_config);
  generator.generate();

  return 0;
}
