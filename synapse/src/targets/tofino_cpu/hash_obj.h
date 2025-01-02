#pragma once

#include "tofino_cpu_module.h"

namespace synapse {
namespace tofino_cpu {

class HashObj : public TofinoCPUModule {
private:
  addr_t obj_addr;
  klee::ref<klee::Expr> size;
  klee::ref<klee::Expr> hash;

public:
  HashObj(const Node *node, addr_t _obj_addr, klee::ref<klee::Expr> _size,
          klee::ref<klee::Expr> _hash)
      : TofinoCPUModule(ModuleType::TofinoCPU_HashObj, "HashObj", node),
        obj_addr(_obj_addr), size(_size), hash(_hash) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new HashObj(node, obj_addr, size, hash);
    return cloned;
  }

  addr_t get_obj_addr() const { return obj_addr; }
  klee::ref<klee::Expr> get_size() const { return size; }
  klee::ref<klee::Expr> get_hash() const { return hash; }
};

class HashObjFactory : public TofinoCPUModuleFactory {
public:
  HashObjFactory() : TofinoCPUModuleFactory(ModuleType::TofinoCPU_HashObj, "HashObj") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace tofino_cpu
} // namespace synapse