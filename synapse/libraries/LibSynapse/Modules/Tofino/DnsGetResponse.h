#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

// libnf's dns_get_response on the data plane: the parser walks the response's question and records
// (ParserStateDnsResponse) and the ingress puts the labels in the NF's order (struct dns_name:
// the count, then the labels top-level first). The labels themselves stay in the pieces the
// parser took them in -- the NF's contiguous bytes cannot be assembled on the chip (see
// tofino/experiments/dns-name-parse) -- so the table's key is that piece layout, and the
// controller writes the NF's prefixes in it (sycon::DnsNameLPM).
class DnsGetResponse : public TofinoModule {
private:
  addr_t msg_addr;
  klee::ref<klee::Expr> length;
  klee::ref<klee::Expr> dns_name;
  klee::ref<klee::Expr> address;
  klee::ref<klee::Expr> found;

public:
  // struct dns_name, from lib/util/dns_hdr.h: the label count, then a slot per label, top-level
  // label first, of its length byte and DNS_LABEL_BYTES bytes of it zero-padded.
  static constexpr const bytes_t LABEL_BYTES = 15;
  static constexpr const u32 MAX_LABELS      = 4;
  static constexpr const bytes_t SLOT_BYTES  = 1 + LABEL_BYTES;
  static constexpr const bytes_t NAME_BYTES  = 1 + MAX_LABELS * SLOT_BYTES;

  // The pieces a label is extracted in, by the binary digits of its length, in wire order, and
  // where each sits in the key's 16-byte label position after the length byte.
  struct slot_chunk_t {
    const char *suffix;
    bytes_t offset;
    bytes_t size;
  };
  static constexpr const slot_chunk_t SLOT_CHUNKS[] = {{"c1", 1, 1}, {"c2", 2, 2}, {"c4", 4, 4}, {"c8", 8, 8}};

  DnsGetResponse(const BDDNode *_node, addr_t _msg_addr, klee::ref<klee::Expr> _length, klee::ref<klee::Expr> _dns_name, klee::ref<klee::Expr> _address,
                 klee::ref<klee::Expr> _found)
      : TofinoModule(ModuleType::Tofino_DnsGetResponse, "DnsGetResponse", _node), msg_addr(_msg_addr), length(_length), dns_name(_dns_name),
        address(_address), found(_found) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    Module *cloned = new DnsGetResponse(node, msg_addr, length, dns_name, address, found);
    return cloned;
  }

  addr_t get_msg_addr() const { return msg_addr; }
  klee::ref<klee::Expr> get_length() const { return length; }
  klee::ref<klee::Expr> get_dns_name() const { return dns_name; }
  klee::ref<klee::Expr> get_address() const { return address; }
  klee::ref<klee::Expr> get_found() const { return found; }

  // The key's fields in order: the count, then per label position the length and the pieces.
  static std::vector<klee::ref<klee::Expr>> name_fields(klee::ref<klee::Expr> name);
};

class DnsGetResponseFactory : public TofinoModuleFactory {
public:
  DnsGetResponseFactory() : TofinoModuleFactory(ModuleType::Tofino_DnsGetResponse, "DnsGetResponse") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
