#pragma once

#include "args.hpp"
#include "asic.hpp"
#include "config.hpp"
#include "externs.hpp"
#include "log.hpp"
#include "modules/modules.hpp"
#include "packet.hpp"
#include "primitives/primitives.hpp"
#include "time.hpp"
#include "util.hpp"

namespace sycon {

void parse_args(int argc, char *argv[]);
void setup_async_signal_handling_thread();
void nf_setup();
void run_cli();
void run_bench_cli();

#define SYNAPSE_CONTROLLER_MAIN(argc, argv)                                                                                                \
  parse_args(argc, argv);                                                                                                                  \
  setup_async_signal_handling_thread();                                                                                                    \
  init_switchd();                                                                                                                          \
  configure_dev();                                                                                                                         \
  register_pcie_pkt_ops();                                                                                                                 \
  nf_setup();                                                                                                                              \
  if (args.run_ucli) {                                                                                                                     \
    run_cli();                                                                                                                             \
  } else if (args.bench_mode) {                                                                                                            \
    run_bench_cli();                                                                                                                       \
  } else {                                                                                                                                 \
    WAIT_FOR_ENTER("Controller is running. Press enter to terminate.");                                                                    \
  }                                                                                                                                        \
  nf_exit();

} // namespace sycon