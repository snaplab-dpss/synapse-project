#pragma once

#include "types.h"

namespace synapse {
constexpr const char THOUSANDS_SEPARATOR = '\'';

constexpr const u64 TRILLION = 1000000000000LLU;
constexpr const u64 BILLION = 1000000000LLU;
constexpr const u64 MILLION = 1000000LLU;
constexpr const u64 THOUSAND = 1000LLU;

constexpr const int NEWTON_MAX_ITERATIONS = 10;
constexpr const hit_rate_t NEWTON_PRECISION = 1e-3;
constexpr const pps_t STABLE_TPUT_PRECISION = 100;
constexpr const hit_rate_t HIT_RATE_EPSILON = 1e-6;

constexpr const u16 ETHERTYPE_IP = 0x0800;       /* IP */
constexpr const u16 ETHERTYPE_ARP = 0x0806;      /* Address resolution */
constexpr const u16 ETHERTYPE_REVARP = 0x8035;   /* Reverse ARP */
constexpr const u16 ETHERTYPE_AT = 0x809B;       /* AppleTalk protocol */
constexpr const u16 ETHERTYPE_AARP = 0x80F3;     /* AppleTalk ARP */
constexpr const u16 ETHERTYPE_VLAN = 0x8100;     /* IEEE 802.1Q VLAN tagging */
constexpr const u16 ETHERTYPE_IPX = 0x8137;      /* IPX */
constexpr const u16 ETHERTYPE_IPV6 = 0x86dd;     /* IP protocol version 6 */
constexpr const u16 ETHERTYPE_LOOPBACK = 0x9000; /* used to test interfaces */

constexpr const bytes_t CRC_SIZE_BYTES = 4;
constexpr const bytes_t PREAMBLE_SIZE_BYTES = 8;
constexpr const bytes_t IPG_SIZE_BYTES = 12;

constexpr const char SYNTHESIZER_INDENTATION_UNIT = ' ';
constexpr const int SYNTHESIZER_INDENTATION_MULTIPLIER = 2;

constexpr const char *const TEMPLATE_MARKER_AFFIX = "/*@{";
constexpr const char *const TEMPLATE_MARKER_SUFFIX = "}@*/";
} // namespace synapse