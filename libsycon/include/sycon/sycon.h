#pragma once

#include "args.h"
#include "asic.h"
#include "config.h"
#include "externs.h"
#include "log.h"
#include "modules/modules.h"
#include "packet.h"
#include "primitives/primitives.h"
#include "time.h"
#include "util.h"

namespace sycon {

void parse_args(int argc, char *argv[]);
void setup_async_signal_handling_thread();
void nf_setup();
void run_cli();

#define SYNAPSE_CONTROLLER_MAIN(argc, argv)                             \
  parse_args(argc, argv);                                               \
  setup_async_signal_handling_thread();                                 \
  init_switchd();                                                       \
  configure_dev();                                                      \
  register_pcie_pkt_ops();                                              \
  nf_setup();                                                           \
  if (args.run_ucli) {                                                  \
    run_cli();                                                          \
  } else {                                                              \
    WAIT_FOR_ENTER("Controller is running. Press enter to terminate."); \
  }                                                                     \
  nf_exit();

};  // namespace sycon