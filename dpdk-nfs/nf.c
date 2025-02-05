#include <inttypes.h>
// DPDK uses these but doesn't include them. :|
#include <linux/limits.h>
#include <sys/types.h>

#include <rte_common.h>
#include <rte_eal.h>
#include <rte_errno.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>

#include "lib/util/boilerplate.h"
#include "lib/util/packet-io.h"
#include "nf-log.h"
#include "nf-util.h"
#include "nf.h"

#ifdef KLEE_VERIFICATION
#include "lib/models/hardware.h"
#include "lib/models/util/time-control.h"
#include <klee/klee.h>
#endif // KLEE_VERIFICATION

// Unverified support for batching, useful for performance comparisons
#ifndef BATCH_SIZE
#ifdef KLEE_VERIFICATION
#define BATCH_SIZE 1
#else
#define BATCH_SIZE 32
#endif // KLEE_VERIFICATION
#endif // BATCH_SIZE

#if BATCH_SIZE == 1
// Queue sizes for receiving/transmitting packets
// NOT powers of 2 so that ixgbe doesn't use vector stuff
// but they have to be multiples of 8, and at least 32,
// otherwise the driver refuses to work
static const uint16_t RX_QUEUE_SIZE = 96;
static const uint16_t TX_QUEUE_SIZE = 96;
#else
// Do the opposite: we want batching!
static const uint16_t RX_QUEUE_SIZE = 1024;
static const uint16_t TX_QUEUE_SIZE = 1024;
#endif

// Buffer count for mempools
static const unsigned MEMPOOL_BUFFER_COUNT = 2048;

// Send the given packet to all devices except the packet's own
void flood(struct rte_mbuf *packet, uint16_t nb_devices) {
  rte_mbuf_refcnt_set(packet, nb_devices - 1);
  int total_sent       = 0;
  uint16_t skip_device = packet->port;
  for (uint16_t device = 0; device < nb_devices; device++) {
    if (device != skip_device) {
      total_sent += rte_eth_tx_burst(device, 0, &packet, 1);
    }
  }
  // should not happen, but in case we couldn't transmit, ensure the packet is
  // freed
  if (total_sent != nb_devices - 1) {
    rte_mbuf_refcnt_set(packet, 1);
    rte_pktmbuf_free(packet);
  }
}

// Initializes the given device using the given memory pool
static int nf_init_device(uint16_t device, struct rte_mempool *mbuf_pool) {
  int retval;

  // device_conf passed to rte_eth_dev_configure cannot be NULL
  struct rte_eth_conf device_conf = {0};
  // device_conf.rxmode.hw_strip_crc = 1;

  // Configure the device (1, 1 == number of RX/TX queues)
  retval = rte_eth_dev_configure(device, 1, 1, &device_conf);
  if (retval != 0) {
    return retval;
  }

  // Allocate and set up a TX queue (NULL == default config)
  retval = rte_eth_tx_queue_setup(device, 0, TX_QUEUE_SIZE, rte_eth_dev_socket_id(device), NULL);
  if (retval != 0) {
    return retval;
  }

  // Allocate and set up RX queues (NULL == default config)
  retval = rte_eth_rx_queue_setup(device, 0, RX_QUEUE_SIZE, rte_eth_dev_socket_id(device), NULL, mbuf_pool);
  if (retval != 0) {
    return retval;
  }

  // Start the device
  retval = rte_eth_dev_start(device);
  if (retval != 0) {
    return retval;
  }

  // Enable RX in promiscuous mode, just in case
  rte_eth_promiscuous_enable(device);
  if (rte_eth_promiscuous_get(device) != 1) {
    return retval;
  }

  return 0;
}

