#pragma once

#include <LibSynapse/Target.h>
#include <LibCore/Debug.h>

#include <string>

namespace LibClone {

using LibSynapse::TargetArchitecture;
using InstanceId = std::string;

class Device {
private:
  const InstanceId id;
  const TargetArchitecture architecture;

public:
  Device(const InstanceId &_id, const std::string &_arch)
      : id(_id), architecture([&] {
          if (_arch == "x86")
            return TargetArchitecture::x86;
          if (_arch == "Tofino")
            return TargetArchitecture::Tofino;
          if (_arch == "Controller")
            return TargetArchitecture::Controller;
          panic("Unknown architecture %s", _arch.c_str());
        }()) {}

  const InstanceId &get_id() const { return id; }
  const TargetArchitecture &get_architecture() const { return architecture; }

  friend std::ostream &operator<<(std::ostream &os, const Device &device) {
    os << "Device{" << device.id << ", ";
    if (device.architecture == TargetArchitecture::x86)
      os << "x86";
    else if (device.architecture == TargetArchitecture::Tofino)
      os << "Tofino";
    else if (device.architecture == TargetArchitecture::Controller)
      os << "Controller";
    os << "}";
    return os;
  }
};

} // namespace LibClone
