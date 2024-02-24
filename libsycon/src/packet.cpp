#include "../include/sycon/packet.h"

#include <iomanip>
#include <sstream>

#include "../include/sycon/log.h"
#include "packet.h"

namespace sycon {

static uint32_t packet_consumed;
static uint32_t packet_size;

void packet_init(uint16_t size) {
  assert(size > 0);
  packet_consumed = 0;
  packet_size = size;
}

byte_t *packet_consume(byte_t *packet_base, uint16_t bytes) {
  assert(packet_consumed + bytes <= packet_size);
  byte_t *header = packet_base + packet_consumed;
  packet_consumed += bytes;
  return header;
}

void packet_log(const cpu_hdr_t *cpu_hdr) {
  assert(cpu_hdr);

  LOG("###[ CPU ]###");
  LOG("code path  %u", SWAP_ENDIAN_16(cpu_hdr->code_path));
  LOG("in port    %u", SWAP_ENDIAN_16(cpu_hdr->in_port));
  LOG("out port   %u", SWAP_ENDIAN_16(cpu_hdr->out_port));
}

void packet_log(const eth_hdr_t *eth_hdr) {
  assert(eth_hdr);

  LOG("###[ Ethernet ]###");
  LOG("dst  %02x:%02x:%02x:%02x:%02x:%02x", eth_hdr->dst_mac[0],
      eth_hdr->dst_mac[1], eth_hdr->dst_mac[2], eth_hdr->dst_mac[3],
      eth_hdr->dst_mac[4], eth_hdr->dst_mac[5]);
  LOG("src  %02x:%02x:%02x:%02x:%02x:%02x", eth_hdr->src_mac[0],
      eth_hdr->src_mac[1], eth_hdr->src_mac[2], eth_hdr->src_mac[3],
      eth_hdr->src_mac[4], eth_hdr->src_mac[5]);
  LOG("type 0x%x", eth_hdr->eth_type);
}

void packet_log(const ipv4_hdr_t *ipv4_hdr) {
  LOG("###[ IP ]###");
  LOG("version %u", (ipv4_hdr->version_ihl & 0xf0) >> 4);
  LOG("ihl     %u", (ipv4_hdr->version_ihl & 0x0f));
  LOG("tos     %u", ipv4_hdr->ecn_dscp);
  LOG("len     %u", SWAP_ENDIAN_16(ipv4_hdr->tot_len));
  LOG("id      %u", SWAP_ENDIAN_16(ipv4_hdr->id));
  LOG("off     %u", SWAP_ENDIAN_16(ipv4_hdr->frag_off));
  LOG("ttl     %u", ipv4_hdr->ttl);
  LOG("proto   %u", ipv4_hdr->protocol);
  LOG("chksum  0x%x", SWAP_ENDIAN_16(ipv4_hdr->check));
  LOG("src     %u.%u.%u.%u", (ipv4_hdr->src_ip >> 0) & 0xff,
      (ipv4_hdr->src_ip >> 8) & 0xff, (ipv4_hdr->src_ip >> 16) & 0xff,
      (ipv4_hdr->src_ip >> 24) & 0xff);
  LOG("dst     %u.%u.%u.%u", (ipv4_hdr->dst_ip >> 0) & 0xff,
      (ipv4_hdr->dst_ip >> 8) & 0xff, (ipv4_hdr->dst_ip >> 16) & 0xff,
      (ipv4_hdr->dst_ip >> 24) & 0xff);
}

void packet_log(const tcpudp_hdr_t *tcpudp_hdr) {
  LOG("###[ TCP/UDP ]###");
  LOG("sport   %u", SWAP_ENDIAN_16(tcpudp_hdr->src_port));
  LOG("dport   %u", SWAP_ENDIAN_16(tcpudp_hdr->dst_port));
}

unsigned ether_addr_hash(mac_addr_t addr) {
  uint8_t addr_bytes_0 = addr[0];
  uint8_t addr_bytes_1 = addr[1];
  uint8_t addr_bytes_2 = addr[2];
  uint8_t addr_bytes_3 = addr[3];
  uint8_t addr_bytes_4 = addr[4];
  uint8_t addr_bytes_5 = addr[5];

  unsigned hash = 0;
  hash = __builtin_ia32_crc32si(hash, addr_bytes_0);
  hash = __builtin_ia32_crc32si(hash, addr_bytes_1);
  hash = __builtin_ia32_crc32si(hash, addr_bytes_2);
  hash = __builtin_ia32_crc32si(hash, addr_bytes_3);
  hash = __builtin_ia32_crc32si(hash, addr_bytes_4);
  hash = __builtin_ia32_crc32si(hash, addr_bytes_5);
  return hash;
}

uint32_t __raw_cksum(const void *buf, size_t len, uint32_t sum) {
  /* workaround gcc strict-aliasing warning */
  uintptr_t ptr = (uintptr_t)buf;
  typedef uint16_t __attribute__((__may_alias__)) u16_p;
  const u16_p *u16_buf = (const u16_p *)ptr;

  while (len >= (sizeof(*u16_buf) * 4)) {
    sum += u16_buf[0];
    sum += u16_buf[1];
    sum += u16_buf[2];
    sum += u16_buf[3];
    len -= sizeof(*u16_buf) * 4;
    u16_buf += 4;
  }
  while (len >= sizeof(*u16_buf)) {
    sum += *u16_buf;
    len -= sizeof(*u16_buf);
    u16_buf += 1;
  }

  /* if length is in odd bytes */
  if (len == 1) {
    uint16_t left = 0;
    *(uint8_t *)&left = *(const uint8_t *)u16_buf;
    sum += left;
  }

  return sum;
}

uint16_t __raw_cksum_reduce(uint32_t sum) {
  sum = ((sum & 0xffff0000) >> 16) + (sum & 0xffff);
  sum = ((sum & 0xffff0000) >> 16) + (sum & 0xffff);
  return (uint16_t)sum;
}

uint16_t raw_cksum(const void *buf, size_t len) {
  uint32_t sum;

  sum = __raw_cksum(buf, len, 0);
  return __raw_cksum_reduce(sum);
}

uint16_t ipv4_cksum(const ipv4_hdr_t *ipv4_hdr) {
  uint16_t cksum;
  cksum = raw_cksum(ipv4_hdr, sizeof(ipv4_hdr_t));
  return (uint16_t)~cksum;
}

uint16_t update_ipv4_tcpudp_checksums(const ipv4_hdr_t *ipv4_hdr,
                                      const void *l4_hdr) {
  uint32_t cksum;
  uint32_t l3_len, l4_len;

  l3_len = __bswap_16(ipv4_hdr->tot_len);
  if (l3_len < sizeof(ipv4_hdr_t)) return 0;

  l4_len = l3_len - sizeof(ipv4_hdr_t);

  cksum = raw_cksum(l4_hdr, l4_len);
  cksum += ipv4_cksum(ipv4_hdr);

  cksum = ((cksum & 0xffff0000) >> 16) + (cksum & 0xffff);
  cksum = (~cksum) & 0xffff;
  /*
   * Per RFC 768:If the computed checksum is zero for UDP,
   * it is transmitted as all ones
   * (the equivalent in one's complement arithmetic).
   */
  if (cksum == 0 && ipv4_hdr->protocol == IPPROTO_UDP) cksum = 0xffff;

  return (uint16_t)cksum;
}

void packet_hexdump(byte_t *pkt, uint16_t size) {
  std::stringstream ss;

  ss << "###[ Packet ]###\n";

  for (uint16_t i = 0; i < size; i++) {
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

}  // namespace sycon