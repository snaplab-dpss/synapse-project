#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class TokenBucketExpire : public ControllerModule {
private:
  addr_t tb_addr;
  klee::ref<klee::Expr> time;

public:
  TokenBucketExpire(const InstanceId _instance_id, const BDDNode *_node, addr_t _tb_addr, klee::ref<klee::Expr> _time)
      : ControllerModule(ModuleType(ModuleCategory::Controller_TokenBucketExpire, _instance_id), "TokenBucketExpire", _node), tb_addr(_tb_addr),
        time(_time) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new TokenBucketExpire(get_type().instance_id, node, tb_addr, time);
    return cloned;
  }

  addr_t get_tb_addr() const { return tb_addr; }
  klee::ref<klee::Expr> get_time() const { return time; }
};

class TokenBucketExpireFactory : public ControllerModuleFactory {
public:
  TokenBucketExpireFactory(const InstanceId _instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_TokenBucketExpire, _instance_id), "TokenBucketExpire") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
