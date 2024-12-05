#pragma once

#include "x86_module.h"

namespace x86 {

class ExpireItemsSingleMap : public x86Module {
private:
  addr_t dchain_addr;
  addr_t vector_addr;
  addr_t map_addr;
  klee::ref<klee::Expr> time;
  klee::ref<klee::Expr> total_freed;

public:
  ExpireItemsSingleMap(const Node *node, addr_t _dchain_addr,
                       addr_t _vector_addr, addr_t _map_addr,
                       klee::ref<klee::Expr> _time,
                       klee::ref<klee::Expr> _total_freed)
      : x86Module(ModuleType::x86_ExpireItemsSingleMap, "ExpireItemsSingleMap",
                  node),
        dchain_addr(_dchain_addr), vector_addr(_vector_addr),
        map_addr(_map_addr), time(_time), total_freed(_total_freed) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    ExpireItemsSingleMap *cloned = new ExpireItemsSingleMap(
        node, dchain_addr, map_addr, vector_addr, time, total_freed);
    return cloned;
  }

  addr_t get_dchain_addr() const { return dchain_addr; }
  addr_t get_vector_addr() const { return vector_addr; }
  addr_t get_map_addr() const { return map_addr; }
  klee::ref<klee::Expr> get_time() const { return time; }
  klee::ref<klee::Expr> get_total_freed() const { return total_freed; }
};

class ExpireItemsSingleMapGenerator : public x86ModuleGenerator {
public:
  ExpireItemsSingleMapGenerator()
      : x86ModuleGenerator(ModuleType::x86_ExpireItemsSingleMap,
                           "ExpireItemsSingleMap") {}

protected:
  bool bdd_node_match_pattern(const Node *node) const {
    if (node->get_type() != NodeType::CALL) {
      return false;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "expire_items_single_map") {
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

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    klee::ref<klee::Expr> dchain = call.args.at("chain").expr;
    klee::ref<klee::Expr> vector = call.args.at("vector").expr;
    klee::ref<klee::Expr> map = call.args.at("map").expr;
    klee::ref<klee::Expr> time = call.args.at("time").expr;
    klee::ref<klee::Expr> total_freed = call.ret;

    addr_t map_addr = expr_addr_to_obj_addr(map);
    addr_t vector_addr = expr_addr_to_obj_addr(vector);
    addr_t dchain_addr = expr_addr_to_obj_addr(dchain);

    Module *module = new ExpireItemsSingleMap(node, dchain_addr, vector_addr,
                                              map_addr, time, total_freed);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }
};

} // namespace x86
