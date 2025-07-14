#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class ParserExtraction : public TofinoModule {
private:
  addr_t hdr_addr;
  klee::ref<klee::Expr> hdr;
  bytes_t length;
  std::vector<klee::ref<klee::Expr>> hdr_fields_guess;

public:
  ParserExtraction(const BDDNode *_node, addr_t _hdr_addr, klee::ref<klee::Expr> _hdr, bytes_t _length,
                   std::vector<klee::ref<klee::Expr>> _hdr_fields_guess)
      : TofinoModule(ModuleType::Tofino_ParserExtraction, "ParserExtraction", _node), hdr_addr(_hdr_addr), hdr(_hdr), length(_length),
        hdr_fields_guess(_hdr_fields_guess) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    ParserExtraction *cloned = new ParserExtraction(node, hdr_addr, hdr, length, hdr_fields_guess);
    return cloned;
  }

  addr_t get_hdr_addr() const { return hdr_addr; }
  klee::ref<klee::Expr> get_hdr() const { return hdr; }
  bytes_t get_length() const { return length; }
  const std::vector<klee::ref<klee::Expr>> &get_hdr_fields_guess() const { return hdr_fields_guess; }
};

class ParserExtractionFactory : public TofinoModuleFactory {
public:
  ParserExtractionFactory() : TofinoModuleFactory(ModuleType::Tofino_ParserExtraction, "ParserExtraction") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse