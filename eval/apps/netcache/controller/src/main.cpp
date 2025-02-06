#include <signal.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <fstream>
#include <vector>
#include <variant>
#include <chrono>

#include <rte_eal.h>
#include <rte_common.h>
#include <rte_ethdev.h>
#include <rte_log.h>
#include <rte_mbuf.h>

#include "constants.h"
#include "netcache.h"
#include "listener.h"
#include "process_query.h"

#define REPORT_FILE "netcache-controller.tsv"

struct args_t {
	std::string conf_file_path;

	args_t(int argc, char **argv) {
		conf_file_path = std::string(argv[argc-1]);
		parse_help(argc, argv);
	}

	void help(char **argv) {
		std::cerr << "Usage: " << argv[0]
				  << " conf -i <listen iface> [--bf-prompt] [--tofino-model] [-h|--help]\n";
		exit(0);
	}

	void parse_help(int argc, char **argv) {
		auto targets = std::vector<std::string>{"-h", "--help"};

		for (auto i = 1; i < argc; i++) {
			auto arg = std::string(argv[i]);
			for (auto target : targets) {
				if (arg.compare(target) == 0) {
					help(argv);
					return;
				}
			}
		}
	}

	void dump() const {
		std::cout << "\n";
		std::cout << "Configuration:\n";
		std::cout << "  conf (w/ topo): " << conf_file_path << "\n";
	}
};

void signalHandler(int signum) {
	auto conf = netcache::Controller::controller->get_conf();
	auto use_tofino_model = netcache::Controller::controller->get_use_tofino_model();

	auto ofs = std::ofstream(REPORT_FILE);

	ofs << "#port\trx packets\ttx packets\n";

	if (!ofs.is_open()) {
		std::cerr << "ERROR: unable to write to \"" << REPORT_FILE << ".\n";
		exit(1);
	}

	// Get tx/rx for all active ports.
	for (auto connection : conf.topology.connections) {
		auto in_port = connection.in.port;

		if (!use_tofino_model) {
			in_port = netcache::Controller::controller->get_dev_port(in_port, 0);
		}

		auto tx = netcache::Controller::controller->get_port_tx(in_port);
		auto rx = netcache::Controller::controller->get_port_rx(in_port);

		ofs << in_port << "\t" << rx << "\t" << tx << "\n";
	}

	ofs.close();

	std::cout << "Report generated at \"" << REPORT_FILE << "\". Exiting.\n";

	exit(signum);
}

static inline int port_init(uint16_t port, struct rte_mempool *mbuf_pool) {
	struct rte_eth_conf port_conf;
	const uint16_t rx_rings = 1, tx_rings = 1;
	uint16_t nb_rxd = RX_RING_SIZE;
	uint16_t nb_txd = TX_RING_SIZE;
	int retval;
	uint16_t q;
	struct rte_eth_dev_info dev_info;
	struct rte_eth_txconf txconf;

	if (!rte_eth_dev_is_valid_port(port)) {
		return -1;
	}

	memset(&port_conf, 0, sizeof(struct rte_eth_conf));

	retval = rte_eth_dev_info_get(port, &dev_info);
	if (retval != 0) {
		printf("Error during getting device (port %u) info: %s\n",
				port, strerror(-retval));
		return retval;
	}

	if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE) {
		port_conf.txmode.offloads |=
			RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;
	}

	// Configure the Ethernet device.
	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;

	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	if (retval != 0)
		return retval;

	// Allocate and set up 1 RX queue per Ethernet port.
	for (q = 0; q < rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(
				port, q, nb_rxd, rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	txconf = dev_info.default_txconf;
	txconf.offloads = port_conf.txmode.offloads;
	// Allocate and set up 1 TX queue per Ethernet port.
	for (q = 0; q < tx_rings; q++) {
		retval = rte_eth_tx_queue_setup(
				port, q, nb_txd, rte_eth_dev_socket_id(port), &txconf);
		if (retval < 0)
			return retval;
	}

	// Start ethernet port.
	retval = rte_eth_dev_start(port);
	if (retval < 0) {
		return retval;
	}

	// Display the port MAC address.
	struct rte_ether_addr addr;
	retval = rte_eth_macaddr_get(port, &addr);
	if (retval != 0) {
		return retval;
	}

	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
			   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
			port, RTE_ETHER_ADDR_BYTES(&addr));

	// Enable RX in promiscuous mode for the ethernet device.
	retval = rte_eth_promiscuous_enable(0);
	if (retval != 0)
		return retval;

	return 0;
}

