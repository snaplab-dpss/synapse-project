#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class ChecksumUpdate : public x86Module {
private:
  addr_t ip_hdr_addr;
  addr_t l4_hdr_addr;
  symbol_t checksum;

public:
  ChecksumUpdate(const BDDNode *_node, addr_t _ip_hdr_addr, addr_t _l4_hdr_addr, symbol_t _checksum)
      : x86Module(ModuleType::x86_ChecksumUpdate, "SetIpChecksum", _node), ip_hdr_addr(_ip_hdr_addr), l4_hdr_addr(_l4_hdr_addr), checksum(_checksum) {
  }

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new ChecksumUpdate(node, ip_hdr_addr, l4_hdr_addr, checksum);
    return cloned;
  }

  addr_t get_ip_hdr_addr() const { return ip_hdr_addr; }
  addr_t get_l4_hdr_addr() const { return l4_hdr_addr; }
  const symbol_t &get_checksum() const { return checksum; }
};

class ChecksumUpdateFactory : public x86ModuleFactory {
public:
  ChecksumUpdateFactory() : x86ModuleFactory(ModuleType::x86_ChecksumUpdate, "ChecksumUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse