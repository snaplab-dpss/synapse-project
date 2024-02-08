#include "../../../boilerplate/tofino.h"

struct state_t {
	std::unique_ptr<Map<int>> map;
	std::unique_ptr<Map<bytes_t>> map2;

	state_t() {
		map = Map<int>::build();
		map2 = Map<bytes_t>::build();
	}
};

std::unique_ptr<state_t> state;

void state_init() { state = std::unique_ptr<state_t>(new state_t()); }

port_t nf_process(time_ns_t now, cpu_t *cpu, pkt_t &pkt) {
	auto ether = pkt.parse_ethernet();
	auto ip = pkt.parse_ipv4();
	auto tcpudp = pkt.parse_tcpudp();

	cpu->dump();
	ether->dump();
	ip->dump();
	tcpudp->dump();

	bytes_t k0(6);
	k0.values[0] = ether->src_mac[0];
	k0.values[1] = ether->src_mac[1];
	k0.values[2] = ether->src_mac[2];
	k0.values[3] = ether->src_mac[3];
	k0.values[4] = ether->src_mac[4];
	k0.values[5] = ether->src_mac[5];

	state->map->put(k0, cpu->in_port);

	bytes_t k1(6);
	k1.values[0] = ether->dst_mac[0];
	k1.values[1] = ether->dst_mac[1];
	k1.values[2] = ether->dst_mac[2];
	k1.values[3] = ether->dst_mac[3];
	k1.values[4] = ether->dst_mac[4];
	k1.values[5] = ether->dst_mac[5];

	int out_port;
	auto contains = state->map->get(k1, out_port);

	if (contains) {
		return out_port;
	}

	return BCAST;
}
