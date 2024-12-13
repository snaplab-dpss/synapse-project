#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class ModifyHeader : public TofinoCPUModule {
private:
  addr_t chunk_addr;
  klee::ref<klee::Expr> original_chunk;
  std::vector<mod_t> changes;

public:
  ModifyHeader(const Node *node, addr_t _chunk_addr,
               const std::vector<mod_t> &_changes)
      : TofinoCPUModule(ModuleType::TofinoCPU_ModifyHeader, "ModifyHeader",
                        node),
        chunk_addr(_chunk_addr), changes(_changes) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    ModifyHeader *cloned = new ModifyHeader(node, chunk_addr, changes);
    return cloned;
  }

  addr_t get_chunk_addr() const { return chunk_addr; }
  const std::vector<mod_t> &get_changes() const { return changes; }
};

class ModifyHeaderGenerator : public TofinoCPUModuleGenerator {
public:
  ModifyHeaderGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_ModifyHeader,
                                 "ModifyHeader") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::Call) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "packet_return_chunk") {
      return std::nullopt;
    }

    return spec_impl_t(decide(ep, node), ctx);
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::Call) {
      return impls;
    }

    const Call *packet_return_chunk = static_cast<const Call *>(node);
    const call_t &call = packet_return_chunk->get_call();

    if (call.function_name != "packet_return_chunk") {
      return impls;
    }

    const Call *packet_borrow_chunk =
        packet_borrow_from_return(ep, packet_return_chunk);
    ASSERT(packet_borrow_chunk,
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

} // namespace tofino_cpu
