#pragma once

#include "tofino_module.h"

namespace tofino {

class VectorRegisterLookup : public TofinoModule {
private:
  std::unordered_set<DS_ID> rids;
  addr_t obj;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> value;

public:
  VectorRegisterLookup(const Node *node, const std::unordered_set<DS_ID> &_rids,
                       addr_t _obj, klee::ref<klee::Expr> _index,
                       klee::ref<klee::Expr> _value)
      : TofinoModule(ModuleType::Tofino_VectorRegisterLookup,
                     "VectorRegisterLookup", node),
        rids(_rids), obj(_obj), index(_index), value(_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new VectorRegisterLookup(node, rids, obj, index, value);
    return cloned;
  }

  const std::unordered_set<DS_ID> &get_rids() const { return rids; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_value() const { return value; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    return rids;
  }
};

class VectorRegisterLookupGenerator : public TofinoModuleGenerator {
public:
  VectorRegisterLookupGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_VectorRegisterLookup,
                              "VectorRegisterLookup") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::Call) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "vector_borrow") {
      return std::nullopt;
    }

    if (!is_vector_read(call_node)) {
      return std::nullopt;
    }

    vector_register_data_t vector_register_data =
        get_vector_register_data(ep, call_node);

    if (!ctx.can_impl_ds(vector_register_data.obj,
                         DSImpl::Tofino_VectorRegister)) {
      return std::nullopt;
    }

    if (!can_build_or_reuse_vector_registers(ep, call_node,
                                             vector_register_data)) {
      return std::nullopt;
    }

    const Node *vector_return =
        get_future_vector_return(ep, node, vector_register_data.obj);

    Context new_ctx = ctx;
    new_ctx.save_ds_impl(vector_register_data.obj,
                         DSImpl::Tofino_VectorRegister);

    spec_impl_t spec_impl(decide(ep, node), new_ctx);

    if (vector_return) {
      spec_impl.skip.insert(vector_return->get_id());
    }

    return spec_impl;
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::Call) {
      return impls;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "vector_borrow") {
      return impls;
    }

    if (!is_vector_read(call_node)) {
      return impls;
    }

    vector_register_data_t vector_register_data =
        get_vector_register_data(ep, call_node);

    if (!ep->get_ctx().can_impl_ds(vector_register_data.obj,
                                   DSImpl::Tofino_VectorRegister)) {
      return impls;
    }

    std::unordered_set<Register *> regs =
        build_or_reuse_vector_registers(ep, call_node, vector_register_data);

    if (regs.empty()) {
      return impls;
    }

    std::unordered_set<DS_ID> rids;
    for (DS *reg : regs) {
      rids.insert(reg->id);
    }

    Module *module = new VectorRegisterLookup(
        node, rids, vector_register_data.obj, vector_register_data.index,
        vector_register_data.value);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    const Node *new_next;
    BDD *bdd = delete_future_vector_return(new_ep, node,
                                           vector_register_data.obj, new_next);

    Context &ctx = new_ep->get_mutable_ctx();
    ctx.save_ds_impl(vector_register_data.obj, DSImpl::Tofino_VectorRegister);

    TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);

    for (Register *reg : regs) {
      tofino_ctx->place(new_ep, node, vector_register_data.obj, reg);
    }

    EPLeaf leaf(ep_node, new_next);
    new_ep->process_leaf(ep_node, {leaf});
    new_ep->replace_bdd(bdd);
    new_ep->assert_integrity();

    return impls;
  }

private:
  vector_register_data_t get_vector_register_data(const EP *ep,
                                                  const Call *node) const {

    const call_t &call = node->get_call();

    klee::ref<klee::Expr> obj_expr = call.args.at("vector").expr;
    klee::ref<klee::Expr> index = call.args.at("index").expr;
    klee::ref<klee::Expr> value = call.extra_vars.at("borrowed_cell").second;

    addr_t obj = expr_addr_to_obj_addr(obj_expr);

    const Context &ctx = ep->get_ctx();
    const vector_config_t &cfg = ctx.get_vector_config(obj);

    vector_register_data_t vector_register_data = {
        .obj = obj,
        .num_entries = static_cast<u32>(cfg.capacity),
        .index = index,
        .value = value,
        .actions = {RegisterAction::READ, RegisterAction::SWAP},
    };

    return vector_register_data;
  }

  const Call *get_future_vector_return(const EP *ep, const Node *node,
                                       addr_t vector) const {
    std::vector<const Call *> ops =
        get_future_functions(node, {"vector_return"});

    for (const Call *op : ops) {
      const call_t &call = op->get_call();
      assert(call.function_name == "vector_return");

      klee::ref<klee::Expr> obj_expr = call.args.at("vector").expr;
      addr_t obj = expr_addr_to_obj_addr(obj_expr);

      if (obj != vector) {
        continue;
      }

      return op;
    }

    return nullptr;
  }

  BDD *delete_future_vector_return(EP *ep, const Node *node, addr_t vector,
                                   const Node *&new_next) const {
    const BDD *old_bdd = ep->get_bdd();
    BDD *new_bdd = new BDD(*old_bdd);

    const Node *next = node->get_next();

    if (next) {
      new_next = new_bdd->get_node_by_id(next->get_id());
    } else {
      new_next = nullptr;
    }

    std::vector<const Call *> ops =
        get_future_functions(node, {"vector_return"});

    for (const Call *op : ops) {
      const call_t &call = op->get_call();
      assert(call.function_name == "vector_return");

      klee::ref<klee::Expr> obj_expr = call.args.at("vector").expr;
      addr_t obj = expr_addr_to_obj_addr(obj_expr);

      if (obj != vector) {
        continue;
      }

      bool replace_next = (op == next);
      Node *replacement =
          delete_non_branch_node_from_bdd(ep, new_bdd, op->get_id());

      if (replace_next) {
        new_next = replacement;
      }
    }

    return new_bdd;
  }
};

} // namespace tofino
