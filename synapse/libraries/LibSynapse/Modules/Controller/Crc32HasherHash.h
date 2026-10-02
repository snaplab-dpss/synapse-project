#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

// crc32_hasher_hash computed by the controller with the hasher kept in its state.
class Crc32HasherHash : public ControllerModule {
private:
  addr_t hasher;
  klee::ref<klee::Expr> in;
  klee::ref<klee::Expr> hash;

public:
  Crc32HasherHash(const BDDNode *_node, addr_t _hasher, klee::ref<klee::Expr> _in, klee::ref<klee::Expr> _hash)
      : ControllerModule(ModuleType::Controller_Crc32HasherHash, "Crc32HasherHash", _node), hasher(_hasher), in(_in), hash(_hash) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new Crc32HasherHash(node, hasher, in, hash); }

  addr_t get_hasher() const { return hasher; }
  klee::ref<klee::Expr> get_in() const { return in; }
  klee::ref<klee::Expr> get_hash() const { return hash; }
};

class Crc32HasherHashFactory : public ControllerModuleFactory {
public:
  Crc32HasherHashFactory() : ControllerModuleFactory(ModuleType::Controller_Crc32HasherHash, "Crc32HasherHash") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
