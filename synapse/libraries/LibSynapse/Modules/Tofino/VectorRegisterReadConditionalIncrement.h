#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

// Read-conditional-increment on a vector register whose branch condition is EXTERNAL
// (not derived from the register's own value) and whose "write" is an increment of the
// register value (value + delta), e.g. HLL's `if (shadow == 0) *nonzero += 1`.
//
// A Tofino register allows only one access per packet, and here the read value (the
// counter) is needed on BOTH branches (the post-op count is compared on each), so it
// can't be split read-on-one-branch / write-on-other (VectorRegisterReadConditionalUpdate).
// Instead this lowers to a single register action `out = value; if (cond) value += delta`
// -- which reads, conditionally increments, and returns the OLD value -- matching the
// expert's counter register action. The old value is bound to metadata so both branch
// continuations can use it (`count` on the no-increment side, `count = old + delta` on
// the increment side); the branch itself stays as an ordinary gateway on `cond`.
//
// The estimator max-swap (condition derived from the register's own value, returns the
// displaced/shadow value) is handled separately by
// VectorRegisterReadConditionalUpdateSingleAction; the two are mutually exclusive on
// whether the branch condition reads the register's own value.
class VectorRegisterReadConditionalIncrement : public TofinoModule {
private:
  DS_ID id;
  addr_t obj;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> read_value;
  klee::ref<klee::Expr> write_value;
  klee::ref<klee::Expr> condition;

public:
  VectorRegisterReadConditionalIncrement(const BDDNode *_node, DS_ID _id, addr_t _obj, klee::ref<klee::Expr> _index,
                                         klee::ref<klee::Expr> _read_value, klee::ref<klee::Expr> _write_value, klee::ref<klee::Expr> _condition)
      : TofinoModule(ModuleType::Tofino_VectorRegisterReadConditionalIncrement, "VectorRegisterReadConditionalIncrement", _node), id(_id), obj(_obj),
        index(_index), read_value(_read_value), write_value(_write_value), condition(_condition) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    return new VectorRegisterReadConditionalIncrement(node, id, obj, index, read_value, write_value, condition);
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_read_value() const { return read_value; }
  klee::ref<klee::Expr> get_write_value() const { return write_value; }
  klee::ref<klee::Expr> get_condition() const { return condition; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {id}; }
};

class VectorRegisterReadConditionalIncrementFactory : public TofinoModuleFactory {
public:
  VectorRegisterReadConditionalIncrementFactory()
      : TofinoModuleFactory(ModuleType::Tofino_VectorRegisterReadConditionalIncrement, "VectorRegisterReadConditionalIncrement") {}

  // Pattern-level check only (no data-structure placement / usage checks): would this
  // module claim `node` (a vector_borrow with a conditional write)? Used by
  // VectorRegisterLookup to decide whether to leave such a borrow to this module.
  static bool matches_pattern(const EP *ep, const BDDNode *node);

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
