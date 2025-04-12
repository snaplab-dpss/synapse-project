#pragma once

#include <LibSynapse/Modules/Module.h>
#include <LibSynapse/Modules/ModuleFactory.h>
#include <LibSynapse/Modules/Controller/ControllerContext.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>

namespace LibSynapse {
namespace Controller {

class ControllerModule : public Module {
public:
  ControllerModule(ModuleType _type, const std::string &_name, const LibBDD::Node *_node) : Module(_type, TargetType::Controller, _name, _node) {}
};

class ControllerModuleFactory : public ModuleFactory {
protected:
  ModuleType type;
  TargetType target;

public:
  ControllerModuleFactory(ModuleType _type, const std::string &_name) : ModuleFactory(_type, TargetType::Controller, _name) {}

protected:
  const Tofino::TofinoContext *get_tofino_ctx(const EP *ep) const;
  Tofino::TofinoContext *get_mutable_tofino_ctx(EP *ep) const;

  const Tofino::TNA &get_tna(const EP *ep) const;
  Tofino::TNA &get_mutable_tna(EP *ep) const;
};

} // namespace Controller
} // namespace LibSynapse