#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class VectorTableLookup : public TofinoModule {
private:
  DS_ID id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;

public:
  VectorTableLookup(const InstanceId _instance_id, const BDDNode *_node, DS_ID _id, addr_t _obj, klee::ref<klee::Expr> _key,
                    klee::ref<klee::Expr> _value)
      : TofinoModule(ModuleType(ModuleCategory::Tofino_VectorTableLookup, _instance_id), "VectorTableLookup", _node), id(_id), obj(_obj), key(_key),
        value(_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new VectorTableLookup(get_type().instance_id, node, id, obj, key, value);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_value() const { return value; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {id}; }
};

class VectorTableLookupFactory : public TofinoModuleFactory {
public:
  VectorTableLookupFactory(const InstanceId _instance_id)
      : TofinoModuleFactory(ModuleType(ModuleCategory::Tofino_VectorTableLookup, _instance_id), "VectorTableLookup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
