#pragma once

#include <LibClone/Device.h>
#include <LibCore/Types.h>

#include <string>
#include <memory>
#include <unordered_map>

namespace LibClone {

using GlobalPortId = u32;
using LocalPortId  = u32;

class Port {
private:
  const Device *device;
  const LocalPortId device_port;
  const GlobalPortId global_port;

public:
  Port(const Device *_device, LocalPortId _device_port, GlobalPortId _global_port)
      : device(_device), device_port(_device_port), global_port(_global_port) {
    assert(device && "Null device pointer in Port constructor");
  }

  ~Port() = default;

  const Device *get_device() const { return device; }
  LocalPortId get_local_port() const { return device_port; }
  GlobalPortId get_global_port() const { return global_port; }

  void debug() const { std::cerr << "Port{dev=" << device->get_id() << ",local=" << device_port << ",global=" << global_port << " }"; }
};

} // namespace LibClone