#include <LibCore/Template.h>
#include <LibCore/Debug.h>

#include <fstream>

namespace LibCore {

namespace {
constexpr const char *const TEMPLATE_MARKER_AFFIX  = "/*@{";
constexpr const char *const TEMPLATE_MARKER_SUFFIX = "}@*/";
} // namespace

std::unordered_map<marker_t, coder_t> Template::get_builders(std::unordered_map<marker_t, indent_t> markers) {
  std::unordered_map<marker_t, coder_t> coders;
  for (const auto &[marker, lvl] : markers) {
    coders[marker] = coder_t(lvl);
  }
  return coders;
}

void Template::assert_markers_in_template(const std::filesystem::path &template_file, const std::unordered_map<marker_t, coder_t> &coders) {
  std::ifstream file(template_file);

  std::stringstream buffer;
  buffer << file.rdbuf();

  const code_t template_str = buffer.str();
  for (const auto &kv : coders) {
    const marker_t &marker = TEMPLATE_MARKER_AFFIX + kv.first + TEMPLATE_MARKER_SUFFIX;
    if (template_str.find(marker) == std::string::npos) {
      panic("Marker \"%s\" not found in template: %s", marker.c_str(), template_file.c_str());
    }
  }

  file.close();
}

Template::Template(std::filesystem::path _template_file, std::unordered_map<marker_t, indent_t> _markers)
    : template_file(_template_file), coders(get_builders(_markers)) {
  if (!std::filesystem::exists(template_file)) {
    panic("Template file not found: %s", template_file.c_str());
  }

  assert_markers_in_template(template_file, coders);
}

coder_t &Template::get(const std::string &marker) {
  auto it = coders.find(marker);
  assert(it != coders.end() && "Marker not found");
  return it->second;
}

code_t Template::dump() const {
  std::ifstream file(template_file);

  std::stringstream buffer;
  buffer << file.rdbuf();

  code_t template_str = buffer.str();

  for (const auto &[marker_label, coder] : coders) {
    const marker_t marker = TEMPLATE_MARKER_AFFIX + marker_label + TEMPLATE_MARKER_SUFFIX;
    const code_t code     = coder.stream.str();
    size_t start_pos      = 0;
    while ((start_pos = template_str.find(marker, start_pos)) != std::string::npos) {
      template_str.replace(start_pos, marker.length(), code);
    }
  }

  return template_str;
}

void Template::dbg_code() const {
  for (const auto &[marker_label, coder] : coders) {
    const marker_t marker = TEMPLATE_MARKER_AFFIX + marker_label + TEMPLATE_MARKER_SUFFIX;
    const code_t code     = coder.stream.str();

    std::cerr << "\n====================\n";
    std::cerr << "Marker: " << marker << "\n";
    std::cerr << "Code:\n" << code << "\n";
    std::cerr << "====================\n";
  }
}

} // namespace LibCore