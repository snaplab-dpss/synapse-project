#pragma once

#include "controller_module.h"

namespace synapse {
namespace ctrl {

class TBUpdateAndCheck : public ControllerModule {
private:
  addr_t tb_addr;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> pkt_len;
  klee::ref<klee::Expr> time;
  klee::ref<klee::Expr> pass;

public:
  TBUpdateAndCheck(const Node *node, addr_t _tb_addr, klee::ref<klee::Expr> _index, klee::ref<klee::Expr> _pkt_len,
                   klee::ref<klee::Expr> _time, klee::ref<klee::Expr> _pass)
      : ControllerModule(ModuleType::Controller_TBUpdateAndCheck, "TBUpdateAndCheck", node), tb_addr(_tb_addr), index(_index),
        pkt_len(_pkt_len), time(_time), pass(_pass) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new TBUpdateAndCheck(node, tb_addr, index, pkt_len, time, pass);
    return cloned;
  }

  addr_t get_tb_addr() const { return tb_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_pkt_len() const { return pkt_len; }
  klee::ref<klee::Expr> get_time() const { return time; }
  klee::ref<klee::Expr> get_pass() const { return pass; }
};

class TBUpdateAndCheckFactory : public ControllerModuleFactory {
public:
  TBUpdateAndCheckFactory() : ControllerModuleFactory(ModuleType::Controller_TBUpdateAndCheck, "TBUpdateAndCheck") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node, SymbolManager *symbol_manager) const override;
};

} // namespace ctrl
} // namespace synapse