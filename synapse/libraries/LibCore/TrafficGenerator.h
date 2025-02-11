#include <LibCore/Pcap.h>
#include <LibCore/RandomEngine.h>
#include <LibCore/Net.h>

#include <unordered_set>

namespace LibCore {

class TrafficGenerator {
public:
  enum class Dev { WAN, LAN };
  enum class TrafficType { Uniform = 0, Zipf = 1 };

  static constexpr const char *const DEFAULT_OUTPUT_DIR   = ".";
  static constexpr const char *const SMAC                 = "02:00:00:ca:fe:ee";
  static constexpr const char *const DMAC                 = "02:00:00:be:ee:ef";
  static constexpr const u64 DEFAULT_TOTAL_PACKETS        = 1'000'000lu;
  static constexpr const u64 DEFAULT_TOTAL_FLOWS          = 65'536lu;
  static constexpr const bytes_t DEFAULT_PACKET_SIZE      = 200;
  static constexpr const bps_t DEFAULT_RATE               = 1'000'000'000lu; // 1 Gbps
  static constexpr const fpm_t DEFAULT_TOTAL_CHURN_FPM    = 0lu;
  static constexpr const TrafficType DEFAULT_TRAFFIC_TYPE = TrafficType::Uniform;
  static constexpr const double DEFAULT_ZIPF_PARAM        = 1.26; // From Castan [SIGCOMM'18]

  struct config_t {
    std::filesystem::path out_dir;
    u64 total_packets;
    u64 total_flows;
    bytes_t packet_size;
    bps_t rate;
    u64 churn_fpm;
    TrafficType traffic_type;
    double zipf_param;
    std::vector<std::pair<u16, u16>> lan_wan_pairs;
    u32 random_seed;
    bool dry_run;

    config_t()
        : out_dir(DEFAULT_OUTPUT_DIR), total_packets(DEFAULT_TOTAL_PACKETS), total_flows(DEFAULT_TOTAL_FLOWS),
          packet_size(DEFAULT_PACKET_SIZE), rate(DEFAULT_RATE), churn_fpm(DEFAULT_TOTAL_CHURN_FPM), traffic_type(DEFAULT_TRAFFIC_TYPE),
          zipf_param(DEFAULT_ZIPF_PARAM), lan_wan_pairs(), random_seed(0), dry_run(false) {}

    void print() const;
  };

  struct pkt_t {
    ether_hdr_t eth_hdr;
    ipv4_hdr_t ip_hdr;
    udp_hdr_t udp_hdr;
    u8 payload[MTU_SIZE_BYTES - (sizeof(ether_hdr_t) + sizeof(ipv4_hdr_t) + sizeof(udp_hdr_t))];
  } __attribute__((packed));

protected:
  const std::string nf;
  const config_t config;
  const std::unordered_map<u16, u16> lan_to_wan_dev;
  const std::vector<u16> lan_devices;
  const std::vector<u16> wan_devices;
  const pkt_t template_packet;

  std::unordered_map<u16, LibCore::PcapWriter> lan_warmup_writers;
  std::unordered_map<u16, LibCore::PcapWriter> lan_writers;
  std::unordered_map<u16, LibCore::PcapWriter> wan_writers;

  LibCore::RandomUniformEngine uniform_rand;
  LibCore::RandomZipfEngine zipf_rand;

  pcap_t *pd;
  pcap_dumper_t *pdumper;

  u16 current_lan_dev_it;

  using flow_idx_t = size_t;
  std::unordered_map<flow_idx_t, Dev> flows_dev_turn;
  std::unordered_map<flow_idx_t, u16> flows_to_lan_dev;
  std::unordered_map<flow_idx_t, u64> counters;
  u64 flows_swapped;

  time_ns_t current_time;
  time_ns_t alarm_tick;
  time_ns_t next_alarm;

public:
  TrafficGenerator(const std::string &_nf, const config_t &_config);
  TrafficGenerator(const TrafficGenerator &)            = delete;
  TrafficGenerator(TrafficGenerator &&)                 = delete;
  TrafficGenerator &operator=(const TrafficGenerator &) = delete;
  virtual ~TrafficGenerator()                           = default;

