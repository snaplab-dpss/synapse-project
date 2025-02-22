#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DchainTableDelete : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key;

public:
  DchainTableDelete(const LibBDD::Node *node, addr_t _obj, klee::ref<klee::Expr> _key)
      : ControllerModule(ModuleType::Controller_DchainTableDelete, "DchainTableDelete", node), obj(_obj), key(_key) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    DchainTableDelete *cloned = new DchainTableDelete(node, obj, key);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
};

class DchainTableDeleteFactory : public ControllerModuleFactory {
public:
  DchainTableDeleteFactory() : ControllerModuleFactory(ModuleType::Controller_DchainTableDelete, "DchainTableDelete") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse