#pragma once

#include <thread>
#include <stdint.h>

inline uint64_t now() {
#if defined(__aarch64__)
  uint64_t ticks;
  asm volatile("mrs %0, CNTVCT_EL0" : "=r"(ticks));
  return ticks;
#else
  // Default to x86, other archs will compile error anyway
  uint32_t lo, hi;
  asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
#endif
}

inline uint64_t clock_scale() {
  static thread_local uint64_t tpus = 0; // ticks / usec

  if (!tpus) {
    const uint64_t start          = now();
    const struct timespec one_sec = {1, 0};
    nanosleep(&one_sec, nullptr);
    const uint64_t end = now();
    tpus               = (uint64_t)((end - start) / 1e6);
  }

  return tpus;
}

extern int64_t delta_ns;
extern uint64_t delta_ticks;

inline void set_delta_ticks(int64_t duration_ns) {
  delta_ns    = duration_ns;
  delta_ticks = duration_ns * clock_scale() / 1000;
}
