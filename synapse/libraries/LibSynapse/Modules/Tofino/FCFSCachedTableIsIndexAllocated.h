#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class FCFSCachedTableIsIndexAllocated : public TofinoModule {
private:
  DS_ID fcfs_ct_id;
  addr_t obj;
  klee::ref<klee::Expr> index;
  symbol_t is_allocated;

public:
  FCFSCachedTableIsIndexAllocated(const BDDNode *_node, DS_ID _fcfs_ct_id, addr_t _obj, klee::ref<klee::Expr> _index, const symbol_t &_is_allocated)
      : TofinoModule(ModuleType::Tofino_FCFSCachedTableIsIndexAllocated, "FCFSCachedTableIsIndexAllocated", _node), fcfs_ct_id(_fcfs_ct_id),
        obj(_obj), index(_index), is_allocated(_is_allocated) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new FCFSCachedTableIsIndexAllocated(node, fcfs_ct_id, obj, index, is_allocated);
    return cloned;
  }

  DS_ID get_fcfs_ct_id() const { return fcfs_ct_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
  const symbol_t &get_is_allocated() const { return is_allocated; }
};

class FCFSCachedTableIsIndexAllocatedFactory : public TofinoModuleFactory {
public:
  FCFSCachedTableIsIndexAllocatedFactory()
      : TofinoModuleFactory(ModuleType::Tofino_FCFSCachedTableIsIndexAllocated, "FCFSCachedTableIsIndexAllocated") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse