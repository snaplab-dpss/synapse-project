#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

// vector_inc_or_swap done by the controller on the data plane's pair register (an offloaded
// packet past the stage): read the cell, decide, write it back.
class DataplaneVectorIncOrSwap : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> pair_in;
  klee::ref<klee::Expr> pair_out;
  bits_t key_size;
  bool evict;

public:
  DataplaneVectorIncOrSwap(const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _index, klee::ref<klee::Expr> _pair_in,
                           klee::ref<klee::Expr> _pair_out, bits_t _key_size, bool _evict)
      : ControllerModule(ModuleType::Controller_DataplaneVectorIncOrSwap, "DataplaneVectorIncOrSwap", _node), obj(_obj), index(_index),
        pair_in(_pair_in), pair_out(_pair_out), key_size(_key_size), evict(_evict) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new DataplaneVectorIncOrSwap(node, obj, index, pair_in, pair_out, key_size, evict); }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_pair_in() const { return pair_in; }
  klee::ref<klee::Expr> get_pair_out() const { return pair_out; }
  bits_t get_key_size() const { return key_size; }
  bool get_evict() const { return evict; }
};

class DataplaneVectorIncOrSwapFactory : public ControllerModuleFactory {
public:
  DataplaneVectorIncOrSwapFactory() : ControllerModuleFactory(ModuleType::Controller_DataplaneVectorIncOrSwap, "DataplaneVectorIncOrSwap") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
