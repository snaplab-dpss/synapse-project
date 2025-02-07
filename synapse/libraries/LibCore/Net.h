#pragma once

#include <LibCore/Types.h>

constexpr const u16 ETHERTYPE_IP       = 0x0800; /* IP */
constexpr const u16 ETHERTYPE_ARP      = 0x0806; /* Address resolution */
constexpr const u16 ETHERTYPE_REVARP   = 0x8035; /* Reverse ARP */
constexpr const u16 ETHERTYPE_AT       = 0x809B; /* AppleTalk protocol */
constexpr const u16 ETHERTYPE_AARP     = 0x80F3; /* AppleTalk ARP */
constexpr const u16 ETHERTYPE_VLAN     = 0x8100; /* IEEE 802.1Q VLAN tagging */
constexpr const u16 ETHERTYPE_IPX      = 0x8137; /* IPX */
constexpr const u16 ETHERTYPE_IPV6     = 0x86dd; /* IP protocol version 6 */
constexpr const u16 ETHERTYPE_LOOPBACK = 0x9000; /* used to test interfaces */

constexpr const bytes_t CRC_SIZE_BYTES      = 4;
constexpr const bytes_t PREAMBLE_SIZE_BYTES = 8;
constexpr const bytes_t IPG_SIZE_BYTES      = 12;

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

namespace LibCore {

inline std::string ipv4_to_str(u32 addr) {
  std::stringstream ss;
  ss << ((addr >> 0) & 0xff);
  ss << ".";
  ss << ((addr >> 8) & 0xff);
  ss << ".";
  ss << ((addr >> 16) & 0xff);
  ss << ".";
  ss << ((addr >> 24) & 0xff);
  return ss.str();
}

inline u32 ipv4_set_prefix(u32 addr, u8 prefix, bits_t prefix_size) {
  assert(prefix_size <= 32);
  const u32 swapped_addr  = bswap32(addr);
  const u32 mask          = (1ull << prefix_size) - 1;
  const u32 prefix_masked = prefix & mask;
  return bswap32((prefix_masked << (32 - prefix_size)) | (swapped_addr >> prefix_size));
}

} // namespace LibCore
