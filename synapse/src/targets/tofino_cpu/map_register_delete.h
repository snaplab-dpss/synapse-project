#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

using namespace tofino;

class MapRegisterDelete : public TofinoCPUModule {
private:
  DS_ID id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;

public:
  MapRegisterDelete(const Node *node, DS_ID _id, addr_t _obj,
                    const std::vector<klee::ref<klee::Expr>> &_keys)
      : TofinoCPUModule(ModuleType::TofinoCPU_MapRegisterDelete,
                        "MapRegisterDelete", node),
        id(_id), obj(_obj), keys(_keys) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    MapRegisterDelete *cloned = new MapRegisterDelete(node, id, obj, keys);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
};

class MapRegisterDeleteGenerator : public TofinoCPUModuleGenerator {
public:
  MapRegisterDeleteGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_MapRegisterDelete,
                                 "MapRegisterDelete") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::Call) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "map_erase") {
      return std::nullopt;
    }

    klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
    addr_t map_addr = expr_addr_to_obj_addr(map_addr_expr);

    if (!ctx.can_impl_ds(map_addr, DSImpl::Tofino_MapRegister)) {
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

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "map_erase") {
      return impls;
    }

    addr_t obj;
    std::vector<klee::ref<klee::Expr>> keys;
    get_map_erase_data(call_node, obj, keys);

    if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_MapRegister)) {
      return impls;
    }

    DS_ID id = get_map_register_id(ep, obj);

    Module *module = new MapRegisterDelete(node, id, obj, keys);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }

private:
  DS_ID get_map_register_id(const EP *ep, addr_t obj) const {
    const Context &ctx = ep->get_ctx();
    const tofino::TofinoContext *tofino_ctx =
        ctx.get_target_ctx<tofino::TofinoContext>();
    const std::unordered_set<tofino::DS *> &data_structures =
        tofino_ctx->get_ds(obj);
    ASSERT(data_structures.size() == 1, "Multiple data structures found");
    tofino::DS *ds = *data_structures.begin();
    ASSERT(ds->type == tofino::DSType::MAP_REGISTER,
           "Unexpected data structure");
    return ds->id;
  }

  void get_map_erase_data(const Call *call_node, addr_t &obj,
                          std::vector<klee::ref<klee::Expr>> &keys) const {
    const call_t &call = call_node->get_call();
    ASSERT(call.function_name == "map_erase", "Unexpected function");

    klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;

    obj = expr_addr_to_obj_addr(map_addr_expr);
    keys = Table::build_keys(key);
  }
};

} // namespace tofino_cpu
