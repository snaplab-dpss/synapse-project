#pragma once

#include <bf_rt/bf_rt.hpp>

extern "C" {
#include <bf_switchd/bf_switchd.h>
}

#include "log.h"
#include "time.h"

namespace sycon {

// DPDK's implementation of an atomic 16b compare and set operation.
static inline int atomic16_cmpset(volatile uint16_t *dst, uint16_t exp, uint16_t src) {
  uint8_t res;
  asm volatile("lock ; "
               "cmpxchgw %[src], %[dst];"
               "sete %[res];"
               : [res] "=a"(res), /* output */
                 [dst] "=m"(*dst)
               : [src] "r"(src), /* input */
                 "a"(exp), "m"(*dst)
               : "memory"); /* no-clobber list */
  return res;
}

extern struct config_t {
  bf_switchd_context_t *switchd_ctx;
  bf_rt_target_t dev_tgt;
  const bfrt::BfRtInfo *info;
  std::shared_ptr<bfrt::BfRtSession> session;
  std::shared_ptr<bfrt::BfRtSession> usr_signal_session;

  volatile int16_t atom;

  std::vector<u16> dev_ports;

  config_t() : atom(0) {}

  void lock() {
    while (!atomic16_cmpset((volatile uint16_t *)&atom, 0, 1)) {
      // prevent the compiler from removing this loop
      __asm__ __volatile__("");
    }
  }

  void unlock() { atom = 0; }

  void begin_transaction() {
    lock();
    const bool atomic     = true;
    bf_status_t bf_status = session->beginTransaction(atomic);
    ASSERT_BF_STATUS(bf_status);
  }

  void end_transaction() {
    const bool block_until_complete = true;
    bf_status_t bf_status           = session->commitTransaction(block_until_complete);
    ASSERT_BF_STATUS(bf_status);
    unlock();
  }

  ~config_t() {
    if (switchd_ctx) {
      free(switchd_ctx);
      switchd_ctx = nullptr;
    }
  }
} cfg;

} // namespace sycon