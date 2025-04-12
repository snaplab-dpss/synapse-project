#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class ParserReject : public TofinoModule {
public:
  ParserReject(const LibBDD::Node *_node) : TofinoModule(ModuleType::Tofino_ParserReject, "ParserReject", _node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    ParserReject *cloned = new ParserReject(node);
    return cloned;
  }
};

class ParserRejectFactory : public TofinoModuleFactory {
public:
  ParserRejectFactory() : TofinoModuleFactory(ModuleType::Tofino_ParserReject, "ParserReject") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse