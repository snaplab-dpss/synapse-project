#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>
#include <LibBDD/Config.h>

namespace LibSynapse {
namespace Controller {

// crc32_hasher_init in the NF's init section: the controller keeps the same hasher (libsycon's
// CRC32 over libnf's crc32_hasher) in its state, for the hashes it computes itself.
class Crc32HasherInit : public ControllerModule {
private:
  addr_t hasher;
  LibBDD::crc32_hasher_config_t config;

public:
  Crc32HasherInit(const BDDNode *_node, addr_t _hasher, const LibBDD::crc32_hasher_config_t &_config)
      : ControllerModule(ModuleType::Controller_Crc32HasherInit, "Crc32HasherInit", _node), hasher(_hasher), config(_config) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new Crc32HasherInit(node, hasher, config); }

  addr_t get_hasher() const { return hasher; }
  const LibBDD::crc32_hasher_config_t &get_config() const { return config; }
};

class Crc32HasherInitFactory : public ControllerModuleFactory {
public:
  Crc32HasherInitFactory() : ControllerModuleFactory(ModuleType::Controller_Crc32HasherInit, "Crc32HasherInit") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
