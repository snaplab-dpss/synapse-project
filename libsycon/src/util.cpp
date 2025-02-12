#include "../include/sycon/util.h"
#include "../include/sycon/log.h"

#include <stdlib.h>

namespace sycon {

std::string read_env(const char *env_var) {
  auto value = getenv(env_var);

  if (!value) {
    ERROR("Env var \"%s\" not found", env_var);
  }

  return std::string(value);
}

} // namespace sycon