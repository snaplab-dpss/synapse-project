#include <LibBDD/BDD.h>
#include <LibBDD/Loops.h>
#include <LibBDD/Unroll.h>
#include <LibCore/Expr.h>

#include <algorithm>
#include <functional>
#include <map>
#include <numeric>
#include <optional>
#include <ostream>
#include <queue>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace LibBDD {

namespace {

// Fewer iterations than this is not reported as a loop.
constexpr size_t MIN_ITERATIONS = 3;

enum class OperandKind { Value, Outside };

struct operand_t {
  OperandKind kind;
  size_t value;               // Value: the index of the value read.
  klee::ref<klee::Expr> expr; // Outside: what is read.
};

// A computed value: an op node, a rotate, or an operation a rotate holds inline in its argument.
struct value_t {
  const Call *node; // The node computing it; for an inline operation, the rotate holding it.
  std::string fn;
  klee::Expr::Kind kind; // Of the operation; Invalid for a rotate.
  bits_t width;
  klee::ref<klee::Expr> expr; // What stands for the value: its symbol, or the inline operation.
  bool is_inline;
  std::vector<operand_t> operands;
  std::vector<size_t> consumers;
  size_t depth;
};

bool is_commutative(klee::Expr::Kind kind) {
  switch (kind) {
  case klee::Expr::Add:
  case klee::Expr::Mul:
  case klee::Expr::And:
  case klee::Expr::Or:
  case klee::Expr::Xor:
    return true;
  default:
    return false;
  }
}

bool same_expr(klee::ref<klee::Expr> a, klee::ref<klee::Expr> b) { return a->compare(*b) == 0; }

std::string short_expr(klee::ref<klee::Expr> expr) { return LibCore::expr_to_string(expr, true).substr(0, 40); }

// The data flow between the BDD's compute nodes.
class graph_t {
public:
  std::vector<value_t> values;
  // Inline operations found to be another value: they take no part in the analysis.
  std::vector<bool> merged;

  explicit graph_t(const BDD &bdd) {
    std::vector<const Call *> nodes;
    bdd.get_root()->visit_nodes([&](const BDDNode *node) {
      if (node->get_type() != BDDNodeType::Call) {
        return BDDNodeVisitAction::Continue;
      }
      const Call *call_node = dynamic_cast<const Call *>(node);
      const call_t &call    = call_node->get_call();
      std::string symbol;
      if ((is_unrolled_op(call) || call.function_name == "rotate_left") && LibCore::is_readLSB(call.ret, symbol)) {
        by_symbol[symbol] = values.size();
        values.push_back({call_node, call.function_name, klee::Expr::InvalidKind, call.ret->getWidth(), call.ret, false, {}, {}, 0});
        nodes.push_back(call_node);
      }
      return BDDNodeVisitAction::Continue;
    });

    // Every value is registered before any operand is resolved: an operand may name a value
    // whichever order the visit met them in.
    for (size_t i = 0; i < nodes.size(); i++) {
      const call_t &call = nodes[i]->get_call();
      std::vector<klee::ref<klee::Expr>> args;
      if (call.function_name == "rotate_left") {
        args = {call.args.at("x").expr, call.args.at("n").expr};
      } else {
        values[i].kind = unrolled_op_value(call)->getKind();
        args.push_back(call.args.at("a").expr);
        if (call.args.count("b")) {
          args.push_back(call.args.at("b").expr);
        }
      }
      std::vector<operand_t> operands;
      for (const klee::ref<klee::Expr> &arg : args) {
        operands.push_back(resolve(arg, nodes[i]));
      }
      values[i].operands = std::move(operands);
    }

    merge_inline_operations();

    for (size_t i = 0; i < values.size(); i++) {
      if (merged[i]) {
        continue;
      }
      for (const operand_t &operand : values[i].operands) {
        if (operand.kind == OperandKind::Value) {
          values[operand.value].consumers.push_back(i);
        }
      }
    }

    std::vector<bool> done(values.size(), false);
    for (size_t i = 0; i < values.size(); i++) {
      compute_depth(i, done);
    }
  }

  size_t count_merged() const { return std::count(merged.begin(), merged.end(), true); }

  std::string label(size_t v) const {
    const value_t &value = values[v];
    std::string label    = value.fn + "/" + std::to_string(value.width);
    if (value.fn == "rotate_left" && value.operands.size() == 2 && value.operands[1].kind == OperandKind::Outside) {
      label += "/" + LibCore::expr_to_string(value.operands[1].expr, true);
    }
    return label;
  }

  // `v` for the trace: its node, function and operands.
  std::string describe(size_t v) const {
    const value_t &value = values[v];
    std::string out      = std::to_string(value.node->get_id()) + ":" + value.fn + "(";
    for (size_t i = 0; i < value.operands.size(); i++) {
      const operand_t &operand = value.operands[i];
      out += i ? ", " : "";
      switch (operand.kind) {
      case OperandKind::Value:
        out += std::to_string(values[operand.value].node->get_id()) + ":" + values[operand.value].fn;
        break;
      case OperandKind::Outside:
        out += short_expr(operand.expr);
        break;
      }
    }
    return out + ")";
  }

  // Values connected by data flow.
  std::vector<std::vector<size_t>> components() const {
    std::vector<size_t> parent(values.size());
    std::iota(parent.begin(), parent.end(), 0);
    std::function<size_t(size_t)> find = [&](size_t v) { return parent[v] == v ? v : parent[v] = find(parent[v]); };
    for (size_t i = 0; i < values.size(); i++) {
      if (merged[i]) {
        continue;
      }
      for (const operand_t &operand : values[i].operands) {
        if (operand.kind == OperandKind::Value) {
          parent[find(i)] = find(operand.value);
        }
      }
    }
    std::map<size_t, std::vector<size_t>> by_root;
    for (size_t i = 0; i < values.size(); i++) {
      if (!merged[i]) {
        by_root[find(i)].push_back(i);
      }
    }
    std::vector<std::vector<size_t>> result;
    for (auto &[root, members] : by_root) {
      result.push_back(std::move(members));
    }
    return result;
  }

private:
  std::unordered_map<std::string, size_t> by_symbol;

  operand_t resolve(klee::ref<klee::Expr> expr, const Call *consumer) {
    if (LibCore::is_constant(expr)) {
      return {OperandKind::Outside, 0, expr};
    }
    std::string symbol;
    if (LibCore::is_readLSB(expr, symbol)) {
      auto found_it = by_symbol.find(symbol);
      if (found_it != by_symbol.end()) {
        return {OperandKind::Value, found_it->second, nullptr};
      }
      return {OperandKind::Outside, 0, expr};
    }
    if (!is_arithmetic_op(expr)) {
      return {OperandKind::Outside, 0, expr};
    }
    // A rotate keeps its argument's operation inline: a value of its own for now, merged with the
    // op node computing the same operation, if its path has one (merge_inline_operations).
    std::vector<operand_t> operands;
    for (unsigned i = 0; i < expr->getNumKids(); i++) {
      operands.push_back(resolve(expr->getKid(i), consumer));
    }
    const size_t index = values.size();
    values.push_back({consumer, unrolled_op_name(expr).value_or("op"), expr->getKind(), expr->getWidth(), expr, true, std::move(operands), {}, 0});
    return {OperandKind::Value, index, nullptr};
  }

  // What an operation computes, from what it reads: equal for two values computing the same
  // operation on the same inputs.
  std::string operation_key(size_t v) const {
    const value_t &value = values[v];
    std::vector<std::string> operands;
    for (const operand_t &operand : value.operands) {
      switch (operand.kind) {
      case OperandKind::Value:
        operands.push_back("v" + std::to_string(operand.value));
        break;
      case OperandKind::Outside:
        operands.push_back(LibCore::expr_to_string(operand.expr, true));
        break;
      }
    }
    if (is_commutative(value.kind)) {
      std::sort(operands.begin(), operands.end());
    }
    std::string key = std::to_string(value.kind) + "/" + std::to_string(value.width);
    for (const std::string &operand : operands) {
      key += "|" + operand;
    }
    return key;
  }

  // An inline operation whose path also computes it in an op node is that op node's value: the
  // node may sit anywhere on the path, the BDD's order is not the program's.
  void merge_inline_operations() {
    merged.assign(values.size(), false);
    bool changed = true;
    while (changed) {
      changed = false;
      std::unordered_map<std::string, std::vector<size_t>> op_nodes;
      for (size_t i = 0; i < values.size(); i++) {
        if (!values[i].is_inline && values[i].fn != "rotate_left") {
          op_nodes[operation_key(i)].push_back(i);
        }
      }
      std::unordered_map<size_t, size_t> redirect;
      for (size_t i = 0; i < values.size(); i++) {
        if (!values[i].is_inline || merged[i]) {
          continue;
        }
        auto found_it = op_nodes.find(operation_key(i));
        if (found_it == op_nodes.end()) {
          continue;
        }
        // An operation on constants and packet bytes alone has the same key on every path: only an
        // op node on the rotate's own path computes its value.
        const BDDNode *rotate = values[i].node;
        for (size_t op : found_it->second) {
          const BDDNode *op_node = values[op].node;
          if (op_node->is_reachable(rotate->get_id()) || rotate->is_reachable(op_node->get_id())) {
            redirect[i] = op;
            merged[i]   = true;
            break;
          }
        }
      }
      for (value_t &value : values) {
        for (operand_t &operand : value.operands) {
          if (operand.kind == OperandKind::Value && redirect.contains(operand.value)) {
            operand.value = redirect.at(operand.value);
            changed       = true;
          }
        }
      }
    }
  }

  size_t compute_depth(size_t v, std::vector<bool> &done) {
    if (done[v]) {
      return values[v].depth;
    }
    size_t depth = 0;
    for (const operand_t &operand : values[v].operands) {
      if (operand.kind == OperandKind::Value) {
        depth = std::max(depth, compute_depth(operand.value, done) + 1);
      }
    }
    values[v].depth = depth;
    done[v]         = true;
    return depth;
  }
};

// Lines iterations up: pairs every value of an iteration (the later) with the value computing
// the same thing in the iteration before (the earlier). A value is inside the loop when it
// depends on one of the anchors, an op every iteration computes once; the rest is outside: what
// the loop starts from, and what steps mix into its state.
//
// The pairs a match needs are solved together, with backtracking: the order of a commutative
// op's operands, and whether an op is a step, are choices that only turn out wrong further on.
class matcher_t {
public:
  std::unordered_map<size_t, size_t> earlier_of; // Later -> earlier.
  std::unordered_map<size_t, size_t> later_of;   // Earlier -> later.
  std::unordered_set<size_t> steps;
  // The deepest comparison that failed, for the trace.
  std::string deepest_failure;

