#pragma once

#include <stdint.h>
#include <unordered_set>
#include <vector>

#include <klee/Constraints.h>

namespace synapse {
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
typedef std::unordered_set<addr_t> objs_t;

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
} __attribute__((__packed__));
} // namespace synapse
