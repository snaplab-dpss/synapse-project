#include <LibSynapse/Modules/Controller/DataplaneBloomFilterQuery.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct bf_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> estimate;
};

bf_data_t get_bf_data(const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert((call.function_name == "bf_query"));

  klee::ref<klee::Expr> bf_addr_expr = call.args.at("bf").expr;
  klee::ref<klee::Expr> key          = call.args.at("key").in;
  const symbol_t estimate            = call_node->get_local_symbol("bf_query_estimate");

  const bf_data_t data = {
      .obj      = expr_addr_to_obj_addr(bf_addr_expr),
      .key      = key,
      .estimate = estimate.expr,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneBloomFilterQueryFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "bf_query") {
    return {};
  }

  const bf_data_t bf_data = get_bf_data(call_node);

  if (!ctx.check_ds_impl(bf_data.obj, DSImpl::Tofino_BloomFilter)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneBloomFilterQueryFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "bf_query") {
    return {};
  }

  const bf_data_t bf_data = get_bf_data(call_node);

  if (!ep->get_ctx().check_ds_impl(bf_data.obj, DSImpl::Tofino_BloomFilter)) {
    return {};
  }

  Module *module  = new DataplaneBloomFilterQuery(node, bf_data.obj, bf_data.key, bf_data.estimate);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneBloomFilterQueryFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "bf_query") {
    return {};
  }

  const bf_data_t bf_data = get_bf_data(call_node);

  if (!ctx.check_ds_impl(bf_data.obj, DSImpl::Tofino_BloomFilter)) {
    return {};
  }

  return std::make_unique<DataplaneBloomFilterQuery>(node, bf_data.obj, bf_data.key, bf_data.estimate);
}

} // namespace Controller
} // namespace LibSynapse