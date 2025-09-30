#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class TokenBucketExpire : public x86Module {
private:
  klee::ref<klee::Expr> tb_addr;
  klee::ref<klee::Expr> time;

public:
  TokenBucketExpire(const std::string &_instance_id, const BDDNode *_node, klee::ref<klee::Expr> _tb_addr, klee::ref<klee::Expr> _time)
      : x86Module(ModuleType(ModuleCategory::x86_TokenBucketExpire, _instance_id), "TokenBucketExpire", _node), tb_addr(_tb_addr), time(_time) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new TokenBucketExpire(get_type().instance_id, node, tb_addr, time);
    return cloned;
  }

  klee::ref<klee::Expr> get_tb_addr() const { return tb_addr; }
  klee::ref<klee::Expr> get_time() const { return time; }
};

class TokenBucketExpireFactory : public x86ModuleFactory {
public:
  TokenBucketExpireFactory(const std::string &_instance_id)
      : x86ModuleFactory(ModuleType(ModuleCategory::x86_TokenBucketExpire, _instance_id), "TokenBucketExpire") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse