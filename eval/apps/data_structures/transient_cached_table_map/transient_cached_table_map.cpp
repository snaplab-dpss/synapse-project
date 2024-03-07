#include <sycon/sycon.h>
#include <unistd.h>

using namespace sycon;

using Forwarder = KeylessTableMap<1>;
using Map = TransientCachedTableMap<4, 1>;

using map_key_t = table_key_t<4>;
using map_value_t = table_value_t<1>;

struct state_t {
  Forwarder forwarder;
  Map map;
  Counter pkt_counter;
  Counter cpu_counter;

  state_t()
      : forwarder("Ingress", "forwarder", map_value_t({cfg.out_dev_port})),
        map("Ingress", "map", args.expiration_time, "cache.expirator.reg",
            "IngressDeparser", "digest_map"),
        pkt_counter("Ingress", "pkt_counter", true, true),
        cpu_counter("Ingress", "cpu_counter", true, true) {
    pkt_counter.set_session(cfg.usr_signal_session);
    cpu_counter.set_session(cfg.usr_signal_session);
  }
};

std::unique_ptr<state_t> state;

void sycon::nf_init() { state = std::make_unique<state_t>(); }

void sycon::nf_exit() {}

void sycon::nf_args(CLI::App &app) {}

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
  eth_hdr_t *eth_hdr = (eth_hdr_t *)packet_consume(pkt, sizeof(eth_hdr_t));
  ipv4_hdr_t *ipv4_hdr = (ipv4_hdr_t *)packet_consume(pkt, sizeof(ipv4_hdr_t));
  tcpudp_hdr_t *tcpudp_hdr =
      (tcpudp_hdr_t *)packet_consume(pkt, sizeof(tcpudp_hdr_t));

  map_key_t flow({SWAP_ENDIAN_32(ipv4_hdr->src_ip),
                  SWAP_ENDIAN_32(ipv4_hdr->dst_ip),
                  SWAP_ENDIAN_16(tcpudp_hdr->src_port),
                  SWAP_ENDIAN_16(tcpudp_hdr->dst_port)});

  map_value_t out_port({cfg.out_dev_port});

  state->map.put(flow, out_port);
  cpu_hdr->out_port = SWAP_ENDIAN_16(cfg.out_dev_port);

  return true;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
