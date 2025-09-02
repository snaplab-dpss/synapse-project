#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class VectorRegisterUpdate : public TofinoModule {
private:
  DS_ID id;
  addr_t obj;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> read_value;
  klee::ref<klee::Expr> write_value;

public:
  VectorRegisterUpdate(const std::string &_instance_id, const BDDNode *_node, DS_ID _id, addr_t _obj, klee::ref<klee::Expr> _index,
                       klee::ref<klee::Expr> _read_value, klee::ref<klee::Expr> _write_value)
      : TofinoModule(ModuleType(ModuleCategory::Tofino_VectorRegisterUpdate, _instance_id), "VectorRegisterUpdate", _node), id(_id), obj(_obj),
        index(_index), read_value(_read_value), write_value(_write_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new VectorRegisterUpdate(get_type().instance_id, node, id, obj, index, read_value, write_value);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_read_value() const { return read_value; }
  klee::ref<klee::Expr> get_write_value() const { return write_value; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {id}; }
};

class VectorRegisterUpdateFactory : public TofinoModuleFactory {
public:
  VectorRegisterUpdateFactory(const std::string &_instance_id)
      : TofinoModuleFactory(ModuleType(ModuleCategory::Tofino_VectorRegisterUpdate, _instance_id), "VectorRegisterUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse