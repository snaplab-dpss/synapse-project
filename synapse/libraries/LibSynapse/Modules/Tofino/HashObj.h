#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class HashObj : public TofinoModule {
private:
  DS_ID hash_id;
  addr_t obj;
  klee::ref<klee::Expr> in;
  klee::ref<klee::Expr> hash;

public:
  HashObj(const BDDNode *_node, DS_ID _hash_id, addr_t _obj, klee::ref<klee::Expr> _in, klee::ref<klee::Expr> _hash)
      : TofinoModule(ModuleType::Tofino_HashObj, "HashObj", _node), hash_id(_hash_id), obj(_obj), in(_in), hash(_hash) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new HashObj(node, hash_id, obj, in, hash); }

  DS_ID get_hash_id() const { return hash_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_in() const { return in; }
  klee::ref<klee::Expr> get_hash() const { return hash; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {hash_id}; }
};

class HashObjFactory : public TofinoModuleFactory {
public:
  HashObjFactory() : TofinoModuleFactory(ModuleType::Tofino_HashObj, "HashObj") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
