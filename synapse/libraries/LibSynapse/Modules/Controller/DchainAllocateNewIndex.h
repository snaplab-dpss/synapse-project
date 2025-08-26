#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DchainAllocateNewIndex : public ControllerModule {
private:
  addr_t dchain_addr;
  klee::ref<klee::Expr> time;
  klee::ref<klee::Expr> index_out;
  std::optional<symbol_t> not_out_of_space;

public:
  DchainAllocateNewIndex(ModuleType _type, const BDDNode *_node, addr_t _dchain_addr, klee::ref<klee::Expr> _time, klee::ref<klee::Expr> _index_out,
                         const symbol_t &_out_of_space)
      : ControllerModule(_type, "DchainAllocate", _node), dchain_addr(_dchain_addr), time(_time), index_out(_index_out),
        not_out_of_space(_out_of_space) {}

  DchainAllocateNewIndex(ModuleType _type, const BDDNode *_node, addr_t _dchain_addr, klee::ref<klee::Expr> _time, klee::ref<klee::Expr> _index_out)
      : ControllerModule(_type, "DchainAllocate", _node), dchain_addr(_dchain_addr), time(_time), index_out(_index_out),
        not_out_of_space(std::nullopt) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned;

    if (not_out_of_space.has_value()) {
      cloned = new DchainAllocateNewIndex(type, node, dchain_addr, time, index_out, *not_out_of_space);
    } else {
      cloned = new DchainAllocateNewIndex(type, node, dchain_addr, time, index_out);
    }

    return cloned;
  }

  addr_t get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_time() const { return time; }
  klee::ref<klee::Expr> get_index_out() const { return index_out; }

  std::optional<symbol_t> get_not_out_of_space() const { return not_out_of_space; }
};

class DchainAllocateNewIndexFactory : public ControllerModuleFactory {
public:
  DchainAllocateNewIndexFactory(const std::string &_instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_DchainAllocateNewIndex, _instance_id), "DchainAllocateNewIndex") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse