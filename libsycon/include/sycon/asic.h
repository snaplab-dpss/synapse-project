#pragma once

#include "util.h"

namespace sycon {

void init_switchd();
void configure_dev();
void register_pcie_pkt_ops();

u64 get_asic_port_rx(u16 dev_port);
u64 get_asic_port_tx(u16 dev_port);

}  // namespace sycon