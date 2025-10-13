#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {
class TokenBucketAllocate : public x86Module {
private:
  klee::ref<klee::Expr> capacity;
  klee::ref<klee::Expr> rate;
  klee::ref<klee::Expr> burst;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> tb_out;
  symbol_t success;

public:
  TokenBucketAllocate(const InstanceId _instance_id, const BDDNode *_node, klee::ref<klee::Expr> _capacity, klee::ref<klee::Expr> _rate,
                      klee::ref<klee::Expr> _burst, klee::ref<klee::Expr> _key_size, klee::ref<klee::Expr> _tb_out, symbol_t _success)
      : x86Module(ModuleType(ModuleCategory::x86_TokenBucketAllocate, _instance_id), "TokenBucketAllocate", _node), capacity(_capacity), rate(_rate),
        burst(_burst), key_size(_key_size), tb_out(_tb_out), success(_success) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new TokenBucketAllocate(get_type().instance_id, node, capacity, rate, burst, key_size, tb_out, success);
    return cloned;
  }

  klee::ref<klee::Expr> get_capacity() const { return capacity; }
  klee::ref<klee::Expr> get_rate() const { return rate; }
  klee::ref<klee::Expr> get_burst() const { return burst; }
  klee::ref<klee::Expr> get_key_size() const { return key_size; }
  klee::ref<klee::Expr> get_tb_out() const { return tb_out; }
  symbol_t get_success() const { return success; }
};

class TokenBucketAllocateFactory : public x86ModuleFactory {
public:
  TokenBucketAllocateFactory(const InstanceId _instance_id)
      : x86ModuleFactory(ModuleType(ModuleCategory::x86_TokenBucketAllocate, _instance_id), "TokenBucketAllocate") {}

private:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse
