#pragma once

#include <LibCore/Types.h>

#include <iostream>
#include <cassert>
#include <string>
#include <unordered_map>
#include <memory>

namespace LibClone {

using DeviceId = std::string;

class Device {
private:
  const DeviceId id;
  std::unordered_map<u32, u32> ports;

public:
  Device(const DeviceId &_id) : id(_id) {}
  ~Device() = default;

  const DeviceId &get_id() const { return id; }

  u32 get_port(const u32 local_port) const {
    assert(ports.find(local_port) != ports.end());
    return ports.at(local_port);
  }

  void add_port(const u32 local_port, const u32 global_port) { ports[local_port] = global_port; }

  void debug() const { std::cerr << "Dev{" << id << "}"; }
};

} // namespace LibClone