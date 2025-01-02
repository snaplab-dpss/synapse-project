#pragma once

#include <toml++/toml.hpp>

#include <memory>
#include <vector>
#include <array>

namespace synapse {
class ModuleFactory;
class TargetContext;

enum class TargetType {
  x86,
  Tofino,
  Controller,
};

std::ostream &operator<<(std::ostream &os, TargetType target);
std::string to_string(TargetType target);

struct TargetView {
  TargetType type;
  std::vector<const ModuleFactory *> module_factories;
  const TargetContext *base_ctx;

  TargetView(TargetType type, std::vector<const ModuleFactory *> module_factories,
             const TargetContext *base_ctx);
  TargetView(const TargetView &other) = default;
  TargetView(TargetView &&other) = default;
};

struct Target {
  TargetType type;
  std::vector<std::unique_ptr<ModuleFactory>> module_factories;
  std::unique_ptr<TargetContext> base_ctx;

  Target(TargetType type, std::vector<std::unique_ptr<ModuleFactory>> module_factories,
         std::unique_ptr<TargetContext> base_ctx);
  Target(const Target &other) = delete;
  Target(Target &&other) = delete;
  virtual ~Target() = default;

  TargetView get_view() const;
};

struct TargetsView {
  std::array<TargetView, 3> elements;

  TargetsView(const std::array<TargetView, 3> &elements);
  TargetsView(const TargetsView &other) = default;
  TargetsView(TargetsView &&other) = default;

  TargetView get_initial_target() const;
};

struct Targets {
  std::array<std::unique_ptr<Target>, 3> elements;

  Targets(const toml::table &config);
  Targets(const Targets &other) = delete;
  Targets(Targets &&other) = delete;

  TargetsView get_view() const;
};
} // namespace synapse