#include "../include/sycon/packet.h"
#include "../include/sycon/log.h"
#include "packet.h"

#include <iomanip>
#include <sstream>

#include <netinet/in.h>

namespace sycon {

namespace {

/*
    Partially taken from DPDK 24.07: https://github.com/DPDK/dpdk/releases/tag/v24.07
*/

constexpr const u8 IPV4_HDR_IHL_MASK{0x0f};
constexpr const u8 IPV4_IHL_MULTIPLIER{4};
constexpr const u64 MBUF_F_TX_TCP_SEG{1ULL << 50};
constexpr const u64 MBUF_F_TX_UDP_SEG{1ULL << 42};

#define PTR_ADD(ptr, x) ((void *)((uintptr_t)(ptr) + (x)))
#define ALIGN_FLOOR(val, align) (typeof(val))((val) & (~((typeof(val))((align)-1))))

inline u8 ipv4_hdr_len(const ipv4_hdr_t *ipv4_hdr) { return (u8)((ipv4_hdr->version_ihl & IPV4_HDR_IHL_MASK) * IPV4_IHL_MULTIPLIER); }

inline u32 __raw_cksum(const void *buf, size_t len, u32 sum) {
  const void *end;

  for (end = PTR_ADD(buf, ALIGN_FLOOR(len, sizeof(u16))); buf != end; buf = PTR_ADD(buf, sizeof(u16))) {
    u16 v;

    memcpy(&v, buf, sizeof(u16));
    sum += v;
  }

  /* if length is odd, keeping it byte order independent */
  if (unlikely(len % 2)) {
    u16 left = 0;

    memcpy(&left, end, 1);
    sum += left;
  }

  return sum;
}

inline u16 __raw_cksum_reduce(u32 sum) {
  sum = ((sum & 0xffff0000) >> 16) + (sum & 0xffff);
  sum = ((sum & 0xffff0000) >> 16) + (sum & 0xffff);
  return (u16)sum;
}

inline u16 raw_cksum(const void *buf, size_t len) {
  u32 sum = __raw_cksum(buf, len, 0);
  return __raw_cksum_reduce(sum);
}

inline u16 ipv4_cksum(const ipv4_hdr_t *ipv4_hdr) {
  u16 cksum;
  cksum = raw_cksum(ipv4_hdr, ipv4_hdr_len(ipv4_hdr));
  return (u16)~cksum;
}

inline u16 ipv4_phdr_cksum(const ipv4_hdr_t *ipv4_hdr, u64 ol_flags) {
  struct ipv4_psd_header {
    u32 src_addr; /* IP address of source host. */
    u32 dst_addr; /* IP address of destination host. */
    u8 zero;      /* zero. */
    u8 proto;     /* L4 protocol type. */
    u16 len;      /* L4 length. */
  } psd_hdr;

  u32 l3_len;

  psd_hdr.src_addr = ipv4_hdr->src_ip;
  psd_hdr.dst_addr = ipv4_hdr->dst_ip;
  psd_hdr.zero     = 0;
  psd_hdr.proto    = ipv4_hdr->protocol;
  if (ol_flags & (MBUF_F_TX_TCP_SEG | MBUF_F_TX_UDP_SEG)) {
    psd_hdr.len = 0;
  } else {
    l3_len      = bswap16(ipv4_hdr->tot_len);
    psd_hdr.len = bswap16((u16)(l3_len - ipv4_hdr_len(ipv4_hdr)));
  }
  return raw_cksum(&psd_hdr, sizeof(psd_hdr));
}

inline u16 __ipv4_udptcp_cksum(const ipv4_hdr_t *ipv4_hdr, const void *l4_hdr) {
  u32 cksum;
  u32 l3_len, l4_len;
  u8 ip_hdr_len;

  ip_hdr_len = ipv4_hdr_len(ipv4_hdr);
  l3_len     = bswap16(ipv4_hdr->tot_len);
  if (l3_len < ip_hdr_len)
    return 0;

  l4_len = l3_len - ip_hdr_len;

  cksum = raw_cksum(l4_hdr, l4_len);
  cksum += ipv4_phdr_cksum(ipv4_hdr, 0);

  cksum = ((cksum & 0xffff0000) >> 16) + (cksum & 0xffff);

  return (u16)cksum;
}

inline u16 ipv4_udptcp_cksum(const ipv4_hdr_t *ipv4_hdr, const void *l4_hdr) {
  u16 cksum = __ipv4_udptcp_cksum(ipv4_hdr, l4_hdr);

  cksum = ~cksum;

  /*
   * Per RFC 768: If the computed checksum is zero for UDP,
   * it is transmitted as all ones
   * (the equivalent in one's complement arithmetic).
   */
  if (cksum == 0 && ipv4_hdr->protocol == IPPROTO_UDP)
    cksum = 0xffff;

  return cksum;
}

} // namespace

bytes_t packet_consumed;
bytes_t packet_size;

void packet_init(bytes_t size) {
  assert(size > 0);
  packet_consumed = 0;
  packet_size     = size;
}

u8 *packet_consume(u8 *packet_base, bytes_t size) {
  assert(packet_consumed + size <= packet_size);
  u8 *header = packet_base + packet_consumed;
  packet_consumed += size;
  return header;
}

void packet_log(const cpu_hdr_t *cpu_hdr) {
  assert(cpu_hdr);

  LOG("###[ CPU ]###");
  LOG("code path   %u", bswap16(cpu_hdr->code_path));
  LOG("egress dev  %u", bswap16(cpu_hdr->egress_dev));
}

void packet_log(const eth_hdr_t *eth_hdr) {
  assert(eth_hdr);

  LOG("###[ Ethernet ]###");
  LOG("dst  %02x:%02x:%02x:%02x:%02x:%02x", eth_hdr->dst_mac[0], eth_hdr->dst_mac[1], eth_hdr->dst_mac[2], eth_hdr->dst_mac[3],
      eth_hdr->dst_mac[4], eth_hdr->dst_mac[5]);
  LOG("src  %02x:%02x:%02x:%02x:%02x:%02x", eth_hdr->src_mac[0], eth_hdr->src_mac[1], eth_hdr->src_mac[2], eth_hdr->src_mac[3],
      eth_hdr->src_mac[4], eth_hdr->src_mac[5]);
  LOG("type 0x%x", eth_hdr->eth_type);
}

void packet_log(const ipv4_hdr_t *ipv4_hdr) {
  LOG("###[ IP ]###");
  LOG("version %u", (ipv4_hdr->version_ihl & 0xf0) >> 4);
  LOG("ihl     %u", (ipv4_hdr->version_ihl & 0x0f));
  LOG("tos     %u", ipv4_hdr->ecn_dscp);
  LOG("len     %u", bswap16(ipv4_hdr->tot_len));
  LOG("id      %u", bswap16(ipv4_hdr->id));
  LOG("off     %u", bswap16(ipv4_hdr->frag_off));
  LOG("ttl     %u", ipv4_hdr->ttl);
  LOG("proto   %u", ipv4_hdr->protocol);
  LOG("chksum  0x%x", bswap16(ipv4_hdr->check));
  LOG("src     %u.%u.%u.%u", (ipv4_hdr->src_ip >> 0) & 0xff, (ipv4_hdr->src_ip >> 8) & 0xff, (ipv4_hdr->src_ip >> 16) & 0xff,
      (ipv4_hdr->src_ip >> 24) & 0xff);
  LOG("dst     %u.%u.%u.%u", (ipv4_hdr->dst_ip >> 0) & 0xff, (ipv4_hdr->dst_ip >> 8) & 0xff, (ipv4_hdr->dst_ip >> 16) & 0xff,
      (ipv4_hdr->dst_ip >> 24) & 0xff);
}

void packet_log(const tcpudp_hdr_t *tcpudp_hdr) {
  LOG("###[ TCP/UDP ]###");
  LOG("sport   %u", bswap16(tcpudp_hdr->src_port));
  LOG("dport   %u", bswap16(tcpudp_hdr->dst_port));
}

unsigned ether_addr_hash(mac_addr_t addr) {
  u8 addr_bytes_0 = addr[0];
  u8 addr_bytes_1 = addr[1];
  u8 addr_bytes_2 = addr[2];
  u8 addr_bytes_3 = addr[3];
  u8 addr_bytes_4 = addr[4];
  u8 addr_bytes_5 = addr[5];

  unsigned hash = 0;
  hash          = __builtin_ia32_crc32si(hash, addr_bytes_0);
  hash          = __builtin_ia32_crc32si(hash, addr_bytes_1);
  hash          = __builtin_ia32_crc32si(hash, addr_bytes_2);
  hash          = __builtin_ia32_crc32si(hash, addr_bytes_3);
  hash          = __builtin_ia32_crc32si(hash, addr_bytes_4);
  hash          = __builtin_ia32_crc32si(hash, addr_bytes_5);
  return hash;
}

void update_ipv4_tcpudp_checksums(void *l3, void *l4) {
  ipv4_hdr_t *ipv4_hdr = (ipv4_hdr_t *)l3;
  ipv4_hdr->check      = 0; // Assumed by cksum calculation
  if (ipv4_hdr->protocol == IPPROTO_TCP) {
    tcp_hdr_t *tcp_header = (tcp_hdr_t *)l4;
    tcp_header->cksum     = 0; // Assumed by cksum calculation
    tcp_header->cksum     = ipv4_udptcp_cksum((ipv4_hdr_t *)ipv4_hdr, tcp_header);
  } else if (ipv4_hdr->protocol == IPPROTO_UDP) {
    udp_hdr_t *udp_header   = (udp_hdr_t *)l4;
    udp_header->dgram_cksum = 0; // Assumed by cksum calculation
    udp_header->dgram_cksum = ipv4_udptcp_cksum((ipv4_hdr_t *)ipv4_hdr, udp_header);
  }
  ipv4_hdr->check = ipv4_cksum((ipv4_hdr_t *)ipv4_hdr);
}

void packet_hexdump(u8 *pkt, u16 size) {
  std::stringstream ss;

  ss << "###[ Packet ]###\n";

  for (u16 i = 0; i < size; i++) {
    if (i % 16 == 0) {
      if (i > 0) {
        ss << "\n";
      }
      ss << std::setfill('0') << std::setw(4) << i << ": ";
    } else if (i % 2 == 0) {
      ss << " ";
    }

    ss << std::hex << std::setfill('0') << std::setw(2) << (int)(pkt[i]);
  }

  LOG("%s", ss.str().c_str());
}

} // namespace sycon