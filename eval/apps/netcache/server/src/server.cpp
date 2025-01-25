#include <cstdint>
#include <iostream>
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <signal.h>
#include <chrono>

// #include <rte_eal.h>
// #include <rte_common.h>
// #include <rte_ethdev.h>
// #include <rte_log.h>
// #include <rte_mbuf.h>

#include "conf.h"
#include "constants.h"
#include "listener.h"
#include "report.h"
#include "store.h"

#define REPORT_FILE "netcache-server.tsv"

netcache::report_t report;

std::shared_ptr<netcache::Listener> netcache::Listener::listener_ptr;

struct args_t {
	std::string conf_file_path;
	/* int dpdk_port; */

	args_t(int argc, char** argv) {

		conf_file_path=std::string(argv[1]);
		parse_help(argc, argv);
		/* parse_dpdk_port(argc, argv); */
	}

	void help(char** argv) const {
		std::cerr << "Usage: " << argv[0] << " [-h|--help]\n";
		exit(1);
	}

	void parse_help(int argc, char** argv) {
		auto args_str = std::vector<std::string>{
			std::string("-h"),
			std::string("--help"),
		};

		for (auto argi = 1u; argi < argc - 1; argi++) {
			auto arg = std::string(argv[argi]);

			for (auto arg_str : args_str) {
				auto cmp = arg.compare(arg_str);

				if (cmp == 0) {
					help(argv);
				}
			}
		}
	}

	/* void parse_dpdk_port(int argc, char** argv) { */
	/* 	auto arg_str = std::string("-p"); */

	/* 	for (auto argi = 1u; argi < argc - 1; argi++) { */
	/* 		auto arg = std::string(argv[argi]); */
	/* 		auto cmp = arg.compare(arg_str); */

	/* 		if (cmp == 0) { */
	/* 			dpdk_port = atoi(argv[argi + 1]); */
	/* 			std::cout << "DPDK port: " << dpdk_port << std::endl; */
	/* 			return; */
	/* 		} */
	/* 	} */

	/* 	std::cerr << "Option -p not found.\n"; */
	/* 	help(argv); */
	/* } */

	void dump() const {
		std::cerr << "Configuration:\n";
		std::cerr << "\n";
	}
};

void signalHandler(int signum) {
	report.dump(REPORT_FILE);

	std::cout << "Report generated at \"" << REPORT_FILE << "\". Exiting.\n";
	exit(signum);
}

/* static inline int */
/* port_init(uint16_t port, struct rte_mempool *mbuf_pool) { */
/* 	struct rte_eth_conf port_conf; */
/* 	const uint16_t rx_rings = 1, tx_rings = 1; */
/* 	uint16_t nb_rxd = RX_RING_SIZE; */
/* 	uint16_t nb_txd = TX_RING_SIZE; */
/* 	int retval; */
/* 	uint16_t q; */
/* 	struct rte_eth_dev_info dev_info; */
/* 	struct rte_eth_txconf txconf; */

/* 	if (!rte_eth_dev_is_valid_port(port)) { */
/* 		return -1; */
/* 	} */

/* 	memset(&port_conf, 0, sizeof(struct rte_eth_conf)); */

/* 	retval = rte_eth_dev_info_get(port, &dev_info); */
/* 	if (retval != 0) { */
/* 		printf("Error during getting device (port %u) info: %s\n", */
/* 				port, strerror(-retval)); */
/* 		return retval; */
/* 	} */

/* 	if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE) { */
/* 		port_conf.txmode.offloads |= */
/* 			RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE; */
/* 	} */

/* 	/1* Configure the Ethernet device. *1/ */
/* 	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf); */
/* 	if (retval != 0) */
/* 		return retval; */

/* 	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd); */
/* 	if (retval != 0) */
/* 		return retval; */

/* 	/1* Allocate and set up 1 RX queue per Ethernet port. *1/ */
/* 	for (q = 0; q < rx_rings; q++) { */
/* 		retval = rte_eth_rx_queue_setup(port, q, nb_rxd, */
/* 				rte_eth_dev_socket_id(port), NULL, mbuf_pool); */
/* 		if (retval < 0) */
/* 			return retval; */
/* 	} */

