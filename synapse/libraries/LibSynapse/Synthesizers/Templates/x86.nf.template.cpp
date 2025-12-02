#include <cstdint>
#include <cstdlib>
#ifdef __cplusplus
extern "C" {
#endif
#include <lib/state/map.h>
#include <lib/state/vector.h>
#include <lib/state/double-chain.h>
#include <lib/state/cht.h>
#include <lib/state/cms.h>
#include <lib/state/token-bucket.h>
#include <lib/state/lpm-dir-24-8.h>

#include <lib/util/hash.h>
#include <lib/util/expirator.h>
#include <lib/util/packet-io.h>
#include <lib/util/tcpudp_hdr.h>
#include <lib/util/time.h>
#ifdef __cplusplus
}
#endif

#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>

#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_random.h>

#include <cstdbool>

#include <iostream>

#define NF_INFO(text, ...)                                                                                             \
  printf(text "\n", ##__VA_ARGS__);                                                                                    \
  fflush(stdout);

#ifdef ENABLE_LOG
#define NF_DEBUG(text, ...)                                                                                            \
  fprintf(stderr, "DEBUG: " text "\n", ##__VA_ARGS__);                                                                 \
  fflush(stderr);
#else // ENABLE_LOG
#define NF_DEBUG(...)
#endif // ENABLE_LOG

#define BATCH_SIZE 32
#define MAX_NUM_DEVICES 32 // this is quite arbitrary...

#define DROP ((uint16_t)-1)
#define FLOOD ((uint16_t)-2)

static const uint16_t RX_QUEUE_SIZE = 1024;
static const uint16_t TX_QUEUE_SIZE = 1024;

static const unsigned MEMPOOL_BUFFER_COUNT = 2048;

struct KeyVecMap {
  Map *map;
  Vector *vector;
  size_t key_size;
};

int key_vec_map_allocate(unsigned capacity, unsigned key_size, struct KeyVecMap **key_vec_map_out) {

    struct KeyVecMap *old_key_vec_map = *key_vec_map_out;
    struct KeyVecMap *key_vec_map_alloc = (struct KeyVecMap *)malloc(sizeof(struct KeyVecMap));

    if (key_vec_map_alloc == 0)
      return 0;
    *key_vec_map_out = (struct KeyVecMap *) key_vec_map_alloc;

    std::cerr << "ALLocating Map\n";
    struct Map *map;
    int map_alloc_success = map_allocate(capacity, key_size, &map);
    if (!map_alloc_success) {
      std::cerr << "MapAllocation failed\n";
      *key_vec_map_out = old_key_vec_map;
      free(key_vec_map_alloc); 
      return 0;
    }

    std::cerr << "ALLocating Vector\n";
    struct Vector *vector;
    int vector_alloc_success = vector_allocate(key_size, capacity, &vector);
    if (!vector_alloc_success) {
      std::cerr << "VectorAllocation Failed\n";
      *key_vec_map_out = old_key_vec_map;
      free(key_vec_map_alloc);
      return 0;
    }

    (*key_vec_map_out)->map = map;
    (*key_vec_map_out)->vector = vector;
    (*key_vec_map_out)->key_size = key_size;

    return 1;
}

int key_vec_map_get(struct KeyVecMap *kvm, void *key, int *value_out) {

  int map_hit = map_get(kvm->map, key, value_out);
  return map_hit;
}

void key_vec_map_put(struct KeyVecMap *kvm, void *key, int value) {

  void *cell;

  vector_borrow(kvm->vector, value, &cell);
  memcpy(cell, key, kvm->key_size);
  map_put(kvm->map, cell, value);
  vector_return(kvm->vector, value, cell);
}

int key_vec_map_expire_items_single_map(struct DoubleChain *dchain, struct KeyVecMap * kvm, time_ns_t time) {
  return expire_items_single_map(dchain, kvm->vector, kvm->map, time);
}

int key_vec_map_expire_items_single_map_iteratively(struct KeyVecMap * kvm, int start, int n_elems) {
    return expire_items_single_map_iteratively(kvm->vector, kvm->map, start, n_elems);
}

void key_vec_map_erase(struct KeyVecMap *kvm, void *key, void **trash);

void debug_packet_info(uint16_t device, uint8_t *buffer, uint16_t length, 
                       time_ns_t now, struct rte_mbuf *mbuf) {
    printf("[NF_PACKET] {dev=%u len=%u now=%lu mbuf=%p pkt_len=%u off=%u ref=%u unread=%u}\n",
           device, length, now, mbuf, mbuf->pkt_len, mbuf->data_off,
           rte_mbuf_refcnt_read(mbuf), packet_get_unread_length(buffer));
}

bool nf_init();
int nf_process(uint16_t device, uint8_t *buffer, uint16_t packet_length, time_ns_t now, struct rte_mbuf *mbuf);

// Send the given packet to all devices except the packet's own
void flood(struct rte_mbuf *packet, uint16_t nb_devices, uint16_t queue_id) {
  rte_mbuf_refcnt_set(packet, nb_devices - 1);
  int total_sent       = 0;
  uint16_t skip_device = packet->port;
  for (uint16_t device = 0; device < nb_devices; device++) {
    if (device != skip_device) {
      total_sent += rte_eth_tx_burst(device, queue_id, &packet, 1);
    }
  }
  // Should not happen, but in case we couldn't transmit, ensure the packet is
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

  retval = rte_eth_dev_start(device);
  if (retval != 0) {
    return retval;
  }

  rte_eth_promiscuous_enable(device);
  if (rte_eth_promiscuous_get(device) != 1) {
    return retval;
  }

  return 0;
}

void debug_dst_device(uint16_t dst_device) {
  if (dst_device == DROP) {
    std::cerr << "DROPPED\n";
  } else if (dst_device == FLOOD) {
    std::cerr << "FLOOD\n";
  } else {
    std::cerr << "FORWARDING PACKET TO: " << dst_device << "\n";
  }
}

static void worker_main(void) {
  if (!nf_init()) {
    rte_exit(EXIT_FAILURE, "Error initializing NF");
  }

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
        uint16_t dst_device = nf_process(mbufs[n]->port, data, mbufs[n]->pkt_len, now, mbufs[n]);

        debug_dst_device(dst_device);
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

int main(int argc, char **argv) {
  int ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    rte_exit(EXIT_FAILURE, "Error with EAL initialization, ret=%d\n", ret);
  }
  argc -= ret;
  argv += ret;

  unsigned nb_devices = rte_eth_dev_count_avail();
  struct rte_mempool *mbuf_pool =
      rte_pktmbuf_pool_create("MEMPOOL",                         // name
                              MEMPOOL_BUFFER_COUNT * nb_devices, // #elements
                              0,                         // cache size (per-core, not useful in a single-threaded app)
                              0,                         // application private area size
                              RTE_MBUF_DEFAULT_BUF_SIZE, // data buffer size
                              rte_socket_id()            // socket ID
      );
  if (mbuf_pool == NULL) {
    rte_exit(EXIT_FAILURE, "Cannot create pool: %s\n", rte_strerror(rte_errno));
  }

  for (uint16_t device = 0; device < nb_devices; device++) {
    ret = nf_init_device(device, mbuf_pool);
    if (ret == 0) {
      NF_INFO("Initialized device %" PRIu16 ".", device);
    } else {
      rte_exit(EXIT_FAILURE, "Cannot init device %" PRIu16 ": %d", device, ret);
    }
  }

  worker_main();

  return 0;
}

/*@{NF_STATE}@*/

/*@{NF_INIT}@*/

/*@{NF_PROCESS}@*/
