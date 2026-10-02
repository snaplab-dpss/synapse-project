#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

// vector_inc_or_swap on the controller's own (libnf) vector.
class VectorIncOrSwap : public ControllerModule {
private:
  addr_t vector_addr;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> pair_in;
  klee::ref<klee::Expr> pair_out;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> value_size;
  klee::ref<klee::Expr> evict;

public:
  VectorIncOrSwap(const BDDNode *_node, addr_t _vector_addr, klee::ref<klee::Expr> _index, klee::ref<klee::Expr> _pair_in,
                  klee::ref<klee::Expr> _pair_out, klee::ref<klee::Expr> _key_size, klee::ref<klee::Expr> _value_size, klee::ref<klee::Expr> _evict)
      : ControllerModule(ModuleType::Controller_VectorIncOrSwap, "VectorIncOrSwap", _node), vector_addr(_vector_addr), index(_index),
        pair_in(_pair_in), pair_out(_pair_out), key_size(_key_size), value_size(_value_size), evict(_evict) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new VectorIncOrSwap(node, vector_addr, index, pair_in, pair_out, key_size, value_size, evict); }

  addr_t get_vector_addr() const { return vector_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_pair_in() const { return pair_in; }
  klee::ref<klee::Expr> get_pair_out() const { return pair_out; }
  klee::ref<klee::Expr> get_key_size() const { return key_size; }
  klee::ref<klee::Expr> get_value_size() const { return value_size; }
  klee::ref<klee::Expr> get_evict() const { return evict; }
};

class VectorIncOrSwapFactory : public ControllerModuleFactory {
public:
  VectorIncOrSwapFactory() : ControllerModuleFactory(ModuleType::Controller_VectorIncOrSwap, "VectorIncOrSwap") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
