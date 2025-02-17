#pragma once

#include <CLI/CLI.hpp>

#include "packet.h"
#include "time.h"

// These should be defined by the application using this library.

namespace sycon {

// Extra app data sent to the CPU.
struct app_t;

// App header printer (for debugging).
extern void print(const struct app_t *);

// Function responsible for initializing NF state.
extern void nf_init();

// Runs on exit (typically used for freeing state allocated by nf_init).
extern void nf_exit();

// Function triggered by the arrival of each packet.
extern bool nf_process(time_ns_t now, u8 *pkt, u16 size);

// Runs on USR1 signal (typically used for reporting or reseting state).
extern void nf_user_signal_handler();

extern void nf_args(CLI::App &app);

} // namespace sycon