#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class CMSAllocate : public x86Module {
private:
  klee::ref<klee::Expr> height;
  klee::ref<klee::Expr> width;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> cleanup_interval;
  klee::ref<klee::Expr> cms_out;
  symbol_t success;

public:
  CMSAllocate(const InstanceId _instance_id, const BDDNode *_node, klee::ref<klee::Expr> _height, klee::ref<klee::Expr> _width,
              klee::ref<klee::Expr> _key_size, klee::ref<klee::Expr> _cleanup_interval, klee::ref<klee::Expr> _cms_out, symbol_t _success)
      : x86Module(ModuleType(ModuleCategory::x86_CMSAllocate, _instance_id), "CMSAllocate", _node), height(_height), width(_width),
        key_size(_key_size), cleanup_interval(_cleanup_interval), cms_out(_cms_out), success(_success) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new CMSAllocate(get_type().instance_id, node, height, width, key_size, cleanup_interval, cms_out, success);
    return cloned;
  }
  klee::ref<klee::Expr> get_height() const { return height; }
  klee::ref<klee::Expr> get_width() const { return width; }
  klee::ref<klee::Expr> get_key_size() const { return key_size; }
  klee::ref<klee::Expr> get_cleanup_interval() const { return cleanup_interval; }
  klee::ref<klee::Expr> get_cms_out() const { return cms_out; }
  symbol_t get_success() const { return success; }
};
class CMSAllocateFactory : public x86ModuleFactory {
public:
  CMSAllocateFactory(const InstanceId _instance_id) : x86ModuleFactory(ModuleType(ModuleCategory::x86_CMSAllocate, _instance_id), "CMSAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};
} // namespace x86
} // namespace LibSynapse
