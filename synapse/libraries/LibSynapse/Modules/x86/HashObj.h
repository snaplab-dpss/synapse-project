#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class HashObj : public x86Module {
private:
  addr_t obj_addr;
  klee::ref<klee::Expr> size;
  klee::ref<klee::Expr> hash;

public:
  HashObj(ModuleType _type, const BDDNode *_node, addr_t _obj_addr, klee::ref<klee::Expr> _size, klee::ref<klee::Expr> _hash)
      : x86Module(_type, "HashObj", _node), obj_addr(_obj_addr), size(_size), hash(_hash) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new HashObj(type, node, obj_addr, size, hash);
    return cloned;
  }

  addr_t get_obj_addr() const { return obj_addr; }
  klee::ref<klee::Expr> get_size() const { return size; }
  klee::ref<klee::Expr> get_hash() const { return hash; }
};

class HashObjFactory : public x86ModuleFactory {
public:
  HashObjFactory(const std::string &_instance_id) : x86ModuleFactory(ModuleType(ModuleCategory::x86_HashObj, _instance_id), "HashObj") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse