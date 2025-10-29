#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace x86 {

using LibCore::expr_mod_t;

class ModifyHeader : public x86Module {
private:
  klee::ref<klee::Expr> chunk_addr_expr;
  klee::ref<klee::Expr> chunk_addr_expr;
  std::vector<expr_mod_t> changes;

public:
  ModifyHeader(const InstanceId _instance_id, const BDDNode *_node, klee::ref<klee::Expr> _chunk_addr_expr, const std::vector<expr_mod_t> &_changes)
      : x86Module(ModuleType(ModuleCategory::x86_ModifyHeader, _instance_id), "ModifyHeader", _node), chunk_addr_expr(_chunk_addr_expr),
        changes(_changes) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    ModifyHeader *cloned = new ModifyHeader(get_type().instance_id, node, chunk_addr_expr, changes);
    ModifyHeader *cloned = new ModifyHeader(get_type().instance_id, node, chunk_addr_expr, changes);
    return cloned;
  }

  klee::ref<klee::Expr> get_chunk_addr_expr() const { return chunk_addr_expr; }
  klee::ref<klee::Expr> get_chunk_addr_expr() const { return chunk_addr_expr; }
  const std::vector<expr_mod_t> &get_changes() const { return changes; }
};

class ModifyHeaderFactory : public x86ModuleFactory {
public:
  ModifyHeaderFactory(const InstanceId _instance_id) : x86ModuleFactory(ModuleType(ModuleCategory::x86_ModifyHeader, _instance_id), "ModifyHeader") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse