#pragma once

#include "x86_module.h"

namespace synapse {
namespace x86 {

class TBIsTracing : public x86Module {
private:
  addr_t tb_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> index_out;
  klee::ref<klee::Expr> is_tracing;

public:
  TBIsTracing(const Node *node, addr_t _tb_addr, klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _index_out,
              klee::ref<klee::Expr> _is_tracing)
      : x86Module(ModuleType::x86_TBIsTracing, "TBIsTracing", node), tb_addr(_tb_addr), key(_key),
        index_out(_index_out), is_tracing(_is_tracing) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new TBIsTracing(node, tb_addr, key, index_out, is_tracing);
    return cloned;
  }

  addr_t get_tb_addr() const { return tb_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_index_out() const { return index_out; }
  klee::ref<klee::Expr> get_is_tracing() const { return is_tracing; }
};

class TBIsTracingFactory : public x86ModuleFactory {
public:
  TBIsTracingFactory() : x86ModuleFactory(ModuleType::x86_TBIsTracing, "TBIsTracing") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace x86
} // namespace synapse