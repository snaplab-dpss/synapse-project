#pragma once

#include <LibSynapse/Modules/Tofino/TNA/TNA.h>

#include <memory>
#include <vector>

namespace LibSynapse {

class ModuleFactory;
class TargetContext;

enum class TargetType { x86, Tofino, Controller };

std::ostream &operator<<(std::ostream &os, TargetType target);
std::string to_string(TargetType target);

struct targets_config_t {
  Tofino::tna_config_t tofino_config;
  bps_t controller_capacity;

  targets_config_t(const std::filesystem::path &targets_config_file);
};

struct TargetView {
  TargetType type;
  std::vector<const ModuleFactory *> module_factories;
  const TargetContext *base_ctx;

  TargetView(TargetType type, std::vector<const ModuleFactory *> module_factories, const TargetContext *base_ctx);
  TargetView(const TargetView &other) = default;
  TargetView(TargetView &&other)      = default;
};

struct Target {
  TargetType type;
  std::vector<std::unique_ptr<ModuleFactory>> module_factories;
  std::unique_ptr<TargetContext> base_ctx;

  Target(TargetType type, std::vector<std::unique_ptr<ModuleFactory>> module_factories, std::unique_ptr<TargetContext> base_ctx);
  Target(const Target &other) = delete;
  Target(Target &&other)      = delete;
  virtual ~Target()           = default;

  TargetView get_view() const;
};

struct TargetsView {
  std::vector<TargetView> elements;

  TargetsView(const std::vector<TargetView> &elements);
  TargetsView(const TargetsView &other) = default;
  TargetsView(TargetsView &&other)      = default;

  TargetView get_initial_target() const;
};

struct Targets {
  std::vector<std::unique_ptr<Target>> elements;

  Targets(const targets_config_t &config);
  Targets(const Targets &other) = delete;
  Targets(Targets &&other)      = delete;

  TargetsView get_view() const;
};

} // namespace LibSynapse