#pragma once

#include "../module.h"
#include "../module_factory.h"
#include "data_structures/data_structures.h"
#include "x86_context.h"

class x86Module : public Module {
public:
  x86Module(ModuleType _type, const std::string &_name, const Node *_node)
      : Module(_type, TargetType::x86, _name, _node) {}
};

class x86ModuleFactory : public ModuleFactory {
protected:
  ModuleType type;
  TargetType target;

public:
  x86ModuleFactory(ModuleType _type, const std::string &_name)
      : ModuleFactory(_type, TargetType::x86, _name) {}
};