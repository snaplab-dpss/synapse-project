#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class VectorRegisterUpdate : public TofinoCPUModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> index;
  addr_t value_addr;
  std::vector<modification_t> modifications;

public:
  VectorRegisterUpdate(const Node *node, addr_t _obj,
                       klee::ref<klee::Expr> _index, addr_t _value_addr,
                       const std::vector<modification_t> &_modifications)
      : TofinoCPUModule(ModuleType::TofinoCPU_VectorRegisterUpdate,
                        "VectorRegisterUpdate", node),
        obj(_obj), index(_index), value_addr(_value_addr),
        modifications(_modifications) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned =
        new VectorRegisterUpdate(node, obj, index, value_addr, modifications);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
  addr_t get_value_addr() const { return value_addr; }

  const std::vector<modification_t> &get_modifications() const {
    return modifications;
  }
};

class VectorRegisterUpdateGenerator : public TofinoCPUModuleGenerator {
public:
  VectorRegisterUpdateGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_VectorRegisterUpdate,
                                 "VectorRegisterUpdate") {}

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

    if (!ctx.can_impl_ds(vector_addr, DSImpl::Tofino_VectorRegister)) {
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

    klee::ref<klee::Expr> obj_expr = call.args.at("vector").expr;
    klee::ref<klee::Expr> index = call.args.at("index").expr;
    klee::ref<klee::Expr> value_addr_expr = call.args.at("value").expr;
    klee::ref<klee::Expr> value = call.args.at("value").in;

    addr_t obj = expr_addr_to_obj_addr(obj_expr);
    addr_t value_addr = expr_addr_to_obj_addr(value_addr_expr);

    if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_VectorRegister)) {
      return impls;
    }

    klee::ref<klee::Expr> original_value =
        get_original_vector_value(ep, node, obj);
    std::vector<modification_t> changes =
        build_modifications(original_value, value);

    // Check the Ignore module.
    if (changes.empty()) {
      return impls;
    }

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    Module *module =
        new VectorRegisterUpdate(node, obj, index, value_addr, changes);
    EPNode *ep_node = new EPNode(module);

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }
};

} // namespace tofino_cpu
