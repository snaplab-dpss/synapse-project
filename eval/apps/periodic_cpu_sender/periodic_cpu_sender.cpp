#include <sycon/sycon.h>
#include <unistd.h>

using namespace sycon;

using Forwarder = KeylessTableMap<1>;
using forwarder_value_t = table_value_t<1>;

using AlarmTable = KeylessTableMap<1>;
using alarm_t = table_value_t<1>;

struct nf_config_t {
  double cpu_pkts_ratio;
} nf_config;

struct state_t {
  Forwarder forwarder;
  AlarmTable alarm_table;
  Counter pkt_counter;
  Counter cpu_counter;

  state_t(u32 alarm)
      : forwarder("Ingress", "forwarder",
                  forwarder_value_t({cfg.out_dev_port})),
        alarm_table("Ingress", "alarm_table", alarm_t({alarm})),
        pkt_counter("Ingress", "pkt_counter", true, true),
        cpu_counter("Ingress", "cpu_counter", true, true) {}
};

std::unique_ptr<state_t> state;

void sycon::nf_init() {
  u32 alarm = static_cast<u32>(1 / nf_config.cpu_pkts_ratio);

  DEBUG("Ratio %lf", nf_config.cpu_pkts_ratio);
  DEBUG("Alarm %u", alarm);

  state = std::make_unique<state_t>(alarm);
}

void sycon::nf_exit() {}

void sycon::nf_args(CLI::App &app) {
  app.add_option("--ratio", nf_config.cpu_pkts_ratio,
                 "Ratio of CPU packets to total input packets")
      ->required();
}

void sycon::nf_user_signal_handler() {
  counter_data_t pkt_counter_data = state->pkt_counter.get(0);
  counter_data_t cpu_counter_data = state->cpu_counter.get(0);

  u64 in_packets = pkt_counter_data.packets;
  u64 cpu_packets = cpu_counter_data.packets;

  // Total packets (including warmup traffic)
  u64 total_packets = get_asic_port_rx(cfg.in_dev_port);

  float ratio = in_packets > 0 ? (float)cpu_packets / in_packets : 0;

  LOG("Packet counters:");
  LOG("In: %lu", in_packets)
  LOG("CPU: %lu", cpu_packets)
  LOG("Total: %lu", total_packets)
}

bool sycon::nf_process(time_ns_t now, byte_t *pkt, u16 size) {
  cpu_hdr_t *cpu_hdr = (cpu_hdr_t *)packet_consume(pkt, sizeof(cpu_hdr_t));
  cpu_hdr->out_port = SWAP_ENDIAN_16(cfg.out_dev_port);
  return true;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
