#include <LibSynapse/Modules/Tofino/LPMLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct lpm_lookup_data_t {
  addr_t obj;
  klee::ref<klee::Expr> original_key;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  klee::ref<klee::Expr> match;
};

lpm_lookup_data_t get_lpm_lookup_data(const Context &ctx, const Call *lpm_lookup) {
  const call_t &call = lpm_lookup->get_call();
  lpm_lookup_data_t data;
  data.obj          = expr_addr_to_obj_addr(call.args.at("lpm").expr);
  data.original_key = call.args.at("prefix").in;
  data.keys         = Table::build_keys(data.original_key, ctx.get_expr_structs());
  // build_keys lists a key's fields from its top bits down, which for a key read whole from
  // memory (little-endian) is the last field first. The table's fields go in memory order, the
  // order the controller writes a prefix in.
  if (data.keys.size() > 1) {
    const std::vector<LibCore::expr_group_t> first = LibCore::get_expr_groups(data.keys.front());
    const std::vector<LibCore::expr_group_t> last  = LibCore::get_expr_groups(data.keys.back());
    if (first.size() == 1 && last.size() == 1 && first[0].has_symbol && last[0].has_symbol && first[0].symbol == last[0].symbol &&
        first[0].offset > last[0].offset) {
      std::reverse(data.keys.begin(), data.keys.end());
    }
  }
  data.value        = call.args.at("value_out").out;
  data.match        = lpm_lookup->get_local_symbol("lpm_lookup_match").expr;
  return data;
}

} // namespace

std::optional<spec_impl_t> LPMLookupFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *lpm_lookup = dynamic_cast<const Call *>(node);
  const call_t &call     = lpm_lookup->get_call();

  if (call.function_name != "lpm_lookup") {
    return {};
  }

  const lpm_lookup_data_t data = get_lpm_lookup_data(speculations.ctx, lpm_lookup);
  const addr_t obj             = data.obj;

  if (!speculations.ctx.can_impl_ds(obj, DSImpl::Tofino_LPM)) {
    return {};
  }

  if (!can_build_lpm(ep, node, obj, data.original_key, data.keys)) {
    return {};
  }

  Context new_ctx = speculations.ctx;
  new_ctx.save_ds_impl(node->get_id(), obj, DSImpl::Tofino_LPM);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> LPMLookupFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *lpm_lookup = dynamic_cast<const Call *>(node);
  const call_t &call     = lpm_lookup->get_call();

  if (call.function_name != "lpm_lookup") {
    return {};
  }

  const lpm_lookup_data_t data = get_lpm_lookup_data(ep->get_ctx(), lpm_lookup);
  const addr_t obj             = data.obj;

  if (!ep->get_ctx().can_impl_ds(obj, DSImpl::Tofino_LPM)) {
    return {};
  }

  LPM *lpm = build_lpm(ep, node, obj, data.original_key, data.keys);

  if (!lpm) {
    return {};
  }

  Module *module  = new LPMLookup(node, lpm->id, obj, data.original_key, data.keys, data.value, data.match);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(node->get_id(), obj, DSImpl::Tofino_LPM);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, obj, lpm);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> LPMLookupFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *lpm_lookup = dynamic_cast<const Call *>(node);
  const call_t &call     = lpm_lookup->get_call();

  if (call.function_name != "lpm_lookup") {
    return {};
  }

  const lpm_lookup_data_t data = get_lpm_lookup_data(ctx, lpm_lookup);
  const addr_t obj             = data.obj;

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_LPM)) {
    return {};
  }

  const std::unordered_set<Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_ds(obj);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const LPM *lpm = dynamic_cast<const LPM *>(*ds.begin());

  return std::make_unique<LPMLookup>(node, lpm->id, obj, data.original_key, data.keys, data.value, data.match);
}

} // namespace Tofino
} // namespace LibSynapse