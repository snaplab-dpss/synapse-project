#pragma once

#include <LibSynapse/Target.h>
#include <LibCore/Debug.h>

#include <string>

namespace LibClone {

using DeviceId = i64;
using LibSynapse::TargetType;

class Device {
private:
  const DeviceId id;
  const TargetType target;

public:
  Device(const DeviceId &_id, const std::string &_arch)
      : id(_id), target([&] {
          if (_arch == "x86")
            return TargetType::x86;
          if (_arch == "Tofino")
            return TargetType::Tofino;
          if (_arch == "Controller")
            return TargetType::Controller;
          panic("Unknown architecture %s", _arch.c_str());
        }()) {}

  const DeviceId &get_id() const { return id; }
  const TargetType &get_target() const { return target; }

  friend std::ostream &operator<<(std::ostream &os, const Device &device) {
    os << "Device{ Id: " << device.id << " Target: ";
    switch (device.target) {
    case TargetType::Controller:
      os << "Ctrl";
      break;
    case TargetType::Tofino:
      os << "Tofino";
      break;
    case TargetType::x86:
      os << "x86";
      break;
    }
    os << " }";
    return os;
  }
};

} // namespace LibClone
