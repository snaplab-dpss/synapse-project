#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class LPMLookup : public TofinoModule {
private:
  DS_ID lpm_id;
  addr_t obj;
  klee::ref<klee::Expr> addr;
  klee::ref<klee::Expr> device;
  klee::ref<klee::Expr> match;

public:
  LPMLookup(const InstanceId _instance_id, const BDDNode *_node, DS_ID _lpm_id, addr_t _obj, klee::ref<klee::Expr> _addr,
            klee::ref<klee::Expr> _device, klee::ref<klee::Expr> _match)
      : TofinoModule(ModuleType(ModuleCategory::Tofino_LPMLookup, _instance_id), "LPMLookup", _node), lpm_id(_lpm_id), obj(_obj), addr(_addr),
        device(_device), match(_match) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new LPMLookup(get_type().instance_id, node, lpm_id, obj, addr, device, match);
    return cloned;
  }

  DS_ID get_lpm_id() const { return lpm_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_addr() const { return addr; }
  klee::ref<klee::Expr> get_device() const { return device; }
  klee::ref<klee::Expr> get_match() const { return match; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {lpm_id}; }
};

class LPMLookupFactory : public TofinoModuleFactory {
public:
  LPMLookupFactory(const InstanceId _instance_id) : TofinoModuleFactory(ModuleType(ModuleCategory::Tofino_LPMLookup, _instance_id), "LPMLookup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse