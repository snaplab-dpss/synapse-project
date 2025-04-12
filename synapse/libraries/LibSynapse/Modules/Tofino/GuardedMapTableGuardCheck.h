#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

/*
 ============================
  GuardedMapTableGuardCheck
 ============================

  - BDD pattern:
  ┌──────────────────────────┐
  │dchain_allocate_new_index │
  └────────────┬─────────────┘
              │
    yes┌──────▼──────┐   no
    ┌──┼out_of_space?┼───┐
    │  └─────────────┘   │
    ┌▼┐               ┌───▼───┐
    │1│               │map_put│
    └─┘               └───────┘

  - New BDD:
    yes  ┌─────────────┐ no
      ┌──┼guard_allows?┼─┐
      │  └─────────────┘ │
  ┌────▼─────┐           ┌▼┐
  │ Old BDD  │           │1│
  │  pattern │           └─┘
  └──────────┘

*/
namespace LibSynapse {
namespace Tofino {

class GuardedMapTableGuardCheck : public TofinoModule {
private:
  DS_ID id;
  addr_t obj;
  LibCore::symbol_t guard_allow;
  klee::ref<klee::Expr> guard_allow_condition;

public:
  GuardedMapTableGuardCheck(const LibBDD::Node *_node, DS_ID _id, addr_t _obj, const LibCore::symbol_t &_guard_allow,
                            klee::ref<klee::Expr> _guard_allow_condition)
      : TofinoModule(ModuleType::Tofino_GuardedMapTableGuardCheck, "GuardedMapTableGuardCheck", _node), id(_id), obj(_obj), guard_allow(_guard_allow),
        guard_allow_condition(_guard_allow_condition) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new GuardedMapTableGuardCheck(node, id, obj, guard_allow, guard_allow_condition);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  const LibCore::symbol_t &get_guard_allow() const { return guard_allow; }
  klee::ref<klee::Expr> get_guard_allow_condition() const { return guard_allow_condition; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {id}; }
};

class GuardedMapTableGuardCheckFactory : public TofinoModuleFactory {
public:
  GuardedMapTableGuardCheckFactory() : TofinoModuleFactory(ModuleType::Tofino_GuardedMapTableGuardCheck, "GuardedMapTableGuardCheck") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse