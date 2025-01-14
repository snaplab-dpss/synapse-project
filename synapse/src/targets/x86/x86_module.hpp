#pragma once

#include "../module.hpp"
#include "../module_factory.hpp"
#include "data_structures/data_structures.hpp"
#include "x86_context.hpp"

namespace synapse {
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
  x86ModuleFactory(ModuleType _type, const std::string &_name) : ModuleFactory(_type, TargetType::x86, _name) {}
};
} // namespace synapse