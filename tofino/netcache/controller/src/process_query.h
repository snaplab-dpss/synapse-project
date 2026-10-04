#pragma once

#include <memory>
#include <string>
#include <vector>

#include "packet.h"

typedef unsigned char byte_t;

namespace netcache {

class ProcessQuery {
public:
  static std::shared_ptr<ProcessQuery> process_query;

  ProcessQuery();
  ~ProcessQuery();

  void update_cache(struct netcache_hdr_t *nc_hdr);
};

} // namespace netcache
