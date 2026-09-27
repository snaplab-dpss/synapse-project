#pragma once

#include <vector>

#include "synapse_ds.h"
#include "../config.h"
#include "../constants.h"
#include "../primitives/table.h"

namespace sycon {

// A longest-prefix match over an arbitrary-width key, as a ternary table: an entry masks its
// prefix's bits and takes a priority from its length, so among the matching entries the longest
// prefix is the one the hardware picks. Prefix bits run from the first key byte, most significant
// bit first, as libnf's LPM compares them.
class LPM : public SynapseDS {
private:
  struct entry_t {
    buffer_t prefix;
    u32 prefixlen;
    u32 value;
  };

  std::vector<entry_t> entries;
  Table table;
  bits_t key_size;

public:
  LPM(const std::string &_name, const std::string &table_name) : SynapseDS(_name), table(table_name), key_size(table.get_ternary_key_size()) {}

  virtual ~LPM() = default;

  void update(const buffer_t &prefix, u32 prefixlen, u32 value) {
    assert(prefixlen <= key_size && "Prefix longer than the key");
    assert(prefix.size * 8 == key_size && "Prefix buffer does not have the key's size");

    const table_action_t &action = table.get_actions().at(0);

    buffer_t value_param(4);
    value_param.set(0, 4, value);

    for (entry_t &entry : entries) {
      if (entry.prefixlen == prefixlen && bits_equal(entry.prefix, prefix, prefixlen)) {
        const ternary_key_t old_key = encode(entry.prefix, prefixlen);
        table.del_entry_ternary(old_key.value, old_key.mask, priority(prefixlen));
        const ternary_key_t new_key = encode(prefix, prefixlen);
        table.add_entry_ternary(new_key.value, new_key.mask, priority(prefixlen), action.name, {value_param});
        entry.prefix = prefix;
        entry.value  = value;
        return;
      }
    }

    if (table.get_usage() >= table.get_capacity()) {
      LOG_DEBUG("WARNING: Attempted to add a prefix to an already full LPM table");
      return;
    }

    const ternary_key_t key = encode(prefix, prefixlen);
    table.add_entry_ternary(key.value, key.mask, priority(prefixlen), action.name, {value_param});
    entries.push_back({prefix, prefixlen, value});
  }

  bool lookup(const buffer_t &key, u32 &value) const {
    assert(key.size * 8 == key_size && "Key buffer does not have the key's size");

    const entry_t *longest = nullptr;
    for (const entry_t &entry : entries) {
      if ((longest && entry.prefixlen <= longest->prefixlen) || !bits_equal(entry.prefix, key, entry.prefixlen)) {
        continue;
      }
      longest = &entry;
    }

    if (!longest) {
      return false;
    }

    value = longest->value;
    return true;
  }

  void dump() const {
    std::stringstream ss;
    dump(ss);
    LOG("%s", ss.str().c_str());
  }

  void dump(std::ostream &os) const {
    os << "================================================\n";
    os << "LPM " << name << " (" << entries.size() << " prefixes, " << key_size << " bit keys)\n";
    for (const entry_t &entry : entries) {
      os << "  " << entry.prefix << "/" << entry.prefixlen << " -> " << entry.value << "\n";
    }
    os << "================================================\n";
  }

protected:
  struct ternary_key_t {
    buffer_t value;
    buffer_t mask;
  };

  // The table entry for a prefix: the prefix's bytes under the mask of its length.
  virtual ternary_key_t encode(const buffer_t &prefix, u32 prefixlen) const { return {prefix, mask(prefixlen)}; }

  // BF-RT ranks ternary entries by ascending priority value.
  u32 priority(u32 prefixlen) const { return key_size - prefixlen; }

  buffer_t mask(u32 prefixlen) const {
    buffer_t m(key_size / 8);
    for (bytes_t i = 0; i < m.size && prefixlen > 0; i++) {
      const u32 bits = std::min<u32>(8, prefixlen);
      m.data[i]      = static_cast<u8>(0xff << (8 - bits));
      prefixlen -= bits;
    }
    return m;
  }

  static bool bits_equal(const buffer_t &a, const buffer_t &b, u32 prefixlen) {
    const bytes_t whole = prefixlen / 8;
    for (bytes_t i = 0; i < whole; i++) {
      if (a.data[i] != b.data[i]) {
        return false;
      }
    }

    const u32 rest = prefixlen % 8;
    if (rest == 0) {
      return true;
    }

    const u8 m = static_cast<u8>(0xff << (8 - rest));
    return (a.data[whole] & m) == (b.data[whole] & m);
  }
};

} // namespace sycon
