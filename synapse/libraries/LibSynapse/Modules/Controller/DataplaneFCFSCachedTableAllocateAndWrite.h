#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

using Tofino::DS_ID;

class DataplaneFCFSCachedTableAllocateAndWrite : public ControllerModule {
private:
  DS_ID id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> write_value;
  symbol_t allocation_successful;

public:
  DataplaneFCFSCachedTableAllocateAndWrite(const BDDNode *_node, DS_ID _id, addr_t _obj, klee::ref<klee::Expr> _key,
                                           klee::ref<klee::Expr> _write_value, const symbol_t &_allocation_successful)
      : ControllerModule(ModuleType::Controller_DataplaneFCFSCachedTableAllocateAndWrite, "DataplaneFCFSCachedTableAllocateAndWrite", _node), id(_id),
        obj(_obj), key(_key), write_value(_write_value), allocation_successful(_allocation_successful) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    DataplaneFCFSCachedTableAllocateAndWrite *cloned =
        new DataplaneFCFSCachedTableAllocateAndWrite(node, id, obj, key, write_value, allocation_successful);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_write_value() const { return write_value; }
  symbol_t get_allocation_successful_symbol() const { return allocation_successful; }
};

class DataplaneFCFSCachedTableAllocateAndWriteFactory : public ControllerModuleFactory {
public:
  DataplaneFCFSCachedTableAllocateAndWriteFactory()
      : ControllerModuleFactory(ModuleType::Controller_DataplaneFCFSCachedTableAllocateAndWrite, "DataplaneFCFSCachedTableAllocateAndWrite") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse