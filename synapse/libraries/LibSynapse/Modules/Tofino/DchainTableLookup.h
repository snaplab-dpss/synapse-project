#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class DchainTableLookup : public TofinoModule {
private:
  DS_ID id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  std::optional<LibCore::symbol_t> hit;

public:
  DchainTableLookup(const LibBDD::Node *node, DS_ID _id, addr_t _obj, klee::ref<klee::Expr> _key, std::optional<LibCore::symbol_t> _hit)
      : TofinoModule(ModuleType::Tofino_DchainTableLookup, "DchainTableLookup", node), id(_id), obj(_obj), key(_key), hit(_hit) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new DchainTableLookup(node, id, obj, key, hit);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  std::optional<LibCore::symbol_t> get_hit() const { return hit; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {id}; }
};

class DchainTableLookupFactory : public TofinoModuleFactory {
public:
  DchainTableLookupFactory() : TofinoModuleFactory(ModuleType::Tofino_DchainTableLookup, "DchainTableLookup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse