#include <LibSynapse/Modules/Tofino/HashObj.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct hash_data_t {
  addr_t obj;
  klee::ref<klee::Expr> in;
  klee::ref<klee::Expr> hash;
  DS_ID hash_id;
  std::vector<bits_t> keys;
  bits_t size;
};

hash_data_t get_hash_data(const Context &ctx, const Call *node) {
  const call_t &call = node->get_call();

  klee::ref<klee::Expr> obj_expr = call.args.at("obj").expr;
  klee::ref<klee::Expr> in       = call.args.at("obj").in;
  klee::ref<klee::Expr> hash     = call.ret;

  const addr_t obj = expr_addr_to_obj_addr(obj_expr);

  const hash_data_t data = {
      .obj     = obj,
      .in      = in,
      .hash    = hash,
      .hash_id = "hash_" + std::to_string(node->get_id()),
      .keys    = {in->getWidth()},
      .size    = hash->getWidth(),
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> HashObjFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "hash_obj") {
    return {};
  }

  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> HashObjFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "hash_obj") {
    return {};
  }

  const hash_data_t data = get_hash_data(ep->get_ctx(), call_node);

  Hash *hash = new Hash(data.hash_id, data.keys, data.size);

  Module *module  = new HashObj(node, data.hash_id, data.obj, data.in, data.hash);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, data.obj, hash);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> HashObjFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "hash_obj") {
    return {};
  }

  const hash_data_t data = get_hash_data(ctx, call_node);

  return std::make_unique<HashObj>(node, data.hash_id, data.obj, data.in, data.hash);
}

} // namespace Tofino
} // namespace LibSynapse
