#include "meter_insert.h"

namespace synapse {
namespace ctrl {

using tofino::Table;

namespace {
void get_tb_data(const Call *tb_trace, addr_t &obj,
                 std::vector<klee::ref<klee::Expr>> &keys,
                 klee::ref<klee::Expr> &success) {
  const call_t &call = tb_trace->get_call();

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  klee::ref<klee::Expr> key = call.args.at("key").in;
  klee::ref<klee::Expr> successfuly_tracing = call.ret;

  obj = expr_addr_to_obj_addr(tb_addr_expr);
  keys = Table::build_keys(key);
  success = successfuly_tracing;
}
} // namespace

std::optional<spec_impl_t> MeterInsertFactory::speculate(const EP *ep, const Node *node,
                                                         const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *tb_trace = dynamic_cast<const Call *>(node);
  const call_t &call = tb_trace->get_call();

  if (call.function_name != "tb_trace") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  addr_t tb_addr = expr_addr_to_obj_addr(tb_addr_expr);

  if (!ctx.can_impl_ds(tb_addr, DSImpl::Tofino_Meter)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> MeterInsertFactory::process_node(const EP *ep,
                                                     const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *tb_trace = dynamic_cast<const Call *>(node);
  const call_t &call = tb_trace->get_call();

  if (call.function_name != "tb_trace") {
    return impls;
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> success;
  get_tb_data(tb_trace, obj, keys, success);

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_Meter)) {
    return impls;
  }

  Module *module = new MeterInsert(node, obj, keys, success);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace ctrl
} // namespace synapse