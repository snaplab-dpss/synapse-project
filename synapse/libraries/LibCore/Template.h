#pragma once

#include <LibCore/Types.h>
#include <LibCore/Coder.h>

#include <filesystem>
#include <unordered_map>

namespace LibCore {

class Template {
private:
  const std::filesystem::path template_file;
  std::unordered_map<marker_t, coder_t> coders;

  static std::unordered_map<marker_t, coder_t> get_builders(std::unordered_map<marker_t, indent_t> markers);
  static void assert_markers_in_template(const std::filesystem::path &template_file, const std::unordered_map<marker_t, coder_t> &coders);

public:
  Template(std::filesystem::path template_file, std::unordered_map<marker_t, indent_t> markers);

  code_t dump() const;
  coder_t &get(const marker_t &marker);
  void dbg_code() const;
};

} // namespace LibCore
