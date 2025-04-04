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

// DPDK's implementation of an atomic 64b inc operation.
static inline void atomic64_inc(volatile uint64_t *v) {
  asm volatile("lock ; incq %[cnt]"
               : [cnt] "=m"(v) /* output */
               : "m"(v)        /* input */
  );
}

// DPDK's implementation of an atomic 64b dec operation.
static inline void atomic64_dec(volatile uint64_t *v) {
  asm volatile("lock ; decq %[cnt]"
               : [cnt] "=m"(v) /* output */
               : "m"(v)        /* input */
  );
}

// DPDK's implementation of an atomic 64b read operation.
static inline uint64_t atomic64_read(volatile uint64_t *v) { return *v; }

extern struct config_t {
  bf_switchd_context_t *switchd_ctx;
  bf_rt_target_t dev_tgt;
  const bfrt::BfRtInfo *info;
  std::shared_ptr<bfrt::BfRtSession> session;
  std::shared_ptr<bfrt::BfRtSession> usr_signal_session;

  volatile uint16_t atom;
  volatile uint64_t pending_dataplane_notifications;

  std::vector<u16> dev_ports;

  config_t() : atom(0), pending_dataplane_notifications(0) {}

  void wait_for_dataplane_notifications() {
    while (atomic64_read(&pending_dataplane_notifications) != 0) {
      // prevent the compiler from removing this loop
      __asm__ __volatile__("");
    }
  }

  void lock() {
    while (!atomic16_cmpset(&atom, 0, 1)) {
      // prevent the compiler from removing this loop
      __asm__ __volatile__("");
    }
  }

  void unlock() { atom = 0; }

  void begin_dataplane_notification_transaction() {
    atomic64_inc(&pending_dataplane_notifications);
    lock();
    const bool atomic{true};
    bf_status_t bf_status = session->beginTransaction(atomic);
    ASSERT_BF_STATUS(bf_status);
  }

  void abort_dataplane_notification_transaction() {
    LOG_DEBUG("***** Aborting transaction *****");
    bf_status_t bf_status = session->abortTransaction();
    ASSERT_BF_STATUS(bf_status);
    atomic64_dec(&pending_dataplane_notifications);
    unlock();
  }

  void commit_dataplane_notification_transaction() {
    const bool block_until_complete{true};
    bf_status_t bf_status = session->commitTransaction(block_until_complete);
    ASSERT_BF_STATUS(bf_status);
    atomic64_dec(&pending_dataplane_notifications);
    unlock();
  }

  void begin_transaction() {
    wait_for_dataplane_notifications();
    lock();
    const bool atomic{true};
    bf_status_t bf_status = session->beginTransaction(atomic);
    ASSERT_BF_STATUS(bf_status);
  }

  void abort_transaction() {
    LOG_DEBUG("***** Aborting transaction *****");
    bf_status_t bf_status = session->abortTransaction();
    ASSERT_BF_STATUS(bf_status);
    unlock();
  }

  void commit_transaction() {
    const bool block_until_complete{true};
    bf_status_t bf_status = session->commitTransaction(block_until_complete);
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