  matcher_t(const graph_t &_graph, const std::vector<size_t> &component, const std::vector<size_t> &anchors)
      : graph(_graph), inside(_graph.values.size(), false), work(0), deepest(0) {
    std::vector<size_t> by_depth = component;
    std::sort(by_depth.begin(), by_depth.end(), [&](size_t a, size_t b) { return graph.values[a].depth < graph.values[b].depth; });
    const std::unordered_set<size_t> anchor_set(anchors.begin(), anchors.end());
    for (size_t v : by_depth) {
      inside[v] = anchor_set.contains(v) ||
                  std::any_of(graph.values[v].operands.begin(), graph.values[v].operands.end(),
                              [&](const operand_t &operand) { return operand.kind == OperandKind::Value && inside[operand.value]; });
    }
  }

  // The comparisons tried so far, backtracking included.
  size_t get_work() const { return work; }

  bool is_inside(size_t v) const { return inside[v]; }

  // An op mixing into one value of the loop something from outside it that is not a constant:
  // a candidate step between iterations.
  bool is_step(size_t v) const {
    const value_t &value = graph.values[v];
    if (value.fn == "rotate_left" || !inside[v]) {
      return false;
    }
    size_t n_inside = 0;
    bool mixes_in   = false;
    for (const operand_t &operand : value.operands) {
      switch (operand.kind) {
      case OperandKind::Value:
        n_inside += inside[operand.value];
        mixes_in |= !inside[operand.value];
        break;
      case OperandKind::Outside:
        mixes_in |= !LibCore::is_constant(operand.expr);
        break;
      }
    }
    return n_inside == 1 && mixes_in;
  }

  // The loop's value a step changes.
  size_t state_operand(size_t step) const {
    for (const operand_t &operand : graph.values[step].operands) {
      if (operand.kind == OperandKind::Value && inside[operand.value]) {
        return operand.value;
      }
    }
    panic("Step %zu changes no value of the loop", step);
  }

  // Whether every (later, earlier) pair in `pairs` computes in its iteration what the other
  // computes in the one before, together. Records the pairs that makes true; nothing when it is
  // not.
  bool match(const std::vector<std::pair<size_t, size_t>> &pairs) {
    std::vector<comparison_t> todo;
    for (auto it = pairs.rbegin(); it != pairs.rend(); it++) {
      todo.push_back({{OperandKind::Value, it->first, nullptr}, {OperandKind::Value, it->second, nullptr}});
    }
    const size_t mark = log.size();
    if (solve(todo)) {
      return true;
    }
    undo(mark);
    return false;
  }

  // Pairs the ops a backward match from the anchors does not reach: those reading, operand by
  // operand, what their counterparts read one iteration earlier. Only already paired values
  // count, so no comparison goes further than one op, and it runs until nothing is added.
  void extend() {
    bool changed = true;
    while (changed) {
      changed = false;
      std::vector<std::pair<size_t, size_t>> pairs(later_of.begin(), later_of.end());
      std::sort(pairs.begin(), pairs.end());
      for (const auto &[y, x] : pairs) {
        std::vector<size_t> candidates = graph.values[x].consumers;
        for (size_t consumer : graph.values[x].consumers) {
          if (is_step(consumer)) {
            const std::vector<size_t> &through = graph.values[consumer].consumers;
            candidates.insert(candidates.end(), through.begin(), through.end());
          }
        }
        for (size_t cy : graph.values[y].consumers) {
          if (later_of.contains(cy) || steps.contains(cy) || is_step(cy)) {
            continue;
          }
          for (size_t cx : candidates) {
            if (!earlier_of.contains(cx) && !later_of.contains(cy) && pair_if_counterparts(cx, cy)) {
              changed = true;
              break;
            }
          }
        }
      }
    }
  }

private:
  struct comparison_t {
    operand_t later;
    operand_t earlier;
  };

  struct log_entry_t {
    bool is_step;
    size_t value;
  };

  const graph_t &graph;
  std::vector<bool> inside;
  size_t work;
  size_t deepest;
  std::vector<log_entry_t> log;

  void fail(size_t depth, const std::string &what) {
    if (depth >= deepest) {
      deepest         = depth;
      deepest_failure = "after " + std::to_string(depth) + " pairs: " + what;
    }
  }

  // The value an operand of the loop reads, past a step: steps mix into the state between
  // iterations, what they change is the value lined up. `through` gets the step, if any.
  size_t past_step(size_t v, std::optional<size_t> &through) const {
    if (!earlier_of.contains(v) && !later_of.contains(v) && is_step(v)) {
      through = v;
      return state_operand(v);
    }
    return v;
  }

  // Whether `ox` reads in its iteration what `oy` reads in the one before, by the pairs made so
  // far; the steps read through are added to `through`.
  bool reads_counterpart(const operand_t &ox, const operand_t &oy, std::vector<size_t> &through) const {
    const bool x_inside = ox.kind == OperandKind::Value && inside[ox.value];
    const bool y_inside = oy.kind == OperandKind::Value && inside[oy.value];
    if (x_inside && y_inside) {
      std::optional<size_t> x_step;
      std::optional<size_t> y_step;
      const size_t x    = past_step(ox.value, x_step);
      const size_t y    = past_step(oy.value, y_step);
      auto found_it     = earlier_of.find(x);
      const bool paired = found_it != earlier_of.end() && found_it->second == y;
      if (paired) {
        for (const std::optional<size_t> &step : {x_step, y_step}) {
          if (step) {
            through.push_back(*step);
          }
        }
      }
      return paired;
    }
    if (x_inside) {
      return true; // The earlier iteration read the loop's start.
    }
    if (y_inside) {
      return false;
    }
    return ox.kind == oy.kind && (ox.kind == OperandKind::Value ? ox.value == oy.value : same_expr(ox.expr, oy.expr));
  }

  // Pairs `x` with `y` when they are the same op reading counterparts, in either operand order
  // for a commutative one.
  bool pair_if_counterparts(size_t x, size_t y) {
    const value_t &vx = graph.values[x];
    const value_t &vy = graph.values[y];
    if (x == y || graph.label(x) != graph.label(y) || vx.operands.size() != vy.operands.size()) {
      return false;
    }
    const bool commutative = vx.operands.size() == 2 && is_commutative(vx.kind);
    for (const bool swapped : {false, true}) {
      if (swapped && !commutative) {
        break;
      }
      std::vector<size_t> through;
      bool counterparts = true;
      for (size_t i = 0; i < vx.operands.size() && counterparts; i++) {
        counterparts = reads_counterpart(vx.operands[i], vy.operands[swapped ? vx.operands.size() - 1 - i : i], through);
      }
      if (counterparts) {
        pair(x, y);
        for (size_t s : through) {
          if (!steps.contains(s)) {
            step(s);
          }
        }
        return true;
      }
    }
    return false;
  }

  void pair(size_t x, size_t y) {
    earlier_of[x] = y;
    later_of[y]   = x;
    log.push_back({false, x});
  }

  void step(size_t s) {
    steps.insert(s);
    log.push_back({true, s});
  }

  void undo(size_t mark) {
    while (log.size() > mark) {
      const log_entry_t entry = log.back();
      log.pop_back();
      if (entry.is_step) {
        steps.erase(entry.value);
      } else {
        later_of.erase(earlier_of.at(entry.value));
        earlier_of.erase(entry.value);
      }
    }
  }

  // Solves the comparisons on `todo` (a stack), and on success leaves their pairs recorded.
  // `todo` comes back as it was either way.
  bool solve(std::vector<comparison_t> &todo) {
    work++;
    if (todo.empty()) {
      return true;
    }
    const comparison_t comparison = todo.back();
    todo.pop_back();
    const bool solved = solve_one(comparison, todo);
    todo.push_back(comparison);
    return solved;
  }

