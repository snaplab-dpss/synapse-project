#pragma once

#include <CLI/CLI.hpp>

#include "packet.h"
#include "time.h"

// These should be defined by the application using this library.

namespace sycon {

// Extra app data sent to the CPU.
struct app_t;

// App header printer (for debugging).
void print(const struct app_t*);

// Function responsible for initializing NF state.
void nf_init();

// Runs on exit (typically used for freeing state allocated by nf_init).
void nf_exit();

// Function triggered by the arrival of each packet.
bool nf_process(time_ns_t now, byte_t* pkt, u16 size);

// Runs on USR1 signal (typically used for reporting or reseting state).
void nf_user_signal_handler();

void nf_args(CLI::App& app);

}  // namespace sycon