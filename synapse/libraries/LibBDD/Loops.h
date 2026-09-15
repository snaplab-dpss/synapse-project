#pragma once

#include <LibBDD/Nodes/Node.h>

#include <klee/Expr.h>

#include <optional>
#include <string>
#include <vector>

namespace LibBDD {

// A loop symbolic execution unrolled into the BDD: the same computation over a state, repeated.
// The BDD keeps no trace of the loop itself, only of its iterations, each a copy of the body's
// ops reading the previous iteration's values. BDD::detect_loops() finds them by lining every
// iteration up against the one before it.
//
// Ops that change the state between two iterations without being part of the body (SipHash's
// message words, mixed in between rounds) are steps, not body ops. An iteration symbolic
// execution evaluated on constants lacks the ops it folded away; those are missing from that
// iteration, the body still has them. Where that leaves the first iterations irregular -- missing
// body ops, and computing the rest in shapes the body does not have -- they are not iterations but
// the loop's prefix, together with what the loop reads from before it: the iterations proper
// start at the first one computing every body op.

enum class LoopOperandKind {
  Body,    // A value the same iteration computed earlier: `index` is the body op computing it.
  State,   // A value the previous iteration computed: `index` is the body op that computed it.
  Outside, // `expr`, read the same in every iteration: a constant, or a value from outside the loop.
};

struct loop_operand_t {
  LoopOperandKind kind;
  size_t index;
  klee::ref<klee::Expr> expr;
};

struct loop_body_op_t {
  std::string fn;                       // The op node's function (op_add, op_xor, ...) or rotate_left.
  bits_t width;                         // The width of the value it computes.
  std::vector<loop_operand_t> operands; // In the call's argument order: a, b; or x, n.
};

struct loop_step_t {
  bdd_node_id_t node;     // The op node.
  size_t after_iteration; // It runs between this iteration (0-based) and the next.
  size_t state;           // The body op whose value it changes on the way to the next iteration.
};

struct loop_t {
  std::vector<loop_body_op_t> body;
  std::vector<size_t> state; // The body ops whose values the next iteration reads.
  // By iteration, then by body op: the node computing it, or nothing where symbolic execution
  // folded it away. An op a rotate holds inline has the rotate's node.
  std::vector<std::vector<std::optional<bdd_node_id_t>>> iterations;
  std::vector<loop_step_t> steps;
  // The ops of the irregular first iterations and of the steps around them, and the ops before
  // the loop whose values those read: what computes the state the first iteration starts from.
  std::vector<bdd_node_id_t> prefix;
};

} // namespace LibBDD
