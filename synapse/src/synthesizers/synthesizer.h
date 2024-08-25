#pragma once

#include <filesystem>
#include <unordered_map>
#include <string>
#include <sstream>

#include "../types.h"

typedef u8 indent_t;
typedef std::string marker_t;
typedef std::string code_t;

struct coder_t {
  std::stringstream stream;
  indent_t lvl;

  coder_t();
  coder_t(indent_t lvl);

  void inc();
  void dec();
  void indent();
  code_t dump() const;

  coder_t &operator<<(const code_t &code);
  coder_t &operator<<(int n);
};

class Synthesizer {
private:
  const std::filesystem::path template_file;
  std::unordered_map<marker_t, coder_t> coders;
  std::ostream &out;

public:
  Synthesizer(std::string template_fname,
              std::unordered_map<marker_t, indent_t> markers,
              std::ostream &_out);

protected:
  coder_t &get(const marker_t &marker);
  void dump() const;
  void dbg() const;
};
