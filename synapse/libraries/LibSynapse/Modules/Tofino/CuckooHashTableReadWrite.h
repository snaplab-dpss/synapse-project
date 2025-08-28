#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class CuckooHashTableReadWrite : public TofinoModule {
private:
  DS_ID cuckoo_hash_table_id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
  klee::ref<klee::Expr> cuckoo_update_condition;
  klee::ref<klee::Expr> cuckoo_success_condition;

public:
  CuckooHashTableReadWrite(const BDDNode *_node, DS_ID _cuckoo_hash_table_id, addr_t _obj, klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _value,
                           klee::ref<klee::Expr> _cuckoo_update_condition, klee::ref<klee::Expr> _cuckoo_success_condition)
      : TofinoModule(ModuleType::Tofino_CuckooHashTableReadWrite, "CuckooHashTableReadWrite", _node), cuckoo_hash_table_id(_cuckoo_hash_table_id),
        obj(_obj), key(_key), value(_value), cuckoo_update_condition(_cuckoo_update_condition), cuckoo_success_condition(_cuckoo_success_condition) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new CuckooHashTableReadWrite(node, cuckoo_hash_table_id, obj, key, value, cuckoo_update_condition, cuckoo_success_condition);
    return cloned;
  }

  DS_ID get_cuckoo_hash_table_id() const { return cuckoo_hash_table_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_value() const { return value; }
  klee::ref<klee::Expr> get_cuckoo_update_condition() const { return cuckoo_update_condition; }
  klee::ref<klee::Expr> get_cuckoo_success_condition() const { return cuckoo_success_condition; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {cuckoo_hash_table_id}; }
};

class CuckooHashTableReadWriteFactory : public TofinoModuleFactory {
public:
  CuckooHashTableReadWriteFactory() : TofinoModuleFactory(ModuleType::Tofino_CuckooHashTableReadWrite, "CuckooHashTableReadWrite") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse