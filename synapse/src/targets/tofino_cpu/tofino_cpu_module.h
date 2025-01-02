#pragma once

#include "../module.h"
#include "../module_factory.h"

#include "tofino_cpu_context.h"
#include "../tofino/tofino_context.h"

namespace synapse {
namespace tofino_cpu {

class TofinoCPUModule : public Module {
public:
  TofinoCPUModule(ModuleType _type, const std::string &_name, const Node *node)
      : Module(_type, TargetType::TofinoCPU, _name, node) {}
};

class TofinoCPUModuleFactory : public ModuleFactory {
protected:
  ModuleType type;
  TargetType target;

public:
  TofinoCPUModuleFactory(ModuleType _type, const std::string &_name)
      : ModuleFactory(_type, TargetType::TofinoCPU, _name) {}

protected:
  const tofino::TofinoContext *get_tofino_ctx(const EP *ep) const;
  tofino::TofinoContext *get_mutable_tofino_ctx(EP *ep) const;

  const tofino::TNA &get_tna(const EP *ep) const;
  tofino::TNA &get_mutable_tna(EP *ep) const;
};

} // namespace tofino_cpu
} // namespace synapse