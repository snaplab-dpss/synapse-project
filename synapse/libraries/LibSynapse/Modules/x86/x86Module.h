#pragma once

#include <LibSynapse/Modules/Module.h>
#include <LibSynapse/Modules/ModuleFactory.h>
#include <LibSynapse/Modules/x86/DataStructures/DataStructures.h>
#include <LibSynapse/Modules/x86/x86Context.h>

namespace LibSynapse {

class x86Module : public Module {
public:
  x86Module(ModuleType _type, const std::string &_name, const BDDNode *_node) : Module(_type, TargetType::x86, _name, _node) {}
};

class x86ModuleFactory : public ModuleFactory {
protected:
  ModuleType type;
  TargetType target;

public:
  x86ModuleFactory(ModuleType _type, const std::string &_name) : ModuleFactory(_type, TargetType::x86, _name) {}
};

} // namespace LibSynapse