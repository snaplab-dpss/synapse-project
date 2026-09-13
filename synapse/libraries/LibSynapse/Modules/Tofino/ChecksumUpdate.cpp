#include <LibSynapse/Modules/Tofino/ChecksumUpdate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

bool is_checksum_update(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }
  const Call *call_node = dynamic_cast<const Call *>(node);
  return call_node->get_call().function_name == "nf_set_rte_ipv4_udptcp_checksum";
}

std::unique_ptr<ChecksumUpdate> build(const BDDNode *node) {
  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  const addr_t ip_hdr_addr = expr_addr_to_obj_addr(call.args.at("ip_header").expr);
  const addr_t l4_hdr_addr = expr_addr_to_obj_addr(call.args.at("l4_header").expr);
  const symbol_t checksum  = call_node->get_local_symbol("checksum");

  return std::make_unique<ChecksumUpdate>(node, ip_hdr_addr, l4_hdr_addr, checksum);
}

} // namespace

std::optional<spec_impl_t> ChecksumUpdateFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (!is_checksum_update(node)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> ChecksumUpdateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!is_checksum_update(node)) {
    return {};
  }

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Module *module  = build(node).release();
  EPNode *ep_node = new EPNode(module);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> ChecksumUpdateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!is_checksum_update(node)) {
    return {};
  }

  return build(node);
}

} // namespace Tofino
} // namespace LibSynapse
