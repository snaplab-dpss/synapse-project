#pragma once

#include <stdint.h>

#include "../include/sycon/types.h"

extern "C" {
#include <bf_switchd/bf_switchd.h>
}

namespace sycon {

constexpr char ENV_SDE_INSTALL[] = "SDE_INSTALL";

constexpr char SDE_CONF_RELATIVE_PATH_TOFINO1[] = "share/p4/targets/tofino";
constexpr char SDE_CONF_RELATIVE_PATH_TOFINO2[] = "share/p4/targets/tofino2";

constexpr time_ms_t NEVER_EXPIRE = 0;

constexpr bool DEFAULT_ARG_UCLI = false;
constexpr int DEFAULT_TNA_VERSION = 2;
constexpr time_ms_t DEFAULT_EXPIRATION_TIME = NEVER_EXPIRE;
constexpr int DEFAULT_IN_PORT = 0;
constexpr int DEFAULT_OUT_PORT = 1;
constexpr bf_port_speed_t DEFAULT_PORT_SPEED = BF_SPEED_100G;
constexpr uint16_t DEFAULT_PORT_LANE = 0;
constexpr bool DEFAULT_RUN_WITH_MODEL = false;
constexpr bool DEFAULT_WAIT_FOR_PORTS = true;

constexpr uint16_t ALL_PIPES = 0xffff;

}  // namespace sycon