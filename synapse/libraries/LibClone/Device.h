#pragma once

#include <LibCore/Debug.h>
#include <LibCore/Types.h>

#include <LibSynapse/Target.h>

#include <string>
#include <cstdint>
#include <utility>

namespace LibClone {

using DeviceId = i64;
using LibSynapse::TargetType;

struct ResourcePair {
  u64 first;
  u64 second;

  ResourcePair(u64 f = 0, u64 s = 0) : first(f), second(s) {}

  bool operator==(const ResourcePair &other) const { return first == other.first && second == other.second; }

  std::string to_string() const { return "(" + std::to_string(first) + ", " + std::to_string(second) + ")"; }
};

struct DeviceResources {
  virtual ~DeviceResources()                       = default;
  virtual TargetType get_target_type() const       = 0;
  virtual std::string to_string() const            = 0;
  virtual ResourcePair get_total_resources() const = 0;
};

struct x86Resources : public DeviceResources {
  u64 capacity_pps;
  u64 memory_bytes;

  x86Resources(u64 pps = 1048976, u64 memory = 4718676) : capacity_pps(pps), memory_bytes(memory) {}

  TargetType get_target_type() const override { return TargetType::x86; }

  ResourcePair get_total_resources() const override { return ResourcePair(capacity_pps, memory_bytes); }

  std::string to_string() const override { return "x86{pps: " + std::to_string(capacity_pps) + ", mem: " + std::to_string(memory_bytes) + "}"; }
};

struct TofinoResources : public DeviceResources {
  // Pipeline structure
  u32 num_pipes;
  u32 num_stages;

  // Resources per stage
  u64 sram_per_stage;
  u64 tcam_per_stage;
  u32 salus_per_stage;

  // Controller capacity (slow path)
  u64 controller_pps;

  // Data plane capacity
  u64 capacity_pps;

  // Totals (computed)
  u64 total_sram() const { return num_pipes * num_stages * sram_per_stage; }
  u64 total_tcam() const { return num_pipes * num_stages * tcam_per_stage; }
  u32 total_salus() const { return num_pipes * num_stages * salus_per_stage; }

  TofinoResources(u32 pipes = 2, u32 stages = 12,
                  u64 sram          = 10485760, // 10MB per stage
                  u64 tcam          = 540672,   // ~540KB per stage
                  u32 salus         = 10,
                  u64 dataplane_pps = 2250000000, // 2.25B pps
                  u64 ctrl_pps      = 100000      // 100K pps controller
                  )
      : num_pipes(pipes), num_stages(stages), sram_per_stage(sram), tcam_per_stage(tcam), salus_per_stage(salus), controller_pps(ctrl_pps),
        capacity_pps(dataplane_pps) {}

  TargetType get_target_type() const override { return TargetType::Tofino; }

  ResourcePair get_total_resources() const override { return ResourcePair(total_sram(), total_tcam()); }

  std::tuple<u64, u64, u64, u32> get_all_resources() const { return {total_sram(), total_tcam(), controller_pps, total_salus()}; }

  std::string to_string() const override {
    return "Tofino{pipes: " + std::to_string(num_pipes) + ", stages: " + std::to_string(num_stages) + ", sram: " + std::to_string(total_sram()) +
           ", tcam: " + std::to_string(total_tcam()) + ", salus: " + std::to_string(total_salus()) +
           ", dataplane_pps: " + std::to_string(capacity_pps) + ", controller_pps: " + std::to_string(controller_pps) + "}";
  }
};

class Device {
private:
  const DeviceId id;
  const TargetType target;
  std::unique_ptr<DeviceResources> resources;

public:
  Device(const DeviceId &_id, u64 pps, u64 memory) : id(_id), target(TargetType::x86), resources(std::make_unique<x86Resources>(pps, memory)) {}

  Device(const DeviceId &_id, u32 pipes, u32 stages, u64 sram_per_stage, u64 tcam_per_stage, u32 salus_per_stage, u64 dataplane_pps,
         u64 controller_pps)
      : id(_id), target(TargetType::Tofino),
        resources(std::make_unique<TofinoResources>(pipes, stages, sram_per_stage, tcam_per_stage, salus_per_stage, dataplane_pps, controller_pps)) {}

  Device(const DeviceId &_id, const std::string &_arch)
      : id(_id), target([&] {
          if (_arch == "x86")
            return TargetType::x86;
          if (_arch == "Tofino")
            return TargetType::Tofino;
          panic("Unknown architecture %s", _arch.c_str());
        }()) {
    switch (target) {
    case TargetType::x86:
      resources = std::make_unique<x86Resources>();
      break;
    case TargetType::Controller:
    case TargetType::Tofino:
      resources = std::make_unique<TofinoResources>();
      break;
    }
  }

  const DeviceId &get_id() const { return id; }
  const TargetType &get_target() const { return target; }
  const DeviceResources *get_resources() const { return resources.get(); }

  ResourcePair get_total_resources() const { return resources->get_total_resources(); }

  u64 get_primary_resource() const { return get_total_resources().first; }
  u64 get_secondary_resource() const { return get_total_resources().second; }

  const x86Resources *get_x86_resources() const {
    assert_or_panic(target == TargetType::x86, "Target is not x86");
    return static_cast<const x86Resources *>(resources.get());
  }

  const TofinoResources *get_tofino_resources() const {
    assert_or_panic(target == TargetType::Tofino, "Target is not Tofino");
    return static_cast<const TofinoResources *>(resources.get());
  }

  friend std::ostream &operator<<(std::ostream &os, const Device &device) {
    os << "Device{id: " << device.id << ", ";
    os << device.resources->to_string() << "}";
    return os;
  }
};

} // namespace LibClone
