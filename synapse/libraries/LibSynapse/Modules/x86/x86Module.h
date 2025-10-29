#pragma once

#include <LibSynapse/Modules/Module.h>
#include <LibSynapse/Modules/ModuleFactory.h>
#include <LibSynapse/Modules/x86/DataStructures/DataStructures.h>
#include <LibSynapse/Modules/x86/x86Context.h>

namespace LibSynapse {
namespace x86 {
class x86Module : public Module {
public:
  x86Module(ModuleType _type, const std::string &_name, const BDDNode *_node)
      : Module(_type, TargetType(TargetArchitecture::x86, _type.instance_id), _name, _node) {}

  x86Module(ModuleType _type, TargetType _next_target, const std::string &_name, const BDDNode *_node)
      : Module(_type, TargetType(TargetArchitecture::x86, _type.instance_id), _next_target, _name, _node) {}
};

class x86ModuleFactory : public ModuleFactory {
protected:
  ModuleType type;
  TargetType target;

public:
  x86ModuleFactory(ModuleType _type, const std::string &_name)
      : ModuleFactory(_type, TargetType(TargetArchitecture::x86, _type.instance_id), _name) {}

  static Symbols get_relevant_dataplane_state(const EP *ep, const BDDNode *node, TargetType type);
};

} // namespace x86
} // namespace LibSynapse
