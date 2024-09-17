#pragma once

#include "x86_module.h"

namespace x86 {

class ParseHeader : public x86Module {
private:
  addr_t chunk_addr;
  klee::ref<klee::Expr> chunk;
  klee::ref<klee::Expr> length;

public:
  ParseHeader(const Node *node, addr_t _chunk_addr,
              klee::ref<klee::Expr> _chunk, klee::ref<klee::Expr> _length)
      : x86Module(ModuleType::x86_ParseHeader, "ParseHeader", node),
        chunk_addr(_chunk_addr), chunk(_chunk), length(_length) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    ParseHeader *cloned = new ParseHeader(node, chunk_addr, chunk, length);
    return cloned;
  }

  addr_t get_chunk_addr() const { return chunk_addr; }
  klee::ref<klee::Expr> get_chunk() const { return chunk; }
  klee::ref<klee::Expr> get_length() const { return length; }
};

class ParseHeaderGenerator : public x86ModuleGenerator {
public:
  ParseHeaderGenerator()
      : x86ModuleGenerator(ModuleType::x86_ParseHeader, "ParseHeader") {}

protected:
  bool bdd_node_match_pattern(const Node *node) const {
    if (node->get_type() != NodeType::CALL) {
      return false;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "packet_borrow_next_chunk") {
      return false;
    }

    return true;
  }

  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (bdd_node_match_pattern(node))
      return spec_impl_t(decide(ep, node), ctx);
    return std::nullopt;
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (!bdd_node_match_pattern(node)) {
      return impls;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    klee::ref<klee::Expr> chunk = call.args.at("chunk").out;
    klee::ref<klee::Expr> out_chunk = call.extra_vars.at("the_chunk").second;
    klee::ref<klee::Expr> length = call.args.at("length").expr;

    addr_t chunk_addr = expr_addr_to_obj_addr(chunk);

    Module *module = new ParseHeader(node, chunk_addr, out_chunk, length);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }
};

} // namespace x86
