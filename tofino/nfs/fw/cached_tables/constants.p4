#ifndef __CONSTANTS__
#define __CONSTANTS__

#if __TARGET_TOFINO__ == 2
#define CPU_PCIE_PORT 0

#define ETH_CPU_PORT_0 2
#define ETH_CPU_PORT_1 3
#define ETH_CPU_PORT_2 4
#define ETH_CPU_PORT_3 5

#define RECIRCULATION_PORT 6

// hardware
// #define IN_PORT 136
// #define OUT_PORT 144

// model
#define IN_PORT 8
#define OUT_PORT 9
#else
// hardware
// #define CPU_PCIE_PORT 192
// #define IN_PORT 164
// #define OUT_PORT 172

// model
#define CPU_PCIE_PORT 320
#define IN_PORT 0
#define OUT_PORT 1
#endif

#define HASH_SIZE_BITS 16
#define CACHE_CAPACITY (1 << HASH_SIZE_BITS)
#define ONE_SECOND 15258 // 1 second in units of 2**16 nanoseconds

struct key_t {
    bit<32> src_addr;
    bit<32> dst_addr;
    bit<16> src_port;
    bit<16> dst_port;
}

typedef bit<16> value_t;
typedef bit<HASH_SIZE_BITS> hash_t;
typedef bit<32> time_t;

typedef bit<9> port_t;
typedef bit<7> port_pad_t;

const time_t EXPIRATION_TIME = 5 * ONE_SECOND;

const bit<16> TYPE_IPV4        = 0x800;
const bit<8>  IP_PROTOCOLS_TCP = 6;
const bit<8>  IP_PROTOCOLS_UDP = 17;

const port_t LAN = IN_PORT;
const port_t WAN = OUT_PORT;

#endif /* __CONSTANTS__ */