#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/DataStructure.h>
#include <LibCore/Types.h>

#include <vector>
#include <optional>

#include <klee/Expr.h>

namespace LibSynapse {
namespace Tofino {

struct LPM : public DS {
  u32 capacity;
  const bits_t key = 32;

  LPM(DS_ID id, u32 capacity);
  LPM(const LPM &other);

  DS *clone() const override;
  void debug() const override;

  bits_t get_match_xbar_consume() const;
  bits_t get_consumed_tcam() const;
};

} // namespace Tofino
} // namespace LibSynapse