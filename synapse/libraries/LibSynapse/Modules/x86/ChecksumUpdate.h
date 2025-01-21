#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class ChecksumUpdate : public x86Module {
private:
  addr_t ip_hdr_addr;
  addr_t l4_hdr_addr;
  LibCore::symbol_t checksum;

public:
  ChecksumUpdate(const LibBDD::Node *node, addr_t _ip_hdr_addr, addr_t _l4_hdr_addr, LibCore::symbol_t _checksum)
      : x86Module(ModuleType::x86_ChecksumUpdate, "SetIpChecksum", node), ip_hdr_addr(_ip_hdr_addr), l4_hdr_addr(_l4_hdr_addr),
        checksum(_checksum) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new ChecksumUpdate(node, ip_hdr_addr, l4_hdr_addr, checksum);
    return cloned;
  }

  addr_t get_ip_hdr_addr() const { return ip_hdr_addr; }
  addr_t get_l4_hdr_addr() const { return l4_hdr_addr; }
  const LibCore::symbol_t &get_checksum() const { return checksum; }
};

class ChecksumUpdateFactory : public x86ModuleFactory {
public:
  ChecksumUpdateFactory() : x86ModuleFactory(ModuleType::x86_ChecksumUpdate, "ChecksumUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
};

} // namespace x86
} // namespace LibSynapse