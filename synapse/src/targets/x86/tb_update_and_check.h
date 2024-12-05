#pragma once

#include "x86_module.h"

namespace x86 {

class TBUpdateAndCheck : public x86Module {
private:
  addr_t tb_addr;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> pkt_len;
  klee::ref<klee::Expr> time;
  klee::ref<klee::Expr> pass;

public:
  TBUpdateAndCheck(const Node *node, addr_t _tb_addr,
                   klee::ref<klee::Expr> _index, klee::ref<klee::Expr> _pkt_len,
                   klee::ref<klee::Expr> _time, klee::ref<klee::Expr> _pass)
      : x86Module(ModuleType::x86_TBUpdateAndCheck, "TBUpdateAndCheck", node),
        tb_addr(_tb_addr), index(_index), pkt_len(_pkt_len), time(_time),
        pass(_pass) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned =
        new TBUpdateAndCheck(node, tb_addr, index, pkt_len, time, pass);
    return cloned;
  }

  addr_t get_tb_addr() const { return tb_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_pkt_len() const { return pkt_len; }
  klee::ref<klee::Expr> get_time() const { return time; }
  klee::ref<klee::Expr> get_pass() const { return pass; }
};

class TBUpdateAndCheckGenerator : public x86ModuleGenerator {
public:
  TBUpdateAndCheckGenerator()
      : x86ModuleGenerator(ModuleType::x86_TBUpdateAndCheck,
                           "TBUpdateAndCheck") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "tb_update_and_check") {
      return std::nullopt;
    }

    klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
    addr_t tb_addr = expr_addr_to_obj_addr(tb_addr_expr);

    if (!ctx.can_impl_ds(tb_addr, DSImpl::x86_TokenBucket)) {
      return std::nullopt;
    }

    return spec_impl_t(decide(ep, node), ctx);
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::CALL) {
      return impls;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "tb_update_and_check") {
      return impls;
    }

    klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
    klee::ref<klee::Expr> index = call.args.at("index").expr;
    klee::ref<klee::Expr> pkt_len = call.args.at("pkt_len").expr;
    klee::ref<klee::Expr> time = call.args.at("time").expr;
    klee::ref<klee::Expr> pass = call.ret;

    addr_t tb_addr = expr_addr_to_obj_addr(tb_addr_expr);

    if (!ep->get_ctx().can_impl_ds(tb_addr, DSImpl::x86_TokenBucket)) {
      return impls;
    }

    Module *module =
        new TBUpdateAndCheck(node, tb_addr, index, pkt_len, time, pass);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    new_ep->get_mutable_ctx().save_ds_impl(tb_addr, DSImpl::x86_TokenBucket);

    return impls;
  }
};

} // namespace x86
