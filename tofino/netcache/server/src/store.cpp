#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <rte_ethdev.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_tcp.h>

#include "packet.h"
#include "store.h"
#include "constants.h"

namespace netcache {

Store::Store(const int in, const int out, rte_mempool *pool) {
  port_in   = in;
  port_out  = out;
  mbuf_pool = pool;
}

Store::~Store() {}

void Store::read_query() {
  uint16_t nb_rx = rte_eth_rx_burst(port_in, 0, buf, BURST_SIZE);

  if (nb_rx == 0) {
    return;
  }
  count++;

  if (!check_pkt(buf[0])) {
    return;
  }

  modify_pkt(buf[0]);

  uint16_t nb_tx = rte_eth_tx_burst(port_out, 0, buf, 1);

  if (nb_tx > 0) {
#ifndef NDEBUG
    std::cout << "Sent " << nb_tx << "packet(s)." << std::endl;
#endif
  } else {
    rte_pktmbuf_free(buf[0]);
  }
}

bool Store::check_pkt(struct rte_mbuf *mbuf) {
  struct rte_ether_hdr *eth_hdr;
  struct rte_ipv4_hdr *ipv4_hdr;
  struct rte_udp_hdr *udp_hdr;
  struct rte_tcp_hdr *tcp_hdr;

  if (mbuf->pkt_len < sizeof(struct rte_ether_hdr)) {
#ifndef NDEBUG
    std::cout << "Received pkt: not enough data for ethernet header." << std::endl;
#endif
    return false;
  }

  eth_hdr = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);

  if (eth_hdr->ether_type != rte_be_to_cpu_16(ETHER_TYPE_IPV4)) {
    if (mbuf->pkt_len < sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr)) {
#ifndef NDEBUG
      std::cout << "Received pkt: not enough data for IPv4 header." << std::endl;
#endif
      return false;
    }
  }

  ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));

  if (ipv4_hdr->next_proto_id != UDP_PROTO || ipv4_hdr->next_proto_id != TCP_PROTO) {
    return false;
  }

  if (ipv4_hdr->next_proto_id == UDP_PROTO) {
    if (mbuf->pkt_len < sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr)) {
#ifndef NDEBUG
      std::cout << "Received pkt: not enough data for udp header." << std::endl;
#endif
      return false;
    }
    udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
    if (udp_hdr->src_port == rte_be_to_cpu_16(NC_PORT) || udp_hdr->dst_port == rte_be_to_cpu_16(NC_PORT)) {
      if (buf[0]->pkt_len >= sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr) + NC_HDR_SIZE) {
        return true;
      } else {
#ifndef NDEBUG
        std::cout << "Received pkt: not enough data for netcache header." << std::endl;
#endif
      }
    }
  }

  if (ipv4_hdr->next_proto_id == TCP_PROTO) {
    if (mbuf->pkt_len < sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_tcp_hdr)) {
#ifndef NDEBUG
      std::cout << "Received pkt: not enough data for tcp header." << std::endl;
#endif
      return false;
    }
    tcp_hdr = (struct rte_tcp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
    if (tcp_hdr->src_port == rte_be_to_cpu_16(NC_PORT) || tcp_hdr->dst_port == rte_be_to_cpu_16(NC_PORT)) {
      if (buf[0]->pkt_len >= sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_tcp_hdr) + NC_HDR_SIZE) {
        return true;
      } else {
#ifndef NDEBUG
        std::cout << "Received pkt: not enough data for netcache header." << std::endl;
#endif
      }
    }
  }

  return false;
}

void Store::modify_pkt(struct rte_mbuf *mbuf) {
  // ------------------------
  // -------- Eth -----------
  // ------------------------

  struct rte_ether_hdr *eth_hdr;
  struct rte_ether_addr eth_tmp;
  eth_hdr = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);

#ifndef NDEBUG
  printf("Original MAC src: %02x:%02x:%02x:%02x:%02x:%02x\n", eth_hdr->src_addr.addr_bytes[0], eth_hdr->src_addr.addr_bytes[1],
         eth_hdr->src_addr.addr_bytes[2], eth_hdr->src_addr.addr_bytes[3], eth_hdr->src_addr.addr_bytes[4],
         eth_hdr->src_addr.addr_bytes[5]);
  printf("Original MAC dst: %02x:%02x:%02x:%02x:%02x:%02x\n", eth_hdr->dst_addr.addr_bytes[0], eth_hdr->dst_addr.addr_bytes[1],
         eth_hdr->dst_addr.addr_bytes[2], eth_hdr->dst_addr.addr_bytes[3], eth_hdr->dst_addr.addr_bytes[4],
         eth_hdr->dst_addr.addr_bytes[5]);
