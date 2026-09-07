#pragma once

#include <LibBDD/BDD.h>

#include <string>
#include <vector>

namespace LibBDD {

// Invariant: a BDD expression contains at most one arithmetic operation.
//
// Arithmetic operations are + - * / %, bitwise & | ^ ~ on values wider than one bit, and
// shifts. Everything else is structural and does not count: bit layout (Extract, Concat, ZExt,
// SExt, and byte swaps, which are canonicalized to layout), comparisons, ?:, and the 1-bit
// connectives joining conditions.
//
// The invariant is enforced by unrolling: every arithmetic operation nested beneath another
// becomes its own call node, op_<kind>(a, b) -> unrolled, inserted before the node that consumes
// it (operands before results, a shared subexpression once per code path). The consumer keeps
// only its topmost operation. The definition unrolled == a <kind> b is part of the constraints
// of every node that follows (see BDD::get_constraints), so the solver loses nothing.
void unroll_arithmetic(BDD &bdd);

bool is_arithmetic_op(klee::ref<klee::Expr> expr);

// Function names of the unrolled op nodes (op_add, op_xor, ...).
const std::vector<std::string> &unrolled_op_function_names();
bool is_unrolled_op(const call_t &call);

// The value an unrolled op node computes, rebuilt from its call: a <kind> b.
klee::ref<klee::Expr> unrolled_op_value(const call_t &call);

} // namespace LibBDD