  pkt_t build_pkt_template() const;
  void generate();
  void generate_warmup();

protected:
  static std::string get_base_pcap_fname(const std::string &nf, const config_t &config) {
    std::stringstream ss;

    ss << nf;
    ss << "-f" << config.total_flows;
    ss << "-c" << config.churn_fpm;
    switch (config.traffic_type) {
    case TrafficType::Uniform:
      ss << "-unif";
      break;
    case TrafficType::Zipf:
      ss << "-zipf" << config.zipf_param;
      break;
    }

    return ss.str();
  }

  static std::filesystem::path get_warmup_pcap_fname(const std::string &nf, const config_t &config, u16 dev) {
    std::stringstream ss;
    ss << get_base_pcap_fname(nf, config);
    ss << "-dev" << dev;
    ss << "-warmup.pcap";
    return config.out_dir / ss.str();
  }

  static std::filesystem::path get_pcap_fname(const std::string &nf, const config_t &config, u16 dev) {
    std::stringstream ss;
    ss << get_base_pcap_fname(nf, config);
    ss << "-dev" << dev;
    ss << ".pcap";
    return config.out_dir / ss.str();
  }

  static std::unordered_map<u16, u16> build_lan_to_wan(const std::vector<std::pair<u16, u16>> &lan_wan_pairs) {
    std::unordered_map<u16, u16> lan_to_wan_dev;
    for (const auto &[lan, wan] : lan_wan_pairs) {
      lan_to_wan_dev[lan] = wan;
    }
    return lan_to_wan_dev;
  }

  static std::vector<u16> build_lan_devices(const std::vector<std::pair<u16, u16>> &lan_wan_pairs) {
    std::vector<u16> lan_devices;
    for (const auto &[lan, _] : lan_wan_pairs) {
      lan_devices.push_back(lan);
    }
    return lan_devices;
  }

  static std::vector<u16> build_wan_devices(const std::vector<std::pair<u16, u16>> &lan_wan_pairs) {
    std::vector<u16> wan_devices;
    for (const auto &[_, wan] : lan_wan_pairs) {
      wan_devices.push_back(wan);
    }
    return wan_devices;
  }

  static in_addr_t mask_addr_from_dev(in_addr_t addr, u16 dev) { return LibCore::ipv4_set_prefix(addr, dev, 5); }

  void advance_lan_dev() { current_lan_dev_it = (current_lan_dev_it + 1) % lan_devices.size(); }
  void reset_lan_dev() { current_lan_dev_it = 0; }
  u16 get_current_lan_dev() const { return lan_devices.at(current_lan_dev_it); }

  void report() const;

  virtual void random_swap_flow(flow_idx_t flow_idx, u16 lan_dev, u16 wan_dev) = 0;
  virtual pkt_t build_lan_packet(flow_idx_t flow_idx)                          = 0;
  virtual pkt_t build_wan_packet(flow_idx_t flow_idx)                          = 0;

  void tick() {
    // To obtain the time in seconds:
    // (pkt.size * 8) / (gbps * 1e9)
    // We want in ns, not seconds, so we multiply by 1e9.
    // This cancels with the 1e9 on the bottom side of the operation.
    // So actually, result in ns = (pkt.size * 8) / gbps
    // Also, don't forget to take the inter packet gap and CRC
    // into consideration.
    const bytes_t wire_bytes = PREAMBLE_SIZE_BYTES + config.packet_size + CRC_SIZE_BYTES + IPG_SIZE_BYTES;
    const time_ns_t dt       = (wire_bytes * 8) / (config.rate / 1'000'000'000);
    current_time += dt;
    if (alarm_tick > 0 && alarm_tick < dt) {
      fprintf(stderr, "Churn is too high: alarm tick (%luns) is smaller than the time step (%luns)\n", alarm_tick, dt);
      exit(1);
    }
  }
};

} // namespace LibCore