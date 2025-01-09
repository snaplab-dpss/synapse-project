#pragma once

#include "../module.h"
#include "../module_factory.h"

#include "controller_context.h"
#include "../tofino/tofino_context.h"

namespace synapse {
namespace ctrl {

class ControllerModule : public Module {
public:
  ControllerModule(ModuleType _type, const std::string &_name, const Node *node) : Module(_type, TargetType::Controller, _name, node) {}
};

class ControllerModuleFactory : public ModuleFactory {
protected:
  ModuleType type;
  TargetType target;

public:
  ControllerModuleFactory(ModuleType _type, const std::string &_name) : ModuleFactory(_type, TargetType::Controller, _name) {}

protected:
  const tofino::TofinoContext *get_tofino_ctx(const EP *ep) const;
  tofino::TofinoContext *get_mutable_tofino_ctx(EP *ep) const;

  const tofino::TNA &get_tna(const EP *ep) const;
  tofino::TNA &get_mutable_tna(EP *ep) const;
};

} // namespace ctrl
} // namespace synapse