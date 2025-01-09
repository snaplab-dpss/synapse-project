#pragma once

#include "x86_module.h"

namespace synapse {
namespace x86 {

class TBExpire : public x86Module {
private:
  addr_t tb_addr;
  klee::ref<klee::Expr> time;

public:
  TBExpire(const Node *node, addr_t _tb_addr, klee::ref<klee::Expr> _time)
      : x86Module(ModuleType::x86_TBExpire, "TBExpire", node), tb_addr(_tb_addr), time(_time) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new TBExpire(node, tb_addr, time);
    return cloned;
  }

  addr_t get_tb_addr() const { return tb_addr; }
  klee::ref<klee::Expr> get_time() const { return time; }
};

class TBExpireFactory : public x86ModuleFactory {
public:
  TBExpireFactory() : x86ModuleFactory(ModuleType::x86_TBExpire, "TBExpire") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node, SymbolManager *symbol_manager) const override;
};

} // namespace x86
} // namespace synapse