#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class CMSCountMin : public x86Module {
private:
  addr_t cms_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> min_estimate;

public:
  CMSCountMin(ModuleType _type, const BDDNode *_node, addr_t _cms_addr, klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _min_estimate)
      : x86Module(_type, "CMSCountMin", _node), cms_addr(_cms_addr), key(_key), min_estimate(_min_estimate) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new CMSCountMin(type, node, cms_addr, key, min_estimate);
    return cloned;
  }

  addr_t get_cms_addr() const { return cms_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_min_estimate() const { return min_estimate; }
};

class CMSCountMinFactory : public x86ModuleFactory {
public:
  CMSCountMinFactory(const std::string &_instance_id) : x86ModuleFactory(ModuleType(ModuleCategory::x86_CMSCountMin, _instance_id), "CMSCountMin") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse