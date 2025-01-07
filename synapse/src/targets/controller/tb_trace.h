#pragma once

#include "controller_module.h"

namespace synapse {
namespace ctrl {

class TBTrace : public ControllerModule {
private:
  addr_t tb_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> pkt_len;
  klee::ref<klee::Expr> time;
  klee::ref<klee::Expr> index_out;
  klee::ref<klee::Expr> successfuly_tracing;

public:
  TBTrace(const Node *node, addr_t _tb_addr, klee::ref<klee::Expr> _key,
          klee::ref<klee::Expr> _pkt_len, klee::ref<klee::Expr> _time,
          klee::ref<klee::Expr> _index_out, klee::ref<klee::Expr> _is_tracing)
      : ControllerModule(ModuleType::Controller_TBTrace, "TBTrace", node), tb_addr(_tb_addr),
        key(_key), pkt_len(_pkt_len), time(_time), index_out(_index_out),
        successfuly_tracing(_is_tracing) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new TBTrace(node, tb_addr, key, pkt_len, time, index_out, successfuly_tracing);
    return cloned;
  }

  addr_t get_tb_addr() const { return tb_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_pkt_len() const { return pkt_len; }
  klee::ref<klee::Expr> get_time() const { return time; }
  klee::ref<klee::Expr> get_index_out() const { return index_out; }
  klee::ref<klee::Expr> get_successfuly_tracing() const { return successfuly_tracing; }
};

class TBTraceFactory : public ControllerModuleFactory {
public:
  TBTraceFactory() : ControllerModuleFactory(ModuleType::Controller_TBTrace, "TBTrace") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace ctrl
} // namespace synapse