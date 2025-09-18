#pragma once

#include <LibSynapse/Target.h>
#include <LibCore/Debug.h>

#include <string>

namespace LibClone {

using InstanceId = std::string;

class Device {
private:
  const LibSynapse::TargetType target;

public:
  Device(const InstanceId &_id, const std::string &_arch)
      : target([&] {
          if (_arch == "x86")
            return LibSynapse::TargetType(LibSynapse::TargetArchitecture::x86, _id);
          if (_arch == "Tofino")
            return LibSynapse::TargetType(LibSynapse::TargetArchitecture::Tofino, _id);
          if (_arch == "Controller")
            return LibSynapse::TargetType(LibSynapse::TargetArchitecture::Controller, _id);
          panic("Unknown architecture %s", _arch.c_str());
        }()) {}

  const LibSynapse::TargetType &get_target() const { return target; }

  friend std::ostream &operator<<(std::ostream &os, const Device &device) {
    os << "Device{" << device.target << "}";
    return os;
  }
};

} // namespace LibClone
