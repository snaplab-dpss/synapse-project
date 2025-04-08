#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class AbortTransaction : public ControllerModule {
public:
  AbortTransaction(const LibBDD::Node *node) : ControllerModule(ModuleType::Controller_AbortTransaction, "AbortTransaction", node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    AbortTransaction *cloned = new AbortTransaction(node);
    return cloned;
  }
};

class AbortTransactionFactory : public ControllerModuleFactory {
public:
  AbortTransactionFactory() : ControllerModuleFactory(ModuleType::Controller_AbortTransaction, "AbortTransaction") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse