#pragma once

#include "x86_module.h"

namespace x86 {

class HashObj : public x86Module {
private:
  addr_t obj_addr;
  klee::ref<klee::Expr> size;
  klee::ref<klee::Expr> hash;

public:
  HashObj(const Node *node, addr_t _obj_addr, klee::ref<klee::Expr> _size,
          klee::ref<klee::Expr> _hash)
      : x86Module(ModuleType::x86_HashObj, "HashObj", node),
        obj_addr(_obj_addr), size(_size), hash(_hash) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new HashObj(node, obj_addr, size, hash);
    return cloned;
  }

  addr_t get_obj_addr() const { return obj_addr; }
  klee::ref<klee::Expr> get_size() const { return size; }
  klee::ref<klee::Expr> get_hash() const { return hash; }
};

class HashObjGenerator : public x86ModuleGenerator {
public:
  HashObjGenerator() : x86ModuleGenerator(ModuleType::x86_HashObj, "HashObj") {}

protected:
  bool bdd_node_match_pattern(const Node *node) const {
    if (node->get_type() != NodeType::CALL) {
      return false;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "hash_obj") {
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

    klee::ref<klee::Expr> obj_addr_expr = call.args.at("obj").expr;
    klee::ref<klee::Expr> size = call.args.at("size").expr;
    klee::ref<klee::Expr> hash = call.ret;

    addr_t obj_addr = expr_addr_to_obj_addr(obj_addr_expr);

    Module *module = new HashObj(node, obj_addr, size, hash);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }
};

} // namespace x86
