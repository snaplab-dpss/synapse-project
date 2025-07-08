#include <LibSynapse/Modules/Controller/DataplaneMeterInsert.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using Tofino::Table;

namespace {

void get_tb_data(const Context &ctx, const LibBDD::Call *tb_trace, addr_t &obj, std::vector<klee::ref<klee::Expr>> &keys,
                 klee::ref<klee::Expr> &success) {
  const LibBDD::call_t &call = tb_trace->get_call();

  klee::ref<klee::Expr> tb_addr_expr        = call.args.at("tb").expr;
  klee::ref<klee::Expr> key                 = call.args.at("key").in;
  klee::ref<klee::Expr> successfuly_tracing = call.ret;

  obj     = LibCore::expr_addr_to_obj_addr(tb_addr_expr);
  keys    = Table::build_keys(key, ctx.get_expr_structs());
  success = successfuly_tracing;
}

} // namespace

std::optional<spec_impl_t> DataplaneMeterInsertFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *tb_trace = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call   = tb_trace->get_call();

  if (call.function_name != "tb_trace") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  addr_t tb_addr                     = LibCore::expr_addr_to_obj_addr(tb_addr_expr);

  if (!ctx.can_impl_ds(tb_addr, DSImpl::Tofino_Meter)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneMeterInsertFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *tb_trace = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call   = tb_trace->get_call();

  if (call.function_name != "tb_trace") {
    return {};
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> success;
  get_tb_data(ep->get_ctx(), tb_trace, obj, keys, success);

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_Meter)) {
    return {};
  }

  Module *module  = new DataplaneMeterInsert(node, obj, keys, success);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneMeterInsertFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *tb_trace = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call   = tb_trace->get_call();

  if (call.function_name != "tb_trace") {
    return {};
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> success;
  get_tb_data(ctx, tb_trace, obj, keys, success);

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_Meter)) {
    return {};
  }

  return std::make_unique<DataplaneMeterInsert>(node, obj, keys, success);
}

} // namespace Controller
} // namespace LibSynapse