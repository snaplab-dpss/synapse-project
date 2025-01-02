#pragma once

#include "tofino_module.h"

namespace synapse {
namespace tofino {

class VectorRegisterUpdate : public TofinoModule {
private:
  std::unordered_set<DS_ID> rids;
  addr_t obj;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> read_value;
  std::vector<mod_t> modifications;

public:
  VectorRegisterUpdate(const Node *node, const std::unordered_set<DS_ID> &_rids,
                       addr_t _obj, klee::ref<klee::Expr> _index,
                       klee::ref<klee::Expr> _read_value,
                       const std::vector<mod_t> &_modifications)
      : TofinoModule(ModuleType::Tofino_VectorRegisterUpdate, "VectorRegisterUpdate",
                     node),
        rids(_rids), obj(_obj), index(_index), read_value(_read_value),
        modifications(_modifications) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned =
        new VectorRegisterUpdate(node, rids, obj, index, read_value, modifications);
    return cloned;
  }

  const std::unordered_set<DS_ID> &get_rids() const { return rids; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_read_value() const { return read_value; }
  const std::vector<mod_t> &get_modifications() const { return modifications; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return rids; }
};

class VectorRegisterUpdateFactory : public TofinoModuleFactory {
public:
  VectorRegisterUpdateFactory()
      : TofinoModuleFactory(ModuleType::Tofino_VectorRegisterUpdate,
                            "VectorRegisterUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace tofino
} // namespace synapse