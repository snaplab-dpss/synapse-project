#include <LibCore/Pcap.h>
#include <LibCore/RandomEngine.h>
#include <LibCore/Net.h>

#include <unordered_set>

namespace LibCore {

class TrafficGenerator {
public:
  using device_t     = u16;
  using flow_idx_t   = size_t;
  using device_idx_t = size_t;

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
    fpm_t churn;
    TrafficType traffic_type;
    double zipf_param;
    std::vector<device_t> devices;
    std::vector<device_t> client_devices;
    u32 random_seed;
    bool dry_run;

    config_t()
        : out_dir(DEFAULT_OUTPUT_DIR), total_packets(DEFAULT_TOTAL_PACKETS), total_flows(DEFAULT_TOTAL_FLOWS),
          packet_size(DEFAULT_PACKET_SIZE), rate(DEFAULT_RATE), churn(DEFAULT_TOTAL_CHURN_FPM), traffic_type(DEFAULT_TRAFFIC_TYPE),
          zipf_param(DEFAULT_ZIPF_PARAM), devices(), client_devices(), random_seed(0), dry_run(false) {}

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
  const pkt_t template_packet;

  std::unordered_map<device_t, LibCore::PcapWriter> warmup_writers;
  std::unordered_map<device_t, LibCore::PcapWriter> writers;

  LibCore::RandomUniformEngine uniform_rand;
  LibCore::RandomZipfEngine zipf_rand;

  pcap_t *pd;
  pcap_dumper_t *pdumper;

  device_idx_t client_dev_it;

  std::unordered_map<flow_idx_t, device_t> flows_to_dev;
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
    ss << "-c" << config.churn;
    switch (config.traffic_type) {
    case TrafficType::Uniform: {
      ss << "-unif";
    } break;
    case TrafficType::Zipf: {
      std::string s = std::to_string(config.zipf_param);
      s.erase(s.find_last_not_of('0') + 1, std::string::npos);
      s.erase(s.find_last_not_of('.') + 1, std::string::npos);
      std::replace(s.begin(), s.end(), '.', '_');
      ss << "-zipf" << s;
    } break;
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

  static in_addr_t mask_addr_from_dev(in_addr_t addr, u16 dev) { return LibCore::ipv4_set_prefix(addr, dev, 6); }

  void advance_client_dev() { client_dev_it = (client_dev_it + 1) % config.client_devices.size(); }
  void reset_client_dev() { client_dev_it = 0; }
  device_t get_current_client_dev() const { return config.client_devices.at(client_dev_it); }

  void report() const;

  virtual void random_swap_flow(flow_idx_t flow_idx)                                        = 0;
  virtual pkt_t build_packet(device_t dev, flow_idx_t flow_idx)                             = 0;
  virtual std::optional<device_t> get_response_dev(device_t dev, flow_idx_t flow_idx) const = 0;

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