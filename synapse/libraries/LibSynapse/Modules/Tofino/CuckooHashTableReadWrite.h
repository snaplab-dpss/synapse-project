#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class CuckooHashTableReadWrite : public TofinoModule {
private:
  DS_ID cuckoo_hash_table_id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> read_value;
  klee::ref<klee::Expr> write_value;
  klee::ref<klee::Expr> cuckoo_update_condition;
  symbol_t cuckoo_success;

public:
  CuckooHashTableReadWrite(const BDDNode *_node, DS_ID _cuckoo_hash_table_id, addr_t _obj, klee::ref<klee::Expr> _key,
                           klee::ref<klee::Expr> _read_value, klee::ref<klee::Expr> _write_value, klee::ref<klee::Expr> _cuckoo_update_condition,
                           const symbol_t &_cuckoo_success)
      : TofinoModule(ModuleType::Tofino_CuckooHashTableReadWrite, "CuckooHashTableReadWrite", _node), cuckoo_hash_table_id(_cuckoo_hash_table_id),
        obj(_obj), key(_key), read_value(_read_value), write_value(_write_value), cuckoo_update_condition(_cuckoo_update_condition),
        cuckoo_success(_cuckoo_success) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned =
        new CuckooHashTableReadWrite(node, cuckoo_hash_table_id, obj, key, read_value, write_value, cuckoo_update_condition, cuckoo_success);
    return cloned;
  }

  DS_ID get_cuckoo_hash_table_id() const { return cuckoo_hash_table_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_read_value() const { return read_value; }
  klee::ref<klee::Expr> get_write_value() const { return write_value; }
  klee::ref<klee::Expr> get_cuckoo_update_condition() const { return cuckoo_update_condition; }
  const symbol_t &get_cuckoo_success() const { return cuckoo_success; }

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