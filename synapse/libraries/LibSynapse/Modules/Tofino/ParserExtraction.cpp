#include <LibSynapse/Modules/Tofino/ParserExtraction.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> ParserExtractionFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "packet_borrow_next_chunk") {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> ParserExtractionFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "packet_borrow_next_chunk") {
    return {};
  }

  klee::ref<klee::Expr> hdr_addr_expr = call.args.at("chunk").out;
  klee::ref<klee::Expr> hdr           = call.extra_vars.at("the_chunk").second;
  klee::ref<klee::Expr> length_expr   = call.args.at("length").expr;

  // Relevant for IPv4 options, but left for future work.
  assert(!call_node->is_hdr_parse_with_var_len() && "Not implemented");
  const bytes_t length  = solver_toolbox.value_from_expr(length_expr);
  const addr_t hdr_addr = expr_addr_to_obj_addr(hdr_addr_expr);

  const std::vector<expr_struct_t> &headers = ep->get_ctx().get_expr_structs();
  std::vector<klee::ref<klee::Expr>> hdr_fields_guess;
  for (const expr_struct_t &header : headers) {
    if (solver_toolbox.are_exprs_always_equal(header.expr, hdr)) {
      hdr_fields_guess = header.fields;
      break;
    }
  }

  Module *module  = new ParserExtraction(ep->get_placement(node->get_id()), node, hdr_addr, hdr, length, hdr_fields_guess);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> ParserExtractionFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "packet_borrow_next_chunk") {
    return {};
  }

  klee::ref<klee::Expr> hdr_addr_expr = call.args.at("chunk").out;
  klee::ref<klee::Expr> hdr           = call.extra_vars.at("the_chunk").second;
  klee::ref<klee::Expr> length_expr   = call.args.at("length").expr;

  const addr_t hdr_addr = expr_addr_to_obj_addr(hdr_addr_expr);
  const bytes_t length  = solver_toolbox.value_from_expr(length_expr);

  const std::vector<expr_struct_t> &headers = ctx.get_expr_structs();
  std::vector<klee::ref<klee::Expr>> hdr_fields_guess;
  for (const expr_struct_t &header : headers) {
    if (solver_toolbox.are_exprs_always_equal(header.expr, hdr)) {
      hdr_fields_guess = header.fields;
      break;
    }
  }

  return std::make_unique<ParserExtraction>(get_type().instance_id, node, hdr_addr, hdr, length, hdr_fields_guess);
}

} // namespace Tofino
} // namespace LibSynapse