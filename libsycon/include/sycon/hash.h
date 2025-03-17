#pragma once

#include "buffer.h"

#include <vector>

namespace sycon {

class CRC32 {
private:
  const bytes_t width;

  buffer_t poly;
  bool reverse;
  buffer_t init;
  buffer_t final_xor;

  std::vector<buffer_t> matrix;

public:
  CRC32(u32 _poly, bool _reverse, u32 _init, u32 _final_xor) : width(4), poly(width), reverse(_reverse), init(width), final_xor(width) {
    poly.set(0, width, _poly);
    init.set(0, width, _init);
    final_xor.set(0, width, _final_xor);

    if (reverse) {
      poly = poly.reverse();
    }

    init_matrix();
  }

  CRC32() : CRC32(0x04C11DB7, true, 0, 0xffffffff) {}

  u32 hash(const buffer_t &input) const {
    buffer_t crc(width);

    crc = init;

    for (bytes_t i = 0; i < input.size; i++) {
      if (reverse) {
        u8 idx = crc[width - 1] ^ input.get(i);

        for (int j = width - 1; j > 0; j--) {
          crc[j] = crc[j - 1] ^ matrix[idx].get(j);
        }

        crc[0] = matrix[idx].get(0);
      } else {
        u8 idx = crc[0] ^ input.get(i);

        for (bytes_t j = 0; j < width - 1; j++) {
          crc[j] = (crc[j + 1] ^ matrix[idx].get(j)) & 0xff;
        }

        crc[width - 1] = matrix[idx].get(width - 1);
      }
    }

    crc = crc ^ final_xor;

    u32 hash_value = 0;
    for (bytes_t i = 0; i < width; i++) {
      const bytes_t rev_i = width - i - 1;
      hash_value          = (hash_value << 8) | (crc[width - rev_i - 1] & 0xff);
    }

    return hash_value;
  }

private:
  void init_matrix() {
    // Clear the matrix first
    matrix.clear();

    // Allocate matrix
    matrix.resize(256);
    for (size_t i = 0; i < matrix.size(); i++) {
      matrix[i] = buffer_t(width);
    }

    buffer_t rem(width);

    if (reverse) {
      for (bytes_t byte = 0; byte <= 0xff; byte++) {
        rem.clear();
        rem[width - 1] = byte;

        for (bits_t bit = 0; bit < 8; bit++) {
          u8 xor_val = rem[width - 1] & 0x1;
          rem        = rem.shift_right();

          if (xor_val) {
            rem = rem ^ poly;
          }
        }

        matrix[byte] = rem;
      }
    } else {
      u8 top_bit  = 0x80;
      u8 top_mask = 0xff;

      for (bytes_t byte = 0; byte < 256; byte++) {
        rem.clear();
        rem[0] = byte;

        for (bits_t bit = 0; bit < 8; bit++) {
          u8 xor_value = rem[0] & top_bit;

          rem = rem.shift_left();

          if (xor_value) {
            rem = rem ^ poly;
          }

          rem[0] &= top_mask;
        }

        matrix[byte] = rem;
      }
    }
  }
};

} // namespace sycon