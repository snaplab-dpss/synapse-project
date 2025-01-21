#pragma once

#include <stdint.h>
#include <klee/Expr.h>

using u64 = __UINT64_TYPE__;
using u32 = __UINT32_TYPE__;
using u16 = __UINT16_TYPE__;
using u8  = __UINT8_TYPE__;

using i64 = __INT64_TYPE__;
using i32 = __INT32_TYPE__;
using i16 = __INT16_TYPE__;
using i8  = __INT8_TYPE__;

using bits_t      = u32;
using bytes_t     = u32;
using code_path_t = u16;
using addr_t      = u64;

using time_s_t  = i64;
using time_ms_t = i64;
using time_us_t = i64;
using time_ns_t = i64;
using time_ps_t = i64;

using pps_t = u64;
using bps_t = u64;
using Bps_t = u64;

using fpm_t = u64;
using fps_t = u64;

using hit_rate_t = double;

constexpr const u64 TRILLION = 1000000000000LLU;
constexpr const u64 BILLION  = 1000000000LLU;
constexpr const u64 MILLION  = 1000000LLU;
constexpr const u64 THOUSAND = 1000LLU;

#define align_to_byte(B) ((B) % 8 == 0 ? (B) : (B) + 8 - (B) % 8)

struct rw_fractions_t {
  hit_rate_t read;
  hit_rate_t write_attempt;
  hit_rate_t write;
};

namespace LibCore {

pps_t bps2pps(bps_t bps, bytes_t pkt_size);
bps_t pps2bps(pps_t pps, bytes_t pkt_size);
std::string int2hr(u64 value);
std::string tput2str(u64 thpt, std::string units, bool human_readable = false);

} // namespace LibCore
