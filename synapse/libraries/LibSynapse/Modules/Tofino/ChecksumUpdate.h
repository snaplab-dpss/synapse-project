#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

// nf_set_rte_ipv4_udptcp_checksum: the IPv4 and L4 checksums of a packet the data plane rewrote,
// recomputed by the deparser of the gress the packet leaves from (TofinoSynthesizer flags the
// path and emits the deparser's Checksum updates). The NF's later write of the checksum symbol
// into the header is dropped by ModifyHeader, the deparser writing the field itself.
class ChecksumUpdate : public TofinoModule {
private:
  addr_t ip_hdr_addr;
  addr_t l4_hdr_addr;
  symbol_t checksum;

public:
  ChecksumUpdate(const BDDNode *_node, addr_t _ip_hdr_addr, addr_t _l4_hdr_addr, const symbol_t &_checksum)
      : TofinoModule(ModuleType::Tofino_ChecksumUpdate, "ChecksumUpdate", _node), ip_hdr_addr(_ip_hdr_addr), l4_hdr_addr(_l4_hdr_addr),
        checksum(_checksum) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new ChecksumUpdate(node, ip_hdr_addr, l4_hdr_addr, checksum); }

  addr_t get_ip_hdr_addr() const { return ip_hdr_addr; }
  addr_t get_l4_hdr_addr() const { return l4_hdr_addr; }
  const symbol_t &get_checksum() const { return checksum; }
};

class ChecksumUpdateFactory : public TofinoModuleFactory {
public:
  ChecksumUpdateFactory() : TofinoModuleFactory(ModuleType::Tofino_ChecksumUpdate, "ChecksumUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
