#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class TokenBucketUpdateAndCheck : public x86Module {
private:
  addr_t tb_addr;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> pkt_len;
  klee::ref<klee::Expr> time;
  klee::ref<klee::Expr> pass;

public:
  TokenBucketUpdateAndCheck(const LibBDD::Node *node, addr_t _tb_addr, klee::ref<klee::Expr> _index, klee::ref<klee::Expr> _pkt_len,
                            klee::ref<klee::Expr> _time, klee::ref<klee::Expr> _pass)
      : x86Module(ModuleType::x86_TBUpdateAndCheck, "TokenBucketUpdateAndCheck", node), tb_addr(_tb_addr), index(_index), pkt_len(_pkt_len),
        time(_time), pass(_pass) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new TokenBucketUpdateAndCheck(node, tb_addr, index, pkt_len, time, pass);
    return cloned;
  }

  addr_t get_tb_addr() const { return tb_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_pkt_len() const { return pkt_len; }
  klee::ref<klee::Expr> get_time() const { return time; }
  klee::ref<klee::Expr> get_pass() const { return pass; }
};

class TBUpdateAndCheckFactory : public x86ModuleFactory {
public:
  TBUpdateAndCheckFactory() : x86ModuleFactory(ModuleType::x86_TBUpdateAndCheck, "TokenBucketUpdateAndCheck") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
};

} // namespace x86
} // namespace LibSynapse