#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class MapPut : public TofinoCPUModule {
private:
  addr_t map_addr;
  addr_t key_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;

public:
  MapPut(const Node *node, addr_t _map_addr, addr_t _key_addr,
         klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _value)
      : TofinoCPUModule(ModuleType::TofinoCPU_MapPut, "MapPut", node),
        map_addr(_map_addr), key_addr(_key_addr), key(_key), value(_value) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new MapPut(node, map_addr, key_addr, key, value);
    return cloned;
  }

  addr_t get_map_addr() const { return map_addr; }
  addr_t get_key_addr() const { return key_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_value() const { return value; }
};

class MapPutGenerator : public TofinoCPUModuleGenerator {
public:
  MapPutGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_MapPut, "MapPut") {}

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

    if (!can_place(ep, call_node, "map", PlacementDecision::TofinoCPU_Map)) {
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

    if (!can_place(ep, call_node, "map", PlacementDecision::TofinoCPU_Map)) {
      return impls;
    }

    klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
    klee::ref<klee::Expr> key_addr_expr = call.args.at("key").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;
    klee::ref<klee::Expr> value = call.args.at("value").expr;

    addr_t map_addr = expr_addr_to_obj_addr(map_addr_expr);
    addr_t key_addr = expr_addr_to_obj_addr(key_addr_expr);

    Module *module = new MapPut(node, map_addr, key_addr, key, value);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    place(new_ep, map_addr, PlacementDecision::TofinoCPU_Map);

    return impls;
  }
};

} // namespace tofino_cpu
