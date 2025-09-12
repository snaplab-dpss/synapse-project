#include <LibCore/Types.h>
#include <LibCore/Net.h>
#include <LibCore/Math.h>

#include <sstream>
#include <iostream>

namespace LibCore {

namespace {
constexpr const char THOUSANDS_SEPARATOR = '\'';

std::pair<double, std::string> n2hr(u64 n) {
  if (n < 1e3) {
    return {n, ""};
  }

  if (n < 1e6) {
    return {n / 1e3, "K"};
  }

  if (n < 1e9) {
    return {n / 1e6, "M"};
  }

  if (n < 1e12) {
    return {n / 1e9, "G"};
  }

  return {n / 1e12, "T"};
}
} // namespace

pps_t bps2pps(bps_t bps, bytes_t pkt_size) { return bps / ((PREAMBLE_SIZE_BYTES + pkt_size + CRC_SIZE_BYTES + IPG_SIZE_BYTES) * 8); }
bps_t pps2bps(pps_t pps, bytes_t pkt_size) { return pps * (PREAMBLE_SIZE_BYTES + pkt_size + CRC_SIZE_BYTES + IPG_SIZE_BYTES) * 8; }

std::string int2hr(i64 value) {
  std::stringstream ss;
  std::string str = std::to_string(value);

  for (int i = str.size() - 3; i > 0; i -= 3) {
    str.insert(i, 1, THOUSANDS_SEPARATOR);
  }

  ss << str;
  return ss.str();
}

std::string scientific(double value) {
  std::stringstream ss;
  ss.setf(std::ios::scientific);
  ss.precision(2);
  ss << value;
  return ss.str();
}

std::string tput2str(u64 thpt, std::string units, bool human_readable) {
  std::stringstream ss;

  if (human_readable) {
    auto [n, multiplier] = n2hr(thpt);

    ss.setf(std::ios::fixed);
    ss.precision(2);

    ss << n;
    ss << " ";
    ss << multiplier;
  } else {
    std::string str = std::to_string(thpt);

    // Add thousands separator
    for (int i = str.size() - 3; i > 0; i -= 3) {
      str.insert(i, 1, THOUSANDS_SEPARATOR);
    }

    ss << str;
    ss << " ";
  }

  ss << units;
  return ss.str();
}

std::string percent2str(double value, int precision) {
  std::stringstream ss;
  ss.setf(std::ios::fixed);
  ss.precision(precision);
  ss << (value * 100.0) << "%";
  return ss.str();
}

std::string percent2str(double numerator, double denominator, int precision) {
  if (denominator == 0) {
    return "infinity%";
  }
  double value = numerator / denominator;
  return percent2str(value, precision);
}

bits_t bits_from_pow2_capacity(size_t capacity) {
  assert(is_power_of_two(capacity) && "Size must be a power of 2");
  bits_t size = 0;
  while (capacity >>= 1) {
    size++;
  }
  return size;
}

} // namespace LibCore