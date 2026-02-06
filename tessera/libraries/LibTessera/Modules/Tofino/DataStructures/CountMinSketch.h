#pragma once

#include <LibTessera/Modules/Tofino/DataStructures/DataStructure.h>
#include <LibTessera/Modules/Tofino/DataStructures/Hash.h>
#include <LibTessera/Modules/Tofino/DataStructures/Register.h>
#include <LibCore/Types.h>

#include <vector>
#include <optional>
#include <unordered_set>

namespace LibTessera {
namespace Tofino {

struct CountMinSketch : public DS {
  static const std::vector<u32> HASH_SALTS;

  u32 width;
  u32 height;
  bits_t hash_size;

  std::vector<Hash> hashes;
  std::vector<Register> rows;

  CountMinSketch(const tna_properties_t &properties, DS_ID id, const std::vector<bits_t> &keys, u32 width, u32 height);

  CountMinSketch(const CountMinSketch &other);

  DS *clone() const override;
  void debug() const override;
  std::vector<std::unordered_set<const DS *>> get_internal() const override;
};

} // namespace Tofino
} // namespace LibTessera