#include <sstream>

#include "target.h"
#include "module.h"
#include "module_factory.h"
#include "targets.h"

Target::Target(TargetType _type,
               std::vector<std::unique_ptr<ModuleFactory>> &&_module_factories,
               std::unique_ptr<TargetContext> &&_ctx)
    : type(_type), module_factories(std::move(_module_factories)), ctx(std::move(_ctx)) {}

Target::~Target() {}

std::ostream &operator<<(std::ostream &os, TargetType target) {
  switch (target) {
  case TargetType::TofinoCPU:
    os << "Ctrl";
    break;
  case TargetType::Tofino:
    os << "Tofino";
    break;
  case TargetType::x86:
    os << "x86";
    break;
  }
  return os;
}

std::string to_string(TargetType target) {
  std::stringstream ss;
  ss << target;
  return ss.str();
}

Targets::Targets(const toml::table &config)
    : elements{
          std::make_shared<tofino::TofinoTarget>(config),
          std::make_shared<tofino_cpu::TofinoCPUTarget>(),
          std::make_shared<x86::x86Target>(),
      } {}

Targets::Targets(const Targets &other) : elements(other.elements) {}