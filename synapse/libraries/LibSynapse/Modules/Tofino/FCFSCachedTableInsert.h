#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class FCFSCachedTableInsert : public TofinoModule {
private:
  DS_ID fcfs_ct_id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  symbol_t success;

public:
  FCFSCachedTableInsert(const BDDNode *_node, DS_ID _fcfs_ct_id, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys,
                        const klee::ref<klee::Expr> &_value, const symbol_t &_success)
      : TofinoModule(ModuleType::Tofino_FCFSCachedTableInsert, "FCFSCachedTableInsert", _node), fcfs_ct_id(_fcfs_ct_id), obj(_obj), keys(_keys),
        value(_value), success(_success) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new FCFSCachedTableInsert(node, fcfs_ct_id, obj, keys, value, success);
    return cloned;
  }

  DS_ID get_fcfs_ct_id() const { return fcfs_ct_id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  const klee::ref<klee::Expr> &get_value() const { return value; }
  const symbol_t &get_success() const { return success; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {fcfs_ct_id}; }
};

class FCFSCachedTableInsertFactory : public TofinoModuleFactory {
public:
  FCFSCachedTableInsertFactory() : TofinoModuleFactory(ModuleType::Tofino_FCFSCachedTableInsert, "FCFSCachedTableInsert") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse