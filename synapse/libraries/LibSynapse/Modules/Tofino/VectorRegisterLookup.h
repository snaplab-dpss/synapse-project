#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class VectorRegisterLookup : public TofinoModule {
private:
  DS_ID id;
  addr_t obj;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> value;
  bool can_be_inlined;

public:
  VectorRegisterLookup(const BDDNode *_node, DS_ID _id, addr_t _obj, klee::ref<klee::Expr> _index, klee::ref<klee::Expr> _value, bool _can_be_inlined)
      : TofinoModule(ModuleType::Tofino_VectorRegisterLookup, "VectorRegisterLookup", _node), id(_id), obj(_obj), index(_index), value(_value),
        can_be_inlined(_can_be_inlined) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new VectorRegisterLookup(node, id, obj, index, value, can_be_inlined);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_value() const { return value; }
  bool get_can_be_inlined() const { return can_be_inlined; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {id}; }
};

class VectorRegisterLookupFactory : public TofinoModuleFactory {
public:
  VectorRegisterLookupFactory() : TofinoModuleFactory(ModuleType::Tofino_VectorRegisterLookup, "VectorRegisterLookup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse