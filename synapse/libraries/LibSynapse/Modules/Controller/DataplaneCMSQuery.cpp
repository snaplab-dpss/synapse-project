#include <LibSynapse/Modules/Controller/DataplaneCMSQuery.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct cms_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> min_estimate;
};

cms_data_t get_cms_data(const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert((call.function_name == "cms_count_min") && "Not a dchain call");

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  const symbol_t min_estimate         = call_node->get_local_symbol("min_estimate");

  const cms_data_t data = {
      .obj          = expr_addr_to_obj_addr(cms_addr_expr),
      .key          = key,
      .min_estimate = min_estimate.expr,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneCMSQueryFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "cms_count_min") {
    return {};
  }

  const cms_data_t cms_data = get_cms_data(call_node);

  if (!ctx.check_ds_impl(cms_data.obj, DSImpl::Tofino_CountMinSketch)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneCMSQueryFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "cms_count_min") {
    return {};
  }

  const cms_data_t cms_data = get_cms_data(call_node);

  if (!ep->get_ctx().check_ds_impl(cms_data.obj, DSImpl::Tofino_CountMinSketch)) {
    return {};
  }

  Module *module  = new DataplaneCMSQuery(node, cms_data.obj, cms_data.key, cms_data.min_estimate);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneCMSQueryFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "cms_count_min") {
    return {};
  }

  const cms_data_t cms_data = get_cms_data(call_node);

  if (!ctx.check_ds_impl(cms_data.obj, DSImpl::Tofino_CountMinSketch)) {
    return {};
  }

  return std::make_unique<DataplaneCMSQuery>(node, cms_data.obj, cms_data.key, cms_data.min_estimate);
}

} // namespace Controller
} // namespace LibSynapse