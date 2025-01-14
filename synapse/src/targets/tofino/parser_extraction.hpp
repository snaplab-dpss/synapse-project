#pragma once

#include "tofino_module.hpp"

namespace synapse {
namespace tofino {

class ParserExtraction : public TofinoModule {
private:
  addr_t hdr_addr;
  klee::ref<klee::Expr> hdr;
  bytes_t length;

public:
  ParserExtraction(const Node *node, addr_t _hdr_addr, klee::ref<klee::Expr> _hdr, bytes_t _length)
      : TofinoModule(ModuleType::Tofino_ParserExtraction, "ParserExtraction", node), hdr_addr(_hdr_addr), hdr(_hdr),
        length(_length) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    ParserExtraction *cloned = new ParserExtraction(node, hdr_addr, hdr, length);
    return cloned;
  }

  addr_t get_hdr_addr() const { return hdr_addr; }
  klee::ref<klee::Expr> get_hdr() const { return hdr; }
  bytes_t get_length() const { return length; }
};

class ParserExtractionFactory : public TofinoModuleFactory {
public:
  ParserExtractionFactory() : TofinoModuleFactory(ModuleType::Tofino_ParserExtraction, "ParserExtraction") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace tofino
} // namespace synapse