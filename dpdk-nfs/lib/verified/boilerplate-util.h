#ifndef _BOILERPLATE_UTIL_H_INCLUDED_
#define _BOILERPLATE_UTIL_H_INCLUDED_

#include <stdint.h>
#include <limits.h>

// verifast doesn't know about these
unsigned __builtin_ia32_crc32si(unsigned acc, unsigned int x);
unsigned long long __builtin_ia32_crc32di(unsigned long long acc,
                                          unsigned long long x);

// KLEE doesn't tolerate && in a klee_assume (see klee/klee#809),
// so we replace them with & during symbex but interpret them as && in the
// validator
#ifdef KLEE_VERIFICATION
#define AND &
#else  // KLEE_VERIFICATION
#define AND &&
#endif  // KLEE_VERIFICATION

#ifdef KLEE_VERIFICATION
#define PACKED_FOR_KLEE_VERIFICATION __attribute__((packed))
#else
#define PACKED_FOR_KLEE_VERIFICATION
#endif

#define DEFAULT_UINT32_T 0

static void null_init(void *obj) { *(uint32_t *)obj = 0; }

#ifdef KLEE_VERIFICATION
#include <klee/klee.h>
static inline void concretize_devices(uint16_t *device, uint16_t count) {
  klee_assert(*device >= 0);
  klee_assert(*device < count);

  for (unsigned d = 0; d < count; d++)
    if (*device == d) {
      *device = d;
      break;
    }
}
#else
static inline void concretize_devices(uint16_t *device, uint16_t count) {
  (void)(device);
  (void)(count);
}
#endif  // KLEE_VERIFICATION

#ifdef KLEE_VERIFICATION
// Put an expression into the asumption heap, provided it is true
static inline void vigor_note(int cond) {
  klee_assert(cond);
  klee_assume(cond);
}
#else   // KLEE_VERIFICATION
static inline void vigor_note(int cond) { (uintptr_t) cond; }
#endif  // KLEE_VERIFICATION

#define NF_STATE(name, type, capacity, key_type, val_type, lower_bound, \
                 upper_bound)                                           \
  "error: NF_STATE is not implemented yet, use dataspec.ml/py instead"

#define NF_EXPORT_STATE(cname, pyname) \
  "error: NF_EXPORT_STATE is not implemented yet, \
   assume state names in the spec are identical to the names used in the C implementation"

#endif  //_BOILERPLATE_UTIL_H_INCLUDED_
