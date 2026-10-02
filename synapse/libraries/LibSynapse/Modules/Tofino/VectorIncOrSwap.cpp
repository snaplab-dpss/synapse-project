#include <LibSynapse/Modules/Tofino/VectorIncOrSwap.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;
using LibCore::solver_toolbox;

namespace {

struct inc_or_swap_data_t {
  addr_t obj;
  u32 capacity;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> pair_in;
  klee::ref<klee::Expr> pair_out;
  bits_t key_size;
  bits_t value_size;
  bool evict;
  bool pair_out_used;
};

// Does any node after `node` read a symbol of `expr` (a call argument or result, a condition)?
bool used_after(const BDDNode *node, klee::ref<klee::Expr> expr) {
  const std::unordered_set<std::string> symbols = LibCore::symbol_t::get_symbols_names(expr);
  const BDDNode *next                           = node->get_next();
  if (!next) {
    return false;
  }
  bool used = false;
  next->visit_nodes([&](const BDDNode *n) {
    std::unordered_set<std::string> names;
    const auto collect = [&names](klee::ref<klee::Expr> e) {
      if (!e.isNull()) {
        names.merge(LibCore::symbol_t::get_symbols_names(e));
      }
    };
    if (n->get_type() == BDDNodeType::Call) {
      const call_t &c = dynamic_cast<const Call *>(n)->get_call();
      for (const auto &[name, arg] : c.args) {
        collect(arg.expr);
        collect(arg.in);
        collect(arg.out);
      }
      collect(c.ret);
    } else if (n->get_type() == BDDNodeType::Branch) {
      collect(dynamic_cast<const LibBDD::Branch *>(n)->get_condition());
    }
    for (const std::string &symbol : symbols) {
      if (names.contains(symbol)) {
        used = true;
        return LibBDD::BDDNodeVisitAction::Stop;
      }
    }
    return LibBDD::BDDNodeVisitAction::Continue;
  });
  return used;
}

bool matches(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }
  return dynamic_cast<const Call *>(node)->get_call().function_name == "vector_inc_or_swap";
}

inc_or_swap_data_t get_data(const Context &ctx, const Call *call_node) {
  const call_t &call = call_node->get_call();

  const addr_t obj           = expr_addr_to_obj_addr(call.args.at("vector").expr);
  const vector_config_t &cfg = ctx.get_vector_config(obj);

  return inc_or_swap_data_t{
      .obj           = obj,
      .capacity      = static_cast<u32>(cfg.capacity),
      .index         = call.args.at("index").expr,
      .pair_in       = call.args.at("pair").in,
      .pair_out      = call.args.at("pair").out,
      .key_size      = static_cast<bits_t>(solver_toolbox.value_from_expr(call.args.at("key_size").expr) * 8),
      .value_size    = static_cast<bits_t>(solver_toolbox.value_from_expr(call.args.at("value_size").expr) * 8),
      .evict         = solver_toolbox.value_from_expr(call.args.at("evict").expr) != 0,
      .pair_out_used = used_after(call_node, call.args.at("pair").out),
  };
}

// The cell is one pair register: both halves the same width, each a SALU word.
bool fits_pair_register(const EP *ep, const inc_or_swap_data_t &data) {
  const tna_properties_t &properties = ep->get_ctx().get_target_ctx<TofinoContext>()->get_tna().tna_config.properties;
  return data.key_size == data.value_size && data.key_size <= properties.max_salu_size && data.pair_in->getWidth() == data.key_size + data.value_size;
}

VectorRegister *get_pair_vector_register(const EP *ep, addr_t obj) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  if (!tofino_ctx->get_data_structures().has(obj)) {
    return nullptr;
  }
  const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(obj);
  assert(ds.size() == 1 && (*ds.begin())->type == DSType::VectorRegister);
  return dynamic_cast<VectorRegister *>(*ds.begin());
}

