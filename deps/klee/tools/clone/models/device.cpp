#include "device.hpp"

#include "../pch.hpp"


namespace Clone {
	Device::Device(string id) : id(id) {}

	Device::~Device() = default;

	void Device::print() const {
		debug("Device ", id);
	}
}
