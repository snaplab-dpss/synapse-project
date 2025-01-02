#pragma once

#include "controller_module.h"

namespace synapse {
namespace controller {

class TBExpire : public ControllerModule {
private:
  addr_t tb_addr;
  klee::ref<klee::Expr> time;

public:
  TBExpire(const Node *node, addr_t _tb_addr, klee::ref<klee::Expr> _time)
      : ControllerModule(ModuleType::Controller_TBExpire, "TBExpire", node),
        tb_addr(_tb_addr), time(_time) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new TBExpire(node, tb_addr, time);
    return cloned;
  }

  addr_t get_tb_addr() const { return tb_addr; }
  klee::ref<klee::Expr> get_time() const { return time; }
};

class TBExpireFactory : public ControllerModuleFactory {
public:
  TBExpireFactory()
      : ControllerModuleFactory(ModuleType::Controller_TBExpire, "TBExpire") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace controller
} // namespace synapse