#pragma once

#include <LibCore/Types.h>

#include <sstream>
#include <vector>

namespace LibCore {

using indent_t = u8;
using marker_t = std::string;
using code_t   = std::string;

struct coder_t {
  std::stringstream stream;
  indent_t lvl;

  coder_t() : lvl(0) {}
  coder_t(indent_t _lvl) : lvl(_lvl) {}
  coder_t(const coder_t &other) : lvl(other.lvl) {}

  coder_t &operator=(const coder_t &other) {
    lvl = other.lvl;
    return *this;
  }

  void inc() { lvl++; }
  void dec() { lvl--; }
  void indent() { stream << code_t(lvl * 2, ' '); }
  code_t dump() const { return stream.str(); }

  std::vector<code_t> split_lines() const {
    std::vector<code_t> lines;
    std::stringstream ss(stream.str());
    code_t line;

    while (std::getline(ss, line, '\n')) {
      lines.push_back(line);
    }

    return lines;
  }

  coder_t &operator<<(const coder_t &coder) {
    stream << coder.stream.str();
    return *this;
  }

  coder_t &operator<<(const code_t &code) {
    stream << code;
    return *this;
  }

  coder_t &operator<<(i64 n) {
    stream << n;
    return *this;
  }

  static code_t hex(u64 n) {
    std::stringstream ss;
    ss << "0x";
    ss << std::hex;
    ss << std::setw(16);
    ss << std::setfill('0');
    ss << n;
    return ss.str();
  }

  static code_t hex(u32 n) {
    std::stringstream ss;
    ss << "0x";
    ss << std::hex;
    ss << std::setw(8);
    ss << std::setfill('0');
    ss << n;
    return ss.str();
  }
};

} // namespace LibCore