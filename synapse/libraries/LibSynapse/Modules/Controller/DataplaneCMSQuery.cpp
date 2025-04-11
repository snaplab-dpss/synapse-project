#include <LibSynapse/Modules/Controller/DataplaneCMSQuery.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace Controller {

namespace {

struct cms_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> min_estimate;
};

cms_data_t get_cms_data(const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();
  assert((call.function_name == "cms_count_min") && "Not a dchain call");

  klee::ref<klee::Expr> cms_addr_expr  = call.args.at("cms").expr;
  klee::ref<klee::Expr> key            = call.args.at("key").in;
  const LibCore::symbol_t min_estimate = call_node->get_local_symbol("min_estimate");

  const cms_data_t data = {
      .obj          = LibCore::expr_addr_to_obj_addr(cms_addr_expr),
      .key          = key,
      .min_estimate = min_estimate.expr,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneCMSQueryFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "cms_count_min") {
    return std::nullopt;
  }

  const cms_data_t cms_data = get_cms_data(call_node);

  if (!ctx.check_ds_impl(cms_data.obj, DSImpl::Tofino_CountMinSketch)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneCMSQueryFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "cms_count_min") {
    return impls;
  }

  const cms_data_t cms_data = get_cms_data(call_node);

  if (!ep->get_ctx().check_ds_impl(cms_data.obj, DSImpl::Tofino_CountMinSketch)) {
    return impls;
  }

  Module *module  = new DataplaneCMSQuery(node, cms_data.obj, cms_data.key, cms_data.min_estimate);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> DataplaneCMSQueryFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

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