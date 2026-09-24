#ifndef _DNS_HDR_H_INCLUDED_
#define _DNS_HDR_H_INCLUDED_

#include <stdbool.h>
#include <stdint.h>

#include "lib/util/boilerplate.h"

#define DNS_PORT 53

// QR bit of the first flags octet: 1 means this is a response.
#define DNS_FLAG_IS_RESPONSE 0x80

// RR TYPE values. A carries an IPv4 address; following a CNAME means walking an unbounded number
// of records, which no NF does yet.
#define DNS_TYPE_A 1
#define DNS_TYPE_CNAME 5

// Bytes of a label kept when a name is read. A label is copied in exactly and zero-padded to this
// width, so two names agree on a label iff the label is the same; a longer label is truncated to
// its first bytes and is then indistinguishable from another sharing them. RFC 1035 allows 63, and
// nothing here costs symbolic execution anything -- the copy runs inside dns_get_response()'s model
// -- but every byte here is a byte of every name, whether the label that lands in it is that long
// or not.
#define DNS_LABEL_BYTES 15

// How many labels of a name are read. A name has to end within this many labels to be recognized,
// which covers the usual host.domain.tld shapes.
#define DNS_MAX_LABELS 4

struct dns_hdr {
  uint16_t id;
  uint8_t flags_hi; // QR(1) opcode(4) AA(1) TC(1) RD(1)
  uint8_t flags_lo; // RA(1) Z(3) rcode(4)
  uint16_t q_count;
  uint16_t answer_count;
  uint16_t auth_rec;
  uint16_t addn_rec;
} __attribute__((__packed__));

// QTYPE and QCLASS, closing the question section. Spelled out rather than named "class", which a
// C++ consumer of this header cannot compile.
struct dns_query_tc {
  uint16_t qtype;
  uint16_t qclass;
} __attribute__((__packed__));

// The fixed part of a resource record, up to but excluding RDATA.
struct dns_answer {
  uint16_t qname_pointer;
  uint16_t type;
  uint16_t rr_class;
  uint32_t ttl;
  uint16_t rd_length;
} __attribute__((__packed__));

// A label of a domain name, as read: how long it was, and the bytes that were kept of it.
struct dns_label {
  uint8_t len;
  uint8_t bytes[DNS_LABEL_BYTES];
} PACKED_FOR_KLEE_VERIFICATION;

// A domain name in fixed-width form, laid out so that a longest-prefix match over these bytes means
// what matching a domain name should mean.
//
// Labels are stored in reverse, the top-level one first, so that dropping labels off the front of a
// name -- which is what a pattern like "*.example.com" does -- drops bytes off the END, where a
// prefix match already ignores them. The label count leads, so that a pattern is only ever matched
// by names with as many labels as it names: "*.example.com" covers www.example.com but not
// example.com itself, and not a.b.example.com.
//
// A pattern is therefore an ordinary prefix of a name: it spans 8 bits for the count plus one slot
// per label it spells out. Nothing here records which of a pattern's labels were wildcards.
struct dns_name {
  uint8_t labels;
  struct dns_label label[DNS_MAX_LABELS];
} PACKED_FOR_KLEE_VERIFICATION;

// How many bits of a name a pattern spelling out `labels` of its labels spans.
#define DNS_NAME_PREFIX_BITS(labels) (8 + (labels) * (uint32_t)sizeof(struct dns_label) * 8)

// Largest DNS message read whole. A single borrow cannot exceed the symbolic-execution model's
// chunk size, and reading the message in one piece is what keeps its variable-length insides out of
// the path space. A longer message is read up to this bound rather than given up on: a DNS message
// carries its header, then the question, then the answers, so what a bound cuts off is the
// authority and additional records, which nothing here looks at.
#define DNS_MAX_MESSAGE 255

// Reads what a response says: the domain name it answers about, and the address that name resolves
// to, which commonly sits behind a chain of CNAMEs. Returns 0 if the message carries no name this
// can represent, or no address in the records it carries.
//
// Returns int rather than bool: a bool return is a converted value, and the BDD builder can only
// bind a modeled function's result symbol to a plain read of it.
//
// Behind a symbolic-execution model. Walking the labels and the answer records means a
// data-dependent number of steps, and every step that could end the name or the chain forks the
// path -- and since paths never rejoin, everything done afterwards is copied into each arm.
// Measured here, that was the whole session-learning path emitted ten times over. The model makes
// the walk one node with a symbolic result, leaving the paths to describe what the NF decides
// rather than how many bytes a name happened to have.
int dns_get_response(const struct dns_hdr *msg, uint16_t length, struct dns_name *name, uint32_t *address);

#define LOG_DNS_NAME(obj, p)                                                                                                                         \
  ;                                                                                                                                                  \
  p("{");                                                                                                                                            \
  p("labels: %u", obj->labels);                                                                                                                      \
  p("}");

#endif //_DNS_HDR_H_INCLUDED_
