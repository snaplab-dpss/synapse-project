#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/DataStructure.h>
#include <LibCore/Types.h>

#include <vector>
#include <optional>

#include <klee/Expr.h>

namespace LibSynapse {
namespace Tofino {

struct Digest : public DS {
  std::vector<bits_t> fields;

  Digest(DS_ID id, const std::vector<bits_t> &fields);
  Digest(const Digest &other);

  DS *clone() const override;
  void debug() const override;

  bits_t get_match_xbar_consume() const;
};

} // namespace Tofino
} // namespace LibSynapse