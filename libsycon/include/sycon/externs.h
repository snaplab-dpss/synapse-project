#pragma once

#include <CLI/CLI.hpp>

#include "packet.h"
#include "time.h"

// These should be defined by the application using this library.

namespace sycon {

struct nf_state_t {
  virtual void rollback() = 0;
  virtual void commit()   = 0;
};

extern std::unique_ptr<nf_state_t> nf_state;

// Function responsible for initializing NF state.
extern void nf_init();

// Runs on exit (typically used for freeing state allocated by nf_init).
extern void nf_exit();

struct nf_process_result_t {
  bool forward;
  bool abort_transaction;

  nf_process_result_t() : forward(true), abort_transaction(false) {}
};

// Function triggered by the arrival of each packet.
extern nf_process_result_t nf_process(time_ns_t now, u8 *pkt, u16 size);

// Runs on USR1 signal (typically used for reporting or reseting state).
extern void nf_user_signal_handler();

extern void nf_args(CLI::App &app);

} // namespace sycon