#pragma once

#include <iostream>
#include <string>

#define SWAP_ENDIAN_16(v) \
  { (v) = __bswap_16((v)); }

#define SWAP_ENDIAN_32(v) \
  { (v) = __bswap_32((v)); }

#define WAIT_FOR_ENTER(msg)                                             \
  {                                                                     \
    std::cout << msg;                                                   \
    fflush(stdout);                                                     \
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); \
  }

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

namespace sycon {

std::string read_env(const char *env_var);

}  // namespace sycon