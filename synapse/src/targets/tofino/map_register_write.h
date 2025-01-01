#pragma once

#include "tofino_module.h"

namespace tofino {

class MapRegisterWrite : public TofinoModule {
private:
  DS_ID map_register_id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> write_value;

public:
  MapRegisterWrite(const Node *node, DS_ID _map_register_id, addr_t _obj,
                   klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _write_value)
      : TofinoModule(ModuleType::Tofino_MapRegisterWrite, "MapRegisterWrite", node),
        map_register_id(_map_register_id), obj(_obj), key(_key),
        write_value(_write_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new MapRegisterWrite(node, map_register_id, obj, key, write_value);
    return cloned;
  }

  DS_ID get_map_register_id() const { return map_register_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_write_value() const { return write_value; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    return {map_register_id};
  }
};

class MapRegisterWriteFactory : public TofinoModuleFactory {
public:
  MapRegisterWriteFactory()
      : TofinoModuleFactory(ModuleType::Tofino_MapRegisterWrite, "MapRegisterWrite") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace tofino
