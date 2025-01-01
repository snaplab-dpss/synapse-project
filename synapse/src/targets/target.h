#pragma once

#include <toml++/toml.hpp>

#include <memory>
#include <vector>
#include <array>

class ModuleFactory;
class TargetContext;

enum class TargetType {
  x86,
  Tofino,
  TofinoCPU,
};

std::ostream &operator<<(std::ostream &os, TargetType target);
std::string to_string(TargetType target);

struct Target {
  TargetType type;
  std::vector<std::unique_ptr<ModuleFactory>> module_factories;
  std::unique_ptr<TargetContext> ctx;

  Target(TargetType type, std::vector<std::unique_ptr<ModuleFactory>> &&module_factories,
         std::unique_ptr<TargetContext> &&ctx);

  Target(const Target &other) = delete;
  Target(Target &&other) = delete;

  virtual ~Target();
};

struct Targets {
  std::array<std::shared_ptr<const Target>, 3> elements;

  Targets(const toml::table &config);
  Targets(const Targets &other);
};
