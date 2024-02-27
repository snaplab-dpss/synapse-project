#include <sycon/sycon.h>
#include <unistd.h>

using namespace sycon;

using Forwarder = KeylessTableMap<1>;
using Map = TableMap<4, 1>;

bf_status_t table_with_timeout_expiration_callback(
    const bf_rt_target_t &target, const bfrt::BfRtTableKey *key, void *cookie) {
  Map *map = static_cast<Map *>(cookie);

  begin_transaction();
  map->del(key);
  end_transaction();

  return BF_SUCCESS;
}

struct state_t {
  Forwarder forwarder;
  Map map;

  state_t()
      : forwarder("Ingress", "forwarder", value_t<1>({nf_config.out_dev_port})),
        map("Ingress", "map") {}
};

state_t *state;

void sycon::nf_init() { state = new state_t(); }

void sycon::nf_cleanup() { delete state; }

bool sycon::nf_process(time_ns_t now, byte_t *pkt, u16 size) {
  cpu_hdr_t *cpu_hdr = (cpu_hdr_t *)packet_consume(pkt, sizeof(cpu_hdr_t));
  eth_hdr_t *eth_hdr = (eth_hdr_t *)packet_consume(pkt, sizeof(eth_hdr_t));
  ipv4_hdr_t *ipv4_hdr = (ipv4_hdr_t *)packet_consume(pkt, sizeof(ipv4_hdr_t));
  tcpudp_hdr_t *tcpudp_hdr =
      (tcpudp_hdr_t *)packet_consume(pkt, sizeof(tcpudp_hdr_t));

  key_t<4> flow({SWAP_ENDIAN_32(ipv4_hdr->src_ip),
                 SWAP_ENDIAN_32(ipv4_hdr->dst_ip),
                 SWAP_ENDIAN_16(tcpudp_hdr->src_port),
                 SWAP_ENDIAN_16(tcpudp_hdr->dst_port)});

  value_t<1> out_port({nf_config.out_dev_port});

  state->map.put(flow, out_port);
  cpu_hdr->out_port = SWAP_ENDIAN_16(nf_config.out_dev_port);

  return true;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
