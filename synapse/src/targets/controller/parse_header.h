#pragma once

#include "controller_module.h"

namespace synapse {
namespace ctrl {

class ParseHeader : public ControllerModule {
private:
  addr_t chunk_addr;
  klee::ref<klee::Expr> chunk;
  klee::ref<klee::Expr> length;

public:
  ParseHeader(const Node *node, addr_t _chunk_addr, klee::ref<klee::Expr> _chunk, klee::ref<klee::Expr> _length)
      : ControllerModule(ModuleType::Controller_ParseHeader, "ParseHeader", node), chunk_addr(_chunk_addr), chunk(_chunk),
        length(_length) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    ParseHeader *cloned = new ParseHeader(node, chunk_addr, chunk, length);
    return cloned;
  }

  addr_t get_chunk_addr() const { return chunk_addr; }
  klee::ref<klee::Expr> get_chunk() const { return chunk; }
  klee::ref<klee::Expr> get_length() const { return length; }
};

class ParseHeaderFactory : public ControllerModuleFactory {
public:
  ParseHeaderFactory() : ControllerModuleFactory(ModuleType::Controller_ParseHeader, "ParseHeader") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node, SymbolManager *symbol_manager) const override;
};

} // namespace ctrl
} // namespace synapse