/* 	txconf = dev_info.default_txconf; */
/* 	txconf.offloads = port_conf.txmode.offloads; */
/* 	/1* Allocate and set up 1 TX queue per Ethernet port. *1/ */
/* 	for (q = 0; q < tx_rings; q++) { */
/* 		retval = rte_eth_tx_queue_setup(port, q, nb_txd, */
/* 				rte_eth_dev_socket_id(port), &txconf); */
/* 		if (retval < 0) */
/* 			return retval; */
/* 	} */

/* 	/1* Starting Ethernet port. 8< *1/ */
/* 	retval = rte_eth_dev_start(port); */
/* 	/1* >8 End of starting of ethernet port. *1/ */
/* 	if (retval < 0) { */
/* 		return retval; */
/* 	} */

/* 	/1* Display the port MAC address. *1/ */
/* 	struct rte_ether_addr addr; */
/* 	retval = rte_eth_macaddr_get(port, &addr); */
/* 	if (retval != 0) { */
/* 		return retval; */
/* 	} */

/* 	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8 */
/* 			   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n", */
/* 			port, RTE_ETHER_ADDR_BYTES(&addr)); */

/* 	/1* Enable RX in promiscuous mode for the Ethernet device. *1/ */
/* 	/1* retval = rte_eth_promiscuous_enable(port); *1/ */
/* 	/1* End of setting RX port in promiscuous mode. *1/ */
/* 	/1* if (retval != 0) *1/ */
/* 		/1* return retval; *1/ */

/* 	return 0; */
/* } */

int main(int argc, char** argv) {
	signal(SIGINT, signalHandler);
	signal(SIGQUIT, signalHandler);
	signal(SIGTERM, signalHandler);

	auto args = args_t(argc, argv);

	args.dump();

	auto conf = netcache::parse_conf_file(args.conf_file_path);

	int query_cntr = 0;

	/* char** dpdk_args = new char*[argc-2]; */
	/* for (int i = 2; i < argc; ++i) { */
        /* dpdk_args[i-2] = argv[i]; */
    /* } */

	/* struct rte_mempool *mbuf_pool; */
	/* unsigned nb_ports; */
	/* uint16_t portid; */

    /* int ret = rte_eal_init(argc-2, dpdk_args); */
    /* if (ret < 0) { */
    /*     std::cerr << "Failed to initialize DPDK environment." << std::endl; */
    /*     return 1; */
    /* } */

	/* nb_ports = rte_eth_dev_count_avail(); */
	/* if(nb_ports < 2 || (nb_ports & 1)) */
	/* 	rte_exit(EXIT_FAILURE,"Invalid port number\n"); */

	/* mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports, */
	/* MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id()); */

	/* RTE_ETH_FOREACH_DEV(portid) */
	/* 	if (port_init(portid, mbuf_pool) != 0) */
	/* 		rte_exit(EXIT_FAILURE, "Cannot init port %u\n", */
	/* 				portid); */


	/* if (rte_lcore_count() > 1) */
	/* 	printf("\nWARNING: Too many lcores enabled. Only 1 used.\n"); */

	/* auto listener = new netcache::Listener(conf.connection.in.port); */
	auto listener = new netcache::Listener();
	netcache::Listener::listener_ptr = std::shared_ptr<netcache::Listener>(listener);

	auto store = netcache::Store();

	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	while (1) {
		// If the received number of queries has reached a limit, wait for some
		// time before processing additional ones.

		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		uint32_t time_diff = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();

		if (time_diff > conf.query.duration) {
			std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
			query_cntr = 0;
		} else {
			if (query_cntr > conf.query.limit) {
				sleep(conf.query.duration - (conf.query.duration - time_diff));
				query_cntr = 0;
			}
		}

		auto query = listener->receive_query();

		if (query.valid) {
			if (query.op == WRITE_QUERY) {
				query_cntr++;
				#ifndef NDEBUG
				std::cout << "Received write query." << std::endl;
				#endif
				store.write_query(query);
			} else if (query.op == DELETE_QUERY) {
				query_cntr++;
				#ifndef NDEBUG
				std::cout << "Received delete query." << std::endl;
				#endif
				store.del_query(query);
			} else if (query.op == READ_QUERY) {
				query_cntr++;
				#ifndef NDEBUG
				std::cout << "Received read query." << std::endl;
				#endif
				store.read_query(query);
			} else {
				std::cerr << "Invalid query received.";
			}
		}
	}

	return 0;
}
