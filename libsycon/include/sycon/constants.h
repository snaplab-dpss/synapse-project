#pragma once

#include <stdint.h>

#include "../include/sycon/time.h"

extern "C" {
#include <bf_switchd/bf_switchd.h>
}

#include <port_mgr/bf_port_if.h>

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
constexpr bf_loopback_mode_e DEFAULT_PORT_LOOPBACK_MODE = BF_LPBK_NONE;
constexpr u16 DEFAULT_PORT_LANE = 0;
constexpr bool DEFAULT_RUN_WITH_MODEL = false;
constexpr bool DEFAULT_WAIT_FOR_PORTS = true;

constexpr u16 ALL_PIPES = 0xffff;
constexpr int SWITCH_PACKET_MAX_BUFFER_SIZE = 10000;

constexpr time_ms_t TOFINO_MIN_EXPIRATION_TIME = 100;
constexpr time_us_t TOFINO_DIGEST_TIMEOUT = 1;

constexpr char DATA_FIELD_NAME_ENTRY_HIT_STATE[] = "$ENTRY_HIT_STATE";
constexpr char DATA_FIELD_NAME_ENTRY_TTL[] = "$ENTRY_TTL";

constexpr char SYNAPSE_TABLE_MAP_ACTION[] = "populate";

}  // namespace sycon