#include <LibCore/Types.h>
#include <LibCore/Net.h>

#include <sstream>
#include <iostream>

namespace LibCore {

namespace {
constexpr const char THOUSANDS_SEPARATOR = '\'';

std::pair<u64, std::string> n2hr(u64 n) {
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

std::string int2hr(u64 value) {
  std::stringstream ss;
  std::string str = std::to_string(value);

  for (int i = str.size() - 3; i > 0; i -= 3) {
    str.insert(i, 1, THOUSANDS_SEPARATOR);
  }

  ss << str;
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

bits_t bits_from_pow2_capacity(size_t capacity) {
  assert((capacity & (capacity - 1)) == 0 && "Size must be a power of 2");
  bits_t size = 0;
  while (capacity >>= 1) {
    size++;
  }
  return size;
}

} // namespace LibCore