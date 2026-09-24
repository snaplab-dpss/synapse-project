#include "lib/util/dns_hdr.h"

#include <stddef.h>
#include <string.h>

// The message is read as plain bytes rather than through the record structs, so that nothing here
// depends on how a compiler lays those out at an offset the message chooses.
static uint16_t be16_at(const uint8_t *bytes, uint16_t at) { return (uint16_t)((uint16_t)bytes[at] << 8 | bytes[at + 1]); }

int dns_get_response(const struct dns_hdr *msg, uint16_t length, struct dns_name *name, uint32_t *address) {
  const uint8_t *bytes = (const uint8_t *)msg;

  memset(name, 0, sizeof(struct dns_name));

  uint16_t at = sizeof(struct dns_hdr);

  // --- the question's name
  //
  // Collected in the order the message spells it and reversed into `name` at the end, because which
  // slot a label belongs in depends on how many there turn out to be.
  struct dns_label spelled[DNS_MAX_LABELS];

  uint32_t label = 0;
  for (; label < DNS_MAX_LABELS; label++) {
    if (at >= length) {
      return 0;
    }

    uint8_t len = bytes[at];

    if (len == 0) {
      break;
    }

    at += 1;
    if ((uint32_t)at + len > length) {
      return 0;
    }

    // Exactly the label's own bytes, zero-padded, and truncated if it is longer than a slot.
    uint8_t kept = len < DNS_LABEL_BYTES ? len : DNS_LABEL_BYTES;
    memset(&spelled[label], 0, sizeof(struct dns_label));
    spelled[label].len = len;
    memcpy(spelled[label].bytes, bytes + at, kept);

    at += len;
  }

  // However many labels were read, the name has to end right here to be one this can represent.
  if (at >= length || bytes[at] != 0) {
    return 0;
  }

  for (uint32_t i = 0; i < label; i++) {
    name->label[label - 1 - i] = spelled[i]; // top-level label first
  }

  name->labels = (uint8_t)label;
  at += 1;                           // the terminating zero label
  at += sizeof(struct dns_query_tc); // QTYPE and QCLASS close the question

  // --- the address, past the CNAMEs a response commonly puts in front of it
  //
  // Every record read moves `at` forward by at least a record header, so the walk ends at the end of
  // the message and needs no count of its own.
  while ((uint32_t)at + sizeof(struct dns_answer) <= length) {
    uint16_t type      = be16_at(bytes, at + offsetof(struct dns_answer, type));
    uint16_t rd_length = be16_at(bytes, at + offsetof(struct dns_answer, rd_length));
    at += sizeof(struct dns_answer);

    if (type == DNS_TYPE_A) {
      if ((uint32_t)at + sizeof(uint32_t) > length) {
        return 0;
      }
      memcpy(address, bytes + at, sizeof(uint32_t));
      return 1;
    }

    // Only a CNAME is worth stepping over; anything else is not leading to an address.
    if (type != DNS_TYPE_CNAME || (uint32_t)at + rd_length > length) {
      return 0;
    }

    at += rd_length;
  }

  return 0;
}
