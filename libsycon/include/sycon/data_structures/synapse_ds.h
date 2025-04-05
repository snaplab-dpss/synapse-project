#pragma once

namespace sycon {

class SynapseDS {
protected:
  const std::string name;

public:
  SynapseDS(const std::string _name) : name(_name) {}

  virtual ~SynapseDS() = default;
};

} // namespace sycon