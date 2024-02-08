#include "tofino.h"

struct state_t {
	state_t() {}
};

std::unique_ptr<state_t> state;

void state_init() { state = std::unique_ptr<state_t>(new state_t()); }

port_t nf_process(time_ns_t now, cpu_t *cpu, pkt_t& pkt) {
	auto ether = pkt.parse_ethernet();
	auto ip = pkt.parse_ipv4();
	auto tcpudp = pkt.parse_tcpudp();

	std::cerr << "\n\n";
	cpu->dump();
	ether->dump();
	ip->dump();
	tcpudp->dump();

	auto port = cpu->out_port;

	if (ntohs(cpu->code_path) > 5) {
		std::cerr << "too far!\n";
		return DROP;
	}

	return port + 1;
}