#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class ExpireItemsSingleMap : public x86Module {
private:
  klee::ref<klee::Expr> dchain_addr;
  klee::ref<klee::Expr> vector_addr;
  klee::ref<klee::Expr> map_addr;
  klee::ref<klee::Expr> time;
  klee::ref<klee::Expr> total_freed;
  klee::ref<klee::Expr> n_freed_flows;

public:
  ExpireItemsSingleMap(const InstanceId _instance_id, const BDDNode *_node, klee::ref<klee::Expr> _dchain_addr, klee::ref<klee::Expr> _vector_addr,
                       klee::ref<klee::Expr> _map_addr, klee::ref<klee::Expr> _time, klee::ref<klee::Expr> _total_freed,
                       klee::ref<klee::Expr> _n_freed_flows)
      : x86Module(ModuleType(ModuleCategory::x86_ExpireItemsSingleMap, _instance_id), "ExpireItemsSingleMap", _node), dchain_addr(_dchain_addr),
        vector_addr(_vector_addr), map_addr(_map_addr), time(_time), total_freed(_total_freed), n_freed_flows(_n_freed_flows) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    ExpireItemsSingleMap *cloned =
        new ExpireItemsSingleMap(get_type().instance_id, node, dchain_addr, vector_addr, map_addr, time, total_freed, n_freed_flows);
    return cloned;
  }

  klee::ref<klee::Expr> get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_vector_addr() const { return vector_addr; }
  klee::ref<klee::Expr> get_map_addr() const { return map_addr; }
  klee::ref<klee::Expr> get_time() const { return time; }
  klee::ref<klee::Expr> get_total_freed() const { return total_freed; }
  klee::ref<klee::Expr> get_n_freed_flows() const { return n_freed_flows; }
};

class ExpireItemsSingleMapFactory : public x86ModuleFactory {
public:
  ExpireItemsSingleMapFactory(const InstanceId _instance_id)
      : x86ModuleFactory(ModuleType(ModuleCategory::x86_ExpireItemsSingleMap, _instance_id), "ExpireItemsSingleMap") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse
