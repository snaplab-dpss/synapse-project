#include <LibSynapse/Modules/Controller/CMSAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> CMSAllocateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "cms_allocate") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> height           = call.args.at("height").expr;
  klee::ref<klee::Expr> width            = call.args.at("width").expr;
  klee::ref<klee::Expr> key_size         = call.args.at("key_size").expr;
  klee::ref<klee::Expr> cleanup_interval = call.args.at("cleanup_interval").expr;
  klee::ref<klee::Expr> cms_out          = call.args.at("cms_out").out;

  addr_t cms_addr = LibCore::expr_addr_to_obj_addr(cms_out);

  if (!ctx.can_impl_ds(cms_addr, DSImpl::Controller_CountMinSketch)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(cms_addr, DSImpl::Controller_CountMinSketch);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> CMSAllocateFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "cms_allocate") {
    return impls;
  }

  klee::ref<klee::Expr> height           = call.args.at("height").expr;
  klee::ref<klee::Expr> width            = call.args.at("width").expr;
  klee::ref<klee::Expr> key_size         = call.args.at("key_size").expr;
  klee::ref<klee::Expr> cleanup_interval = call.args.at("cleanup_interval").expr;
  klee::ref<klee::Expr> cms_out          = call.args.at("cms_out").out;

  addr_t cms_addr = LibCore::expr_addr_to_obj_addr(cms_out);

  if (!ep->get_ctx().can_impl_ds(cms_addr, DSImpl::Controller_CountMinSketch)) {
    return impls;
  }

  Module *module  = new CMSAllocate(node, cms_addr, height, width, key_size, cleanup_interval);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  new_ep->get_mutable_ctx().save_ds_impl(cms_addr, DSImpl::Controller_CountMinSketch);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> CMSAllocateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "cms_allocate") {
    return {};
  }

  klee::ref<klee::Expr> height           = call.args.at("height").expr;
  klee::ref<klee::Expr> width            = call.args.at("width").expr;
  klee::ref<klee::Expr> key_size         = call.args.at("key_size").expr;
  klee::ref<klee::Expr> cleanup_interval = call.args.at("cleanup_interval").expr;
  klee::ref<klee::Expr> cms_out          = call.args.at("cms_out").out;

  const addr_t cms_addr = LibCore::expr_addr_to_obj_addr(cms_out);

  if (!ctx.check_ds_impl(cms_addr, DSImpl::Controller_CountMinSketch)) {
    return {};
  }

  return std::make_unique<CMSAllocate>(node, cms_addr, height, width, key_size, cleanup_interval);
}

} // namespace Controller
} // namespace LibSynapse