#ifdef KLEE_VERIFICATION
static void worker_loop() {
  unsigned lcore_id      = 0; /* no multicore support for now */
  time_ns_t start        = start_time();
  int loop_termination   = klee_int("loop_termination");
  unsigned devices_count = rte_eth_dev_count_avail();
  while (klee_induce_invariants() & loop_termination) {
    nf_loop_iteration_border(lcore_id, start);
    time_ns_t now   = current_time();
    uint16_t device = klee_range(0, devices_count, "DEVICE");
    klee_assume(device != DROP);
    klee_assume(device != FLOOD);
    stub_hardware_receive_packet(device);

    struct rte_mbuf *mbuf;
    if (rte_eth_rx_burst(device, 0, &mbuf, 1) != 0) {
      uint8_t *data = rte_pktmbuf_mtod(mbuf, uint8_t *);
      packet_state_total_length(data, &(mbuf->pkt_len));

      uint16_t dst_device = nf_process(device, &data, mbuf->pkt_len, now, mbuf);
      nf_return_all_chunks(data);

      if (dst_device == DROP) {
        rte_pktmbuf_free(mbuf);
      } else if (dst_device == FLOOD) {
        packet_broadcast(&data, device);
      } else {
        concretize_devices(&dst_device, devices_count);
        int i = rte_eth_tx_burst(dst_device, 0, &mbuf, 1);
        klee_assert(i == 1);
      }
    }

    stub_hardware_reset_receive(device);
  }
}
#else
static void worker_loop() {
  NF_INFO("Core %u forwarding packets.", rte_lcore_id());

  struct tx_mbuf_batch {
    struct rte_mbuf *batch[BATCH_SIZE];
    uint16_t tx_count;
  };

  uint16_t devices_count                  = rte_eth_dev_count_avail();
  struct tx_mbuf_batch *tx_batch_per_port = (struct tx_mbuf_batch *)calloc(devices_count, sizeof(struct tx_mbuf_batch));

  while (1) {
    for (uint16_t device = 0; device < devices_count; device++) {
      struct rte_mbuf *mbufs[BATCH_SIZE];
      uint16_t rx_count = rte_eth_rx_burst(device, 0, mbufs, BATCH_SIZE);

      for (uint16_t n = 0; n < rx_count; n++) {
        uint8_t *data = rte_pktmbuf_mtod(mbufs[n], uint8_t *);
        packet_state_total_length(data, &(mbufs[n]->pkt_len));
        time_ns_t now       = current_time();
        uint16_t dst_device = nf_process(mbufs[n]->port, &data, mbufs[n]->pkt_len, now, mbufs[n]);
        nf_return_all_chunks(data);

        if (dst_device == DROP) {
          rte_pktmbuf_free(mbufs[n]);
        } else {
          uint16_t tx_count                             = tx_batch_per_port[dst_device].tx_count;
          tx_batch_per_port[dst_device].batch[tx_count] = mbufs[n];
          tx_batch_per_port[dst_device].tx_count++;
        }
      }

      for (uint16_t dst_device = 0; dst_device < devices_count; dst_device++) {
        uint16_t sent_count = rte_eth_tx_burst(dst_device, 0, tx_batch_per_port[dst_device].batch, tx_batch_per_port[dst_device].tx_count);
        for (uint16_t n = sent_count; n < tx_batch_per_port[dst_device].tx_count; n++) {
          rte_pktmbuf_free(mbufs[n]); // should not happen, but we're in
                                      // the unverified case anyway
        }
        tx_batch_per_port[dst_device].tx_count = 0;
      }
    }
  }
}
#endif

// Main worker method
static void worker_main(void) {
  if (!nf_init()) {
    rte_exit(EXIT_FAILURE, "Error initializing NF");
  }

  worker_loop();
}

// Entry point
int main(int argc, char **argv) {
  // Initialize the DPDK Environment Abstraction Layer (EAL)
  int ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    rte_exit(EXIT_FAILURE, "Error with EAL initialization, ret=%d\n", ret);
  }
  argc -= ret;
  argv += ret;

  // NF-specific config
  nf_config_init(argc, argv);
  nf_config_print();

  // Create a memory pool
  unsigned nb_devices           = rte_eth_dev_count_avail();
  struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create("MEMPOOL",                         // name
                                                          MEMPOOL_BUFFER_COUNT * nb_devices, // #elements
                                                          0, // cache size (per-core, not useful in a single-threaded app)
                                                          0, // application private area size
                                                          RTE_MBUF_DEFAULT_BUF_SIZE, // data buffer size
                                                          rte_socket_id()            // socket ID
  );
  if (mbuf_pool == NULL) {
    rte_exit(EXIT_FAILURE, "Cannot create pool: %s\n", rte_strerror(rte_errno));
  }

  // Initialize all devices
  for (uint16_t device = 0; device < nb_devices; device++) {
    ret = nf_init_device(device, mbuf_pool);
    if (ret == 0) {
      NF_INFO("Initialized device %" PRIu16 ".", device);
    } else {
      rte_exit(EXIT_FAILURE, "Cannot init device %" PRIu16 ": %d", device, ret);
    }
  }

  // Run!
  worker_main();

  return 0;
}
