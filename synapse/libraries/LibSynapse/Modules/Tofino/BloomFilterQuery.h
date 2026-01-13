#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class BloomFilterQuery : public TofinoModule {
private:
  DS_ID bf_id;
  addr_t bf_addr;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> estimate;

public:
  BloomFilterQuery(const BDDNode *_node, DS_ID _bf_id, addr_t _bf_addr, const std::vector<klee::ref<klee::Expr>> &_keys,
                   klee::ref<klee::Expr> _estimate)
      : TofinoModule(ModuleType::Tofino_BloomFilterQuery, "BloomFilterQuery", _node), bf_id(_bf_id), bf_addr(_bf_addr), keys(_keys),
        estimate(_estimate) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new BloomFilterQuery(node, bf_id, bf_addr, keys, estimate); }

  DS_ID get_bf_id() const { return bf_id; }
  addr_t get_bf_addr() const { return bf_addr; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_estimate() const { return estimate; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {bf_id}; }
};

class BloomFilterQueryFactory : public TofinoModuleFactory {
public:
  BloomFilterQueryFactory() : TofinoModuleFactory(ModuleType::Tofino_BloomFilterQuery, "BloomFilterQuery") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse