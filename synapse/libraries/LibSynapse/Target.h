#pragma once

#include <LibCore/Types.h>

#include <LibSynapse/Modules/Tofino/TNA/TNA.h>

#include <memory>
#include <vector>

namespace LibClone {
class PhysicalNetwork;
} // namespace LibClone

namespace LibSynapse {

using InstanceId = i32;

class ModuleFactory;
class TargetContext;

enum class TargetArchitecture { x86, Tofino, Controller };

struct TargetType {
  TargetArchitecture type;
  InstanceId instance_id;

  TargetType() = default;
  TargetType(TargetArchitecture _type, InstanceId _instance_id) : type(_type), instance_id(_instance_id) {}
  bool operator==(const TargetType &other) const { return type == other.type && instance_id == other.instance_id; }
};

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

  Targets(const targets_config_t &config, const LibClone::PhysicalNetwork &physical_network);
  Targets(const Targets &other) = delete;
  Targets(Targets &&other)      = delete;

  TargetsView get_view() const;
};

} // namespace LibSynapse

namespace std {
template <> struct hash<LibSynapse::TargetType> {
  std::size_t operator()(const LibSynapse::TargetType &tt) const {
    return std::hash<int>()(static_cast<int>(tt.type)) ^ std::hash<LibSynapse::InstanceId>()(tt.instance_id);
  }
};
} // namespace std
