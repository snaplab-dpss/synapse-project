#pragma once

#include <memory>
#include <string>

#include "conf.h"

namespace netcache {

class PortStats {
private:
  conf_t conf;
  std::vector<uint16_t> ports;
  bool use_tofino_model;

public:
  PortStats();
  ~PortStats();

  void get_stats();
  void reset_stats();
};

} // namespace netcache
