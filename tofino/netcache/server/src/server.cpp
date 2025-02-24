#include <cstdint>
#include <iostream>
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <signal.h>
#include <chrono>

#include <rte_eal.h>
#include <rte_common.h>
#include <rte_ethdev.h>
#include <rte_log.h>
#include <rte_mbuf.h>

#include "conf.h"
#include "constants.h"
#include "store.h"

struct args_t {
  std::string conf_file_path;

  args_t(int argc, char **argv) {

    conf_file_path = std::string(argv[argc - 1]);
    parse_help(argc, argv);
  }

  void help(char **argv) const {
    std::cerr << "Usage: " << argv[0] << " [-h|--help]\n";
    exit(1);
  }

  void parse_help(int argc, char **argv) {
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

  void dump() const {
    std::cerr << "Configuration:\n";
    std::cerr << "\n";
  }
};

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
    printf("Error during getting device (port %u) info: %s\n", port, strerror(-retval));
    return retval;
  }

  if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE) {
    port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;
  }

  // Configure the Ethernet device.
  retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
  if (retval != 0) {
    return retval;
  }

  retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
  if (retval != 0) {
    return retval;
  }

  // Allocate and set up 1 RX queue per Ethernet port.
  for (q = 0; q < rx_rings; q++) {
    retval = rte_eth_rx_queue_setup(port, q, nb_rxd, rte_eth_dev_socket_id(port), NULL, mbuf_pool);
    if (retval < 0) {
      return retval;
    }
  }

  txconf          = dev_info.default_txconf;
  txconf.offloads = port_conf.txmode.offloads;
  // Allocate and set up 1 TX queue per Ethernet port.
  for (q = 0; q < tx_rings; q++) {
    retval = rte_eth_tx_queue_setup(port, q, nb_txd, rte_eth_dev_socket_id(port), &txconf);
    if (retval < 0) {
      return retval;
    }
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

  // Enable RX in promiscuous mode for the ethernet device.
  retval = rte_eth_promiscuous_enable(0);
  if (retval != 0) {
    return retval;
  }

  return 0;
}

int main(int argc, char **argv) {
  auto args = args_t(argc, argv);
  args.dump();

  auto conf = netcache::parse_conf_file(args.conf_file_path);

  int query_cntr = 0;

  struct rte_mempool *mbuf_pool;
  unsigned nb_ports;
  uint16_t portid;

  int ret0 = rte_eal_init(argc, argv);
  if (ret0 < 0) {
    std::cerr << "Failed to initialize DPDK environment." << std::endl;
    return 1;
  }

  nb_ports = rte_eth_dev_count_avail();

  if (nb_ports < 2) {
    rte_exit(EXIT_FAILURE, "Invalid port number\n");
  }

  mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

  if (mbuf_pool == NULL)
    rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

  RTE_ETH_FOREACH_DEV(portid)
  if (port_init(portid, mbuf_pool) != 0)
    rte_exit(EXIT_FAILURE, "Cannot init port %u\n", portid);

  auto store = new netcache::Store(conf.connection.in.port, conf.connection.out.port, mbuf_pool);

  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

  std::cout << "***** Server started *****" << std::endl;

  while (1) {
    // If the received number of queries has reached a limit, wait for some
    // time before processing additional ones.

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    uint32_t time_diff                        = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();

    if (time_diff > conf.query.duration) {
      std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
      query_cntr                                  = 0;
    } else {
      if (query_cntr > conf.query.limit) {
        sleep(conf.query.duration - (conf.query.duration - time_diff));
        query_cntr = 0;
      }
    }

    store->read_query();
  }

  return 0;
}
