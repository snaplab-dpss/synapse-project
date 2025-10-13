#pragma once

namespace LibSynapse {
namespace x86 {
class LPMLookup : public x86Module {
private:
  klee::ref<klee::Expr> lpm_addr;
  klee::ref<klee::Expr> prefix;
  klee::ref<klee::Expr> value_out;
  klee::ref<klee::Expr> lpm_lookup_match;

  LPMLookup(const InstanceId _instance_id, const BDDNode *_node, klee::ref<klee::Expr> _lpm_addr, klee::ref<klee::Expr> _prefix,
            klee::ref<klee::Expr> _value_out, klee::ref<klee::Expr> _lpm_lookup_match)
      : x86Module(ModuleType(ModuleCategory::x86_LPMLookup, _instance_id), "LPMLookup", _node), lpm_addr(_lpm_addr), prefix(_prefix),
        value_out(_value_out) lpm_lookup_match(_lpm_lookup_match) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new LPMLookup(get_type().instance_id, node, lpm_addr, prefix, value_out, lpm_lookup_match);
    return cloned;
  }

  klee::ref<klee::Expr> get_lpm_addr() const { return lpm_addr; }
  klee::ref<klee::Expr> get_prefix() const { return prefix; }
  klee::ref<klee::Expr> get_value_out() const { return value_out; }
  klee::ref<klee::Expr> get_lpm_lookup_match() const { return lpm_lookup_match; }
};

class LPMLookupFactory : public x86ModuleFactory {
public:
  LPMLookupFactory(const InstanceId _instance_id) : x86ModuleFactory(ModuleType(ModuleCategory::x86_LPMLookup, _instance_id), "LPMLookup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};
} // namespace x86
} // namespace LibSynapse