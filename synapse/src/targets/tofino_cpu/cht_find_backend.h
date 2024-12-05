#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class ChtFindBackend : public TofinoCPUModule {
private:
  addr_t cht_addr;
  addr_t backends_addr;
  klee::ref<klee::Expr> hash;
  klee::ref<klee::Expr> height;
  klee::ref<klee::Expr> capacity;
  klee::ref<klee::Expr> backend;
  symbol_t found;

public:
  ChtFindBackend(const Node *node, addr_t _cht_addr, addr_t _backends_addr,
                 klee::ref<klee::Expr> _hash, klee::ref<klee::Expr> _height,
                 klee::ref<klee::Expr> _capacity,
                 klee::ref<klee::Expr> _backend, const symbol_t &_found)
      : TofinoCPUModule(ModuleType::TofinoCPU_ChtFindBackend, "ChtFindBackend",
                        node),
        cht_addr(_cht_addr), backends_addr(_backends_addr), hash(_hash),
        height(_height), capacity(_capacity), backend(_backend), found(_found) {
  }

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new ChtFindBackend(node, cht_addr, backends_addr, hash,
                                        height, capacity, backend, found);
    return cloned;
  }

  klee::ref<klee::Expr> get_hash() const { return hash; }
  addr_t get_cht_addr() const { return cht_addr; }
  addr_t get_backends_addr() const { return backends_addr; }
  klee::ref<klee::Expr> get_height() const { return height; }
  klee::ref<klee::Expr> get_capacity() const { return capacity; }
  klee::ref<klee::Expr> get_backend() const { return backend; }
  const symbol_t &get_found() const { return found; }
};

class ChtFindBackendGenerator : public TofinoCPUModuleGenerator {
public:
  ChtFindBackendGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_ChtFindBackend,
                                 "ChtFindBackend") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "cht_find_preferred_available_backend") {
      return std::nullopt;
    }

    klee::ref<klee::Expr> cht = call.args.at("cht").expr;
    addr_t cht_addr = expr_addr_to_obj_addr(cht);

    if (!ctx.can_impl_ds(cht_addr, DSImpl::TofinoCPU_ConsistentHashTable)) {
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

    if (call.function_name != "cht_find_preferred_available_backend") {
      return impls;
    }

    klee::ref<klee::Expr> cht = call.args.at("cht").expr;
    klee::ref<klee::Expr> backends = call.args.at("active_backends").expr;
    klee::ref<klee::Expr> hash = call.args.at("hash").expr;
    klee::ref<klee::Expr> height = call.args.at("cht_height").expr;
    klee::ref<klee::Expr> capacity = call.args.at("backend_capacity").expr;
    klee::ref<klee::Expr> backend = call.args.at("chosen_backend").out;

    addr_t cht_addr = expr_addr_to_obj_addr(cht);
    addr_t backends_addr = expr_addr_to_obj_addr(backends);

    symbols_t symbols = call_node->get_locally_generated_symbols();
    symbol_t backend_found;
    bool found = get_symbol(symbols, "prefered_backend_found", backend_found);
    assert(found && "Symbol prefered_backend_found not found");

    if (!ep->get_ctx().can_impl_ds(cht_addr,
                                   DSImpl::TofinoCPU_ConsistentHashTable)) {
      return impls;
    }

    Module *module =
        new ChtFindBackend(node, cht_addr, backends_addr, hash, height,
                           capacity, backend, backend_found);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    new_ep->get_mutable_ctx().save_ds_impl(
        cht_addr, DSImpl::TofinoCPU_ConsistentHashTable);

    return impls;
  }
};

} // namespace tofino_cpu
