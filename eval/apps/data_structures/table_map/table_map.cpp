#include <sycon/sycon.h>
#include <unistd.h>

using namespace sycon;

using Map = TableMap<4, 1>;
using map_key_t = table_key_t<4>;
using map_value_t = table_value_t<1>;

struct state_t {
  Map map;
  Counter cpu_counter;

  state_t()
      : map("Ingress", "table_with_timeout", args.expiration_time),
        cpu_counter("Ingress", "cpu_counter", true, true) {}
};

state_t *state;

void sycon::nf_init() { state = new state_t(); }

void sycon::nf_cleanup() {
  auto counter_data = state->cpu_counter.get(0);
  LOG("Packets: %lu", counter_data.packets)
  LOG("Bytes: %lu", counter_data.bytes)
  delete state;
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

  map_value_t out_port({nf_config.out_dev_port});

  state->map.put(flow, out_port);
  cpu_hdr->out_port = SWAP_ENDIAN_16(nf_config.out_dev_port);

  return true;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
