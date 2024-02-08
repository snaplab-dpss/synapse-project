#include "../../../boilerplate/tofino.h"

struct state_t {
	std::unique_ptr<Map<int>> map;

	state_t() { map = Map<int>::build(); }
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

	bytes_t key(4);
	key.values[0] = (ip->src_ip >> 0) & 0xff;
	key.values[1] = (ip->src_ip >> 8) & 0xff;
	key.values[2] = (ip->src_ip >> 16) & 0xff;
	key.values[3] = (ip->src_ip >> 24) & 0xff;

	int counter;
	auto contains = state->map->get(key, counter);

	if (contains) {
		counter++;
	} else {
		counter = 0;
	}

	state->map->put(key, counter);
	
	std::cerr << "\ncounter " << counter << "\n";

	tcpudp->dst_port = htons(counter);

	return 1;
}

