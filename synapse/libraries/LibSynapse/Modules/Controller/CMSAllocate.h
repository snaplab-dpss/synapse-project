#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class CMSAllocate : public ControllerModule {
private:
  addr_t cms_addr;
  klee::ref<klee::Expr> height;
  klee::ref<klee::Expr> width;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> cleanup_interval;

public:
  CMSAllocate(const LibBDD::Node *_node, addr_t _cms_addr, klee::ref<klee::Expr> _height, klee::ref<klee::Expr> _width,
              klee::ref<klee::Expr> _key_size, klee::ref<klee::Expr> _cleanup_interval)
      : ControllerModule(ModuleType::Controller_CMSAllocate, "CMSAllocate", _node), cms_addr(_cms_addr), height(_height), width(_width),
        key_size(_key_size), cleanup_interval(_cleanup_interval) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new CMSAllocate(node, cms_addr, height, width, key_size, cleanup_interval);
    return cloned;
  }

  addr_t get_cms_addr() const { return cms_addr; }
  klee::ref<klee::Expr> get_height() const { return height; }
  klee::ref<klee::Expr> get_width() const { return width; }
  klee::ref<klee::Expr> get_key_size() const { return key_size; }
  klee::ref<klee::Expr> get_cleanup_interval() const { return cleanup_interval; }
};

class CMSAllocateFactory : public ControllerModuleFactory {
public:
  CMSAllocateFactory() : ControllerModuleFactory(ModuleType::Controller_CMSAllocate, "CMSAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse