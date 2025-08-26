#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class ParseHeader : public x86Module {
private:
  addr_t chunk_addr;
  klee::ref<klee::Expr> chunk;
  klee::ref<klee::Expr> length;

public:
  ParseHeader(ModuleType _type, const BDDNode *_node, addr_t _chunk_addr, klee::ref<klee::Expr> _chunk, klee::ref<klee::Expr> _length)
      : x86Module(_type, "ParseHeader", _node), chunk_addr(_chunk_addr), chunk(_chunk), length(_length) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    ParseHeader *cloned = new ParseHeader(type, node, chunk_addr, chunk, length);
    return cloned;
  }

  addr_t get_chunk_addr() const { return chunk_addr; }
  klee::ref<klee::Expr> get_chunk() const { return chunk; }
  klee::ref<klee::Expr> get_length() const { return length; }
};

class ParseHeaderFactory : public x86ModuleFactory {
public:
  ParseHeaderFactory(const std::string &_instance_id) : x86ModuleFactory(ModuleType(ModuleCategory::x86_ParseHeader, _instance_id), "ParseHeader") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse