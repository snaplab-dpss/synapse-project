#pragma once

#include <LibTessera/Modules/Tofino/TofinoModule.h>

namespace LibTessera {
namespace Tofino {

class BloomFilterSet : public TofinoModule {
private:
  DS_ID bf_id;
  addr_t bf_addr;
  std::vector<klee::ref<klee::Expr>> keys;

public:
  BloomFilterSet(const BDDNode *_node, DS_ID _bf_id, addr_t _bf_addr, const std::vector<klee::ref<klee::Expr>> &_keys)
      : TofinoModule(ModuleType::Tofino_BloomFilterSet, "BloomFilterSet", _node), bf_id(_bf_id), bf_addr(_bf_addr), keys(_keys) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new BloomFilterSet(node, bf_id, bf_addr, keys); }

  DS_ID get_bf_id() const { return bf_id; }
  addr_t get_bf_addr() const { return bf_addr; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {bf_id}; }
};

class BloomFilterSetFactory : public TofinoModuleFactory {
public:
  BloomFilterSetFactory() : TofinoModuleFactory(ModuleType::Tofino_BloomFilterSet, "BloomFilterSet") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibTessera