#include <LibSynapse/Modules/Tofino/DnsGetResponse.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct dns_get_response_data_t {
  addr_t msg_addr;
  klee::ref<klee::Expr> length;
  klee::ref<klee::Expr> dns_name;
  klee::ref<klee::Expr> address;
  klee::ref<klee::Expr> found;
};

dns_get_response_data_t get_data(const Call *dns_get_response) {
  const call_t &call = dns_get_response->get_call();

  dns_get_response_data_t data;
  data.msg_addr = expr_addr_to_obj_addr(call.args.at("msg").expr);
  data.length   = call.args.at("length").expr;
  data.dns_name     = call.args.at("name").out;
  data.address  = call.args.at("address").out;
  data.found    = dns_get_response->get_local_symbol("dns_response_found").expr;
  return data;
}

} // namespace

std::vector<klee::ref<klee::Expr>> DnsGetResponse::name_fields(klee::ref<klee::Expr> name) {
  assert(name->getWidth() == NAME_BYTES * 8 && "Unexpected dns_name width");

  auto field = [name](bytes_t offset, bytes_t size) -> klee::ref<klee::Expr> {
    return solver_toolbox.exprBuilder->Extract(name, offset * 8, size * 8);
  };

  std::vector<klee::ref<klee::Expr>> fields{field(0, 1)};
  for (u32 slot = 0; slot < MAX_LABELS; slot++) {
    const bytes_t base = 1 + slot * SLOT_BYTES;
    fields.push_back(field(base, 1));
    for (const slot_chunk_t &chunk : SLOT_CHUNKS) {
      fields.push_back(field(base + chunk.offset, chunk.size));
    }
  }

  return fields;
}

std::optional<spec_impl_t> DnsGetResponseFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "dns_get_response") {
    return {};
  }

  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> DnsGetResponseFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "dns_get_response") {
    return {};
  }

  const dns_get_response_data_t data = get_data(call_node);

  Module *module  = new DnsGetResponse(node, data.msg_addr, data.length, data.dns_name, data.address, data.found);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DnsGetResponseFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "dns_get_response") {
    return {};
  }

  const dns_get_response_data_t data = get_data(call_node);
  return std::make_unique<DnsGetResponse>(node, data.msg_addr, data.length, data.dns_name, data.address, data.found);
}

} // namespace Tofino
} // namespace LibSynapse
