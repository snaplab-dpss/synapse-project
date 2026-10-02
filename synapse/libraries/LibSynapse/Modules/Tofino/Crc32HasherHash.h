#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

// crc32_hasher_hash: the hash unit computes the CRC the hasher was initialized with (the NF's
// init section), so a hasher built from CRC32_BANK[i] hashes as the i-th polynomial here too.
class Crc32HasherHash : public TofinoModule {
private:
  DS_ID hash_id;
  addr_t hasher;
  klee::ref<klee::Expr> in;
  klee::ref<klee::Expr> hash;

public:
  Crc32HasherHash(const BDDNode *_node, DS_ID _hash_id, addr_t _hasher, klee::ref<klee::Expr> _in, klee::ref<klee::Expr> _hash)
      : TofinoModule(ModuleType::Tofino_Crc32HasherHash, "Crc32HasherHash", _node), hash_id(_hash_id), hasher(_hasher), in(_in), hash(_hash) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new Crc32HasherHash(node, hash_id, hasher, in, hash); }

  DS_ID get_hash_id() const { return hash_id; }
  addr_t get_hasher() const { return hasher; }
  klee::ref<klee::Expr> get_in() const { return in; }
  klee::ref<klee::Expr> get_hash() const { return hash; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {hash_id}; }
};

class Crc32HasherHashFactory : public TofinoModuleFactory {
public:
  Crc32HasherHashFactory() : TofinoModuleFactory(ModuleType::Tofino_Crc32HasherHash, "Crc32HasherHash") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
