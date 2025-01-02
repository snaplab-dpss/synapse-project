#include "hh_table_update.h"
#include "hh_table_read.h"
#include "../tofino/hh_table_read.h"

namespace synapse {
namespace controller {

using tofino::Table;

namespace {
struct table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  std::vector<klee::ref<klee::Expr>> table_keys;
  klee::ref<klee::Expr> value;

  table_data_t(const Call *map_put) {
    const call_t &call = map_put->get_call();
    ASSERT(call.function_name == "map_put", "Not a map_put call");

    obj = expr_addr_to_obj_addr(call.args.at("map").expr);
    key = call.args.at("key").in;
    table_keys = Table::build_keys(key);
    value = call.args.at("value").expr;
  }
};

klee::ref<klee::Expr> get_min_estimate(const EP *ep) {
  EPLeaf leaf = ep->get_active_leaf();
  const EPNode *node = leaf.node;

  while (node) {
    if (node->get_module()->get_type() == ModuleType::Tofino_HHTableRead) {
      const tofino::HHTableRead *hh_table_read =
          dynamic_cast<const tofino::HHTableRead *>(node->get_module());
      return hh_table_read->get_min_estimate();
    } else if (node->get_module()->get_type() == ModuleType::Controller_HHTableRead) {
      const controller::HHTableRead *hh_table_read =
          dynamic_cast<const controller::HHTableRead *>(node->get_module());
      return hh_table_read->get_min_estimate();
    }
    node = node->get_prev();
  }

  return nullptr;
}
} // namespace

using tofino::Table;

std::optional<spec_impl_t> HHTableUpdateFactory::speculate(const EP *ep, const Node *node,
                                                           const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *map_put = dynamic_cast<const Call *>(node);
  const call_t &call = map_put->get_call();

  if (call.function_name != "map_put") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;
  addr_t obj = expr_addr_to_obj_addr(obj_expr);

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> HHTableUpdateFactory::process_node(const EP *ep,
                                                       const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *map_put = dynamic_cast<const Call *>(node);
  const call_t &call = map_put->get_call();

  if (call.function_name != "map_put") {
    return impls;
  }

  klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;
  addr_t obj = expr_addr_to_obj_addr(obj_expr);

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    return impls;
  }

  klee::ref<klee::Expr> min_estimate = get_min_estimate(ep);
  ASSERT(!min_estimate.isNull(), "TODO: HHTableRead not found, so we should "
                                 "query the CMS for the min estimate");

  table_data_t table_data(map_put);

  Module *module = new HHTableUpdate(node, table_data.obj, table_data.table_keys,
                                     table_data.value, min_estimate);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace controller
} // namespace synapse