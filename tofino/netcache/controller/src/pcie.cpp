#include "pcie.h"

#include "netcache.h"
#include "constants.h"
#include "process_query.h"
#include "packet.h"

extern "C" {
#include <bf_switchd/bf_switchd.h>
#include <pkt_mgr/pkt_mgr_intf.h>
}

#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

namespace netcache {

volatile int16_t atom;

// DPDK's implementation of an atomic 16b compare and set operation.
static inline int atomic16_cmpset(volatile uint16_t *dst, uint16_t exp, uint16_t src) {
  uint8_t res;
  asm volatile("lock ; "
               "cmpxchgw %[src], %[dst];"
               "sete %[res];"
               : [res] "=a"(res), /* output */
                 [dst] "=m"(*dst)
               : [src] "r"(src), /* input */
                 "a"(exp), "m"(*dst)
               : "memory"); /* no-clobber list */
  return res;
}

void lock() {
  while (!atomic16_cmpset((volatile uint16_t *)&atom, 0, 1)) {
    // prevent the compiler from removing this loop
    __asm__ __volatile__("");
  }
}

void unlock() { atom = 0; }

void pcie_tx(bf_dev_id_t device, uint8_t *pkt, uint32_t packet_size) {
  bf_pkt *tx_pkt = nullptr;

  bf_status_t bf_status = bf_pkt_alloc(Controller::controller->dev_tgt.dev_id, &tx_pkt, packet_size, BF_DMA_CPU_PKT_TRANSMIT_0);
  if (bf_status != BF_SUCCESS) {
    exit(1);
  }

  bf_status = bf_pkt_data_copy(tx_pkt, pkt, packet_size);

  if (bf_status != BF_SUCCESS) {
    bf_pkt_free(device, tx_pkt);
    exit(1);
  }

  bf_status = bf_pkt_tx(device, tx_pkt, BF_PKT_TX_RING_0, (void *)tx_pkt);

  if (bf_status != BF_SUCCESS) {
    bf_pkt_free(device, tx_pkt);
    exit(1);
  }
}

static bf_status_t txComplete(bf_dev_id_t device, bf_pkt_tx_ring_t tx_ring, uint64_t tx_cookie, uint32_t status) {
  // Now we can free the packet.
  bf_pkt_free(device, (bf_pkt *)((uintptr_t)tx_cookie));
  return BF_SUCCESS;
}

static bf_status_t pcie_rx(bf_dev_id_t device, bf_pkt *pkt, void *data, bf_pkt_rx_ring_t rx_ring) {
  bf_pkt *orig_pkt = nullptr;
  char in_packet[SWITCH_PACKET_MAX_BUFFER_SIZE];
  char *pkt_buf        = nullptr;
  char *bufp           = nullptr;
  uint32_t packet_size = 0;
  uint16_t pkt_len     = 0;
  bool pkt_tx          = false;

  // Save a pointer to the packet.
  orig_pkt = pkt;

  // Assemble the received packet.
  bufp = &in_packet[0];

  do {
    pkt_buf = (char *)bf_pkt_get_pkt_data(pkt);
    pkt_len = bf_pkt_get_pkt_size(pkt);

    if ((packet_size + pkt_len) > SWITCH_PACKET_MAX_BUFFER_SIZE) {
      DEBUG("Packet too large to transmit - skipping");
      break;
    }

    memcpy(bufp, pkt_buf, pkt_len);
    bufp += pkt_len;
    packet_size += pkt_len;
    pkt = bf_pkt_get_nextseg(pkt);
  } while (pkt);

  uint8_t *packet = reinterpret_cast<uint8_t *>(&in_packet);

  // Begin transaction.
  bool atomic = true;
  lock();
  bf_status_t bf_status = Controller::controller->session->beginTransaction(atomic);
  ASSERT_BF_STATUS(bf_status);

  auto pkt_hdr = (pkt_hdr_t *)(packet);

  bool valid = pkt_hdr->has_valid_protocol();

  if (!valid) {
    DEBUG("Invalid protocol packet. Ignoring.");
    int fail = bf_pkt_free(device, orig_pkt);
    assert(fail == 0);

    return BF_SUCCESS;
  }

  valid = (packet_size >= (pkt_hdr->get_l2_size() + pkt_hdr->get_l3_size() + pkt_hdr->get_l4_size() + pkt_hdr->get_netcache_hdr_size()));

  if (!valid) {
    DEBUG("Packet too small. Ignoring.");
    int fail = bf_pkt_free(device, orig_pkt);
    assert(fail == 0);

    return BF_SUCCESS;
  }

#ifdef DEBUG
  pkt_hdr->pretty_print_base();
  pkt_hdr->pretty_print_netcache();
#endif

  auto nc_hdr = pkt_hdr->get_netcache_hdr();

	// If pkt doesn't have the status flag set, then it is a HH report sent from the data plane.
	if (nc_hdr->status == 0) {
		if (Controller::controller->available_keys.empty()) {
			// Obtain a vector of sampled values from the data plane.
			std::vector<std::vector<uint32_t>> sampl_vec = 
					netcache::ProcessQuery::process_query->sample_values();

			// Retrieve the index of the smallest counter value from the vector.
			uint32_t smallest_val = std::numeric_limits<uint32_t>::max();
			int smallest_idx      = 0;

			for (size_t i = 0; i < sampl_vec.size(); ++i) {
				if (sampl_vec[i][1] < smallest_val) {
					smallest_val = sampl_vec[i][1];
					smallest_idx = i;
				}
			}

			// If the data plane value counter < the HH report counter,
			// Evict the data plane key/value and send the HH report to the server.
			uint32_t val = 0;
			for (size_t i = 0; i < 4; ++i) {
				val |= (static_cast<uint32_t>(nc_hdr->val[124 + i]) << (i * 8));
			}
			if (sampl_vec[smallest_idx][1] < val) {
				// Remove the key from the keys table and the controller map.
				// Insert the corresponding index to the available_keys set.

				std::array<uint8_t, 16> key_tmp =
						Controller::controller->key_storage[sampl_vec[smallest_idx][0]];
				uint8_t key[KV_KEY_SIZE];
				std::memcpy(key, key_tmp.data(), sizeof(key_tmp));

				Controller::controller->keys.del_entry(key);
				Controller::controller->key_storage[sampl_vec[smallest_idx][0]] = {0};
				Controller::controller->available_keys.insert(sampl_vec[smallest_idx][0]);

				// Remove the corresponding value from the value registers.

				Controller::controller->reg_v0_31.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v32_63.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v64_95.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v96_127.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v128_159.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v160_191.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v192_223.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v224_255.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v256_287.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v288_319.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v320_351.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v352_383.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v384_415.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v416_447.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v448_479.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v480_511.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v512_543.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v544_575.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v576_607.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v608_639.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v640_671.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v672_703.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v704_735.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v736_767.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v768_799.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v800_831.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v832_863.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v864_895.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v896_927.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v928_959.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v960_991.allocate(sampl_vec[smallest_idx][0], 0);
				Controller::controller->reg_v992_1023.allocate(sampl_vec[smallest_idx][0], 0);

				nc_hdr->status = 1;
				pkt_tx         = true;
			}
		}
		else {
			nc_hdr->status = 1;
			pkt_tx         = true;
		}
	// Else: pkt has the status flag set, so it is a server reply.
	} else {
		netcache::ProcessQuery::process_query->update_cache(nc_hdr);
	}

  // End transaction.
  bool block_until_complete = true;
  bf_status                 = Controller::controller->session->commitTransaction(block_until_complete);
  if (bf_status != BF_SUCCESS) {
    exit(1);
  }
  unlock();

  if (pkt_tx) {
    pcie_tx(device, packet, packet_size);
  }

  int fail = bf_pkt_free(device, orig_pkt);
  assert(fail == 0);

  return BF_SUCCESS;
}

void register_pcie_pkt_ops() {
  // register callback for TX complete
  for (int tx_ring = BF_PKT_TX_RING_0; tx_ring < BF_PKT_TX_RING_MAX; tx_ring++) {
    bf_pkt_tx_done_notif_register(Controller::controller->dev_tgt.dev_id, txComplete, (bf_pkt_tx_ring_t)tx_ring);
  }

  // register callback for RX
  for (int rx_ring = BF_PKT_RX_RING_0; rx_ring < BF_PKT_RX_RING_MAX; rx_ring++) {
    bf_status_t bf_status = bf_pkt_rx_register(Controller::controller->dev_tgt.dev_id, pcie_rx, (bf_pkt_rx_ring_t)rx_ring, 0);
    if (bf_status != BF_SUCCESS) {
      exit(1);
    }
  }
}

} // namespace netcache
