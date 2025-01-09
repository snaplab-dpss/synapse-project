#pragma once

#include <iomanip>
#include <sstream>
#include <stdint.h>
#include <vector>
#include <string>
#include <filesystem>

#include "../types.h"

namespace synapse {
struct rgb_t {
  u8 r;
  u8 g;
  u8 b;
  u8 o;

  rgb_t(u8 r, u8 g, u8 b);
  rgb_t(u8 r, u8 g, u8 b, u8 o);

  std::string to_gv_repr() const;
};

class Graphviz {
protected:
  std::filesystem::path fpath;
  std::stringstream ss;

public:
  Graphviz(const std::filesystem::path &fpath);
  Graphviz();

  void write() const;
  void show(bool interrupt) const;

  static void find_and_replace(std::string &str, const std::vector<std::pair<std::string, std::string>> &replacements);
  static void sanitize_html_label(std::string &label);
};
} // namespace synapse