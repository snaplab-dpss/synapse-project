#include "../boilerplate/tofino.h"

struct state_t {
	state_t() {
		Map<int> my_map({"map"});

		bytes_t key(12);
		key[0] = 0;
		key[1] = 1;
		key[2] = 2;
		key[3] = 3;
		key[4] = 4;
		key[5] = 5;
		key[6] = 6;
		key[7] = 7;
		key[8] = 8;
		key[9] = 9;
		key[10] = 10;
		key[11] = 11;

		my_map.put(key, 0x0a0b0c0d);  // adding
		my_map.put(key, 0xfff0f1f2);  // modifying entry
	}
};

std::unique_ptr<state_t> state;

void state_init() { state = std::unique_ptr<state_t>(new state_t()); }

port_t nf_process(time_ns_t now, cpu_t* cpu, pkt_t& pkt) {
	auto ether = pkt.parse_ethernet();
	auto ip = pkt.parse_ipv4();
	auto tcpudp = pkt.parse_tcpudp();

	std::cerr << "\n\n";
	cpu->dump();
	ether->dump();
	ip->dump();
	tcpudp->dump();

	auto port = cpu->out_port;

	if (cpu->code_path > 5) {
		std::cerr << "too far!\n";
		return DROP;
	}

	return port + 1;
}
