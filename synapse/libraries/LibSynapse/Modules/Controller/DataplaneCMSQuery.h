#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneCMSQuery : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> min_estimate;

public:
  DataplaneCMSQuery(const LibBDD::Node *_node, addr_t _obj, klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _min_estimate)
      : ControllerModule(ModuleType::Controller_DataplaneCMSQuery, "DataplaneCMSQuery", _node), obj(_obj), key(_key), min_estimate(_min_estimate) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new DataplaneCMSQuery(node, obj, key, min_estimate); }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_min_estimate() const { return min_estimate; }
};

class DataplaneCMSQueryFactory : public ControllerModuleFactory {
public:
  DataplaneCMSQueryFactory() : ControllerModuleFactory(ModuleType::Controller_DataplaneCMSQuery, "DataplaneCMSQuery") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse