#pragma once

#include <LibTessera/Modules/Module.h>
#include <LibTessera/Modules/ModuleFactory.h>
#include <LibTessera/Modules/x86/DataStructures/DataStructures.h>
#include <LibTessera/Modules/x86/x86Context.h>

namespace LibTessera {

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

} // namespace LibTessera