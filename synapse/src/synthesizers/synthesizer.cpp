#include <fstream>

#include "synthesizer.h"
#include "../log.h"

#define INDENTATION_UNIT ' '
#define INDENTATION_MULTIPLIER 2

#define MARKER_AFFIX "/*@{"
#define MARKER_SUFFIX "}@*/"

static std::filesystem::path get_template_path(std::string template_name) {
  std::filesystem::path src_file = __FILE__;
  return src_file.parent_path() / "templates" / template_name;
}

static std::unordered_map<marker_t, coder_t>
get_builders(std::unordered_map<marker_t, indent_t> markers) {
  std::unordered_map<marker_t, coder_t> coders;
  for (const auto &[marker, lvl] : markers) {
    coders[marker] = coder_t(lvl);
  }
  return coders;
}

static void assert_markers_in_template(
    const std::filesystem::path &template_file,
    const std::unordered_map<marker_t, coder_t> &coders) {
  std::ifstream file(template_file);

  std::stringstream buffer;
  buffer << file.rdbuf();

  code_t template_str = buffer.str();
  for (const auto &kv : coders) {
    const marker_t &marker = MARKER_AFFIX + kv.first + MARKER_SUFFIX;
    if (template_str.find(marker) == std::string::npos) {
      PANIC("Marker \"%s\" not found in template: %s\n", marker.c_str(),
            template_file.c_str());
    }
  }
}

Synthesizer::Synthesizer(std::string _template_fname,
                         std::unordered_map<marker_t, indent_t> _markers,
                         std::ostream &_out)
    : template_file(get_template_path(_template_fname)),
      coders(get_builders(_markers)), out(_out) {
  if (!std::filesystem::exists(template_file)) {
    PANIC("Template file not found: %s\n", template_file.c_str());
  }

  assert_markers_in_template(template_file, coders);
}

coder_t::coder_t() : lvl(0) {}
coder_t::coder_t(indent_t _lvl) : lvl(_lvl) {}

void coder_t::inc() { lvl++; }
void coder_t::dec() { lvl--; }

void coder_t::indent() {
  stream << code_t(lvl * INDENTATION_MULTIPLIER, INDENTATION_UNIT);
}

code_t coder_t::dump() const { return stream.str(); }

coder_t &coder_t::operator<<(const code_t &code) {
  stream << code;
  return *this;
}

coder_t &coder_t::operator<<(i64 n) {
  stream << n;
  return *this;
}

coder_t &Synthesizer::get(const std::string &marker) {
  auto it = coders.find(marker);
  ASSERT(it != coders.end(), "Marker not found.");
  return it->second;
}

void Synthesizer::dump() const {
  std::ifstream file(template_file);

  std::stringstream buffer;
  buffer << file.rdbuf();

  code_t template_str = buffer.str();

  for (const auto &[marker_label, coder] : coders) {
    marker_t marker = MARKER_AFFIX + marker_label + MARKER_SUFFIX;
    code_t code = coder.stream.str();

    size_t pos = template_str.find(marker);
    ASSERT(pos != std::string::npos, "Marker not found in template.");

    template_str.replace(pos, marker.size(), code);
  }

  out << template_str;
  out.flush();
}

void Synthesizer::dbg() const {
  for (const auto &[marker_label, coder] : coders) {
    marker_t marker = MARKER_AFFIX + marker_label + MARKER_SUFFIX;
    code_t code = coder.stream.str();

    Log::dbg() << "\n====================\n";
    Log::dbg() << "Marker: " << marker << "\n";
    Log::dbg() << "Code:\n" << code << "\n";
    Log::dbg() << "====================\n";
  }
}
