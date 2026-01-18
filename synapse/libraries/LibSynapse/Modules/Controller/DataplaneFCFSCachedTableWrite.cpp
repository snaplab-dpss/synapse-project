#include <LibSynapse/Modules/Controller/DataplaneFCFSCachedTableWrite.h>
#include <LibSynapse/Modules/Tofino/FCFSCachedTableReadWrite.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibSynapse::Tofino::FCFSCachedTableReadWrite;

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

using Tofino::DS_ID;
using Tofino::Table;

namespace {

DS_ID get_fcfs_ct_id(const Context &ctx, addr_t obj) {
  const Tofino::TofinoContext *tofino_ctx                 = ctx.get_target_ctx<Tofino::TofinoContext>();
  const std::unordered_set<Tofino::DS *> &data_structures = tofino_ctx->get_data_structures().get_ds(obj);
  assert(data_structures.size() == 1 && "Multiple data structures found");
  const Tofino::DS *ds = *data_structures.begin();
  assert(ds->type == Tofino::DSType::FCFSCachedTable && "Not a FCFS cached table");
  return ds->id;
}

const FCFSCachedTableReadWrite *get_fcfs_ct_read_write_op(const EP *ep, const DS_ID &fcfs_ct_id) {
  const EPLeaf active_leaf = ep->get_active_leaf();
  const EPNode *prev       = active_leaf.node->get_prev();
  while (prev != nullptr) {
    const Module *module = prev->get_module();
    if (module->get_type() == ModuleType::Tofino_FCFSCachedTableReadWrite) {
      const FCFSCachedTableReadWrite *fcfs_ct_read_write = dynamic_cast<const FCFSCachedTableReadWrite *>(module);
      if (fcfs_ct_read_write->get_fcfs_ct_id() == fcfs_ct_id) {
        return fcfs_ct_read_write;
      }
    }
    prev = prev->get_prev();
  }

  return nullptr;
}

} // namespace

std::optional<spec_impl_t> DataplaneFCFSCachedTableWriteFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_put") {
    return {};
  }

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  const addr_t map_addr               = expr_addr_to_obj_addr(map_addr_expr);

  if (!ctx.check_ds_impl(map_addr, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneFCFSCachedTableWriteFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_put") {
    return {};
  }

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  const addr_t obj                    = expr_addr_to_obj_addr(map_addr_expr);

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  const DS_ID id                                        = get_fcfs_ct_id(ep->get_ctx(), obj);
  const FCFSCachedTableReadWrite *fcfs_ct_read_write_op = get_fcfs_ct_read_write_op(ep, id);

  if (!fcfs_ct_read_write_op) {
    return {};
  }

  Module *module  = new DataplaneFCFSCachedTableWrite(node, id, obj, fcfs_ct_read_write_op->get_keys(), fcfs_ct_read_write_op->get_write_value());
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneFCFSCachedTableWriteFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  // We don't need this for now.
  return {};
}

} // namespace Controller
} // namespace LibSynapse