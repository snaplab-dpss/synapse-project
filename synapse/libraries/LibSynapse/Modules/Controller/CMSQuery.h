#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class CMSQuery : public ControllerModule {
private:
  addr_t cms_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> min_estimate;

public:
  CMSQuery(const LibBDD::Node *node, addr_t _cms_addr, klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _min_estimate)
      : ControllerModule(ModuleType::Controller_CMSQuery, "CMSQuery", node), cms_addr(_cms_addr), key(_key), min_estimate(_min_estimate) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new CMSQuery(node, cms_addr, key, min_estimate);
    return cloned;
  }

  addr_t get_cms_addr() const { return cms_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_min_estimate() const { return min_estimate; }
};

class CMSQueryFactory : public ControllerModuleFactory {
public:
  CMSQueryFactory() : ControllerModuleFactory(ModuleType::Controller_CMSQuery, "CMSQuery") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
};

} // namespace Controller
} // namespace LibSynapse