#pragma once

#include "tofino_module.h"

namespace tofino {

class MapRegisterReadOrWrite : public TofinoModule {
private:
  DS_ID map_register_id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> read_value;
  klee::ref<klee::Expr> write_value;
  klee::ref<klee::Expr> extra_write_condition;
  symbol_t map_has_this_key;
  symbol_t map_reg_successful_write;

public:
  MapRegisterReadOrWrite(const Node *node, DS_ID _map_register_id, addr_t _obj,
                         klee::ref<klee::Expr> _key,
                         klee::ref<klee::Expr> _read_value,
                         klee::ref<klee::Expr> _write_value,
                         klee::ref<klee::Expr> _extra_write_condition,
                         const symbol_t &_map_has_this_key,
                         const symbol_t &_map_reg_successful_write)
      : TofinoModule(ModuleType::Tofino_MapRegisterReadOrWrite,
                     "MapRegisterReadOrWrite", node),
        map_register_id(_map_register_id), obj(_obj), key(_key),
        read_value(_read_value), write_value(_write_value),
        extra_write_condition(_extra_write_condition),
        map_has_this_key(_map_has_this_key),
        map_reg_successful_write(_map_reg_successful_write) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new MapRegisterReadOrWrite(
        node, map_register_id, obj, key, read_value, write_value,
        extra_write_condition, map_has_this_key, map_reg_successful_write);
    return cloned;
  }

  DS_ID get_map_register_id() const { return map_register_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_read_value() const { return read_value; }
  klee::ref<klee::Expr> get_write_value() const { return write_value; }
  klee::ref<klee::Expr> get_extra_write_condition() const {
    return extra_write_condition;
  }
  const symbol_t &get_map_has_this_key() const { return map_has_this_key; }
  const symbol_t &get_map_reg_successful_write() const {
    return map_reg_successful_write;
  }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    return {map_register_id};
  }
};

class MapRegisterReadOrWriteGenerator : public TofinoModuleGenerator {
public:
  MapRegisterReadOrWriteGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_MapRegisterReadOrWrite,
                              "MapRegisterReadOrWrite") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override;
};

} // namespace tofino