VectorRegister *build_pair_vector_register(const EP *ep, const BDDNode *node, const inc_or_swap_data_t &data) {
  const TofinoContext *tofino_ctx    = ep->get_ctx().get_target_ctx<TofinoContext>();
  const tna_properties_t &properties = tofino_ctx->get_tna().tna_config.properties;

  VectorRegister *vector_register =
      new VectorRegister(properties, TofinoModuleFactory::build_vector_register_id(data.obj), data.capacity, data.index->getWidth(), {data.key_size});
  assert(vector_register->regs.size() == 1);
  vector_register->regs[0].actions = {RegisterActionType::IncOrSwap};

  if (!tofino_ctx->can_place(ep, node, vector_register)) {
    delete vector_register;
    return nullptr;
  }

  return vector_register;
}

// Reused when the vector is already a register of this kind (another stage's call on the same
// vector would be a second site, which the search rejects below); built otherwise.
VectorRegister *build_or_reuse_pair_vector_register(const EP *ep, const BDDNode *node, const inc_or_swap_data_t &data) {
  if (ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_VectorRegister)) {
    VectorRegister *vector_register = get_pair_vector_register(ep, data.obj);
    if (vector_register && vector_register->regs.size() == 1 && vector_register->regs[0].actions.contains(RegisterActionType::IncOrSwap)) {
      return vector_register;
    }
    return nullptr;
  }
  return build_pair_vector_register(ep, node, data);
}

} // namespace

std::optional<spec_impl_t> VectorIncOrSwapFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (!matches(node)) {
    return {};
  }

  const inc_or_swap_data_t data = get_data(ep->get_ctx(), dynamic_cast<const Call *>(node));

  if (!speculations.ctx.can_impl_ds(data.obj, DSImpl::Tofino_VectorRegister) || !fits_pair_register(ep, data)) {
    return {};
  }

  const bool reusing              = ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_VectorRegister);
  VectorRegister *vector_register = build_or_reuse_pair_vector_register(ep, node, data);
  if (!vector_register) {
    return {};
  }
  if (!reusing) {
    delete vector_register;
  }

  if (was_ds_already_used(ep->get_leaf_ep_node_from_bdd_node(node), speculations, node, data.obj, DSImpl::Tofino_VectorRegister,
                          build_vector_register_id(data.obj))) {
    return {};
  }

  Context new_ctx = speculations.ctx;
  new_ctx.save_ds_impl(node->get_id(), data.obj, DSImpl::Tofino_VectorRegister);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> VectorIncOrSwapFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!matches(node)) {
    return {};
  }

  const inc_or_swap_data_t data = get_data(ep->get_ctx(), dynamic_cast<const Call *>(node));

  if (!ep->get_ctx().can_impl_ds(data.obj, DSImpl::Tofino_VectorRegister) || !fits_pair_register(ep, data)) {
    return {};
  }

  VectorRegister *vector_register = build_or_reuse_pair_vector_register(ep, node, data);
  if (!vector_register) {
    return {};
  }

  const EPNode *ep_node_leaf = ep->get_active_leaf().node;
  if (ep_node_leaf && was_ds_already_used(ep_node_leaf, vector_register->id)) {
    return {};
  }

  Module *module  = new VectorIncOrSwap(node, vector_register->id, data.obj, data.index, data.pair_in, data.pair_out, data.key_size, data.evict,
                                        data.pair_out_used);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(node->get_id(), data.obj, DSImpl::Tofino_VectorRegister);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, data.obj, vector_register);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> VectorIncOrSwapFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!matches(node)) {
    return {};
  }

  const inc_or_swap_data_t data = get_data(ctx, dynamic_cast<const Call *>(node));

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  const std::unordered_set<DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_ds(data.obj);
  assert(ds.size() == 1 && (*ds.begin())->type == DSType::VectorRegister);
  const VectorRegister *vector_register = dynamic_cast<const VectorRegister *>(*ds.begin());

  return std::make_unique<VectorIncOrSwap>(node, vector_register->id, data.obj, data.index, data.pair_in, data.pair_out, data.key_size, data.evict,
                                           data.pair_out_used);
}

} // namespace Tofino
} // namespace LibSynapse
