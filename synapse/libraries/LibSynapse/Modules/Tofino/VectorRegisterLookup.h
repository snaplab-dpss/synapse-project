#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class VectorRegisterLookup : public TofinoModule {
private:
  std::unordered_set<DS_ID> rids;
  addr_t obj;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> value;

public:
  VectorRegisterLookup(const LibBDD::Node *node, const std::unordered_set<DS_ID> &_rids, addr_t _obj, klee::ref<klee::Expr> _index,
                       klee::ref<klee::Expr> _value)
      : TofinoModule(ModuleType::Tofino_VectorRegisterLookup, "VectorRegisterLookup", node), rids(_rids), obj(_obj), index(_index),
        value(_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new VectorRegisterLookup(node, rids, obj, index, value);
    return cloned;
  }

  const std::unordered_set<DS_ID> &get_rids() const { return rids; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_value() const { return value; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return rids; }
};

class VectorRegisterLookupFactory : public TofinoModuleFactory {
public:
  VectorRegisterLookupFactory() : TofinoModuleFactory(ModuleType::Tofino_VectorRegisterLookup, "VectorRegisterLookup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
};

} // namespace Tofino
} // namespace LibSynapse