  // Tries each way `comparison` can hold, each followed by everything still to solve.
  bool solve_one(const comparison_t &comparison, std::vector<comparison_t> &todo) {
    const operand_t &ox = comparison.later;
    const operand_t &oy = comparison.earlier;

    // What the earlier iteration reads from outside the loop where the later one reads a loop
    // value is the loop's start: symbolic execution evaluated the first iteration's inputs, and
    // rearranged what it could. A value computed there still lines up with the later iteration's
    // when it computes the same thing (the first iterations no anchor reaches); only when it does
    // not is it merely the start.
    const bool x_inside = ox.kind == OperandKind::Value && inside[ox.value];
    const bool y_inside = oy.kind == OperandKind::Value && inside[oy.value];
    const bool entry    = x_inside && !y_inside;
    if (entry && oy.kind == OperandKind::Outside) {
      return solve(todo);
    }
    if (!x_inside && y_inside) {
      fail(earlier_of.size(), "outside operand vs " + graph.describe(oy.value));
      return false;
    }
    if (!x_inside) {
      // Both from outside: the same thing, read by every iteration.
      const bool same = ox.kind == oy.kind && (ox.kind == OperandKind::Value ? ox.value == oy.value : same_expr(ox.expr, oy.expr));
      if (!same) {
        fail(earlier_of.size(), "different outside operands");
        return false;
      }
      return solve(todo);
    }

    const size_t x = ox.value;
    const size_t y = oy.value;

    // A step already skipped stays skipped: the value it changes is the one to line up.
    for (const bool later_side : {true, false}) {
      const size_t candidate = later_side ? x : y;
      if (steps.contains(candidate)) {
        const operand_t through{OperandKind::Value, state_operand(candidate), nullptr};
        todo.push_back(later_side ? comparison_t{through, oy} : comparison_t{ox, through});
        const bool solved = solve(todo);
        todo.pop_back();
        return solved;
      }
    }

    const bool x_paired = earlier_of.contains(x);
    const bool y_paired = later_of.contains(y);
    if (x_paired && earlier_of.at(x) == y) {
      return solve(todo);
    }

    const value_t &vx = graph.values[x];
    const value_t &vy = graph.values[y];
    if (!x_paired && !y_paired && x != y && graph.label(x) == graph.label(y) && vx.operands.size() == vy.operands.size()) {
      const bool commutative = vx.operands.size() == 2 && is_commutative(vx.kind);
      for (const bool swapped : {false, true}) {
        if (swapped && !commutative) {
          break;
        }
        const size_t mark = log.size();
        pair(x, y);
        for (size_t i = vx.operands.size(); i-- > 0;) {
          todo.push_back({vx.operands[i], vy.operands[swapped ? vx.operands.size() - 1 - i : i]});
        }
        if (solve(todo)) {
          return true;
        }
        todo.resize(todo.size() - vx.operands.size());
        undo(mark);
      }
    }

    // A step mixes something into the state between two iterations: the value it changes is the
    // one to line up.
    for (const bool later_side : {true, false}) {
      const size_t candidate = later_side ? x : y;
      if (!is_step(candidate) || earlier_of.contains(candidate) || later_of.contains(candidate)) {
        continue;
      }
      const size_t mark = log.size();
      step(candidate);
      const operand_t through{OperandKind::Value, state_operand(candidate), nullptr};
      todo.push_back(later_side ? comparison_t{through, oy} : comparison_t{ox, through});
      if (solve(todo)) {
        return true;
      }
      todo.pop_back();
      undo(mark);
    }

    if (entry) {
      return solve(todo);
    }
    fail(earlier_of.size(), graph.describe(x) + " vs " + graph.describe(y));
    return false;
  }
};

// The loop a matcher lined up, if its pairs make one: every value in exactly one iteration, and
// every op reading its own iteration's values or the previous iteration's. `why` says what
// broke otherwise.
std::optional<loop_t> build_loop(const graph_t &graph, const matcher_t &matcher, size_t anchor, std::string &why) {
  const auto node_of = [&](size_t v) { return std::to_string(graph.values[v].node->get_id()); };

  std::vector<size_t> members;
  for (const auto &[later, earlier] : matcher.earlier_of) {
    members.push_back(later);
    members.push_back(earlier);
  }
  std::sort(members.begin(), members.end());
  members.erase(std::unique(members.begin(), members.end()), members.end());

  // A chain is one body op through the iterations, from its last iteration down.
  std::vector<std::vector<size_t>> chains;
  for (size_t v : members) {
    if (matcher.later_of.contains(v)) {
      continue;
    }
    std::vector<size_t> chain{v};
    for (auto found_it = matcher.earlier_of.find(v); found_it != matcher.earlier_of.end(); found_it = matcher.earlier_of.find(found_it->second)) {
      chain.push_back(found_it->second);
    }
    chains.push_back(std::move(chain));
  }

  size_t n_iterations = 0;
  for (const std::vector<size_t> &chain : chains) {
    if (std::find(chain.begin(), chain.end(), anchor) != chain.end()) {
      n_iterations = chain.size();
    }
  }
  if (n_iterations < MIN_ITERATIONS) {
    why = "the anchor spans " + std::to_string(n_iterations) + " iterations";
    return {};
  }

  // Body ops in data-flow order.
  std::sort(chains.begin(), chains.end(), [&](const std::vector<size_t> &a, const std::vector<size_t> &b) {
    const value_t &va = graph.values[a.front()];
    const value_t &vb = graph.values[b.front()];
    if (va.depth != vb.depth) {
      return va.depth < vb.depth;
    }
    return va.node->get_id() < vb.node->get_id();
  });

  std::unordered_map<size_t, size_t> iteration_of;
  std::unordered_map<size_t, size_t> body_op_of;
  for (size_t c = 0; c < chains.size(); c++) {
    if (chains[c].size() > n_iterations) {
      // Lined up past the first iteration: what the loop starts from.
      chains[c].resize(n_iterations);
    }
    for (size_t k = 0; k < chains[c].size(); k++) {
      iteration_of[chains[c][k]] = n_iterations - 1 - k;
      body_op_of[chains[c][k]]   = c;
    }
  }

  loop_t loop;
  loop.iterations.assign(n_iterations, std::vector<std::optional<bdd_node_id_t>>(chains.size()));
  for (size_t c = 0; c < chains.size(); c++) {
    for (size_t v : chains[c]) {
      loop.iterations[iteration_of.at(v)][c] = graph.values[v].node->get_id();
    }
  }

  std::unordered_set<bdd_node_id_t> entry;
  for (const auto &[v, iteration] : iteration_of) {
    for (const operand_t &operand : graph.values[v].operands) {
      if (operand.kind != OperandKind::Value) {
        continue;
      }
      size_t read = operand.value;
      if (matcher.steps.contains(read)) {
        read = matcher.state_operand(read);
        if (!iteration_of.contains(read) || iteration_of.at(read) + 1 != iteration) {
          why = "node " + node_of(v) + " (iteration " + std::to_string(iteration) + ") reads through step " + node_of(operand.value) +
                " a value not of the previous iteration";
          return {};
        }
        continue;
      }
      if (iteration_of.contains(read)) {
        const size_t read_iteration = iteration_of.at(read);
        if (read_iteration != iteration && read_iteration + 1 != iteration) {
          why = "node " + node_of(v) + " (iteration " + std::to_string(iteration) + ") reads node " + node_of(read) + " of iteration " +
                std::to_string(read_iteration);
          return {};
        }
        continue;
      }
      if (matcher.is_inside(read)) {
        why = "node " + node_of(v) + " (iteration " + std::to_string(iteration) + ") reads node " + node_of(read) +
              ", a value of the loop no iteration lines up";
        return {};
      }
      if (iteration == 0) {
        entry.insert(graph.values[read].node->get_id());
      }
    }
  }
  loop.prefix.assign(entry.begin(), entry.end());
  std::sort(loop.prefix.begin(), loop.prefix.end());

  // The body as the last iteration computes it: the one no folding touched.
  std::unordered_set<size_t> state;
  for (size_t c = 0; c < chains.size(); c++) {
    const size_t v = chains[c].front();
    if (iteration_of.at(v) != n_iterations - 1) {
      why = "node " + node_of(v) + " is a body op the last iteration does not compute";
      return {};
    }
    loop_body_op_t op{graph.values[v].fn, graph.values[v].width, {}};
    for (const operand_t &operand : graph.values[v].operands) {
      switch (operand.kind) {
      case OperandKind::Value: {
        size_t read = operand.value;
        if (matcher.steps.contains(read)) {
          read = matcher.state_operand(read);
        }
        if (!body_op_of.contains(read)) {
          op.operands.push_back({LoopOperandKind::Outside, 0, graph.values[read].expr});
        } else if (iteration_of.at(read) == n_iterations - 1) {
          op.operands.push_back({LoopOperandKind::Body, body_op_of.at(read), nullptr});
        } else {
          op.operands.push_back({LoopOperandKind::State, body_op_of.at(read), nullptr});
          state.insert(body_op_of.at(read));
        }
      } break;
      case OperandKind::Outside:
        op.operands.push_back({LoopOperandKind::Outside, 0, operand.expr});
        break;
      }
    }
    loop.body.push_back(std::move(op));
  }
  loop.state.assign(state.begin(), state.end());
  std::sort(loop.state.begin(), loop.state.end());

  for (size_t s : matcher.steps) {
    const size_t read = matcher.state_operand(s);
    if (iteration_of.contains(read)) {
      loop.steps.push_back({graph.values[s].node->get_id(), iteration_of.at(read), body_op_of.at(read)});
    }
  }
  std::sort(loop.steps.begin(), loop.steps.end(),
            [](const loop_step_t &a, const loop_step_t &b) { return std::tie(a.after_iteration, a.node) < std::tie(b.after_iteration, b.node); });

  return loop;
}

// The first iterations symbolic execution left irregular -- body ops folded into constants, the
// rest rearranged around them -- belong to the prefix, with the steps before the first full
// iteration: the iterations proper start at the first one computing every body op.
void move_irregular_iterations_to_prefix(loop_t &loop) {
  size_t first_full = 0;
  while (first_full < loop.iterations.size() && std::any_of(loop.iterations[first_full].begin(), loop.iterations[first_full].end(),
                                                            [](const std::optional<bdd_node_id_t> &node) { return !node.has_value(); })) {
    first_full++;
  }
  if (first_full == 0 || first_full == loop.iterations.size()) {
    return;
  }
  for (size_t k = 0; k < first_full; k++) {
    for (const std::optional<bdd_node_id_t> &node : loop.iterations[k]) {
      if (node) {
        loop.prefix.push_back(*node);
      }
    }
  }
  loop.iterations.erase(loop.iterations.begin(), loop.iterations.begin() + first_full);
  std::vector<loop_step_t> steps;
  for (loop_step_t step : loop.steps) {
    if (step.after_iteration < first_full) {
      loop.prefix.push_back(step.node);
    } else {
      step.after_iteration -= first_full;
      steps.push_back(step);
    }
  }
  loop.steps = std::move(steps);
  std::sort(loop.prefix.begin(), loop.prefix.end());
  loop.prefix.erase(std::unique(loop.prefix.begin(), loop.prefix.end()), loop.prefix.end());
}

size_t count_ops(const loop_t &loop) {
  size_t n = 0;
  for (const std::vector<std::optional<bdd_node_id_t>> &iteration : loop.iterations) {
    n += std::count_if(iteration.begin(), iteration.end(), [](const std::optional<bdd_node_id_t> &node) { return node.has_value(); });
  }
  return n;
}

} // namespace

std::vector<loop_t> BDD::detect_loops(std::ostream *trace) const {
  const graph_t graph(*this);
  const std::vector<std::vector<size_t>> components = graph.components();
  std::vector<loop_t> loops;

  if (trace) {
    *trace << "[loops] " << graph.values.size() - graph.count_merged() << " computed values (" << graph.count_merged()
           << " inline operations merged into their op nodes) in " << components.size() << " data-flow components\n";
  }

  for (const std::vector<size_t> &component : components) {
    // An anchor is an op every iteration computes once: try each label repeated often enough,
    // its occurrences in data-flow order as consecutive iterations, and keep the candidate that
    // lines up the most ops.
    std::map<std::string, std::vector<size_t>> by_label;
    for (size_t v : component) {
      by_label[graph.label(v)].push_back(v);
    }
    if (trace && component.size() >= MIN_ITERATIONS) {
      *trace << "[loops] component of " << component.size() << " values, first node " << graph.values[component.front()].node->get_id() << "\n";
    }

    std::optional<loop_t> best;
    std::vector<size_t> joint_anchors;
    std::vector<std::pair<size_t, size_t>> joint_seeds;
    size_t joint_last = 0;
    size_t n_lined_up = 0;
    for (auto &[label, occurrences] : by_label) {
      if (occurrences.size() < MIN_ITERATIONS) {
        continue;
      }
      std::sort(occurrences.begin(), occurrences.end(), [&](size_t a, size_t b) { return graph.values[a].depth < graph.values[b].depth; });
      bool increasing = true;
      for (size_t i = 1; i < occurrences.size(); i++) {
        increasing &= graph.values[occurrences[i - 1]].depth < graph.values[occurrences[i]].depth;
      }
      if (!increasing) {
        if (trace) {
          *trace << "[loops]   " << label << " x" << occurrences.size() << ": two at the same depth, not one per iteration\n";
        }
        continue;
      }

      // Every anchor lines up with the one before it, all at once.
      matcher_t matcher(graph, component, occurrences);
      std::vector<std::pair<size_t, size_t>> seeds;
      for (size_t i = occurrences.size() - 1; i > 0; i--) {
        seeds.push_back({occurrences[i], occurrences[i - 1]});
      }
      if (!matcher.match(seeds)) {
        if (trace) {
          *trace << "[loops]   " << label << " x" << occurrences.size() << ": "
                 << "the anchors do not line up after " << matcher.get_work() << " comparisons; deepest failure " << matcher.deepest_failure << "\n";
        }
        continue;
      }
      const size_t backward = matcher.earlier_of.size();
      if (trace) {
        *trace << "[loops]   " << label << " x" << occurrences.size() << ": the anchors line up after " << matcher.get_work() << " comparisons\n";
      }
      matcher.extend();

      std::string why;
      std::optional<loop_t> loop = build_loop(graph, matcher, occurrences.back(), why);
      if (trace) {
        *trace << "[loops]   " << label << " x" << occurrences.size() << ": " << backward << " pairs backwards, " << matcher.earlier_of.size()
               << " after extending, " << matcher.steps.size() << " steps, " << matcher.get_work() << " comparisons; "
               << (loop ? std::to_string(loop->iterations.size()) + " iterations of " + std::to_string(loop->body.size()) + " ops" : why) << "\n";
      }
      if (loop && (!best || count_ops(*loop) > count_ops(*best))) {
        best = std::move(loop);
      }
      if (loop) {
        joint_anchors.insert(joint_anchors.end(), occurrences.begin(), occurrences.end());
        joint_seeds.insert(joint_seeds.end(), seeds.begin(), seeds.end());
        joint_last = occurrences.back();
        n_lined_up++;
      }
    }

    // Each anchor lines up what depends on it; the anchors that work alone, together, line up the
    // most: the first iterations, which symbolic execution folded, are what no single anchor reaches.
    if (n_lined_up > 1) {
      matcher_t matcher(graph, component, joint_anchors);
      if (matcher.match(joint_seeds)) {
        matcher.extend();
        std::string why;
        std::optional<loop_t> loop = build_loop(graph, matcher, joint_last, why);
        if (trace) {
          *trace << "[loops]   " << n_lined_up << " anchors together, " << matcher.get_work()
                 << " comparisons: " << (loop ? std::to_string(count_ops(*loop)) + " ops lined up" : why) << "\n";
        }
        if (loop && (!best || count_ops(*loop) > count_ops(*best))) {
          best = std::move(loop);
        }
      } else if (trace) {
        *trace << "[loops]   " << n_lined_up << " anchors together do not line up; deepest failure " << matcher.deepest_failure << "\n";
      }
    }

    if (best) {
      move_irregular_iterations_to_prefix(*best);
      loops.push_back(std::move(*best));
    }
  }

  return loops;
}

