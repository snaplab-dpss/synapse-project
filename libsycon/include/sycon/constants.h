#pragma once

#include <stdint.h>

#include "../include/sycon/time.h"

extern "C" {
#include <bf_switchd/bf_switchd.h>
#include <port_mgr/bf_port_if.h>
}

namespace sycon {

constexpr const char ENV_SDE_INSTALL[] = "SDE_INSTALL";

constexpr const char SDE_CONF_RELATIVE_PATH_TOFINO1[] = "share/p4/targets/tofino";
constexpr const char SDE_CONF_RELATIVE_PATH_TOFINO2[] = "share/p4/targets/tofino2";

constexpr const time_ms_t NEVER_EXPIRE = 0;

constexpr const bool DEFAULT_ARG_UCLI                         = false;
constexpr const bool DEFAULT_RUN_WITH_MODEL                   = false;
constexpr const bool DEFAULT_BENCH_CLI                        = false;
constexpr const int DEFAULT_TNA_VERSION                       = 2;
constexpr const time_ms_t DEFAULT_EXPIRATION_TIME             = NEVER_EXPIRE;
constexpr const bf_port_speed_t DEFAULT_PORT_SPEED            = BF_SPEED_100G;
constexpr const bf_loopback_mode_e DEFAULT_PORT_LOOPBACK_MODE = BF_LPBK_NONE;
constexpr const u16 DEFAULT_PORT_LANE                         = 0;
constexpr const bool DEFAULT_WAIT_FOR_PORTS                   = true;

constexpr const u16 ALL_PIPES                     = 0xffff;
constexpr const int SWITCH_PACKET_MAX_BUFFER_SIZE = 10000;

constexpr const time_ms_t TOFINO_MIN_EXPIRATION_TIME = 100;
constexpr const time_us_t TOFINO_DIGEST_TIMEOUT      = 1;
constexpr const char TOFINO_NO_ACTION_NAME[]         = "NoAction";

constexpr const char DATA_FIELD_NAME_ENTRY_HIT_STATE[] = "$ENTRY_HIT_STATE";
constexpr const char DATA_FIELD_NAME_ENTRY_TTL[]       = "$ENTRY_TTL";

constexpr const char SYNAPSE_TABLE_MAP_ACTION[] = "populate";

} // namespace sycon