int main(int argc, char **argv) {
	signal(SIGINT, signalHandler);
	signal(SIGQUIT, signalHandler);
	signal(SIGTERM, signalHandler);

	args_t args(argc, argv);
	args.dump();

	netcache::conf_t conf = netcache::parse_conf_file(args.conf_file_path);

	netcache::init_bf_switchd(conf.misc.tofino_model, conf.misc.bf_prompt);
	netcache::setup_controller(conf, conf.misc.tofino_model);

	struct rte_mempool *mbuf_pool;
	unsigned nb_ports;
	uint16_t portid;

    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        std::cerr << "Failed to initialize DPDK environment." << std::endl;
        return 1;
    }

	nb_ports = rte_eth_dev_count_avail();

	if(nb_ports < 2) {
		rte_exit(EXIT_FAILURE,"Invalid port number\n");
	}

	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports,
	MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

	RTE_ETH_FOREACH_DEV(portid)
		if (port_init(portid, mbuf_pool) != 0)
			rte_exit(EXIT_FAILURE, "Cannot init port %u\n", portid);

	if (rte_lcore_count() > 1)
		printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");

	auto listener = new netcache::Listener(conf.connection.in.port);
	auto process_query = new netcache::ProcessQuery(conf.connection.out.port);

	std::cerr << "\nNetCache controller is ready.\n";

	auto last_time = std::chrono::steady_clock::now();

	while (1) {
		// Check if the reset timers have elapsed.
		// If so, reset the respective counters.
		auto cur_time = std::chrono::steady_clock::now();
		auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(cur_time-last_time);

		if (elapsed_time.count() >= netcache::Controller::controller->conf.key_cntr.reset_timer) {
			netcache::Controller::controller->reg_key_count.set_all_false();
			#ifndef NDEBUG
			std::cout << "Reset timer: data plane reg_key_count." << std::endl;
			#endif
		}

		if (elapsed_time.count() >= netcache::Controller::controller->conf.cm.reset_timer) {
			netcache::Controller::controller->reg_cm_0.set_all_false();
			netcache::Controller::controller->reg_cm_1.set_all_false();
			netcache::Controller::controller->reg_cm_2.set_all_false();
			netcache::Controller::controller->reg_cm_3.set_all_false();
			#ifndef NDEBUG
			std::cout << "Reset timer: data plane reg_cm_0." << std::endl;
			std::cout << "Reset timer: data plane reg_cm_1." << std::endl;
			std::cout << "Reset timer: data plane reg_cm_2." << std::endl;
			std::cout << "Reset timer: data plane reg_cm_3." << std::endl;
			#endif
		}

		if (elapsed_time.count() >= netcache::Controller::controller->conf.bloom.reset_timer) {
			netcache::Controller::controller->reg_bloom_0.set_all_false();
			netcache::Controller::controller->reg_bloom_1.set_all_false();
			netcache::Controller::controller->reg_bloom_2.set_all_false();
			#ifndef NDEBUG
			std::cout << "Reset timer: data plane reg_bloom_0." << std::endl;
			std::cout << "Reset timer: data plane reg_bloom_1." << std::endl;
			std::cout << "Reset timer: data plane reg_bloom_2." << std::endl;
			#endif
		}

		auto pkt = listener->receive_query();

		if (pkt.has_valid_protocol()) {
			uint8_t cur_op = pkt.get_netcache_hdr()->op;
			if (cur_op == READ_QUERY) {
				#ifndef NDEBUG
				std::cout << "Received read query." << std::endl;
				#endif
				process_query->read_query(pkt);
			} else {
				std::cerr << "Invalid query received.";
			}
		} else {
			std::cerr << "Invalid packet received.";
		}
	}

	return 0;
}
