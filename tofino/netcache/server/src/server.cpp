#include <cstdint>
#include <iostream>
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <signal.h>
#include <CLI/CLI.hpp>
#include <filesystem>

#include <rte_eal.h>
#include <rte_common.h>
#include <rte_ethdev.h>
#include <rte_log.h>
#include <rte_mbuf.h>

#include "constants.h"
#include "store.h"

struct args_t {
  int in_port;
  int out_port;
  int64_t processing_delay_per_query_us;

  args_t() : in_port(0), out_port(1), processing_delay_per_query_us(0) {}

  void dump() const {
    std::cerr << "Configuration:\n";
    std::cerr << "  Input port: " << in_port << "\n";
    std::cerr << "  Output port: " << out_port << "\n";
    std::cerr << "  Processing delay per query (us): " << processing_delay_per_query_us << "\n";
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
  int ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    rte_exit(EXIT_FAILURE, "Cannot init EAL\n");
  }
  argc -= ret;
  argv += ret;

  CLI::App app{"KVS server"};

  args_t args;
  bool dry_run{false};

  app.add_option("--in", args.in_port, "Ingress port.");
  app.add_option("--out", args.out_port, "Egress port.");
  app.add_option("--delay", args.processing_delay_per_query_us, "Processing per-query delay (us).");
  app.add_flag("--dry-run", dry_run, "Don't run, just print out the configuration.");

  CLI11_PARSE(app, argc, argv);

  args.dump();

  if (dry_run) {
    return 0;
  }

  unsigned nb_ports = rte_eth_dev_count_avail();

  if (nb_ports == 0) {
    rte_exit(EXIT_FAILURE, "No ports provided\n");
  }

  struct rte_mempool *mbuf_pool =
      rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

  if (mbuf_pool == NULL)
    rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

  uint16_t portid;
  RTE_ETH_FOREACH_DEV(portid)
  if (port_init(portid, mbuf_pool) != 0)
    rte_exit(EXIT_FAILURE, "Cannot init port %u\n", portid);

  netcache::Store store(args.processing_delay_per_query_us, args.in_port, args.out_port);

  std::cout << "***** Server started *****" << std::endl;
  store.run();

  return 0;
}
