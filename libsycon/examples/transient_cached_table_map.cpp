#include <sycon/sycon.hpp>
#include <unistd.h>

using namespace sycon;

struct state_t {
  TransientCachedTableMap<4, 1> map;

  state_t() : map("Ingress", "map", args.expiration_time, "map_cache_timer", "IngressDeparser", "digest") {}
};

state_t *state;

void sycon::nf_init() { state = new state_t(); }

void sycon::nf_exit() {
  state->map.dump();
  delete state;
}

void sycon::nf_user_signal_handler() {}

void sycon::nf_args(CLI::App &app) {}

bool sycon::nf_process(time_ns_t now, byte_t *pkt, u16 size) {
  cpu_hdr_t *cpu_hdr       = (cpu_hdr_t *)packet_consume(pkt, sizeof(cpu_hdr_t));
  eth_hdr_t *eth_hdr       = (eth_hdr_t *)packet_consume(pkt, sizeof(eth_hdr_t));
  ipv4_hdr_t *ipv4_hdr     = (ipv4_hdr_t *)packet_consume(pkt, sizeof(ipv4_hdr_t));
  tcpudp_hdr_t *tcpudp_hdr = (tcpudp_hdr_t *)packet_consume(pkt, sizeof(tcpudp_hdr_t));

  table_key_t<4> flow({SWAP_ENDIAN_32(ipv4_hdr->src_ip), SWAP_ENDIAN_32(ipv4_hdr->dst_ip), SWAP_ENDIAN_16(tcpudp_hdr->src_port),
                       SWAP_ENDIAN_16(tcpudp_hdr->dst_port)});

  table_value_t<1> out_port({cfg.dev_ports[0]});

  state->map.put(flow, out_port);
  cpu_hdr->out_port = SWAP_ENDIAN_16(cfg.dev_ports[0]);

  return true;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
