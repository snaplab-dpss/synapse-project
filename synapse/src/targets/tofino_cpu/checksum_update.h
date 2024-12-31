#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class ChecksumUpdate : public TofinoCPUModule {
private:
  addr_t ip_hdr_addr;
  addr_t l4_hdr_addr;
  symbol_t checksum;

public:
  ChecksumUpdate(const Node *node, addr_t _ip_hdr_addr, addr_t _l4_hdr_addr,
                 symbol_t _checksum)
      : TofinoCPUModule(ModuleType::TofinoCPU_ChecksumUpdate, "SetIpChecksum",
                        node),
        ip_hdr_addr(_ip_hdr_addr), l4_hdr_addr(_l4_hdr_addr),
        checksum(_checksum) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned =
        new ChecksumUpdate(node, ip_hdr_addr, l4_hdr_addr, checksum);
    return cloned;
  }

  addr_t get_ip_hdr_addr() const { return ip_hdr_addr; }
  addr_t get_l4_hdr_addr() const { return l4_hdr_addr; }
  const symbol_t &get_checksum() const { return checksum; }
};

class ChecksumUpdateGenerator : public TofinoCPUModuleGenerator {
public:
  ChecksumUpdateGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_ChecksumUpdate,
                                 "ChecksumUpdate") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override;
};

} // namespace tofino_cpu
