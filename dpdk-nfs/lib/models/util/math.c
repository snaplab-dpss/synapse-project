#include "lib/util/math.h"

#include <klee/klee.h>
#include <stdint.h>

unsigned hash_obj(void *obj, unsigned size_bytes) {
  klee_trace_ret();
  klee_trace_param_u64((uint64_t)obj, "obj");
  klee_trace_param_tagged_ptr(obj, size_bytes, "obj", "obj", TD_IN);
  klee_trace_param_u32(size_bytes, "size");
  return klee_int("hash");
}

unsigned count_trailing_zeros(unsigned x) {
  klee_trace_ret();
  klee_trace_param_u32(x, "x");
  return klee_int("trailing_zeros");
}

unsigned power_of_two(unsigned exponent) {
  klee_trace_ret();
  klee_trace_param_u32(exponent, "exponent");
  return klee_int("power_of_two");
}

unsigned divide(unsigned numerator, unsigned denominator) {
  klee_trace_ret();
  klee_trace_param_u32(numerator, "numerator");
  klee_trace_param_u32(denominator, "denominator");
  return klee_int("quotient");
}

unsigned ln(unsigned x, unsigned scale) {
  klee_trace_ret();
  klee_trace_param_u32(x, "x");
  klee_trace_param_u32(scale, "scale");
  return klee_int("ln");
}
