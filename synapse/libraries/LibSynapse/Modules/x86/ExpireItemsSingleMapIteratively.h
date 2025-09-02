#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class ExpireItemsSingleMapIteratively : public x86Module {
private:
  addr_t vector_addr;
  addr_t map_addr;
  klee::ref<klee::Expr> start;
  klee::ref<klee::Expr> n_elems;

public:
  ExpireItemsSingleMapIteratively(const std::string &_instance_id, const BDDNode *_node, addr_t _vector_addr, addr_t _map_addr,
                                  klee::ref<klee::Expr> _start, klee::ref<klee::Expr> _n_elems)
      : x86Module(ModuleType(ModuleCategory::x86_ExpireItemsSingleMapIteratively, _instance_id), "ExpireItemsSingleMapIteratively", _node),
        vector_addr(_vector_addr), map_addr(_map_addr), start(_start), n_elems(_n_elems) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new ExpireItemsSingleMapIteratively(get_type().instance_id, node, map_addr, vector_addr, start, n_elems);
    return cloned;
  }

  addr_t get_vector_addr() const { return vector_addr; }
  addr_t get_map_addr() const { return map_addr; }
  klee::ref<klee::Expr> get_start() const { return start; }
  klee::ref<klee::Expr> get_n_elems() const { return n_elems; }
};

class ExpireItemsSingleMapIterativelyFactory : public x86ModuleFactory {
public:
  ExpireItemsSingleMapIterativelyFactory(const std::string &_instance_id)
      : x86ModuleFactory(ModuleType(ModuleCategory::x86_ExpireItemsSingleMapIteratively, _instance_id), "ExpireItemsSingleMapIteratively") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse