#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneBloomFilterAllocate : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> height;
  klee::ref<klee::Expr> width;
  klee::ref<klee::Expr> key_size;
  time_ns_t cleanup_internal;

public:
  DataplaneBloomFilterAllocate(const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _height, klee::ref<klee::Expr> _width,
                               klee::ref<klee::Expr> _key_size, time_ns_t _cleanup_internal)
      : ControllerModule(ModuleType::Controller_DataplaneBloomFilterAllocate, "DataplaneBloomFilterAllocate", _node), obj(_obj), height(_height),
        width(_width), key_size(_key_size), cleanup_internal(_cleanup_internal) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneBloomFilterAllocate(node, obj, height, width, key_size, cleanup_internal);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const klee::ref<klee::Expr> &get_height() const { return height; }
  const klee::ref<klee::Expr> &get_width() const { return width; }
  const klee::ref<klee::Expr> &get_key_size() const { return key_size; }
  time_ns_t get_cleanup_internal() const { return cleanup_internal; }
};

class DataplaneBloomFilterAllocateFactory : public ControllerModuleFactory {
public:
  DataplaneBloomFilterAllocateFactory()
      : ControllerModuleFactory(ModuleType::Controller_DataplaneBloomFilterAllocate, "DataplaneBloomFilterAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse