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
#include <rte_malloc.h>

#include "constants.h"
#include "store.h"

struct args_t {
  int in_port;
  int out_port;
  int64_t processing_delay_per_query_ns;

  args_t() : in_port(0), out_port(1), processing_delay_per_query_ns(0) {}

  void dump() const {
    std::cerr << "Configuration:\n";
    std::cerr << "  Input port: " << in_port << "\n";
    std::cerr << "  Output port: " << out_port << "\n";
    std::cerr << "  Processing delay per query (ns): " << processing_delay_per_query_ns << "\n";
    std::cerr << "\n";
  }
};

static inline int port_init(uint16_t port, struct rte_mempool **mbuf_pools, std::unordered_map<uint16_t, uint16_t> &lcore_to_rx_queue) {
  int retval = rte_eth_dev_is_valid_port(port);

  if (retval == 0) {
    rte_exit(EXIT_FAILURE, "Invalid port %u\n", port);
    return retval;
  }

  struct rte_eth_dev_info dev_info;
  retval = rte_eth_dev_info_get(port, &dev_info);
  if (retval != 0) {
    rte_exit(EXIT_FAILURE, "Error during getting device (port %u) info: %s\n", port, strerror(-retval));
    return retval;
  }

  struct rte_eth_conf port_conf         = {0};
  port_conf.rxmode.mq_mode              = RTE_ETH_MQ_RX_RSS;
  port_conf.rx_adv_conf.rss_conf.rss_hf = RTE_ETH_RSS_NONFRAG_IPV4_UDP;
  if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE) {
    port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;
  }

  const uint16_t num_queues = rte_lcore_count();
  const uint16_t rx_rings   = num_queues;
  const uint16_t tx_rings   = num_queues;
  retval                    = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
  if (retval != 0) {
    rte_exit(EXIT_FAILURE, "Cannot configure device: %s\n", rte_strerror(-retval));
    return retval;
  }

  uint16_t nb_rxd = RX_RING_SIZE;
  uint16_t nb_txd = TX_RING_SIZE;
  retval          = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
  if (retval != 0) {
    rte_exit(EXIT_FAILURE, "rte_eth_dev_adjust_nb_rx_tx_desc: %s\n", rte_strerror(-retval));
    return retval;
  }

  uint16_t rxq = 0;
  uint16_t lcore_id;
  RTE_LCORE_FOREACH(lcore_id) {
    lcore_to_rx_queue[lcore_id] = rxq;
    retval                      = rte_eth_rx_queue_setup(port, rxq, nb_rxd, rte_eth_dev_socket_id(port), NULL, mbuf_pools[rxq]);
    if (retval != 0) {
      rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup: %s\n", rte_strerror(-retval));
      return retval;
    }
    rxq++;
  }

  for (uint16_t q = 0; q < tx_rings; q++) {
    retval = rte_eth_tx_queue_setup(port, q, nb_txd, rte_eth_dev_socket_id(port), NULL);
    if (retval < 0) {
      rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup: %s\n", rte_strerror(-retval));
      return retval;
    }
  }

  retval = rte_eth_dev_start(port);
  if (retval < 0) {
    rte_exit(EXIT_FAILURE, "rte_eth_dev_start: %s\n", rte_strerror(-retval));
    return retval;
  }

  retval = rte_eth_promiscuous_enable(port);
  if (retval != 0) {
    rte_exit(EXIT_FAILURE, "rte_eth_promiscuous_enable: %s\n", rte_strerror(-retval));
    return retval;
  }

  return 0;
}

void print_port_stats(int) {
  struct rte_eth_xstat xstats[128];
  struct rte_eth_xstat_name xstat_names[128];
  int num_xstats, i;

  num_xstats = rte_eth_xstats_get_names(0, xstat_names, 128);
  if (num_xstats < 0) {
    printf("Failed to get xstat names for port %u\n", 0);
    return;
  }

  num_xstats = rte_eth_xstats_get(0, xstats, 128);
  if (num_xstats < 0) {
    printf("Failed to get xstats for port %u\n", 0);
    return;
  }

  for (i = 0; i < num_xstats; i++) {
    printf("%s: %lu\n", xstat_names[xstats[i].id].name, xstats[i].value);
  }
  exit(0);
}

struct worker_args_t {
  int64_t processing_delay_per_query_ns;
  int in_port;
  int out_port;
  uint16_t rx_queue;
};

static void worker_main(void *args) {
  worker_args_t worker_args = *(worker_args_t *)args;

  printf("KVS server started on lcore %u\n", rte_lcore_id());
  fflush(stdout);

  netcache::Store store(worker_args.processing_delay_per_query_ns, worker_args.in_port, worker_args.out_port, worker_args.rx_queue);
  store.run();
}

int main(int argc, char **argv) {
  signal(SIGINT, print_port_stats);

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
  app.add_option("--delay", args.processing_delay_per_query_ns, "Processing per-query delay (ns).");
  app.add_flag("--dry-run", dry_run, "Don't run, just print out the configuration.");

  CLI11_PARSE(app, argc, argv);

  args.dump();

  if (dry_run) {
    return 0;
  }

  const uint16_t nb_ports = rte_eth_dev_count_avail();

  if (nb_ports != 1) {
    rte_exit(EXIT_FAILURE, "KVS look only at a single port, but %u were provided\n", nb_ports);
  }

  struct rte_mempool **mbuf_pools;
  mbuf_pools = (struct rte_mempool **)rte_malloc(NULL, sizeof(struct rte_mempool *) * rte_lcore_count(), 64);

  uint16_t lcore_id;
  uint16_t lcore_idx = 0;
  RTE_LCORE_FOREACH(lcore_id) {
    char MBUF_POOL_NAME[20];
    sprintf(MBUF_POOL_NAME, "MEMORY_POOL_%u", lcore_idx);

    mbuf_pools[lcore_idx] =
        rte_pktmbuf_pool_create(MBUF_POOL_NAME, NUM_MBUFS * nb_ports, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if (mbuf_pools[lcore_idx] == NULL) {
      rte_exit(EXIT_FAILURE, "Cannot create mbuf pool: %s\n", rte_strerror(rte_errno));
    }

    lcore_idx++;
  }

  const uint16_t portid = 0;
  std::unordered_map<uint16_t, uint16_t> lcore_to_rx_queue;
  if (port_init(portid, mbuf_pools, lcore_to_rx_queue) != 0) {
    rte_exit(EXIT_FAILURE, "Cannot init port %u\n", portid);
  }

  std::unordered_map<uint16_t, worker_args_t> lcore_to_worker_args;
  RTE_LCORE_FOREACH(lcore_id) {
    lcore_to_worker_args[lcore_id] = worker_args_t{
        .processing_delay_per_query_ns = args.processing_delay_per_query_ns,
        .in_port                       = args.in_port,
        .out_port                      = args.out_port,
        .rx_queue                      = lcore_to_rx_queue.at(lcore_id),
    };
  }

  RTE_LCORE_FOREACH_WORKER(lcore_id) {
    rte_eal_remote_launch((lcore_function_t *)worker_main, (void *)&lcore_to_worker_args.at(lcore_id), lcore_id);
  }

  worker_main((void *)&lcore_to_worker_args.at(rte_lcore_id()));

  return 0;
}
