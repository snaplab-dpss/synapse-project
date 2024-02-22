#pragma once

#include "args.h"
#include "externs.h"
#include "log.h"
#include "modules/modules.h"
#include "nf_config.h"
#include "packet.h"
#include "primitives/primitives.h"
#include "time.h"
#include "transactions.h"
#include "util.h"

namespace sycon {

void parse_args(int argc, char *argv[]);
void init_switchd();
void configure_ports();
void register_pcie_pkt_ops();
void nf_setup();
void run_cli();

};  // namespace sycon