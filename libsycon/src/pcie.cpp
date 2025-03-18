#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "../include/sycon/config.h"
#include "../include/sycon/constants.h"
#include "../include/sycon/externs.h"
#include "../include/sycon/log.h"
#include "../include/sycon/packet.h"
#include "../include/sycon/sycon.h"
#include "packet.h"

extern "C" {
#include <bf_switchd/bf_switchd.h>
#include <pkt_mgr/pkt_mgr_intf.h>
}

#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

namespace sycon {

static void pcie_tx(bf_dev_id_t device, u8 *pkt, u32 packet_size) {
  bf_pkt *tx_pkt = nullptr;

  bf_status_t bf_status = bf_pkt_alloc(cfg.dev_tgt.dev_id, &tx_pkt, packet_size, BF_DMA_CPU_PKT_TRANSMIT_0);
  ASSERT_BF_STATUS(bf_status);

  bf_status = bf_pkt_data_copy(tx_pkt, pkt, packet_size);

  if (bf_status != BF_SUCCESS) {
    bf_pkt_free(device, tx_pkt);
    ASSERT_BF_STATUS(bf_status);
  }

  bf_status = bf_pkt_tx(device, tx_pkt, BF_PKT_TX_RING_0, (void *)tx_pkt);

  if (bf_status != BF_SUCCESS) {
    bf_pkt_free(device, tx_pkt);
    ASSERT_BF_STATUS(bf_status);
  }
}

static bf_status_t txComplete(bf_dev_id_t device, bf_pkt_tx_ring_t tx_ring, u64 tx_cookie, u32 status) {
  // Now we can free the packet.
  bf_pkt_free(device, (bf_pkt *)((uintptr_t)tx_cookie));
  return BF_SUCCESS;
}

static bf_status_t pcie_rx(bf_dev_id_t device, bf_pkt *pkt, void *data, bf_pkt_rx_ring_t rx_ring) {
  bf_pkt *orig_pkt = nullptr;
  char in_packet[SWITCH_PACKET_MAX_BUFFER_SIZE];
  char *pkt_buf   = nullptr;
  char *bufp      = nullptr;
  u32 packet_size = 0;
  u16 pkt_len     = 0;

  // save a pointer to the packet
  orig_pkt = pkt;

  // assemble the received packet
  bufp = &in_packet[0];

  do {
    pkt_buf = (char *)bf_pkt_get_pkt_data(pkt);
    pkt_len = bf_pkt_get_pkt_size(pkt);

    if ((packet_size + pkt_len) > SWITCH_PACKET_MAX_BUFFER_SIZE) {
      LOG_DEBUG("Packet too large to transmit - skipping");
      break;
    }

    memcpy(bufp, pkt_buf, pkt_len);
    bufp += pkt_len;
    packet_size += pkt_len;
    pkt = bf_pkt_get_nextseg(pkt);
  } while (pkt);

  time_ns_t now = get_time();
  u8 *packet    = reinterpret_cast<u8 *>(&in_packet);

  LOG_DEBUG("RX tid=%lu t=%lu", now, syscall(__NR_gettid));

  packet_init(packet_size);

  cfg.begin_transaction();
  bool fwd = nf_process(now, packet, packet_size);
  cfg.end_transaction();

  if (fwd) {
    pcie_tx(device, packet, packet_size);
  }

  int fail = bf_pkt_free(device, orig_pkt);
  assert(fail == 0);

  return BF_SUCCESS;
}

void register_pcie_pkt_ops() {
  if (!bf_pkt_is_inited(cfg.dev_tgt.dev_id)) {
    ERROR("kdrv kernel module not loaded. Exiting.");
    exit(1);
  }

  // register callback for TX complete
  for (int tx_ring = BF_PKT_TX_RING_0; tx_ring < BF_PKT_TX_RING_MAX; tx_ring++) {
    bf_status_t bf_status = bf_pkt_tx_done_notif_register(cfg.dev_tgt.dev_id, txComplete, (bf_pkt_tx_ring_t)tx_ring);
    ASSERT_BF_STATUS(bf_status);
  }

  // register callback for RX
  for (int rx_ring = BF_PKT_RX_RING_0; rx_ring < BF_PKT_RX_RING_MAX; rx_ring++) {
    bf_status_t bf_status = bf_pkt_rx_register(cfg.dev_tgt.dev_id, pcie_rx, (bf_pkt_rx_ring_t)rx_ring, 0);
    ASSERT_BF_STATUS(bf_status);
  }
}

} // namespace sycon