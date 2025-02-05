#include <LibCore/Synthesizer.h>
#include <LibCore/Debug.h>

#include <fstream>

namespace LibCore {

namespace {
constexpr const char SYNTHESIZER_INDENTATION_UNIT      = ' ';
constexpr const int SYNTHESIZER_INDENTATION_MULTIPLIER = 2;

constexpr const char *const TEMPLATE_MARKER_AFFIX  = "/*@{";
constexpr const char *const TEMPLATE_MARKER_SUFFIX = "}@*/";

} // namespace

void Synthesizer::assert_markers_in_template(const std::filesystem::path &template_file,
                                             const std::unordered_map<marker_t, coder_t> &coders) {
  std::ifstream file(template_file);

  std::stringstream buffer;
  buffer << file.rdbuf();

  code_t template_str = buffer.str();
  for (const auto &kv : coders) {
    const marker_t &marker = TEMPLATE_MARKER_AFFIX + kv.first + TEMPLATE_MARKER_SUFFIX;
    if (template_str.find(marker) == std::string::npos) {
      panic("Marker \"%s\" not found in template: %s\n", marker.c_str(), template_file.c_str());
    }
  }
}

std::unordered_map<Synthesizer::marker_t, Synthesizer::coder_t> Synthesizer::get_builders(std::unordered_map<marker_t, indent_t> markers) {
  std::unordered_map<marker_t, coder_t> coders;
  for (const auto &[marker, lvl] : markers) {
    coders[marker] = coder_t(lvl);
  }
  return coders;
}

Synthesizer::Synthesizer(std::filesystem::path _template_file, std::unordered_map<marker_t, indent_t> _markers, std::ostream &_out)
    : template_file(_template_file), coders(get_builders(_markers)), out(_out) {
  if (!std::filesystem::exists(template_file)) {
    panic("Template file not found: %s\n", template_file.c_str());
  }

  assert_markers_in_template(template_file, coders);
}

Synthesizer::coder_t::coder_t() : lvl(0) {}
Synthesizer::coder_t::coder_t(indent_t _lvl) : lvl(_lvl) {}
Synthesizer::coder_t::coder_t(const coder_t &other) : lvl(other.lvl) {}

Synthesizer::coder_t &Synthesizer::coder_t::operator=(const coder_t &other) {
  lvl = other.lvl;
  return *this;
}

void Synthesizer::coder_t::inc() { lvl++; }
void Synthesizer::coder_t::dec() { lvl--; }

void Synthesizer::coder_t::indent() { stream << code_t(lvl * SYNTHESIZER_INDENTATION_MULTIPLIER, SYNTHESIZER_INDENTATION_UNIT); }

Synthesizer::code_t Synthesizer::coder_t::dump() const { return stream.str(); }

Synthesizer::coder_t &Synthesizer::coder_t::operator<<(const coder_t &coder) {
  stream << coder.stream.str();
  return *this;
}

Synthesizer::coder_t &Synthesizer::coder_t::operator<<(const code_t &code) {
  stream << code;
  return *this;
}

Synthesizer::coder_t &Synthesizer::coder_t::operator<<(i64 n) {
  stream << n;
  return *this;
}

Synthesizer::coder_t &Synthesizer::get(const std::string &marker) {
  auto it = coders.find(marker);
  assert(it != coders.end() && "Marker not found");
  return it->second;
}

void Synthesizer::dump() const {
  std::ifstream file(template_file);

  std::stringstream buffer;
  buffer << file.rdbuf();

  code_t template_str = buffer.str();

  for (const auto &[marker_label, coder] : coders) {
    const marker_t marker = TEMPLATE_MARKER_AFFIX + marker_label + TEMPLATE_MARKER_SUFFIX;
    const code_t code     = coder.stream.str();
    const size_t pos      = template_str.find(marker);

    assert(pos != std::string::npos && "Marker not found in template.");
    template_str.replace(pos, marker.size(), code);
  }

  out << template_str;
  out.flush();
}

void Synthesizer::dbg_code() const {
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