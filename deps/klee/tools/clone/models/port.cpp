#include "port.hpp"
#include "../pch.hpp"
#include "device.hpp"

namespace Clone {
	Port::Port(const DevicePtr device, unsigned local_port, unsigned global_port): 
		device(device), device_port(local_port), global_port(global_port) {}

	Port::~Port() = default;
}