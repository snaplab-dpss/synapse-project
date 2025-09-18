#include <LibSynapse/Modules/x86/ChecksumUpdate.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace x86 {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {
bool bdd_node_match_pattern(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "nf_set_rte_ipv4_udptcp_checksum") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> ChecksumUpdateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return {};
}

std::vector<impl_t> ChecksumUpdateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> ip_hdr_addr_expr = call.args.at("ip_header").expr;
  klee::ref<klee::Expr> l4_hdr_addr_expr = call.args.at("l4_header").expr;
  klee::ref<klee::Expr> p                = call.args.at("packet").expr;

  const symbol_t checksum  = call_node->get_local_symbol("checksum");
  const addr_t ip_hdr_addr = expr_addr_to_obj_addr(ip_hdr_addr_expr);
  const addr_t l4_hdr_addr = expr_addr_to_obj_addr(l4_hdr_addr_expr);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Module *module  = new ChecksumUpdate(get_type().instance_id, node, ip_hdr_addr, l4_hdr_addr, checksum);
  EPNode *ep_node = new EPNode(module);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> ChecksumUpdateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> ip_hdr_addr_expr = call.args.at("ip_header").expr;
  klee::ref<klee::Expr> l4_hdr_addr_expr = call.args.at("l4_header").expr;
  klee::ref<klee::Expr> p                = call.args.at("packet").expr;

  const symbol_t checksum  = call_node->get_local_symbol("checksum");
  const addr_t ip_hdr_addr = expr_addr_to_obj_addr(ip_hdr_addr_expr);
  const addr_t l4_hdr_addr = expr_addr_to_obj_addr(l4_hdr_addr_expr);

  return std::make_unique<ChecksumUpdate>(get_type().instance_id, node, ip_hdr_addr, l4_hdr_addr, checksum);
}

} // namespace x86
} // namespace LibSynapse