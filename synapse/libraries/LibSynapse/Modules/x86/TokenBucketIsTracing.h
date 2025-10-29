#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class TokenBucketIsTracing : public x86Module {
private:
  klee::ref<klee::Expr> tb_addr;
  klee::ref<klee::Expr> key_addr;
  klee::ref<klee::Expr> tb_addr;
  klee::ref<klee::Expr> key_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> index_out;
  klee::ref<klee::Expr> is_tracing;

public:
  TokenBucketIsTracing(const InstanceId _instance_id, const BDDNode *_node, klee::ref<klee::Expr> _tb_addr, klee::ref<klee::Expr> _key_addr,
                       klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _index_out, klee::ref<klee::Expr> _is_tracing)
      : x86Module(ModuleType(ModuleCategory::x86_TokenBucketIsTracing, _instance_id), "TokenBucketIsTracing", _node), tb_addr(_tb_addr),
        key_addr(_key_addr), key(_key), index_out(_index_out), is_tracing(_is_tracing) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new TokenBucketIsTracing(get_type().instance_id, node, tb_addr, key_addr, key, index_out, is_tracing);
    Module *cloned = new TokenBucketIsTracing(get_type().instance_id, node, tb_addr, key_addr, key, index_out, is_tracing);
    return cloned;
  }

  klee::ref<klee::Expr> get_tb_addr() const { return tb_addr; }
  klee::ref<klee::Expr> get_key_addr() const { return key_addr; }
  klee::ref<klee::Expr> get_tb_addr() const { return tb_addr; }
  klee::ref<klee::Expr> get_key_addr() const { return key_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_index_out() const { return index_out; }
  klee::ref<klee::Expr> get_is_tracing() const { return is_tracing; }
};

class TokenBucketIsTracingFactory : public x86ModuleFactory {
public:
  TokenBucketIsTracingFactory(const InstanceId _instance_id)
      : x86ModuleFactory(ModuleType(ModuleCategory::x86_TokenBucketIsTracing, _instance_id), "TokenBucketIsTracing") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse
