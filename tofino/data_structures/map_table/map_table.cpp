#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  MapTable map_table;

  state_t() : map_table("map_table", {"Ingress.map_table_0"}, 100) {}
};

state_t *state = nullptr;

void sycon::nf_init() {
  nf_state = std::make_unique<state_t>();
  state    = dynamic_cast<state_t *>(nf_state.get());
}

void sycon::nf_exit() {}

void sycon::nf_user_signal_handler() {}

void sycon::nf_args(CLI::App &app) {}

struct cpu_hdr_extra_t {
  u16 ingress_dev;
} __attribute__((packed));

nf_process_result_t sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  nf_process_result_t result;

  cpu_hdr_t *cpu_hdr             = packet_consume<cpu_hdr_t>(pkt);
  cpu_hdr_extra_t *cpu_hdr_extra = packet_consume<cpu_hdr_extra_t>(pkt);

  eth_hdr_t *eth_hdr   = packet_consume<eth_hdr_t>(pkt);
  ipv4_hdr_t *ipv4_hdr = packet_consume<ipv4_hdr_t>(pkt);
  udp_hdr_t *udp_hdr   = packet_consume<udp_hdr_t>(pkt);

  buffer_t key(12);
  key.set(0, 4, bswap32(ipv4_hdr->src_ip));
  key.set(4, 4, bswap32(ipv4_hdr->dst_ip));
  key.set(8, 2, bswap16(udp_hdr->src_port));
  key.set(10, 2, bswap16(udp_hdr->dst_port));

  u32 value = 0;
  if (!state->map_table.get(key, value)) {
    state->map_table.put(key, value);
  }

  cpu_hdr->egress_dev = cpu_hdr_extra->ingress_dev;

  // packet_hexdump(pkt, size);
  // packet_log(cpu_hdr);

  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
