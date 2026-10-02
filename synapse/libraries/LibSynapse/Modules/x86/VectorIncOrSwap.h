#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class VectorIncOrSwap : public x86Module {
private:
  addr_t obj;

public:
  VectorIncOrSwap(const BDDNode *_node, addr_t _obj) : x86Module(ModuleType::x86_VectorIncOrSwap, "VectorIncOrSwap", _node), obj(_obj) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new VectorIncOrSwap(node, obj); }

  addr_t get_obj() const { return obj; }
};

class VectorIncOrSwapFactory : public x86ModuleFactory {
public:
  VectorIncOrSwapFactory() : x86ModuleFactory(ModuleType::x86_VectorIncOrSwap, "VectorIncOrSwap") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse
