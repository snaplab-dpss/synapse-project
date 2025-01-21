#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class TokenBucketExpire : public x86Module {
private:
  addr_t tb_addr;
  klee::ref<klee::Expr> time;

public:
  TokenBucketExpire(const LibBDD::Node *node, addr_t _tb_addr, klee::ref<klee::Expr> _time)
      : x86Module(ModuleType::x86_TBExpire, "TokenBucketExpire", node), tb_addr(_tb_addr), time(_time) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new TokenBucketExpire(node, tb_addr, time);
    return cloned;
  }

  addr_t get_tb_addr() const { return tb_addr; }
  klee::ref<klee::Expr> get_time() const { return time; }
};

class TBExpireFactory : public x86ModuleFactory {
public:
  TBExpireFactory() : x86ModuleFactory(ModuleType::x86_TBExpire, "TokenBucketExpire") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
};

} // namespace x86
} // namespace LibSynapse