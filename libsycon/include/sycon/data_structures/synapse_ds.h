#pragma once

namespace sycon {

class SynapseDS {
protected:
  const std::string name;

public:
  SynapseDS(const std::string _name) : name(_name) {}

  virtual ~SynapseDS() = default;

  virtual void rollback() = 0;
  virtual void commit()   = 0;
};

} // namespace sycon