#endif

  // Swap src and dst MAC
  eth_tmp           = eth_hdr->src_addr;
  eth_hdr->src_addr = eth_hdr->dst_addr;
  eth_hdr->dst_addr = eth_tmp;

#ifndef NDEBUG
  printf("Swapped MAC src: %02x:%02x:%02x:%02x:%02x:%02x\n", eth_hdr->src_addr.addr_bytes[0], eth_hdr->src_addr.addr_bytes[1],
         eth_hdr->src_addr.addr_bytes[2], eth_hdr->src_addr.addr_bytes[3], eth_hdr->src_addr.addr_bytes[4],
         eth_hdr->src_addr.addr_bytes[5]);
  printf("Swapped MAC dst: %02x:%02x:%02x:%02x:%02x:%02x\n", eth_hdr->dst_addr.addr_bytes[0], eth_hdr->dst_addr.addr_bytes[1],
         eth_hdr->dst_addr.addr_bytes[2], eth_hdr->dst_addr.addr_bytes[3], eth_hdr->dst_addr.addr_bytes[4],
         eth_hdr->dst_addr.addr_bytes[5]);
#endif

  // ------------------------
  // ---------- IP ----------
  // ------------------------

  struct rte_ipv4_hdr *ip_hdr;
  ip_hdr = rte_pktmbuf_mtod(mbuf, struct rte_ipv4_hdr *);

  char src_ip[INET_ADDRSTRLEN];
  char dst_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &ip_hdr->src_addr, src_ip, INET_ADDRSTRLEN);
  inet_ntop(AF_INET, &ip_hdr->dst_addr, dst_ip, INET_ADDRSTRLEN);

#ifndef NDEBUG
  printf("Original IP src: %s\n", src_ip);
  printf("Original IP dst: %s\n", dst_ip);
#endif

  // Swap src and dst IP
  uint32_t temp_src = ip_hdr->src_addr;
  ip_hdr->src_addr  = ip_hdr->dst_addr;
  ip_hdr->dst_addr  = temp_src;

  // Update the IP header checksum
  ip_hdr->hdr_checksum = 0; // Reset checksum
  ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);

  inet_ntop(AF_INET, &ip_hdr->src_addr, src_ip, INET_ADDRSTRLEN);
  inet_ntop(AF_INET, &ip_hdr->dst_addr, dst_ip, INET_ADDRSTRLEN);

#ifndef NDEBUG
  printf("Swapped IP src: %s\n", src_ip);
  printf("Swapped IP dst: %s\n", dst_ip);
#endif

  // ------------------------
  // ------- UDP/TCP --------
  // ------------------------

  struct rte_udp_hdr *udp_hdr;
  struct rte_tcp_hdr *tcp_hdr;

  if (ip_hdr->next_proto_id == IPPROTO_UDP) {
    udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ip_hdr + (ip_hdr->version_ihl & 0x0F) * 4);

#ifndef NDEBUG
    printf("Original UDP Ports: src %u, dst %u\n", rte_be_to_cpu_16(udp_hdr->src_port), rte_be_to_cpu_16(udp_hdr->dst_port));
#endif

    // Swap UDP ports
    uint16_t temp_port = udp_hdr->src_port;
    udp_hdr->src_port  = udp_hdr->dst_port;
    udp_hdr->dst_port  = temp_port;

#ifndef NDEBUG
    printf("Swapped UDP Ports: src %u, dst %u\n", rte_be_to_cpu_16(udp_hdr->src_port), rte_be_to_cpu_16(udp_hdr->dst_port));
