#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneDchainTableRefreshIndex : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> index;

public:
  DataplaneDchainTableRefreshIndex(const LibBDD::Node *_node, addr_t _obj, klee::ref<klee::Expr> _index)
      : ControllerModule(ModuleType::Controller_DataplaneDchainTableRefreshIndex, "DataplaneDchainTableRefreshIndex", _node), obj(_obj),
        index(_index) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneDchainTableRefreshIndex(node, obj, index);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
};

class DataplaneDchainTableRefreshIndexFactory : public ControllerModuleFactory {
public:
  DataplaneDchainTableRefreshIndexFactory()
      : ControllerModuleFactory(ModuleType::Controller_DataplaneDchainTableRefreshIndex, "DataplaneDchainTableRefreshIndex") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse