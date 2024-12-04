#pragma once

#include <stdint.h>
#include <unordered_set>
#include <vector>

#include <klee/Constraints.h>

#define THOUSANDS_SEPARATOR "'"

#define TRILLION 1000000000000LLU
#define BILLION 1000000000LLU

#define THOUSAND 1000LLU
#define MILLION 1000000LLU
#define BILLION 1000000000LLU

#define NEWTON_MAX_ITERATIONS 10
#define NEWTON_PRECISION 1e-3

#define UINT_16_SWAP_ENDIANNESS(p) ((((p) & 0xff) << 8) | ((p) >> 8 & 0xff))

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef u32 bits_t;
typedef u32 bytes_t;

typedef u64 addr_t;
typedef u64 obj_t;
typedef std::unordered_set<obj_t> objs_t;

typedef i64 time_s_t;
typedef i64 time_ms_t;
typedef i64 time_us_t;
typedef i64 time_ns_t;
typedef i64 time_ps_t;

typedef u64 pps_t;
typedef u64 bps_t;
typedef u64 Bps_t;

typedef u64 fpm_t; // churn in flows per minute
typedef u64 fps_t; // churn in flows per second

typedef double hit_rate_t;

#define MIN_THROUGHPUT_GIGABIT_PER_SEC ((gbps_t)1)
#define MAX_THROUGHPUT_GIGABIT_PER_SEC ((gbps_t)100)

#define MIN_PACKET_SIZE_BYTES ((byte_t)64)
#define MAX_PACKET_SIZE_BYTES ((byte_t)1500)

#define bit_to_byte(B) ((byte_t)((B) / 8))
#define byte_to_bit(B) ((bit_t)((B) * 8))
#define align_to_byte(B) ((B) % 8 == 0 ? (B) : (B) + 8 - (B) % 8)

#define gbps_to_bps(R) ((bps_t)((R) * BILLION))

#define min_to_s(T) ((time_s_t)((T) * 60))

#define s_to_min(T) ((time_min_t)((T) / (double)(60)))
#define s_to_us(T) ((time_us_t)((T) * MILLION))
#define s_to_ns(T) ((time_ns_t)((T) * BILLION))

#define us_to_s(T) ((time_s_t)((double)(T) / (double)(MILLION)))
#define us_to_ns(T) ((time_ns_t)((T) * THOUSAND))

#define ns_to_s(T) ((time_s_t)((double)(T) / (double)(BILLION)))
#define ns_to_us(T) ((time_us_t)((T) / THOUSAND))
#define ns_to_ps(T) ((time_ps_t)((T) * THOUSAND))

#define ps_to_ns(T) ((time_ns_t)((double)(T) / (double)THOUSAND))

#define STABLE_TPUT_PRECISION ((pps_t)100) // pps

typedef u64 ep_id_t;
typedef u64 ep_node_id_t;
typedef std::vector<klee::ref<klee::Expr>> constraints_t;

struct ether_addr_t {
  u8 addr_bytes[6];
} __attribute__((__packed__));

struct ether_hdr_t {
  ether_addr_t daddr;
  ether_addr_t saddr;
  u16 ether_type;
} __attribute__((__packed__));

struct ipv4_hdr_t {
  u8 ihl : 4;
  u8 version : 4;
  u8 type_of_service;
  u16 total_length;
  u16 packet_id;
  u16 fragment_offset;
  u8 time_to_live;
  u8 next_proto_id;
  u16 hdr_checksum;
  u32 src_addr;
  u32 dst_addr;
} __attribute__((__packed__));

struct udp_hdr_t {
  u16 src_port;
  u16 dst_port;
  u16 len;
  u16 checksum;
} __attribute__((__packed__));

struct tcp_hdr_t {
  u16 src_port;
  u16 dst_port;
  u32 sent_seq;
  u32 recv_ack;
  u8 data_off;
  u8 tcp_flags;
  u16 rx_win;
  u16 cksum;
  u16 tcp_urp;
} __attribute__((__packed__));

struct vlan_hdr_t {
  u16 vlan_tpid;
  u16 vlan_tci;
};

#define ETHERTYPE_IP 0x0800       /* IP */
#define ETHERTYPE_ARP 0x0806      /* Address resolution */
#define ETHERTYPE_REVARP 0x8035   /* Reverse ARP */
#define ETHERTYPE_AT 0x809B       /* AppleTalk protocol */
#define ETHERTYPE_AARP 0x80F3     /* AppleTalk ARP */
#define ETHERTYPE_VLAN 0x8100     /* IEEE 802.1Q VLAN tagging */
#define ETHERTYPE_IPX 0x8137      /* IPX */
#define ETHERTYPE_IPV6 0x86dd     /* IP protocol version 6 */
#define ETHERTYPE_LOOPBACK 0x9000 /* used to test interfaces */

#define CRC_SIZE_BYTES 4
#define PREAMBLE_SIZE_BYTES 8
#define IPG_SIZE_BYTES 12