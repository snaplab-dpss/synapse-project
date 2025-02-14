#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

using Tofino::DS_ID;

class FCFSCachedTableWrite : public ControllerModule {
private:
  DS_ID id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;

public:
  FCFSCachedTableWrite(const LibBDD::Node *node, DS_ID _id, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys,
                       klee::ref<klee::Expr> _value)
      : ControllerModule(ModuleType::Controller_FCFSCachedTableWrite, "FCFSCachedTableWrite", node), id(_id), obj(_obj), keys(_keys),
        value(_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    FCFSCachedTableWrite *cloned = new FCFSCachedTableWrite(node, id, obj, keys, value);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
};

class FCFSCachedTableWriteFactory : public ControllerModuleFactory {
public:
  FCFSCachedTableWriteFactory() : ControllerModuleFactory(ModuleType::Controller_FCFSCachedTableWrite, "FCFSCachedTableWrite") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse