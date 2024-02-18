#pragma once

#include "args.h"
#include "config.h"
#include "log.h"
#include "primitives/primitives.h"
#include "types.h"
#include "util.h"

namespace sycon {

void parse_args(int argc, char *argv[]);
void init_switchd();
void configure_ports();
void run_cli();

};  // namespace sycon