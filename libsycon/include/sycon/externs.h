#pragma once

#include "packet.h"
#include "time.h"

// These should be defined by the application using this library.

namespace sycon {

// Extra app data sent to the CPU
struct app_t;

// App header printer (for debugging)
void print(const struct app_t*);

// Function responsible for initializing NF state
void nf_init();

// CLeanup function that runs on exit (typically for freeing state allocated by
// nf_init)
void nf_cleanup();

// Function triggered by the arrival of each packet
bool nf_process(time_ns_t now, byte_t* pkt, uint16_t size);

}  // namespace sycon