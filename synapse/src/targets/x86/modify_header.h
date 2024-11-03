#pragma once

#include "x86_module.h"

namespace x86 {

class ModifyHeader : public x86Module {
private:
  addr_t chunk_addr;
  std::vector<mod_t> changes;

public:
  ModifyHeader(const Node *node, addr_t _chunk_addr,
               const std::vector<mod_t> &_changes)
      : x86Module(ModuleType::x86_ModifyHeader, "ModifyHeader", node),
        chunk_addr(_chunk_addr), changes(_changes) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    ModifyHeader *cloned = new ModifyHeader(node, chunk_addr, changes);
    return cloned;
  }

  addr_t get_chunk_addr() const { return chunk_addr; }
  const std::vector<mod_t> &get_changes() const { return changes; }
};

class ModifyHeaderGenerator : public x86ModuleGenerator {
public:
  ModifyHeaderGenerator()
      : x86ModuleGenerator(ModuleType::x86_ModifyHeader, "ModifyHeader") {}

protected:
  bool bdd_node_match_pattern(const Node *node) const {
    if (node->get_type() != NodeType::CALL) {
      return false;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "packet_return_chunk") {
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

    const Call *packet_return_chunk = static_cast<const Call *>(node);
    const call_t &call = packet_return_chunk->get_call();

    if (call.function_name != "packet_return_chunk") {
      return impls;
    }

    const Call *packet_borrow_chunk =
        packet_borrow_from_return(ep, packet_return_chunk);
    assert(packet_borrow_chunk &&
           "Failed to find packet_borrow_next_chunk from packet_return_chunk");

    klee::ref<klee::Expr> hdr = call.args.at("the_chunk").expr;
    addr_t hdr_addr = expr_addr_to_obj_addr(hdr);

    std::vector<mod_t> changes =
        build_hdr_modifications(packet_borrow_chunk, packet_return_chunk);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    if (changes.size() == 0) {
      new_ep->process_leaf(node->get_next());
      return impls;
    }

    Module *module = new ModifyHeader(node, hdr_addr, changes);
    EPNode *ep_node = new EPNode(module);

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }
};

} // namespace x86
