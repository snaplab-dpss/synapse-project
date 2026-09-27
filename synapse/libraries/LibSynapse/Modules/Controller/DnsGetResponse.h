#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DnsGetResponse : public ControllerModule {
private:
  addr_t msg_addr;
  klee::ref<klee::Expr> length;
  klee::ref<klee::Expr> dns_name;
  klee::ref<klee::Expr> address;
  klee::ref<klee::Expr> found;

public:
  DnsGetResponse(const BDDNode *_node, addr_t _msg_addr, klee::ref<klee::Expr> _length, klee::ref<klee::Expr> _dns_name, klee::ref<klee::Expr> _address,
                 klee::ref<klee::Expr> _found)
      : ControllerModule(ModuleType::Controller_DnsGetResponse, "DnsGetResponse", _node), msg_addr(_msg_addr), length(_length), dns_name(_dns_name),
        address(_address), found(_found) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DnsGetResponse(node, msg_addr, length, dns_name, address, found);
    return cloned;
  }

  addr_t get_msg_addr() const { return msg_addr; }
  klee::ref<klee::Expr> get_length() const { return length; }
  klee::ref<klee::Expr> get_dns_name() const { return dns_name; }
  klee::ref<klee::Expr> get_address() const { return address; }
  klee::ref<klee::Expr> get_found() const { return found; }
};

class DnsGetResponseFactory : public ControllerModuleFactory {
public:
  DnsGetResponseFactory() : ControllerModuleFactory(ModuleType::Controller_DnsGetResponse, "DnsGetResponse") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
