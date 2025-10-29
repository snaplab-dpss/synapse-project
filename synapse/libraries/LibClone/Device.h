#pragma once

#include <LibSynapse/Target.h>
#include <LibCore/Debug.h>

#include <string>

namespace LibClone {

using DeviceId = LibSynapse::InstanceId;

class Device {
private:
  const DeviceId id;
  const LibSynapse::TargetType target;

public:
  Device(const DeviceId &_id, const std::string &_arch)
      : id(_id), target([&] {
          if (_arch == "x86")
            return LibSynapse::TargetType(LibSynapse::TargetArchitecture::x86, _id);
          if (_arch == "Tofino")
            return LibSynapse::TargetType(LibSynapse::TargetArchitecture::Tofino, _id);
          if (_arch == "Controller")
            return LibSynapse::TargetType(LibSynapse::TargetArchitecture::Controller, _id);
          panic("Unknown architecture %s", _arch.c_str());
        }()) {}

  const DeviceId &get_id() const { return id; }
  const LibSynapse::TargetType &get_target() const { return target; }

  friend std::ostream &operator<<(std::ostream &os, const Device &device) {
    os << "Device{" << device.target << "}";
    return os;
  }
};

} // namespace LibClone
