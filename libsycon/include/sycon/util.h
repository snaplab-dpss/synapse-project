#pragma once

#include <array>
#include <iostream>
#include <limits>
#include <string>
#include <iostream>

#include "log.h"

#define SWAP_ENDIAN_16(v) __builtin_bswap16((v))
#define SWAP_ENDIAN_32(v) __builtin_bswap32((v))
#define SWAP_ENDIAN_64(v) __builtin_bswap64((v))

#define WAIT_FOR_ENTER(msg)                                                                                                                \
  do {                                                                                                                                     \
    LOG(msg);                                                                                                                              \
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');                                                                    \
  } while (0)

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#define TOFINO_ARCH(version) (std::string((version) == 1 ? "tf1" : "tf2"))

namespace sycon {

using u64 = __UINT64_TYPE__;
using u32 = __UINT32_TYPE__;
using u16 = __UINT16_TYPE__;
using u8  = __UINT8_TYPE__;

using i64 = __INT64_TYPE__;
using i32 = __INT32_TYPE__;
using i16 = __INT16_TYPE__;
using i8  = __INT8_TYPE__;

using bits_t  = u64;
using bytes_t = u64;

using field_t = u64;

std::string read_env(const char *env_var);

} // namespace sycon