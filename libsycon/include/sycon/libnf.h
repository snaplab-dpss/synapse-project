#pragma once

// Namespaced view of libnf's portable CPU data structures + math, whose
// implementations are compiled into libsycon (see the sycon_libnf object library in
// CMakeLists.txt). Controllers that need a pure-CPU (controller-side / non-dataplane)
// data structure or a math helper can `#include <sycon/libnf.h>` and call e.g.
// `libnf::vector_borrow(...)`, `libnf::find_first_set_bit(...)` -- reusing libnf's
// implementations instead of reimplementing them.
//
// libnf's headers are C. They are wrapped in `namespace libnf` here to match the
// namespaced symbols in libsycon; the standard headers are pulled in FIRST, outside
// the namespace, so their declarations keep global linkage (the include guards then
// make the re-includes from within the libnf headers no-ops).

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Don't let math.h's __builtin_ia32_crc32* redeclarations be pulled into namespace
// libnf (they'd shadow the real compiler intrinsics); see math.h.
#define LIBNF_SKIP_BUILTIN_DECLS

namespace libnf {
#include <lib/util/math.h>
#include <lib/state/vector.h>
#include <lib/state/map.h>
#include <lib/state/double-chain.h>
#include <lib/state/cht.h>
#include <lib/state/cms.h>
#include <lib/state/bloom-filter.h>
#include <lib/state/token-bucket.h>
#include <lib/state/lpm-dir-24-8.h>
} // namespace libnf

// libnf's headers define a handful of bare macros (a namespace can't contain them);
// undo the ones that would pollute / clash with controller or sycon code. `time_ns_t`
// especially would otherwise clobber sycon's own typedef. (#undef of an undefined
// macro is a harmless no-op.)
#undef time_ns_t
#undef time_us_t
#undef time_ms_t
#undef time_s_t
#undef AND
#undef OR
#undef PRVIGT
#undef PACKED_FOR_KLEE_VERIFICATION
