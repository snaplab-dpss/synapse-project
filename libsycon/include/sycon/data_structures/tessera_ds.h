#pragma once

#include <string>

namespace sycon {

class TesseraDS {
protected:
  const std::string name;

public:
  TesseraDS(const std::string _name) : name(_name) {}

  virtual ~TesseraDS() = default;
};

} // namespace sycon