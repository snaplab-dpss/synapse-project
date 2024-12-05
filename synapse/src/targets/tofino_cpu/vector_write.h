#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class VectorWrite : public TofinoCPUModule {
private:
  addr_t vector_addr;
  klee::ref<klee::Expr> index;
  addr_t value_addr;
  std::vector<mod_t> modifications;

public:
  VectorWrite(const Node *node, addr_t _vector_addr,
              klee::ref<klee::Expr> _index, addr_t _value_addr,
              const std::vector<mod_t> &_modifications)
      : TofinoCPUModule(ModuleType::TofinoCPU_VectorWrite, "VectorWrite", node),
        vector_addr(_vector_addr), index(_index), value_addr(_value_addr),
        modifications(_modifications) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned =
        new VectorWrite(node, vector_addr, index, value_addr, modifications);
    return cloned;
  }

  addr_t get_vector_addr() const { return vector_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  addr_t get_value_addr() const { return value_addr; }

  const std::vector<mod_t> &get_modifications() const { return modifications; }
};

class VectorWriteGenerator : public TofinoCPUModuleGenerator {
public:
  VectorWriteGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_VectorWrite,
                                 "VectorWrite") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "vector_return") {
      return std::nullopt;
    }

    klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
    addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);

    if (!ctx.can_impl_ds(vector_addr, DSImpl::TofinoCPU_Vector)) {
      return std::nullopt;
    }

    return spec_impl_t(decide(ep, node), ctx);
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::CALL) {
      return impls;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "vector_return") {
      return impls;
    }

    klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
    klee::ref<klee::Expr> index = call.args.at("index").expr;
    klee::ref<klee::Expr> value_addr_expr = call.args.at("value").expr;
    klee::ref<klee::Expr> value = call.args.at("value").in;

    addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);
    addr_t value_addr = expr_addr_to_obj_addr(value_addr_expr);

    // We don't need to place the vector, we will never get a vector_return
    // before a vector_borrow.
    if (!ep->get_ctx().check_ds_impl(vector_addr, DSImpl::TofinoCPU_Vector)) {
      return impls;
    }

    klee::ref<klee::Expr> original_value =
        get_original_vector_value(ep, node, vector_addr);
    std::vector<mod_t> changes = build_expr_mods(original_value, value);

    // Check the Ignore module.
    if (changes.empty()) {
      return impls;
    }

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    Module *module =
        new VectorWrite(node, vector_addr, index, value_addr, changes);
    EPNode *ep_node = new EPNode(module);

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }
};

} // namespace tofino_cpu
