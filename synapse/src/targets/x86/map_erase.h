#pragma once

#include "x86_module.h"

namespace synapse {
namespace x86 {

class MapErase : public x86Module {
private:
  addr_t map_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> trash;

public:
  MapErase(const Node *node, addr_t _map_addr, klee::ref<klee::Expr> _key,
           klee::ref<klee::Expr> _trash)
      : x86Module(ModuleType::x86_MapErase, "MapErase", node), map_addr(_map_addr),
        key(_key), trash(_trash) {}

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

class MapEraseFactory : public x86ModuleFactory {
public:
  MapEraseFactory() : x86ModuleFactory(ModuleType::x86_MapErase, "MapErase") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace x86
} // namespace synapse