#endif
  } else if (ip_hdr->next_proto_id == IPPROTO_TCP) {
    tcp_hdr = (struct rte_tcp_hdr *)((uint8_t *)ip_hdr + (ip_hdr->version_ihl & 0x0F) * 4);

#ifndef NDEBUG
    printf("Original TCP Ports: src %u, dst %u\n", rte_be_to_cpu_16(tcp_hdr->src_port), rte_be_to_cpu_16(tcp_hdr->dst_port));
#endif

    // Swap TCP ports
    uint16_t temp_port = tcp_hdr->src_port;
    tcp_hdr->src_port  = tcp_hdr->dst_port;
    tcp_hdr->dst_port  = temp_port;

#ifndef NDEBUG
    printf("Swapped TCP Ports: src: %u, dst: %u\n", rte_be_to_cpu_16(tcp_hdr->src_port), rte_be_to_cpu_16(tcp_hdr->dst_port));
#endif
  }

  // ------------------------
  // ------- Netcache -------
  // ------------------------

	std::array<uint8_t, KV_KEY_SIZE> kv_key;

  if (ip_hdr->next_proto_id == IPPROTO_UDP) {
    uint8_t *nc_hdr_ptr = (uint8_t *)(udp_hdr + 1);

    struct netcache_hdr_t *nc_hdr = (struct netcache_hdr_t *)nc_hdr_ptr;

	#ifndef NDEBUG
		printf("Original NC header: ");
		std::cout << "op: " << nc_hdr->op << std::endl;
		std::cout << "key: ";
		for (int i = 0; i < sizeof(nc_hdr->key); ++i) {
			std::cout << static_cast<uint8_t>(nc_hdr->key[i]);
		}
		std::cout << std::endl;
		std::cout << "value: ";
		for (int i = 0; i < sizeof(nc_hdr->val); ++i) {
			std::cout << static_cast<uint8_t>(nc_hdr->val[i]);
		}
		std::cout << std::endl;
		std::cout << "status: " << nc_hdr->status << std::endl;
		std::cout << "port: " << nc_hdr->port << std::endl;
	#endif

	std::copy(nc_hdr->key, nc_hdr->key + 16, kv_key.begin());

    auto it = kv_map.find(kv_key);
    if (it == kv_map.end()) {
		memset(nc_hdr->val, 0, sizeof(nc_hdr->val));
		nc_hdr->status = 1;
    } else {
		memcpy(nc_hdr->val, it->second, sizeof(nc_hdr->val));
		nc_hdr->status = 0;
    }

	#ifndef NDEBUG
		printf("Modified NC header: ");
		std::cout << "op: " << nc_hdr->op << std::endl;
		std::cout << "key: ";
		for (int i = 0; i < sizeof(nc_hdr->key); ++i) {
			std::cout << static_cast<uint8_t>(nc_hdr->key[i]);
		}
		std::cout << std::endl;
		std::cout << "value: ";
		for (int i = 0; i < sizeof(nc_hdr->val); ++i) {
			std::cout << static_cast<uint8_t>(nc_hdr->val[i]);
		}
		std::cout << std::endl;
		std::cout << "status: " << nc_hdr->status << std::endl;
		std::cout << "port: " << nc_hdr->port << std::endl;
	#endif

  } else if (ip_hdr->next_proto_id == IPPROTO_TCP) {
    uint8_t *nc_hdr_ptr = (uint8_t *)(tcp_hdr + 1);

    struct netcache_hdr_t *nc_hdr = (struct netcache_hdr_t *)nc_hdr_ptr;

	#ifndef NDEBUG
		printf("Original NC header: ");
		std::cout << "op: " << nc_hdr->op << std::endl;
		std::cout << "key: ";
		for (int i = 0; i < sizeof(nc_hdr->key); ++i) {
			std::cout << static_cast<uint8_t>(nc_hdr->key[i]);
		}
		std::cout << std::endl;
		std::cout << "value: ";
		for (int i = 0; i < sizeof(nc_hdr->val); ++i) {
			std::cout << static_cast<uint8_t>(nc_hdr->val[i]);
		}
		std::cout << std::endl;
		std::cout << "status: " << nc_hdr->status << std::endl;
		std::cout << "port: " << nc_hdr->port << std::endl;
	#endif

	std::copy(nc_hdr->key, nc_hdr->key + 16, kv_key.begin());

    auto it = kv_map.find(kv_key);
    if (it == kv_map.end()) {
		memset(nc_hdr->val, 0, sizeof(nc_hdr->val));
		nc_hdr->status = 1;
    } else {
		memcpy(nc_hdr->val, it->second, sizeof(nc_hdr->val));
		nc_hdr->status = 0;
    }

	#ifndef NDEBUG
		printf("Modified NC header: ");
		std::cout << "op: " << nc_hdr->op << std::endl;
		std::cout << "key: ";
		for (int i = 0; i < sizeof(nc_hdr->key); ++i) {
			std::cout << static_cast<uint8_t>(nc_hdr->key[i]);
		}
		std::cout << std::endl;
		std::cout << "value: ";
		for (int i = 0; i < sizeof(nc_hdr->val); ++i) {
			std::cout << static_cast<uint8_t>(nc_hdr->val[i]);
		}
		std::cout << std::endl;
		std::cout << "status: " << nc_hdr->status << std::endl;
		std::cout << "port: " << nc_hdr->port << std::endl;
	#endif
  }
}

} // namespace netcache
