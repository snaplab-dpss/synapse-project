#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneHHTableIsIndexAllocated : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> index;
  LibCore::symbol_t is_allocated;

public:
  DataplaneHHTableIsIndexAllocated(const LibBDD::Node *_node, addr_t _obj, klee::ref<klee::Expr> _index, const LibCore::symbol_t &_is_allocated)
      : ControllerModule(ModuleType::Controller_DataplaneHHTableIsIndexAllocated, "DataplaneHHTableIsIndexAllocated", _node), obj(_obj),
        index(_index), is_allocated(_is_allocated) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneHHTableIsIndexAllocated(node, obj, index, is_allocated);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
  const LibCore::symbol_t &get_is_allocated() const { return is_allocated; };
};

class DataplaneHHTableIsIndexAllocatedFactory : public ControllerModuleFactory {
public:
  DataplaneHHTableIsIndexAllocatedFactory()
      : ControllerModuleFactory(ModuleType::Controller_DataplaneHHTableIsIndexAllocated, "DataplaneHHTableIsIndexAllocated") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse