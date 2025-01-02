#pragma once

#include "tofino_cpu_module.h"

namespace synapse {
namespace tofino_cpu {

using tofino::DS_ID;

class MapRegisterDelete : public TofinoCPUModule {
private:
  DS_ID id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;

public:
  MapRegisterDelete(const Node *node, DS_ID _id, addr_t _obj,
                    const std::vector<klee::ref<klee::Expr>> &_keys)
      : TofinoCPUModule(ModuleType::TofinoCPU_MapRegisterDelete, "MapRegisterDelete",
                        node),
        id(_id), obj(_obj), keys(_keys) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    MapRegisterDelete *cloned = new MapRegisterDelete(node, id, obj, keys);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
};

class MapRegisterDeleteFactory : public TofinoCPUModuleFactory {
public:
  MapRegisterDeleteFactory()
      : TofinoCPUModuleFactory(ModuleType::TofinoCPU_MapRegisterDelete,
                               "MapRegisterDelete") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace tofino_cpu
} // namespace synapse