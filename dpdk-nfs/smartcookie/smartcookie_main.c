#include <stdint.h>
#include <string.h>

#include <rte_byteorder.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>

#include "nf.h"
#include "nf-log.h"
#include "nf-util.h"
#include "config.h"
#include "state.h"
#include "flow_id.h"
#include "halfsiphash.h"

struct nf_config config;
struct State *state;

// Payload of the server agent's time-sync packets: its clock in 2^16 ns ticks.
struct timesync_payload {
  uint32_t ticks;
};

#ifdef KLEE_VERIFICATION
static struct str_field_descr timesync_fields[] = {{offsetof(struct timesync_payload, ticks), sizeof(uint32_t), 0, "ticks"}};
#endif // KLEE_VERIFICATION

bool nf_init(void) {
  state = alloc_state();
  return state != NULL;
}

// Time in 2^16 ns ticks, the resolution the switch agent works with (bits [47:16] of the MAC
// timestamp).
static uint32_t ticks(time_ns_t now) { return (uint32_t)(((uint64_t)now) >> 16); }

static void set_timedelta(uint32_t delta) {
  uint32_t *timedelta;
  vector_borrow(state->timedelta, 0, (void **)&timedelta);
  *timedelta = delta;
  vector_return(state->timedelta, 0, timedelta);
}

// Current cookie epoch (2^28 ns) on the server's clock.
static uint32_t cookie_time(time_ns_t now) {
  uint32_t *timedelta;
  vector_borrow(state->timedelta, 0, (void **)&timedelta);
  uint32_t delta = *timedelta;
  vector_return(state->timedelta, 0, timedelta);
  return (ticks(now) - delta) >> 12;
}

static uint32_t cookie_hash(struct rte_ipv4_hdr *ipv4, struct rte_tcp_hdr *tcp, uint32_t seq) {
  uint32_t ports = ((uint32_t)rte_bswap16(tcp->src_port) << 16) | rte_bswap16(tcp->dst_port);
  return halfsiphash(config.sip_init, rte_bswap32(ipv4->src_addr), rte_bswap32(ipv4->dst_addr), ports, seq);
}

// The switch agent's naive_routing: the egress device is the first octet of the destination
// address.
static uint16_t naive_routing(struct rte_ipv4_hdr *ipv4) { return rte_bswap32(ipv4->dst_addr) >> 24; }

// Crafted packets carry no options and no payload.
static void finish_crafted_packet(struct rte_ipv4_hdr *ipv4, struct rte_tcp_hdr *tcp, uint8_t **buffer) {
  tcp->data_off      = 5 << 4;
  ipv4->version_ihl  = 0x45;
  ipv4->total_length = rte_bswap16(sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_tcp_hdr));
  nf_set_rte_ipv4_udptcp_checksum(ipv4, tcp, buffer);
}

// Answer a client's SYN with a SYN-ACK whose sequence number is the cookie, back to the client.
static int reply_synack(uint16_t device, struct rte_ipv4_hdr *ipv4, struct rte_tcp_hdr *tcp, uint8_t **buffer, time_ns_t now) {
  uint32_t seq    = rte_bswap32(tcp->sent_seq);
  uint32_t cookie = cookie_time(now) ^ cookie_hash(ipv4, tcp, seq);

  uint32_t src_addr = ipv4->src_addr;
  ipv4->src_addr    = ipv4->dst_addr;
  ipv4->dst_addr    = src_addr;

  uint16_t src_port = tcp->src_port;
  tcp->src_port     = tcp->dst_port;
  tcp->dst_port     = src_port;

  tcp->recv_ack = rte_bswap32(seq + 1);
  tcp->sent_seq = rte_bswap32(cookie);
  tcp->tcp_flags |= RTE_TCP_SYN_FLAG | RTE_TCP_ACK_FLAG;

  finish_crafted_packet(ipv4, tcp, buffer);
  return device;
}

// Check the cookie a client returns in its ACK (ack = cookie + 1, seq = original seq + 1). Cookies
// from the current epoch and the two before it are accepted. Verified packets go to the server
// tagged with ECE and with the original sequence number, for the server agent to set the
// connection up.
static int verify_cookie(struct rte_ipv4_hdr *ipv4, struct rte_tcp_hdr *tcp, uint8_t **buffer, time_ns_t now) {
  uint32_t seq        = rte_bswap32(tcp->sent_seq);
  uint32_t ack        = rte_bswap32(tcp->recv_ack);
  uint32_t cookie_val = (ack - 1) ^ cookie_hash(ipv4, tcp, seq - 1);
  uint32_t age        = cookie_time(now) - cookie_val;

  if (age > 2) {
    return DROP;
  }

  tcp->sent_seq = rte_bswap32(seq - 1);
  tcp->tcp_flags |= RTE_TCP_ECE_FLAG;

  finish_crafted_packet(ipv4, tcp, buffer);
  return config.server_dev;
}

int nf_process(uint16_t device, uint8_t **buffer, uint16_t packet_length, time_ns_t now, struct rte_mbuf *mbuf) {
  (void)packet_length;
  (void)mbuf;

  struct rte_ether_hdr *ether_header = nf_then_get_ether_header(buffer);

  struct rte_ipv4_hdr *ipv4_header = nf_then_get_ipv4_header(ether_header, buffer);
  if (ipv4_header == NULL) {
    return DROP;
  }

  if (ipv4_header->next_proto_id == IP_PROTOCOL_UDP) {
    // The server agent periodically sends its clock, so cookies can be verified on both sides.
    struct rte_udp_hdr *udp_header = nf_then_get_udp_header(ipv4_header, buffer);
    if (udp_header != NULL && device == config.server_dev && udp_header->dst_port == rte_bswap16(TIMESYNC_PORT) &&
        packet_get_unread_length(buffer) >= sizeof(struct timesync_payload)) {
      CHUNK_LAYOUT(*buffer, timesync_payload, timesync_fields);
      struct timesync_payload *payload = (struct timesync_payload *)nf_borrow_next_chunk(buffer, sizeof(struct timesync_payload));
      set_timedelta(ticks(now) - rte_bswap32(payload->ticks));
      return DROP;
    }
    return naive_routing(ipv4_header);
  }

  if (ipv4_header->next_proto_id != IP_PROTOCOL_TCP) {
    return naive_routing(ipv4_header);
  }

  struct rte_tcp_hdr *tcp_header = nf_then_get_tcp_header(ipv4_header, buffer);
  if (tcp_header == NULL) {
    return DROP;
  }

  struct FlowId flow = {
      .src_ip   = ipv4_header->src_addr,
      .dst_ip   = ipv4_header->dst_addr,
      .src_port = tcp_header->src_port,
      .dst_port = tcp_header->dst_port,
  };

  if (device == config.server_dev) {
    // A confirmation from the server agent (the client->server 4-tuple, tagged with ECE) marks the
    // connection as verified; everything else from the server is plain traffic to a client.
    if (tcp_header->tcp_flags & RTE_TCP_ECE_FLAG) {
      bf_set(state->verified, &flow);
      return DROP;
    }
    return naive_routing(ipv4_header);
  }

  bool syn = tcp_header->tcp_flags & RTE_TCP_SYN_FLAG;
  bool ack = tcp_header->tcp_flags & RTE_TCP_ACK_FLAG;

  if (syn && !ack) {
    return reply_synack(device, ipv4_header, tcp_header, buffer, now);
  }

  if (syn) {
    return DROP;
  }

  if (bf_query(state->verified, &flow)) {
    return config.server_dev;
  }

  return verify_cookie(ipv4_header, tcp_header, buffer, now);
}
