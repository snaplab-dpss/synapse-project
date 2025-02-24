#pragma once

#include <bf_switchd/bf_switchd.h>
#include <pkt_mgr/pkt_mgr_intf.h>
#include <sys/types.h>

namespace netcache {

static inline int atomic16_cmpset(volatile uint16_t *dst, uint16_t exp, uint16_t src);
void lock();
void unlock();
static void pcie_tx(bf_dev_id_t device, uint8_t *pkt, uint32_t packet_size);
static bf_status_t txComplete(bf_dev_id_t device, bf_pkt_tx_ring_t tx_ring,
							  uint64_t tx_cookie, uint32_t status);
void register_pcie_pkt_ops();

}
