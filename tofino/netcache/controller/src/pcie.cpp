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

  Controller::controller->begin_transaction();
  bool pkt_tx = Controller::controller->process_pkt((pkt_hdr_t *)(packet), packet_size);
  Controller::controller->end_transaction();

  if (pkt_tx) {
    pcie_tx(device, packet, packet_size);
  }

  int fail = bf_pkt_free(device, orig_pkt);
  assert(fail == 0);

  return BF_SUCCESS;
}

void register_pcie_pkt_ops() {
  if (!bf_pkt_is_inited(Controller::controller->dev_tgt.dev_id)) {
    ERROR("kdrv kernel module not loaded. Exiting.");
    exit(1);
  }

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
