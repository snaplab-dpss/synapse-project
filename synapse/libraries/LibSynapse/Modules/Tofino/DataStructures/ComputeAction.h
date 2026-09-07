#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/DataStructure.h>
#include <LibCore/Types.h>

#include <vector>
#include <string>

namespace LibSynapse {
namespace Tofino {

// Where an operation executes. ALU ops (add, xor, byte-aligned rotates, ...) are one VLIW
// instruction each; other rotates go through the hash distribution unit (@in_hash).
enum class ComputeOpKind { ALU, Hash };

struct compute_op_t {
  std::string id; // Output symbol / BDD node this op computes.
  ComputeOpKind kind;
  bits_t width; // Output width.
};

// One P4 action holding independent stateless computations, called bare from the apply block
// (the compiler makes a keyless table out of it). Ops are appended, not entries: ops in the same
// action execute in the same stage, so appending an op costs no stage and no logical ID.
//
// Limits measured with bf-p4c 9.13.4 (Tofino 2), see the SmartCookie notes: an action can carry
// at most 32 bits produced by the hash unit (its immediate pathway), and a 32-bit @in_hash op
// takes 2 of a stage's 6 hash distribution units (16 bits each).
struct ComputeAction : public DS {
  static constexpr bits_t MAX_HASH_BITS_PER_ACTION = 32;
  static constexpr bits_t HASH_DIST_UNIT_BITS      = 16;

  std::vector<compute_op_t> ops;

  ComputeAction(DS_ID id, const std::vector<compute_op_t> &ops = {});
  ComputeAction(const ComputeAction &other);

  DS *clone() const override;
  void debug() const override;

  bool can_take(const compute_op_t &op) const;

  bits_t get_hash_bits() const;
  int get_hash_dist_units() const;
  size_t get_num_alu_ops() const;
  size_t get_num_hash_ops() const;

  static int hash_dist_units_for(bits_t width);
};

} // namespace Tofino
} // namespace LibSynapse
