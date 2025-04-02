#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneDchainTableIsIndexAllocated : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> index;
  LibCore::symbol_t is_allocated;

public:
  DataplaneDchainTableIsIndexAllocated(const LibBDD::Node *node, addr_t _obj, klee::ref<klee::Expr> _index, const LibCore::symbol_t &_is_allocated)
      : ControllerModule(ModuleType::Controller_DataplaneDchainTableIsIndexAllocated, "DataplaneDchainTableIsIndexAllocated", node), obj(_obj),
        index(_index), is_allocated(_is_allocated) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneDchainTableIsIndexAllocated(node, obj, index, is_allocated);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
  const LibCore::symbol_t &get_is_allocated() const { return is_allocated; }
};

class DataplaneDchainTableIsIndexAllocatedFactory : public ControllerModuleFactory {
public:
  DataplaneDchainTableIsIndexAllocatedFactory()
      : ControllerModuleFactory(ModuleType::Controller_DataplaneDchainTableIsIndexAllocated, "DataplaneDchainTableIsIndexAllocated") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse