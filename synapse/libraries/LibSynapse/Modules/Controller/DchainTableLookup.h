#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DchainTableLookup : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key;
  std::optional<LibCore::symbol_t> hit;

public:
  DchainTableLookup(const LibBDD::Node *node, addr_t _obj, klee::ref<klee::Expr> _key, const std::optional<LibCore::symbol_t> &_hit)
      : ControllerModule(ModuleType::Controller_DchainTableLookup, "DchainTableLookup", node), obj(_obj), key(_key), hit(_hit) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new DchainTableLookup(node, obj, key, hit);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  std::optional<LibCore::symbol_t> get_hit() const { return hit; }
};

class DchainTableLookupFactory : public ControllerModuleFactory {
public:
  DchainTableLookupFactory() : ControllerModuleFactory(ModuleType::Controller_DchainTableLookup, "DchainTableLookup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse