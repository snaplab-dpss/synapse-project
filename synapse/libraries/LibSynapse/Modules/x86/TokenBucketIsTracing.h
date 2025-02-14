#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class TokenBucketIsTracing : public x86Module {
private:
  addr_t tb_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> index_out;
  klee::ref<klee::Expr> is_tracing;

public:
  TokenBucketIsTracing(const LibBDD::Node *node, addr_t _tb_addr, klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _index_out,
                       klee::ref<klee::Expr> _is_tracing)
      : x86Module(ModuleType::x86_TBIsTracing, "TokenBucketIsTracing", node), tb_addr(_tb_addr), key(_key), index_out(_index_out),
        is_tracing(_is_tracing) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new TokenBucketIsTracing(node, tb_addr, key, index_out, is_tracing);
    return cloned;
  }

  addr_t get_tb_addr() const { return tb_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_index_out() const { return index_out; }
  klee::ref<klee::Expr> get_is_tracing() const { return is_tracing; }
};

class TBIsTracingFactory : public x86ModuleFactory {
public:
  TBIsTracingFactory() : x86ModuleFactory(ModuleType::x86_TBIsTracing, "TokenBucketIsTracing") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace x86
} // namespace LibSynapse