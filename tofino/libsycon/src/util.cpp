#include "../include/sycon/util.h"

#include <stdlib.h>

#include "../include/sycon/log.h"

namespace sycon {

std::string read_env(const char *env_var) {
  auto value = getenv(env_var);

  if (!value) {
    ERROR("Env var \"%s\" not found", env_var);
  }

  return std::string(value);
}

}  // namespace sycon