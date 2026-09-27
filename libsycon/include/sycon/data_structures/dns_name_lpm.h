#pragma once

#include "lpm.h"
#include "../libnf.h"

namespace sycon {

// The LPM over a DNS name (libnf's struct dns_name: the label count, then the labels top-level
// first, each its length byte and 15 bytes zero-padded). The data plane cannot lay a label's bytes
// out contiguously: its parser takes a label in pieces of 1, 2, 4 and 8 bytes selected by the bits
// of the length, and those pieces are the table's key, each at a fixed place of the label's 16
// bytes. An entry is therefore the NF's prefix with every label's bytes moved into the pieces its
// length selects, and the pieces it does not select left unmatched.
class DnsNameLPM : public LPM {
public:
  DnsNameLPM(const std::string &_name, const std::string &table_name) : LPM(_name, table_name) {}

protected:
  ternary_key_t encode(const buffer_t &prefix, u32 prefixlen) const override {
    static constexpr bytes_t LABEL_BYTES = sizeof(libnf::dns_label);
    static constexpr bytes_t PIECES[]    = {1, 2, 4, 8};

    assert(prefix.size == sizeof(libnf::dns_name) && "Not a DNS name");
    assert(prefixlen % (LABEL_BYTES * 8) == 8 && "A DNS name prefix is whole labels");

    buffer_t value(prefix.size);
    buffer_t mask(prefix.size);

    value.data[0] = prefix.data[0]; // the label count
    mask.data[0]  = 0xff;

    const u32 labels = (prefixlen - 8) / (LABEL_BYTES * 8);
    for (u32 label = 0; label < labels; label++) {
      const bytes_t base = 1 + label * LABEL_BYTES;
      const u8 len       = prefix.data[base];
      const u8 kept      = len < LABEL_BYTES - 1 ? len : LABEL_BYTES - 1; // as many as the NF keeps

      value.data[base] = len;
      mask.data[base]  = 0xff;

      // The next bytes of the label go to the next piece its length selects, at that piece's place.
      bytes_t from  = base + 1;
      bytes_t place = base + 1;
      for (const bytes_t piece : PIECES) {
        if (kept & piece) {
          for (bytes_t i = 0; i < piece; i++) {
            value.data[place + i] = prefix.data[from + i];
            mask.data[place + i]  = 0xff;
          }
          from += piece;
        }
        place += piece;
      }
    }

    return {value, mask};
  }
};

} // namespace sycon
