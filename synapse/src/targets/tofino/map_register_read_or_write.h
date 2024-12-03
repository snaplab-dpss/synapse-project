#pragma once

#include "tofino_module.h"

#include "if.h"
#include "then.h"
#include "else.h"
#include "send_to_controller.h"

namespace tofino {

class MapRegisterReadOrWrite : public TofinoModule {
private:
  DS_ID map_register_id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> read_value;
  klee::ref<klee::Expr> write_value;
  symbol_t map_has_this_key;

public:
  MapRegisterReadOrWrite(const Node *node, DS_ID _map_register_id, addr_t _obj,
                         klee::ref<klee::Expr> _key,
                         klee::ref<klee::Expr> _read_value,
                         klee::ref<klee::Expr> _write_value,
                         const symbol_t &_map_has_this_key)
      : TofinoModule(ModuleType::Tofino_MapRegisterReadOrWrite,
                     "MapRegisterReadOrWrite", node),
        map_register_id(_map_register_id), obj(_obj), key(_key),
        read_value(_read_value), write_value(_write_value),
        map_has_this_key(_map_has_this_key) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned =
        new MapRegisterReadOrWrite(node, map_register_id, obj, key, read_value,
                                   write_value, map_has_this_key);
    return cloned;
  }

  DS_ID get_map_register_id() const { return map_register_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_read_value() const { return read_value; }
  klee::ref<klee::Expr> get_write_value() const { return write_value; }
  const symbol_t &get_map_has_this_key() const { return map_has_this_key; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    return {map_register_id};
  }
};

class MapRegisterReadOrWriteGenerator : public TofinoModuleGenerator {
public:
  MapRegisterReadOrWriteGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_MapRegisterReadOrWrite,
                              "MapRegisterReadOrWrite") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *map_get = static_cast<const Call *>(node);

    std::vector<const Call *> future_map_puts;
    if (!is_map_get_followed_by_map_puts_on_miss(ep->get_bdd(), map_get,
                                                 future_map_puts)) {
      // The cached table read should deal with these cases.
      return std::nullopt;
    }

    map_coalescing_objs_t map_objs;
    if (!get_map_coalescing_objs_from_map_op(ep, map_get, map_objs)) {
      return std::nullopt;
    }

    if (!ctx.can_impl_ds(map_objs.map, DSImpl::Tofino_MapRegister) ||
        !ctx.can_impl_ds(map_objs.dchain, DSImpl::Tofino_MapRegister) ||
        !ctx.can_impl_ds(map_objs.vector_key, DSImpl::Tofino_MapRegister)) {
      return std::nullopt;
    }

    map_register_data_t map_register_data =
        get_map_register_data(ep, map_get, future_map_puts);

    if (!can_build_or_reuse_map_register(ep, node, map_register_data.obj,
                                         map_register_data.key,
                                         map_register_data.num_entries)) {
      return std::nullopt;
    }

    Context new_ctx = ctx;
    new_ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_MapRegister);
    new_ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_MapRegister);
    new_ctx.save_ds_impl(map_objs.vector_key, DSImpl::Tofino_MapRegister);

    spec_impl_t spec_impl(decide(ep, node), new_ctx);

    std::vector<const Node *> ignore_nodes = get_nodes_to_speculatively_ignore(
        ep, map_get, map_objs, map_register_data.key);

    for (const Node *op : ignore_nodes) {
      spec_impl.skip.insert(op->get_id());
    }

    return spec_impl;
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::CALL) {
      return impls;
    }

    const Call *map_get = static_cast<const Call *>(node);

    std::vector<const Call *> future_map_puts;
    if (!is_map_get_followed_by_map_puts_on_miss(ep->get_bdd(), map_get,
                                                 future_map_puts)) {
      // The map register read should deal with these cases.
      return impls;
    }

    map_coalescing_objs_t map_objs;
    if (!get_map_coalescing_objs_from_map_op(ep, map_get, map_objs)) {
      return impls;
    }

    if (!ep->get_ctx().can_impl_ds(map_objs.map, DSImpl::Tofino_MapRegister) ||
        !ep->get_ctx().can_impl_ds(map_objs.dchain,
                                   DSImpl::Tofino_MapRegister) ||
        !ep->get_ctx().can_impl_ds(map_objs.vector_key,
                                   DSImpl::Tofino_MapRegister)) {
      return impls;
    }

    map_register_data_t map_register_data =
        get_map_register_data(ep, map_get, future_map_puts);

    MapRegister *map_register = build_or_reuse_map_register(
        ep, node, map_register_data.obj, map_register_data.key,
        map_register_data.num_entries);

    if (!map_register) {
      return impls;
    }

    Module *module = new MapRegisterReadOrWrite(
        node, map_register->id, map_register_data.obj, map_register_data.key,
        map_register_data.read_value, map_register_data.write_value,
        map_register_data.map_has_this_key);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    const Node *new_next;
    std::optional<constraints_t> deleted_branch_constraints;
    BDD *new_bdd =
        delete_coalescing_nodes(new_ep, node, map_objs, map_register_data.key,
                                new_next, deleted_branch_constraints);

    Context &ctx = new_ep->get_mutable_ctx();
    ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_MapRegister);
    ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_MapRegister);
    ctx.save_ds_impl(map_objs.vector_key, DSImpl::Tofino_MapRegister);

    if (deleted_branch_constraints.has_value()) {
      ctx.get_mutable_profiler().remove(deleted_branch_constraints.value());
    }

    TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
    tofino_ctx->place(new_ep, node, map_objs.map, map_register);

    EPLeaf leaf(ep_node, new_next);
    new_ep->process_leaf(ep_node, {leaf});
    new_ep->replace_bdd(new_bdd);
    new_ep->assert_integrity();

    return impls;
  }

private:
  struct map_register_data_t {
    addr_t obj;
    klee::ref<klee::Expr> key;
    klee::ref<klee::Expr> read_value;
    klee::ref<klee::Expr> write_value;
    symbol_t map_has_this_key;
    u32 num_entries;
  };

  map_register_data_t
  get_map_register_data(const EP *ep, const Call *map_get,
                        std::vector<const Call *> future_map_puts) const {
    map_register_data_t map_register_data;

    assert(!future_map_puts.empty());
    const Call *map_put = future_map_puts.front();

    const call_t &get_call = map_get->get_call();
    const call_t &put_call = map_put->get_call();

    symbols_t symbols = map_get->get_locally_generated_symbols();
    klee::ref<klee::Expr> obj_expr = get_call.args.at("map").expr;

    map_register_data.obj = expr_addr_to_obj_addr(obj_expr);
    map_register_data.key = get_call.args.at("key").in;
    map_register_data.read_value = get_call.args.at("value_out").out;
    map_register_data.write_value = put_call.args.at("value").expr;

    bool found = get_symbol(symbols, "map_has_this_key",
                            map_register_data.map_has_this_key);
    assert(found && "Symbol map_has_this_key not found");

    const Context &ctx = ep->get_ctx();
    const map_config_t &cfg = ctx.get_map_config(map_register_data.obj);

    map_register_data.num_entries = cfg.capacity;
    return map_register_data;
  }

  std::vector<const Node *>
  get_nodes_to_speculatively_ignore(const EP *ep, const Node *on_success,
                                    const map_coalescing_objs_t &map_objs,
                                    klee::ref<klee::Expr> key) const {
    std::vector<const Node *> nodes_to_ignore;

    const BDD *bdd = ep->get_bdd();
    std::vector<const Call *> coalescing_nodes =
        get_coalescing_nodes_from_key(bdd, on_success, key, map_objs);

    for (const Call *coalescing_node : coalescing_nodes) {
      nodes_to_ignore.push_back(coalescing_node);

      const call_t &call = coalescing_node->get_call();

      if (call.function_name == "dchain_allocate_new_index") {
        symbol_t out_of_space;
        bool found =
            get_symbol(coalescing_node->get_locally_generated_symbols(),
                       "out_of_space", out_of_space);
        assert(found && "Symbol out_of_space not found");

        const Branch *branch =
            find_branch_checking_index_alloc(ep, on_success, out_of_space);

        // FIXME: We ignore all logic happening when the index is not
        // successfuly allocated. This is a major simplification, as the NF
        // might be doing something useful here. We never encountered a scenario
        // where this was the case, but it could happen nonetheless.
        if (branch) {
          nodes_to_ignore.push_back(branch);

          bool direction_to_keep = true;
          const Node *next = direction_to_keep ? branch->get_on_false()
                                               : branch->get_on_true();

          next->visit_nodes([&nodes_to_ignore](const Node *node) {
            nodes_to_ignore.push_back(node);
            return NodeVisitAction::VISIT_CHILDREN;
          });
        }
      }
    }

    return nodes_to_ignore;
  }

  BDD *delete_coalescing_nodes(
      const EP *ep, const Node *node, const map_coalescing_objs_t &map_objs,
      klee::ref<klee::Expr> key, const Node *&new_next,
      std::optional<constraints_t> &deleted_branch_constraints) const {
    const BDD *old_bdd = ep->get_bdd();
    BDD *new_bdd = new BDD(*old_bdd);

    const Node *next = node->get_next();

    if (next) {
      new_next = new_bdd->get_node_by_id(next->get_id());
    } else {
      new_next = nullptr;
    }

    const std::vector<const Call *> targets =
        get_coalescing_nodes_from_key(new_bdd, new_next, key, map_objs);

    for (const Node *target : targets) {
      const Call *call_target = static_cast<const Call *>(target);
      const call_t &call = call_target->get_call();

      if (call.function_name == "dchain_allocate_new_index") {
        symbol_t out_of_space;
        bool found = get_symbol(call_target->get_locally_generated_symbols(),
                                "out_of_space", out_of_space);
        assert(found && "Symbol out_of_space not found");

        const Branch *branch =
            find_branch_checking_index_alloc(ep, node, out_of_space);

        if (branch) {
          assert(!deleted_branch_constraints.has_value() &&
                 "Multiple branch checking index allocation detected");
          deleted_branch_constraints = branch->get_ordered_branch_constraints();
          bool direction_to_keep = true;

          klee::ref<klee::Expr> extra_constraint = branch->get_condition();

          // If we want to keep the direction on true, we must remove the on
          // false.
          if (direction_to_keep) {
            extra_constraint =
                solver_toolbox.exprBuilder->Not(extra_constraint);
          }

          deleted_branch_constraints->push_back(extra_constraint);

          bool replace_next = (branch == next);
          Node *replacement = delete_branch_node_from_bdd(
              ep, new_bdd, branch->get_id(), direction_to_keep);
          if (replace_next) {
            new_next = replacement;
          }
        }
      }

      bool replace_next = (target == next);
      Node *replacement =
          delete_non_branch_node_from_bdd(ep, new_bdd, target->get_id());
      if (replace_next) {
        new_next = replacement;
      }
    }

    return new_bdd;
  }
};

} // namespace tofino
