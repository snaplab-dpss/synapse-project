#pragma once

#include <unistd.h>

#include "args.h"
#include "asic.h"
#include "config.h"
#include "externs.h"
#include "log.h"
#include "data_structures/data_structures.h"
#include "packet.h"
#include "primitives/primitives.h"
#include "time.h"
#include "util.h"
#include "buffer.h"
#include "field.h"
#include "hash.h"

namespace sycon {

void parse_args(int argc, char *argv[]);
void setup_async_signal_handling_thread();
void nf_setup();
void run_cli();
void run_bench_cli();

#define SYNAPSE_CONTROLLER_MAIN(argc, argv)                                                                                                          \
  parse_args(argc, argv);                                                                                                                            \
  setup_async_signal_handling_thread();                                                                                                              \
  init_switchd();                                                                                                                                    \
  configure_dev();                                                                                                                                   \
  register_pcie_pkt_ops();                                                                                                                           \
  nf_setup();                                                                                                                                        \
  if (args.run_ucli) {                                                                                                                               \
    run_cli();                                                                                                                                       \
  } else if (args.bench_mode) {                                                                                                                      \
    run_bench_cli();                                                                                                                                 \
  } else {                                                                                                                                           \
    LOG_DEBUG("Warning: running in debug mode");                                                                                                     \
    WAIT_FOR_ENTER("Controller is running. Press enter to terminate.");                                                                              \
  }                                                                                                                                                  \
  nf_exit();

} // namespace sycon