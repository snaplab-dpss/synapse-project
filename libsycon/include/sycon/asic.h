#pragma once

#include "util.h"

namespace sycon {

void init_switchd();
void configure_dev();
void register_pcie_pkt_ops();

u16 asic_get_dev_port(u16 front_panel_port);
u16 asic_get_front_panel_port_from_dev_port(u16 dev_port);
u64 asic_get_port_rx(u16 dev_port);
u64 asic_get_port_tx(u16 dev_port);
void asic_reset_port_stats();

} // namespace sycon