#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class ChecksumUpdate : public x86Module {
private:
  klee::ref<klee::Expr> ip_hdr_addr;
  klee::ref<klee::Expr> l4_hdr_addr;
  symbol_t checksum;

public:
  ChecksumUpdate(const std::string &_instance_id, const BDDNode *_node, klee::ref<klee::Expr> _ip_hdr_addr, klee::ref<klee::Expr> _l4_hdr_addr,
                 symbol_t _checksum)
      : x86Module(ModuleType(ModuleCategory::x86_ChecksumUpdate, _instance_id), "SetIpChecksum", _node), ip_hdr_addr(_ip_hdr_addr),
        l4_hdr_addr(_l4_hdr_addr), checksum(_checksum) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new ChecksumUpdate(get_type().instance_id, node, ip_hdr_addr, l4_hdr_addr, checksum);
    return cloned;
  }

  klee::ref<klee::Expr> get_ip_hdr_addr() const { return ip_hdr_addr; }
  klee::ref<klee::Expr> get_l4_hdr_addr() const { return l4_hdr_addr; }
  const symbol_t &get_checksum() const { return checksum; }
};

class ChecksumUpdateFactory : public x86ModuleFactory {
public:
  ChecksumUpdateFactory(const std::string &_instance_id)
      : x86ModuleFactory(ModuleType(ModuleCategory::x86_ChecksumUpdate, _instance_id), "ChecksumUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse