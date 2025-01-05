#pragma once

#include "controller_module.h"

namespace synapse {
namespace ctrl {

using tofino::DS_ID;

class FCFSCachedTableDelete : public ControllerModule {
private:
  DS_ID id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;

public:
  FCFSCachedTableDelete(const Node *node, DS_ID _id, addr_t _obj,
                        const std::vector<klee::ref<klee::Expr>> &_keys)
      : ControllerModule(ModuleType::Controller_FCFSCachedTableDelete,
                         "FCFSCachedTableDelete", node),
        id(_id), obj(_obj), keys(_keys) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    FCFSCachedTableDelete *cloned = new FCFSCachedTableDelete(node, id, obj, keys);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
};

class FCFSCachedTableDeleteFactory : public ControllerModuleFactory {
public:
  FCFSCachedTableDeleteFactory()
      : ControllerModuleFactory(ModuleType::Controller_FCFSCachedTableDelete,
                                "FCFSCachedTableDelete") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace ctrl
} // namespace synapse