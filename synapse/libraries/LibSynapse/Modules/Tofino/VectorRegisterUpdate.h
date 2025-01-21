#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class VectorRegisterUpdate : public TofinoModule {
private:
  std::unordered_set<DS_ID> rids;
  addr_t obj;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> read_value;
  klee::ref<klee::Expr> write_value;

public:
  VectorRegisterUpdate(const LibBDD::Node *node, const std::unordered_set<DS_ID> &_rids, addr_t _obj, klee::ref<klee::Expr> _index,
                       klee::ref<klee::Expr> _read_value, klee::ref<klee::Expr> _write_value)
      : TofinoModule(ModuleType::Tofino_VectorRegisterUpdate, "VectorRegisterUpdate", node), rids(_rids), obj(_obj), index(_index),
        read_value(_read_value), write_value(_write_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new VectorRegisterUpdate(node, rids, obj, index, read_value, write_value);
    return cloned;
  }

  const std::unordered_set<DS_ID> &get_rids() const { return rids; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_read_value() const { return read_value; }
  klee::ref<klee::Expr> get_write_value() const { return write_value; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return rids; }
};

class VectorRegisterUpdateFactory : public TofinoModuleFactory {
public:
  VectorRegisterUpdateFactory() : TofinoModuleFactory(ModuleType::Tofino_VectorRegisterUpdate, "VectorRegisterUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
};

} // namespace Tofino
} // namespace LibSynapse