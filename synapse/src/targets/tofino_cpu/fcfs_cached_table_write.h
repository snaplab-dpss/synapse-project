#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

using namespace tofino;

class FCFSCachedTableWrite : public TofinoCPUModule {
private:
  DS_ID id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;

public:
  FCFSCachedTableWrite(const Node *node, DS_ID _id, addr_t _obj,
                       const std::vector<klee::ref<klee::Expr>> &_keys,
                       klee::ref<klee::Expr> _value)
      : TofinoCPUModule(ModuleType::TofinoCPU_FCFSCachedTableWrite,
                        "FCFSCachedTableWrite", node),
        id(_id), obj(_obj), keys(_keys), value(_value) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    FCFSCachedTableWrite *cloned =
        new FCFSCachedTableWrite(node, id, obj, keys, value);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
};

class FCFSCachedTableWriteGenerator : public TofinoCPUModuleGenerator {
public:
  FCFSCachedTableWriteGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_FCFSCachedTableWrite,
                                 "FCFSCachedTableWrite") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "map_put") {
      return std::nullopt;
    }

    if (!can_place(ep, call_node, "map",
                   PlacementDecision::Tofino_FCFSCachedTable)) {
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

    if (call.function_name != "map_put") {
      return impls;
    }

    if (!check_placement(ep, call_node, "map",
                         PlacementDecision::Tofino_FCFSCachedTable)) {
      return impls;
    }

    addr_t obj;
    std::vector<klee::ref<klee::Expr>> keys;
    klee::ref<klee::Expr> value;
    get_data(call_node, obj, keys, value);

    DS_ID id = get_cached_table_id(ep, obj);

    Module *module = new FCFSCachedTableWrite(node, id, obj, keys, value);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }

private:
  DS_ID get_cached_table_id(const EP *ep, addr_t obj) const {
    const Context &ctx = ep->get_ctx();
    const tofino::TofinoContext *tofino_ctx =
        ctx.get_target_ctx<tofino::TofinoContext>();
    const std::unordered_set<tofino::DS *> &data_structures =
        tofino_ctx->get_ds(obj);
    assert(data_structures.size() == 1);
    tofino::DS *ds = *data_structures.begin();
    assert(ds->type == tofino::DSType::FCFS_CACHED_TABLE);
    return ds->id;
  }

  void get_data(const Call *call_node, addr_t &obj,
                std::vector<klee::ref<klee::Expr>> &keys,
                klee::ref<klee::Expr> &value) const {
    const call_t &call = call_node->get_call();
    assert(call.function_name == "map_put");

    klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;
    value = call.args.at("value").expr;

    obj = expr_addr_to_obj_addr(map_addr_expr);
    keys = Table::build_keys(key);
    value = value;
  }
};

} // namespace tofino_cpu
