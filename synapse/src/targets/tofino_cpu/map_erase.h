#pragma once

#include "tofino_cpu_module.h"

namespace synapse {
namespace tofino_cpu {

class MapErase : public TofinoCPUModule {
private:
  addr_t map_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> trash;

public:
  MapErase(const Node *node, addr_t _map_addr, klee::ref<klee::Expr> _key,
           klee::ref<klee::Expr> _trash)
      : TofinoCPUModule(ModuleType::TofinoCPU_MapErase, "MapErase", node),
        map_addr(_map_addr), key(_key), trash(_trash) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new MapErase(node, map_addr, key, trash);
    return cloned;
  }

  addr_t get_map_addr() const { return map_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_trash() const { return trash; }
};

class MapEraseFactory : public TofinoCPUModuleFactory {
public:
  MapEraseFactory()
      : TofinoCPUModuleFactory(ModuleType::TofinoCPU_MapErase, "MapErase") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace tofino_cpu
} // namespace synapse