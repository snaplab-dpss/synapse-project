#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class Forward : public TofinoModule {
private:
  klee::ref<klee::Expr> dst_device;

public:
  Forward(const LibBDD::Node *node, klee::ref<klee::Expr> _dst_device)
      : TofinoModule(ModuleType::Tofino_Forward, "Forward", node), dst_device(_dst_device) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Forward *cloned = new Forward(node, dst_device);
    return cloned;
  }

  klee::ref<klee::Expr> get_dst_device() const { return dst_device; }
};

class ForwardFactory : public TofinoModuleFactory {
public:
  ForwardFactory() : TofinoModuleFactory(ModuleType::Tofino_Forward, "Forward") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
};

} // namespace Tofino
} // namespace LibSynapse