namespace {

bool is_compute_call(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }
  const call_t &call = dynamic_cast<const Call *>(node)->get_call();
  return is_unrolled_op(call) || call.function_name == "rotate_left";
}

std::unordered_set<std::string> produced_names(const BDDNode *node) {
  std::unordered_set<std::string> names;
  if (node->get_type() == BDDNodeType::Call) {
    for (const symbol_t &symbol : dynamic_cast<const Call *>(node)->get_local_symbols().get()) {
      names.insert(symbol.name);
    }
  }
  return names;
}

// The straight run of calls `node` belongs to: from the node after the closest branch above it
// (or the root) down to the node before the next branch or route.
std::vector<BDDNode *> straight_run(BDDNode *node) {
  BDDNode *start = node;
  while (start->get_mutable_prev() && start->get_mutable_prev()->get_type() == BDDNodeType::Call) {
    start = start->get_mutable_prev();
  }
  std::vector<BDDNode *> run;
  for (BDDNode *n = start; n && n->get_type() == BDDNodeType::Call; n = n->get_mutable_next()) {
    run.push_back(n);
  }
  return run;
}

// Rearranges `span`, contiguous calls, into the order ready nodes are taken by (rank, position),
// under data flow and the original order of the calls that are not computations; then relinks.
void sort_span(BDD &bdd, const std::vector<BDDNode *> &span, const std::unordered_map<bdd_node_id_t, size_t> &ranks) {
  const size_t n = span.size();
  if (n < 2) {
    return;
  }

  std::unordered_map<std::string, size_t> producer;
  for (size_t i = 0; i < n; i++) {
    for (const std::string &name : produced_names(span[i])) {
      producer[name] = i;
    }
  }
  std::vector<std::vector<size_t>> successors(n);
  std::vector<size_t> n_predecessors(n, 0);
  const auto add_edge = [&](size_t from, size_t to) {
    successors[from].push_back(to);
    n_predecessors[to]++;
  };
  std::vector<std::vector<size_t>> readers(n);
  for (size_t i = 0; i < n; i++) {
    std::unordered_set<size_t> from;
    for (const symbol_t &symbol : span[i]->get_used_symbols().get()) {
      auto found_it = producer.find(symbol.name);
      if (found_it != producer.end() && found_it->second != i) {
        from.insert(found_it->second);
      }
    }
    for (size_t j : from) {
      add_edge(j, i);
      readers[j].push_back(i);
    }
  }
  // Calls that are not computations may do more than return values: they keep their order.
  std::optional<size_t> previous_call;
  for (size_t i = 0; i < n; i++) {
    if (!is_compute_call(span[i])) {
      if (previous_call) {
        add_edge(*previous_call, i);
      }
      previous_call = i;
    }
  }

  // A node of no loop goes just before its first reader that has a rank; one read by none stays
  // with the loop node it followed.
  std::vector<size_t> rank(n, 0);
  std::vector<bool> ranked(n, false);
  for (size_t i = 0; i < n; i++) {
    auto found_it = ranks.find(span[i]->get_id());
    if (found_it != ranks.end()) {
      rank[i]   = found_it->second;
      ranked[i] = true;
    }
  }
  std::vector<size_t> following(n, 0);
  size_t last_rank = 0;
  for (size_t i = 0; i < n; i++) {
    last_rank    = ranked[i] ? rank[i] : last_rank;
    following[i] = last_rank;
  }
  std::vector<bool> by_readers(n, false);
  for (size_t i = n; i-- > 0;) {
    if (ranked[i]) {
      continue;
    }
    for (size_t r : readers[i]) {
      if (ranked[r] || by_readers[r]) {
        rank[i]       = by_readers[i] ? std::min(rank[i], rank[r]) : rank[r];
        by_readers[i] = true;
      }
    }
    if (!by_readers[i]) {
      rank[i] = following[i];
    }
  }

  using entry_t = std::pair<std::pair<size_t, size_t>, size_t>;
  std::priority_queue<entry_t, std::vector<entry_t>, std::greater<entry_t>> ready;
  for (size_t i = 0; i < n; i++) {
    if (n_predecessors[i] == 0) {
      ready.push({{rank[i], i}, i});
    }
  }
  std::vector<BDDNode *> order;
  while (!ready.empty()) {
    const size_t i = ready.top().second;
    ready.pop();
    order.push_back(span[i]);
    for (size_t s : successors[i]) {
      if (--n_predecessors[s] == 0) {
        ready.push({{rank[s], s}, s});
      }
    }
  }
  assert_or_panic(order.size() == n, "The nodes around a loop have a cyclic data flow");
  if (order == span) {
    return;
  }

  BDDNode *before = span.front()->get_mutable_prev();
  BDDNode *after  = span.back()->get_mutable_next();
  for (size_t i = 0; i < n; i++) {
    order[i]->set_prev(i ? order[i - 1] : before);
    order[i]->set_next(i + 1 < n ? order[i + 1] : after);
  }
  if (!before) {
    bdd.set_root(order.front());
  } else if (before->get_type() == BDDNodeType::Branch) {
    Branch *branch = dynamic_cast<Branch *>(before);
    if (branch->get_on_true() == span.front()) {
      branch->set_on_true(order.front());
    } else {
      branch->set_on_false(order.front());
    }
  } else {
    before->set_next(order.front());
  }
  if (after) {
    after->set_prev(order.back());
  }
}

} // namespace

void BDD::group_loop_iterations(const std::vector<loop_t> &loops) {
  // Looked up, not created: get_reordering_barrier_symbol() would add it to a BDD that has none.
  const std::string barrier_name = "__BDD_REORDERING_BARRIER__";
  const std::string barrier      = symbol_manager->has_symbol(barrier_name) ? barrier_name : "";

  for (const loop_t &loop : loops) {
    // The prefix ranks 0, iteration k 2k + 2, a step after it 2k + 3.
    std::unordered_map<bdd_node_id_t, size_t> ranks;
    for (bdd_node_id_t node : loop.prefix) {
      ranks[node] = 0;
    }
    for (size_t k = 0; k < loop.iterations.size(); k++) {
      for (const std::optional<bdd_node_id_t> &node : loop.iterations[k]) {
        if (node) {
          ranks[*node] = 2 * k + 2;
        }
      }
    }
    for (const loop_step_t &step : loop.steps) {
      ranks[step.node] = 2 * step.after_iteration + 3;
    }
    std::unordered_set<bdd_node_id_t> touched;
    for (const auto &[node, rank] : ranks) {
      touched.insert(node);
    }

    // Each straight run holding nodes of the loop, from the first such node to the last, split
    // where a node generates the reordering barrier.
    std::unordered_set<bdd_node_id_t> done;
    for (bdd_node_id_t touched_id : touched) {
      if (done.contains(touched_id)) {
        continue;
      }
      const std::vector<BDDNode *> run = straight_run(get_mutable_node_by_id(touched_id));
      size_t first                     = run.size();
      size_t last                      = 0;
      for (size_t i = 0; i < run.size(); i++) {
        if (touched.contains(run[i]->get_id())) {
          done.insert(run[i]->get_id());
          first = std::min(first, i);
          last  = i;
        }
      }
      std::vector<BDDNode *> span;
      for (size_t i = first; i <= last; i++) {
        if (produced_names(run[i]).contains(barrier)) {
          sort_span(*this, span, ranks);
          span.clear();
          continue;
        }
        span.push_back(run[i]);
      }
      sort_span(*this, span, ranks);
    }
  }
}

bool same_body(const loop_t &a, const loop_t &b) {
  if (a.body.size() != b.body.size() || a.state != b.state) {
    return false;
  }
  for (size_t i = 0; i < a.body.size(); i++) {
    const loop_body_op_t &oa = a.body[i];
    const loop_body_op_t &ob = b.body[i];
    if (oa.fn != ob.fn || oa.width != ob.width || oa.operands.size() != ob.operands.size()) {
      return false;
    }
    for (size_t j = 0; j < oa.operands.size(); j++) {
      const loop_operand_t &pa = oa.operands[j];
      const loop_operand_t &pb = ob.operands[j];
      if (pa.kind != pb.kind) {
        return false;
      }
      switch (pa.kind) {
      case LoopOperandKind::Body:
      case LoopOperandKind::State:
        if (pa.index != pb.index) {
          return false;
        }
        break;
      case LoopOperandKind::Outside:
        if (pa.expr.isNull() != pb.expr.isNull() || (!pa.expr.isNull() && !same_expr(pa.expr, pb.expr))) {
          return false;
        }
        break;
      }
    }
  }
  return true;
}

} // namespace LibBDD
