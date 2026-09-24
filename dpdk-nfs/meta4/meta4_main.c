#include <stdint.h>
#include <assert.h>
#include <string.h>

#include <rte_byteorder.h>

#include "lib/util/expirator.h"

#include "nf.h"
#include "nf-log.h"
#include "nf-util.h"
#include "config.h"
#include "state.h"
#include "session.h"

struct nf_config config;
struct State *state;

bool nf_init(void) {
  state = alloc_state();
  return state != NULL;
}

static void expire_sessions(time_ns_t now) {
  assert(now >= 0); // we don't support the past
  assert(sizeof(time_ns_t) <= sizeof(uint64_t));
  uint64_t time_u             = (uint64_t)now; // OK because of the two asserts
  uint64_t expiration_time_ns = config.drt_expiration_time * 1000;
  time_ns_t last_time         = time_u - expiration_time_ns;
  expire_items_single_map(state->drt_allocator, state->drt_keys, state->drt, last_time);
}

static void counter_increment(struct Vector *counters, uint32_t index, uint32_t amount) {
  uint32_t *counter;
  vector_borrow(counters, index, (void **)&counter);
  *counter += amount;
  vector_return(counters, index, counter);
}

// Records that this client and server now belong to a watched domain.
static void learn_session(struct session *session, uint32_t domain_id, time_ns_t now) {
  int index = -1;

  if (map_get(state->drt, session, &index)) {
    // Already tracked: refresh it, and let the newest response decide the domain, since a server
    // address can be reassigned to another name.
    NF_DEBUG("refresh session %u -> %u domain %u", session->client_ip, session->server_ip, domain_id);
    dchain_rejuvenate_index(state->drt_allocator, index, now);

    uint32_t *tracked_domain;
    vector_borrow(state->drt_domains, index, (void **)&tracked_domain);
    *tracked_domain = domain_id;
    vector_return(state->drt_domains, index, tracked_domain);

    return;
  }

  if (!dchain_allocate_new_index(state->drt_allocator, &index, now)) {
    // No room, and nothing has timed out: this response goes unrecorded, and the traffic it would
    // have attributed will be missed.
    NF_DEBUG("missed domain %u", domain_id);
    counter_increment(state->dns_missed, domain_id, 1);
    return;
  }

  struct session *stored_session;
  vector_borrow(state->drt_keys, index, (void **)&stored_session);
  memcpy(stored_session, session, sizeof(struct session));
  map_put(state->drt, stored_session, index);
  vector_return(state->drt_keys, index, stored_session);

  uint32_t *tracked_domain;
  vector_borrow(state->drt_domains, index, (void **)&tracked_domain);
  *tracked_domain = domain_id;
  vector_return(state->drt_domains, index, tracked_domain);

  NF_DEBUG("learn session %u -> %u domain %u", session->client_ip, session->server_ip, domain_id);
}

static int process_dns_response(uint16_t device, uint8_t **buffer, struct rte_udp_hdr *udp_header, struct rte_ipv4_hdr *ipv4_header,
                                time_ns_t now) {
  uint16_t length            = 0;
  struct dns_hdr *dns_header = nf_then_get_dns_message(udp_header, buffer, &length);
  if (dns_header == NULL) {
    NF_DEBUG("dns message not read");
    return device;
  }

  if ((dns_header->flags_hi & DNS_FLAG_IS_RESPONSE) == 0) {
    NF_DEBUG("dns query, ignored");
    return device;
  }

  // The name the response answers, and the address it resolves to, which may sit behind CNAMEs.
  struct dns_name name;
  uint32_t address = 0;
  if (!dns_get_response(dns_header, length, &name, &address)) {
    NF_DEBUG("no name and address in the response");
    return device;
  }

  // The watched pattern that fits the name furthest, which is the most specific one that covers it.
  int domain_id = -1;
  if (!lpm_lookup(state->known_domains, &name, &domain_id)) {
    NF_DEBUG("dns response for an unwatched name");
    return device;
  }

  // A watched name still goes unrecorded if the client it was answered to is one whose traffic is
  // left out of the accounting.
  int ignored = 0;
  if (lpm_lookup(state->ignored_clients, &ipv4_header->dst_addr, &ignored)) {
    NF_DEBUG("dns response to an ignored client %u", ipv4_header->dst_addr);
    return device;
  }

  counter_increment(state->dns_queried, domain_id, 1);

  struct session session = {
      .client_ip = ipv4_header->dst_addr,
      .server_ip = address,
  };

  learn_session(&session, domain_id, now);

  return device;
}

// Data packets travel server -> client, the opposite direction of the DNS response that taught us the pair.
static int process_data_packet(uint16_t device, struct rte_ipv4_hdr *ipv4_header, uint16_t packet_length, time_ns_t now) {
  struct session session = {
      .client_ip = ipv4_header->dst_addr,
      .server_ip = ipv4_header->src_addr,
  };

  int index = -1;
  if (!map_get(state->drt, &session, &index)) {
    // Not part of a session we know the domain of.
    NF_DEBUG("unattributed %u -> %u", session.server_ip, session.client_ip);
    return device;
  }

  // Traffic keeps the session alive, so a busy session is not evicted.
  dchain_rejuvenate_index(state->drt_allocator, index, now);

  uint32_t *tracked_domain;
  vector_borrow(state->drt_domains, index, (void **)&tracked_domain);
  uint32_t domain_id = *tracked_domain;
  vector_return(state->drt_domains, index, tracked_domain);

  NF_DEBUG("attribute %u bytes of %u -> %u to domain %u", packet_length, session.server_ip, session.client_ip, domain_id);
  counter_increment(state->pkt_counts, domain_id, 1);
  counter_increment(state->byte_counts, domain_id, packet_length);

  return device;
}

int nf_process(uint16_t device, uint8_t **buffer, uint16_t packet_length, time_ns_t now, struct rte_mbuf *mbuf) {
  (void)mbuf;

  expire_sessions(now);

  struct rte_ether_hdr *ether_header = nf_then_get_ether_header(buffer);

  struct rte_ipv4_hdr *ipv4_header = nf_then_get_ipv4_header(ether_header, buffer);
  if (ipv4_header == NULL) {
    return device;
  }

  struct rte_udp_hdr *udp_header = nf_then_get_udp_header(ipv4_header, buffer);

  if (udp_header != NULL && nf_has_dns_header(udp_header)) {
    return process_dns_response(device, buffer, udp_header, ipv4_header, now);
  }

  return process_data_packet(device, ipv4_header, packet_length, now);
}
