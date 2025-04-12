#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class TokenBucketTrace : public ControllerModule {
private:
  addr_t tb_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> pkt_len;
  klee::ref<klee::Expr> time;
  klee::ref<klee::Expr> index_out;
  klee::ref<klee::Expr> successfuly_tracing;

public:
  TokenBucketTrace(const LibBDD::Node *_node, addr_t _tb_addr, klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _pkt_len,
                   klee::ref<klee::Expr> _time, klee::ref<klee::Expr> _index_out, klee::ref<klee::Expr> _is_tracing)
      : ControllerModule(ModuleType::Controller_TokenBucketTrace, "TokenBucketTrace", _node), tb_addr(_tb_addr), key(_key), pkt_len(_pkt_len),
        time(_time), index_out(_index_out), successfuly_tracing(_is_tracing) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new TokenBucketTrace(node, tb_addr, key, pkt_len, time, index_out, successfuly_tracing);
    return cloned;
  }

  addr_t get_tb_addr() const { return tb_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_pkt_len() const { return pkt_len; }
  klee::ref<klee::Expr> get_time() const { return time; }
  klee::ref<klee::Expr> get_index_out() const { return index_out; }
  klee::ref<klee::Expr> get_successfuly_tracing() const { return successfuly_tracing; }
};

class TokenBucketTraceFactory : public ControllerModuleFactory {
public:
  TokenBucketTraceFactory() : ControllerModuleFactory(ModuleType::Controller_TokenBucketTrace, "TokenBucketTrace") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse