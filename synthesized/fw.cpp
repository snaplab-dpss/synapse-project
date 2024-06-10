#ifdef __cplusplus
extern "C" {
#endif
#include <lib/verified/cht.h>
#include <lib/verified/double-chain.h>
#include <lib/verified/map.h>
#include <lib/verified/vector.h>
#include <lib/unverified/sketch.h>
#include <lib/unverified/hash.h>
#include <lib/unverified/expirator.h>

#include <lib/verified/expirator.h>
#include <lib/verified/packet-io.h>
#include <lib/verified/tcpudp_hdr.h>
#include <lib/verified/vigor-time.h>
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

#include <stdbool.h>

#define NF_INFO(text, ...)                                                     \
  printf(text "\n", ##__VA_ARGS__);                                            \
  fflush(stdout);

#ifdef ENABLE_LOG
#define NF_DEBUG(text, ...)                                                    \
  fprintf(stderr, "DEBUG: " text "\n", ##__VA_ARGS__);                         \
  fflush(stderr);
#else // ENABLE_LOG
#define NF_DEBUG(...)
#endif // ENABLE_LOG

#define BATCH_SIZE 32

#define MBUF_CACHE_SIZE 256
#define MAX_NUM_DEVICES 32 // this is quite arbitrary...

#define IP_MIN_SIZE_WORDS 5
#define WORD_SIZE 4

#define DROP ((uint16_t)-1)
#define FLOOD ((uint16_t)-2)

static const uint16_t RX_QUEUE_SIZE = 1024;
static const uint16_t TX_QUEUE_SIZE = 1024;

static const unsigned MEMPOOL_BUFFER_COUNT = 2048;

uintmax_t nf_util_parse_int(const char *str, const char *name, int base,
                            char next) {
  char *temp;
  intmax_t result = strtoimax(str, &temp, base);

  // There's also a weird failure case with overflows, but let's not care
  if (temp == str || *temp != next) {
    rte_exit(EXIT_FAILURE, "Error while parsing '%s': %s\n", name, str);
  }

  return result;
}

bool nf_init(void);
int nf_process(uint16_t device, uint8_t *buffer, uint16_t packet_length,
               time_ns_t now);

// Send the given packet to all devices except the packet's own
void flood(struct rte_mbuf *packet, uint16_t nb_devices, uint16_t queue_id) {
  rte_mbuf_refcnt_set(packet, nb_devices - 1);
  int total_sent = 0;
  uint16_t skip_device = packet->port;
  for (uint16_t device = 0; device < nb_devices; device++) {
    if (device != skip_device) {
      total_sent += rte_eth_tx_burst(device, queue_id, &packet, 1);
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
  retval = rte_eth_tx_queue_setup(device, 0, TX_QUEUE_SIZE,
                                  rte_eth_dev_socket_id(device), NULL);
  if (retval != 0) {
    return retval;
  }

  // Allocate and set up RX queues (NULL == default config)
  retval = rte_eth_rx_queue_setup(
      device, 0, RX_QUEUE_SIZE, rte_eth_dev_socket_id(device), NULL, mbuf_pool);
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

// Main worker method (for now used on a single thread...)
static void worker_main(void) {
  if (!nf_init()) {
    rte_exit(EXIT_FAILURE, "Error initializing NF");
  }

  NF_INFO("Core %u forwarding packets.", rte_lcore_id());

  if (rte_eth_dev_count_avail() != 2) {
    rte_exit(EXIT_FAILURE, "We assume there will be exactly 2 devices for our "
                           "simple batching implementation.");
  }

  while (1) {
    unsigned DEVICES_COUNT = rte_eth_dev_count_avail();
    for (uint16_t dev = 0; dev < DEVICES_COUNT; dev++) {
      struct rte_mbuf *mbufs[BATCH_SIZE];
      uint16_t rx_count = rte_eth_rx_burst(dev, 0, mbufs, BATCH_SIZE);

      struct rte_mbuf *mbufs_to_send[BATCH_SIZE];
      uint16_t tx_count = 0;
      for (uint16_t n = 0; n < rx_count; n++) {
        uint8_t *data = rte_pktmbuf_mtod(mbufs[n], uint8_t *);
        packet_state_total_length(data, &(mbufs[n]->pkt_len));
        time_ns_t now = current_time();
        uint16_t dst_device =
            nf_process(mbufs[n]->port, data, mbufs[n]->pkt_len, now);

        if (dst_device == DROP) {
          rte_pktmbuf_free(mbufs[n]);
        } else { // includes flood when 2 devices, which is equivalent
                 // to just
                 // a
                 // send
          mbufs_to_send[tx_count] = mbufs[n];
          tx_count++;
        }
      }

      uint16_t sent_count =
          rte_eth_tx_burst(1 - dev, 0, mbufs_to_send, tx_count);
      for (uint16_t n = sent_count; n < tx_count; n++) {
        rte_pktmbuf_free(mbufs[n]); // should not happen, but we're in
                                    // the unverified case anyway
      }
    }
  }
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

  // Create a memory pool
  unsigned nb_devices = rte_eth_dev_count_avail();
  struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create(
      "MEMPOOL",                         // name
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

struct Map* map;
struct Vector* vector;
struct Vector* vector_1;
struct DoubleChain* dchain;

bool nf_init() {

  if (map_allocate(65536u, 13u, &map) == 0) {
    return 0;
  }


  if (vector_allocate(13u, 65536u, &vector) == 0) {
    return 0;
  }


  if (vector_allocate(4u, 65536u, &vector_1) == 0) {
    return 0;
  }


  if (dchain_allocate(65536u, &dchain) == 0) {
    return 0;
  }

  return 1;
}

int nf_process(uint16_t device, uint8_t* packet, uint16_t packet_length, int64_t now) {
  uint16_t dst_device;
  int number_of_freed_flows = expire_items_single_map(dchain, vector, map, now - 1000000000ul);
  struct rte_ether_hdr* ether_header_0 = (struct rte_ether_hdr*)(packet);

  // 12
  // 19
  // 32
  // 36
  // 41
  // 44
  if ((8u == (ether_header_0->ether_type)) & (20ul <= (4294967282u + packet_length))) {
    struct rte_ipv4_hdr* ipv4_header_0 = (struct rte_ipv4_hdr*)(packet + 14u);

    // 12
    // 19
    // 32
    // 36
    // 41
    if (((6u == (ipv4_header_0->next_proto_id)) | (17u == (ipv4_header_0->next_proto_id))) & ((4294967262u + packet_length) >= 4ul)) {
      struct tcpudp_hdr* tcpudp_header_0 = (struct tcpudp_hdr*)(packet + (14u + 20u));

      // 12
      // 19
      if (0u != device) {
        uint8_t map_key[13];
        map_key[0u] = tcpudp_header_0->dst_port & 0xff;
        map_key[1u] = (tcpudp_header_0->dst_port >> 8) & 0xff;
        map_key[2u] = tcpudp_header_0->src_port & 0xff;
        map_key[3u] = (tcpudp_header_0->src_port >> 8) & 0xff;
        map_key[4u] = ipv4_header_0->dst_addr & 0xff;
        map_key[5u] = (ipv4_header_0->dst_addr >> 8) & 0xff;
        map_key[6u] = (ipv4_header_0->dst_addr >> 16) & 0xff;
        map_key[7u] = (ipv4_header_0->dst_addr >> 24) & 0xff;
        map_key[8u] = ipv4_header_0->src_addr & 0xff;
        map_key[9u] = (ipv4_header_0->src_addr >> 8) & 0xff;
        map_key[10u] = (ipv4_header_0->src_addr >> 16) & 0xff;
        map_key[11u] = (ipv4_header_0->src_addr >> 24) & 0xff;
        map_key[12u] = ipv4_header_0->next_proto_id;
        int map_value_out;
        int map_has_this_key = map_get(map, map_key, &map_value_out);

        // 12
        if (0u == map_has_this_key) {
          dst_device = DROP;
          return dst_device;
        }

        // 19
        else {
          uint8_t* vector_value_out = 0u;
          vector_borrow(vector_1, map_value_out, (void**)(&vector_value_out));
          vector_return(vector_1, map_value_out, vector_value_out);
          dchain_rejuvenate_index(dchain, map_value_out, now);
          *(uint8_t*)(ether_header_0) = 1u;
          ((uint8_t*)ether_header_0)[1u] = 35u;
          ((uint8_t*)ether_header_0)[2u] = 69u;
          ((uint8_t*)ether_header_0)[3u] = 103u;
          ((uint8_t*)ether_header_0)[4u] = 137u;
          ((uint8_t*)ether_header_0)[5u] = 0u;
          ((uint8_t*)ether_header_0)[6u] = 0u;
          ((uint8_t*)ether_header_0)[7u] = 0u;
          ((uint8_t*)ether_header_0)[8u] = 0u;
          ((uint8_t*)ether_header_0)[9u] = 0u;
          ((uint8_t*)ether_header_0)[10u] = 0u;
          ((uint8_t*)ether_header_0)[11u] = 0u;
          dst_device = 0;
          return dst_device;
        } // !(0u == map_has_this_key)

      }

      // 32
      // 36
      // 41
      else {
        uint8_t map_key[13];
        map_key[0u] = tcpudp_header_0->src_port & 0xff;
        map_key[1u] = (tcpudp_header_0->src_port >> 8) & 0xff;
        map_key[2u] = tcpudp_header_0->dst_port & 0xff;
        map_key[3u] = (tcpudp_header_0->dst_port >> 8) & 0xff;
        map_key[4u] = ipv4_header_0->src_addr & 0xff;
        map_key[5u] = (ipv4_header_0->src_addr >> 8) & 0xff;
        map_key[6u] = (ipv4_header_0->src_addr >> 16) & 0xff;
        map_key[7u] = (ipv4_header_0->src_addr >> 24) & 0xff;
        map_key[8u] = ipv4_header_0->dst_addr & 0xff;
        map_key[9u] = (ipv4_header_0->dst_addr >> 8) & 0xff;
        map_key[10u] = (ipv4_header_0->dst_addr >> 16) & 0xff;
        map_key[11u] = (ipv4_header_0->dst_addr >> 24) & 0xff;
        map_key[12u] = ipv4_header_0->next_proto_id;
        int map_value_out;
        int map_has_this_key = map_get(map, map_key, &map_value_out);

        // 32
        // 36
        if (0u == map_has_this_key) {
          int32_t new_index_1;
          int out_of_space_1 = !dchain_allocate_new_index(dchain, &new_index_1, now);

          // 32
          if (false == ((out_of_space_1) & (0u == number_of_freed_flows))) {
            uint8_t* vector_value_out = 0u;
            vector_borrow(vector, new_index_1, (void**)(&vector_value_out));
            memcpy(vector_value_out + 0ul, tcpudp_header_0, 4ul);
            memcpy(vector_value_out + 4ul, (void*)(&ipv4_header_0->src_addr), 8ul);
            memcpy(vector_value_out + 12ul, (void*)(&ipv4_header_0->next_proto_id), 1ul);
            map_put(map, vector_value_out, new_index_1);
            vector_return(vector, new_index_1, vector_value_out);
            uint8_t* vector_value_out_1 = 0u;
            vector_borrow(vector_1, new_index_1, (void**)(&vector_value_out_1));
            vector_value_out_1[0u] = 0u;
            vector_value_out_1[1u] = 0u;
            vector_value_out_1[2u] = 0u;
            vector_value_out_1[3u] = 0u;
            vector_return(vector_1, new_index_1, vector_value_out_1);
            *(uint8_t*)(ether_header_0) = 1u;
            ((uint8_t*)ether_header_0)[1u] = 35u;
            ((uint8_t*)ether_header_0)[2u] = 69u;
            ((uint8_t*)ether_header_0)[3u] = 103u;
            ((uint8_t*)ether_header_0)[4u] = 137u;
            ((uint8_t*)ether_header_0)[5u] = 1u;
            ((uint8_t*)ether_header_0)[6u] = 0u;
            ((uint8_t*)ether_header_0)[7u] = 0u;
            ((uint8_t*)ether_header_0)[8u] = 0u;
            ((uint8_t*)ether_header_0)[9u] = 0u;
            ((uint8_t*)ether_header_0)[10u] = 0u;
            ((uint8_t*)ether_header_0)[11u] = 0u;
            dst_device = 1;
            return dst_device;
          }

          // 36
          else {
            *(uint8_t*)(ether_header_0) = 1u;
            ((uint8_t*)ether_header_0)[1u] = 35u;
            ((uint8_t*)ether_header_0)[2u] = 69u;
            ((uint8_t*)ether_header_0)[3u] = 103u;
            ((uint8_t*)ether_header_0)[4u] = 137u;
            ((uint8_t*)ether_header_0)[5u] = 1u;
            ((uint8_t*)ether_header_0)[6u] = 0u;
            ((uint8_t*)ether_header_0)[7u] = 0u;
            ((uint8_t*)ether_header_0)[8u] = 0u;
            ((uint8_t*)ether_header_0)[9u] = 0u;
            ((uint8_t*)ether_header_0)[10u] = 0u;
            ((uint8_t*)ether_header_0)[11u] = 0u;
            dst_device = 1;
            return dst_device;
          } // !(false == ((out_of_space_1) & (0u == number_of_freed_flows)))

        }

        // 41
        else {
          dchain_rejuvenate_index(dchain, map_value_out, now);
          *(uint8_t*)(ether_header_0) = 1u;
          ((uint8_t*)ether_header_0)[1u] = 35u;
          ((uint8_t*)ether_header_0)[2u] = 69u;
          ((uint8_t*)ether_header_0)[3u] = 103u;
          ((uint8_t*)ether_header_0)[4u] = 137u;
          ((uint8_t*)ether_header_0)[5u] = 1u;
          ((uint8_t*)ether_header_0)[6u] = 0u;
          ((uint8_t*)ether_header_0)[7u] = 0u;
          ((uint8_t*)ether_header_0)[8u] = 0u;
          ((uint8_t*)ether_header_0)[9u] = 0u;
          ((uint8_t*)ether_header_0)[10u] = 0u;
          ((uint8_t*)ether_header_0)[11u] = 0u;
          dst_device = 1;
          return dst_device;
        } // !(0u == map_has_this_key)

      } // !(0u != device)

    }

    // 44
    else {
      dst_device = DROP;
      return dst_device;
    } // !(((6u == (ipv4_header_0->next_proto_id)) | (17u == (ipv4_header_0->next_proto_id))) & ((4294967262u + packet_length) >= 4ul))

  }

  // 46
  else {
    dst_device = DROP;
    return dst_device;
  } // !((8u == (ether_header_0->ether_type)) & (20ul <= (4294967282u + packet_length)))

}
