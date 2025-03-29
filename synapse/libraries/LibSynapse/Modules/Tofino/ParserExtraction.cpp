#include <LibSynapse/Modules/Tofino/ParserExtraction.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

std::optional<spec_impl_t> ParserExtractionFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "packet_borrow_next_chunk") {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> ParserExtractionFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                          LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "packet_borrow_next_chunk") {
    return impls;
  }

  klee::ref<klee::Expr> hdr_addr_expr = call.args.at("chunk").out;
  klee::ref<klee::Expr> hdr           = call.extra_vars.at("the_chunk").second;
  klee::ref<klee::Expr> length_expr   = call.args.at("length").expr;

  // Relevant for IPv4 options, but left for future work.
  assert(!call_node->is_hdr_parse_with_var_len() && "Not implemented");
  const bytes_t length  = LibCore::solver_toolbox.value_from_expr(length_expr);
  const addr_t hdr_addr = LibCore::expr_addr_to_obj_addr(hdr_addr_expr);

  const std::vector<LibCore::expr_struct_t> &headers = ep->get_ctx().get_expr_structs();
  std::vector<klee::ref<klee::Expr>> hdr_fields_guess;
  for (const LibCore::expr_struct_t &header : headers) {
    if (LibCore::solver_toolbox.are_exprs_always_equal(header.expr, hdr)) {
      hdr_fields_guess = header.fields;
      break;
    }
  }

  Module *module  = new ParserExtraction(node, hdr_addr, hdr, length, hdr_fields_guess);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> ParserExtractionFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "packet_borrow_next_chunk") {
    return {};
  }

  klee::ref<klee::Expr> hdr_addr_expr = call.args.at("chunk").out;
  klee::ref<klee::Expr> hdr           = call.extra_vars.at("the_chunk").second;
  klee::ref<klee::Expr> length_expr   = call.args.at("length").expr;

  const addr_t hdr_addr = LibCore::expr_addr_to_obj_addr(hdr_addr_expr);
  const bytes_t length  = LibCore::solver_toolbox.value_from_expr(length_expr);

  const std::vector<LibCore::expr_struct_t> &headers = ctx.get_expr_structs();
  std::vector<klee::ref<klee::Expr>> hdr_fields_guess;
  for (const LibCore::expr_struct_t &header : headers) {
    if (LibCore::solver_toolbox.are_exprs_always_equal(header.expr, hdr)) {
      hdr_fields_guess = header.fields;
      break;
    }
  }

  return std::make_unique<ParserExtraction>(node, hdr_addr, hdr, length, hdr_fields_guess);
}

} // namespace Tofino
} // namespace LibSynapse