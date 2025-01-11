#pragma once

#include "tofino_module.h"

namespace synapse {
namespace tofino {

class MeterUpdate : public TofinoModule {
private:
  DS_ID table_id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> pkt_len;
  klee::ref<klee::Expr> hit;
  klee::ref<klee::Expr> pass;

public:
  MeterUpdate(const Node *node, DS_ID _table_id, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys,
              klee::ref<klee::Expr> _pkt_len, klee::ref<klee::Expr> _hit, klee::ref<klee::Expr> _pass)
      : TofinoModule(ModuleType::Tofino_MeterUpdate, "MeterUpdate", node), table_id(_table_id), obj(_obj), keys(_keys),
        pkt_len(_pkt_len), hit(_hit), pass(_pass) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new MeterUpdate(node, table_id, obj, keys, pkt_len, hit, pass);
    return cloned;
  }

  DS_ID get_table_id() const { return table_id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_pkt_len() const { return pkt_len; }
  klee::ref<klee::Expr> get_hit() const { return hit; }
  klee::ref<klee::Expr> get_pass() const { return pass; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {table_id}; }
};

class MeterUpdateFactory : public TofinoModuleFactory {
public:
  MeterUpdateFactory() : TofinoModuleFactory(ModuleType::Tofino_MeterUpdate, "MeterUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node, SymbolManager *symbol_manager) const override;
};

} // namespace tofino
} // namespace synapse