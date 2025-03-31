#pragma once

#include <cstdint>
#include <klee/Expr.h>

#include <iomanip>

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

constexpr const double EPSILON = 1e-8;

constexpr const u64 TRILLION = 1000000000000LLU;
constexpr const u64 BILLION  = 1000000000LLU;
constexpr const u64 MILLION  = 1000000LLU;
constexpr const u64 THOUSAND = 1000LLU;

#define align_to_byte(B) ((B) % 8 == 0 ? (B) : (B) + 8 - (B) % 8)
#define bswap16(x) __builtin_bswap16(x)
#define bswap32(x) __builtin_bswap32(x)

struct hit_rate_t {
  double value;

  hit_rate_t() : value(0) {}
  hit_rate_t(double numerator, double denominator) : hit_rate_t(denominator == 0 ? 0.0 : numerator / denominator) {}
  hit_rate_t(const hit_rate_t &other) : value(other.value) { clamp(); }
  hit_rate_t(hit_rate_t &&other) : value(std::move(other.value)) { clamp(); }

  explicit hit_rate_t(double value) : value(value) { clamp(); }

  hit_rate_t &operator=(const hit_rate_t &other) {
    value = other.value;
    clamp();
    return *this;
  }

  hit_rate_t &operator=(hit_rate_t &&other) {
    value = std::move(other.value);
    clamp();
    return *this;
  }

  hit_rate_t operator+(const hit_rate_t other) const { return hit_rate_t(value + other.value); }
  hit_rate_t operator-(const hit_rate_t other) const { return hit_rate_t(value - other.value); }

  // Multiplication and division between hit rates result in a scaler.
  double operator*(const hit_rate_t other) const { return value * other.value; }
  double operator/(const hit_rate_t other) const { return value / other.value; }

  hit_rate_t operator+(const double n) const { return hit_rate_t(value + n); }
  hit_rate_t operator-(const double n) const { return hit_rate_t(value - n); }
  hit_rate_t operator*(const double n) const { return hit_rate_t(value * n); }
  hit_rate_t operator/(const double n) const { return hit_rate_t(value / n); }

  bool operator==(const hit_rate_t &other) const { return std::abs(value - other.value) <= EPSILON; }
  bool operator!=(const hit_rate_t &other) const { return !(*this == other); }
  bool operator<(const hit_rate_t &other) const { return value < other.value; }
  bool operator>(const hit_rate_t &other) const { return !((*this < other) || (*this == other)); }
  bool operator<=(const hit_rate_t &other) const { return (*this < other) || (*this == other); }
  bool operator>=(const hit_rate_t &other) const { return !(*this < other); }

  bool operator==(const double n) const { return std::abs(value - n) <= EPSILON; }
  bool operator!=(const double n) const { return !(*this == n); }
  bool operator<(const double n) const { return value < n; }
  bool operator>(const double n) const { return !((*this < n) || (*this == n)); }
  bool operator<=(const double n) const { return (*this < n) || (*this == n); }
  bool operator>=(const double n) const { return !(*this < n); }

  hit_rate_t pow(const double exp) const { return hit_rate_t(std::pow(value, exp)); }
  hit_rate_t pow(const int exp) const { return hit_rate_t(std::pow(value, exp)); }

  void clamp() {
    value = std::max(0.0, value);
    value = std::min(1.0, value);
    if (value < EPSILON) {
      value = 0;
    }
  }
};

inline hit_rate_t operator"" _hr(long double n) { return hit_rate_t(n); }
inline hit_rate_t operator"" _hr(unsigned long long n) { return hit_rate_t(static_cast<double>(n)); }

inline hit_rate_t operator+(const double n, const hit_rate_t hr) { return hit_rate_t(n + hr.value); }
inline hit_rate_t operator-(const double n, const hit_rate_t hr) { return hit_rate_t(n - hr.value); }
inline hit_rate_t operator*(const double n, const hit_rate_t hr) { return hit_rate_t(n * hr.value); }
inline hit_rate_t operator/(const double n, const hit_rate_t hr) { return hit_rate_t(n / hr.value); }

inline std::ostream &operator<<(std::ostream &os, const hit_rate_t &hr) {
  os << std::fixed << std::setprecision(6) << hr.value;
  return os;
}

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
bits_t bits_from_pow2_capacity(size_t capacity);

} // namespace LibCore
