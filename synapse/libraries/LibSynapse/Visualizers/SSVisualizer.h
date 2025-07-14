#pragma once

#include <LibSynapse/SearchSpace.h>
#include <LibCore/Graphviz.h>

#include <set>
#include <filesystem>

namespace LibSynapse {

using LibCore::Graphviz;

class SSViz : public Graphviz {
private:
  std::set<ep_id_t> highlight;

  SSViz();
  SSViz(const EP *highlight);

  void visit(const SearchSpace *search_space);

public:
  static void visualize(const SearchSpace *search_space, bool interrupt);
  static void visualize(const SearchSpace *search_space, const EP *highlight, bool interrupt);

  static void dump_to_file(const SearchSpace *search_space, const std::filesystem::path &file_name);
  static void dump_to_file(const SearchSpace *search_space, const EP *highlight, const std::filesystem::path &file_name);
};

} // namespace LibSynapse