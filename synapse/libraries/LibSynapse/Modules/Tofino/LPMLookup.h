#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class LPMLookup : public TofinoModule {
private:
  DS_ID lpm_id;
  addr_t obj;
  klee::ref<klee::Expr> original_key; // the prefix as the NF passes it
  std::vector<klee::ref<klee::Expr>> keys; // its fields, as the table matches them
  klee::ref<klee::Expr> value;
  klee::ref<klee::Expr> match;

public:
  LPMLookup(const BDDNode *_node, DS_ID _lpm_id, addr_t _obj, klee::ref<klee::Expr> _original_key, const std::vector<klee::ref<klee::Expr>> &_keys,
            klee::ref<klee::Expr> _value, klee::ref<klee::Expr> _match)
      : TofinoModule(ModuleType::Tofino_LPMLookup, "LPMLookup", _node), lpm_id(_lpm_id), obj(_obj), original_key(_original_key), keys(_keys),
        value(_value), match(_match) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new LPMLookup(node, lpm_id, obj, original_key, keys, value, match);
    return cloned;
  }

  DS_ID get_lpm_id() const { return lpm_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_original_key() const { return original_key; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
  klee::ref<klee::Expr> get_match() const { return match; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {lpm_id}; }
};

class LPMLookupFactory : public TofinoModuleFactory {
public:
  LPMLookupFactory() : TofinoModuleFactory(ModuleType::Tofino_LPMLookup, "LPMLookup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse