#pragma once

#include <iostream>
#include <limits>
#include <string>

#include "log.h"

#define SWAP_ENDIAN_16(v) __bswap_16((v))
#define SWAP_ENDIAN_32(v) __bswap_32((v))

#define WAIT_FOR_ENTER(msg)                                             \
  {                                                                     \
    LOG(msg);                                                           \
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); \
  }

#define SYNAPSE_CONTROLLER_MAIN(argc, argv)                             \
  parse_args(argc, argv);                                               \
  init_switchd();                                                       \
  configure_ports();                                                    \
  register_pcie_pkt_ops();                                              \
  nf_setup();                                                           \
  if (args.run_ucli) {                                                  \
    run_cli();                                                          \
  } else {                                                              \
    WAIT_FOR_ENTER("Controller is running. Press enter to terminate."); \
  }

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

namespace sycon {

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef size_t bit_len_t;
typedef size_t byte_len_t;

typedef u8 byte_t;

std::string read_env(const char *env_var);

}  // namespace sycon