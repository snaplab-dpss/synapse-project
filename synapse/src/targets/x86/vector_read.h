#pragma once

#include "x86_module.h"

namespace x86 {

class VectorRead : public x86Module {
private:
  addr_t vector_addr;
  klee::ref<klee::Expr> index;
  addr_t value_addr;
  klee::ref<klee::Expr> value;

public:
  VectorRead(const Node *node, addr_t _vector_addr,
             klee::ref<klee::Expr> _index, addr_t _value_addr,
             klee::ref<klee::Expr> _value)
      : x86Module(ModuleType::x86_VectorRead, "VectorRead", node),
        vector_addr(_vector_addr), index(_index), value_addr(_value_addr),
        value(_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned =
        new VectorRead(node, vector_addr, index, value_addr, value);
    return cloned;
  }

  addr_t get_vector_addr() const { return vector_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  addr_t get_value_addr() const { return value_addr; }
  klee::ref<klee::Expr> get_value() const { return value; }
};

class VectorReadGenerator : public x86ModuleGenerator {
public:
  VectorReadGenerator()
      : x86ModuleGenerator(ModuleType::x86_VectorRead, "VectorRead") {}

protected:
  bool bdd_node_match_pattern(const Node *node) const {
    if (node->get_type() != NodeType::CALL) {
      return false;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "vector_borrow") {
      return false;
    }

    return true;
  }

  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (!bdd_node_match_pattern(node)) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
    addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);

    if (!ctx.can_impl_ds(vector_addr, DSImpl::x86_Vector)) {
      return std::nullopt;
    }

    return spec_impl_t(decide(ep, node), ctx);
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (!bdd_node_match_pattern(node)) {
      return impls;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
    klee::ref<klee::Expr> index = call.args.at("index").expr;
    klee::ref<klee::Expr> value_addr_expr = call.args.at("val_out").out;
    klee::ref<klee::Expr> value = call.extra_vars.at("borrowed_cell").second;

    addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);
    addr_t value_addr = expr_addr_to_obj_addr(value_addr_expr);

    if (!ep->get_ctx().can_impl_ds(vector_addr, DSImpl::x86_Vector)) {
      return impls;
    }

    Module *module =
        new VectorRead(node, vector_addr, index, value_addr, value);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    new_ep->get_mutable_ctx().save_ds_impl(vector_addr, DSImpl::x86_Vector);

    return impls;
  }
};

} // namespace x86