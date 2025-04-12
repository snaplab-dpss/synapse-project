#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneCMSAllocate : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> height;
  klee::ref<klee::Expr> width;
  klee::ref<klee::Expr> key_size;
  time_ns_t cleanup_internal;

public:
  DataplaneCMSAllocate(const LibBDD::Node *_node, addr_t _obj, klee::ref<klee::Expr> _height, klee::ref<klee::Expr> _width,
                       klee::ref<klee::Expr> _key_size, time_ns_t _cleanup_internal)
      : ControllerModule(ModuleType::Controller_DataplaneCMSAllocate, "DataplaneCMSAllocate", _node), obj(_obj), height(_height), width(_width),
        key_size(_key_size), cleanup_internal(_cleanup_internal) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneCMSAllocate(node, obj, height, width, key_size, cleanup_internal);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const klee::ref<klee::Expr> &get_height() const { return height; }
  const klee::ref<klee::Expr> &get_width() const { return width; }
  const klee::ref<klee::Expr> &get_key_size() const { return key_size; }
  time_ns_t get_cleanup_internal() const { return cleanup_internal; }
};

class DataplaneCMSAllocateFactory : public ControllerModuleFactory {
public:
  DataplaneCMSAllocateFactory() : ControllerModuleFactory(ModuleType::Controller_DataplaneCMSAllocate, "DataplaneCMSAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse