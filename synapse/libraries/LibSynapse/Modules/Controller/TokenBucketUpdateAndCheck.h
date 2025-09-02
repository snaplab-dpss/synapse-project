#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class TokenBucketUpdateAndCheck : public ControllerModule {
private:
  addr_t tb_addr;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> pkt_len;
  klee::ref<klee::Expr> time;
  klee::ref<klee::Expr> pass;

public:
  TokenBucketUpdateAndCheck(const std::string &_instance_id, const BDDNode *_node, addr_t _tb_addr, klee::ref<klee::Expr> _index,
                            klee::ref<klee::Expr> _pkt_len, klee::ref<klee::Expr> _time, klee::ref<klee::Expr> _pass)
      : ControllerModule(ModuleType(ModuleCategory::Controller_TokenBucketUpdateAndCheck, _instance_id), "TokenBucketUpdateAndCheck", _node),
        tb_addr(_tb_addr), index(_index), pkt_len(_pkt_len), time(_time), pass(_pass) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new TokenBucketUpdateAndCheck(get_type().instance_id, node, tb_addr, index, pkt_len, time, pass);
    return cloned;
  }

  addr_t get_tb_addr() const { return tb_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_pkt_len() const { return pkt_len; }
  klee::ref<klee::Expr> get_time() const { return time; }
  klee::ref<klee::Expr> get_pass() const { return pass; }
};

class TokenBucketUpdateAndCheckFactory : public ControllerModuleFactory {
public:
  TokenBucketUpdateAndCheckFactory(const std::string &_instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_TokenBucketUpdateAndCheck, _instance_id), "TokenBucketUpdateAndCheck") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse