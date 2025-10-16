#pragma once

#include <LibCore/Types.h>

#include <LibSynapse/Modules/Tofino/TNA/TNA.h>

#include <memory>
#include <vector>

namespace LibClone {
class PhysicalNetwork;
} // namespace LibClone

namespace LibSynapse {

using InstanceId = u64;

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

struct TargetViewHasher {
  std::size_t operator()(const LibSynapse::TargetView &tv) const noexcept {
    return std::hash<int>()(static_cast<int>(tv.type.type)) ^ std::hash<LibSynapse::InstanceId>()(tv.type.instance_id);
  }
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

struct TargetHasher {
  std::size_t operator()(const std::unique_ptr<LibSynapse::Target> &target) const noexcept {
    return std::hash<int>()(static_cast<int>(target->type.type)) ^ std::hash<LibSynapse::InstanceId>()(target->type.instance_id);
  }
};

struct TargetsView {
  std::unordered_map<TargetView, bool, TargetViewHasher> elements;

  TargetsView(const std::unordered_map<TargetView, bool, TargetViewHasher> &_elements);
  TargetsView(const TargetsView &other) = default;
  TargetsView(TargetsView &&other)      = default;

  TargetView get_initial_target() const;
};

inline bool operator==(const TargetView &a, const TargetView &b) {
  return a.type == b.type && a.module_factories == b.module_factories && a.base_ctx == b.base_ctx;
}

struct Targets {
  std::unordered_map<std::unique_ptr<Target>, bool, TargetHasher> elements;

  Targets(const targets_config_t &config, const std::unordered_map<LibSynapse::TargetType, bool> &targets);
  Targets(const Targets &other) = delete;
  Targets(Targets &&other)      = delete;

  TargetsView get_view() const;
};

inline bool operator==(const Target &a, const Target &b) {
  return a.type == b.type && a.module_factories == b.module_factories && a.base_ctx == b.base_ctx;
}

} // namespace LibSynapse

namespace std {
template <> struct hash<LibSynapse::TargetType> {
  std::size_t operator()(const LibSynapse::TargetType &tt) const {
    return std::hash<int>()(static_cast<int>(tt.type)) ^ std::hash<LibSynapse::InstanceId>()(tt.instance_id);
  }
};

} // namespace std
