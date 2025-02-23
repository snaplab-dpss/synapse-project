#pragma once

#include <LibCore/Types.h>

#include <filesystem>
#include <unordered_map>
#include <string>
#include <sstream>
#include <iomanip>

namespace LibCore {

class Synthesizer {
protected:
  using indent_t = u8;
  using marker_t = std::string;
  using code_t   = std::string;

  struct coder_t {
    std::stringstream stream;
    indent_t lvl;

    coder_t();
    coder_t(const coder_t &);
    coder_t(indent_t lvl);
    coder_t &operator=(const coder_t &other);

    void inc();
    void dec();
    void indent();
    code_t dump() const;

    coder_t &operator<<(const coder_t &coder);
    coder_t &operator<<(const code_t &code);
    coder_t &operator<<(i64 n);
  };

private:
  const std::filesystem::path template_file;
  const std::filesystem::path out_file;
  std::unordered_map<marker_t, coder_t> coders;

  static std::unordered_map<marker_t, coder_t> get_builders(std::unordered_map<marker_t, indent_t> markers);
  static void assert_markers_in_template(const std::filesystem::path &template_file, const std::unordered_map<marker_t, coder_t> &coders);

public:
  Synthesizer(std::filesystem::path template_file, std::unordered_map<marker_t, indent_t> markers, std::filesystem::path out_file);
  virtual ~Synthesizer() = default;

  virtual void synthesize() = 0;

protected:
  virtual coder_t &get(const marker_t &marker);
  void dump() const;
  void dbg_code() const;
};

} // namespace LibCore