#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class Forward : public x86Module {
private:
  klee::ref<klee::Expr> dst_device;

public:
  Forward(const LibBDD::Node *_node, klee::ref<klee::Expr> _dst_device)
      : x86Module(ModuleType::x86_Forward, "Forward", _node), dst_device(_dst_device) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    Forward *cloned = new Forward(node, dst_device);
    return cloned;
  }

  klee::ref<klee::Expr> get_dst_device() const { return dst_device; }
};

class ForwardFactory : public x86ModuleFactory {
public:
  ForwardFactory() : x86ModuleFactory(ModuleType::x86_Forward, "Forward") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace x86
} // namespace LibSynapse