#pragma once

#include <LibSynapse/SearchSpace.h>
#include <LibCore/TreeViz.h>

#include <set>
#include <filesystem>

namespace LibSynapse {

using LibCore::TreeViz;

class SSViz {
private:
  struct ss_opts_t {
    const EP *highlight;
    std::filesystem::path fpath;
  };

  TreeViz treeviz;
  std::set<ep_id_t> highlight;

  SSViz();
  SSViz(const ss_opts_t &opts);

  void visit(const SearchSpace *search_space);
  void visit_definitions(const SSNode *ssnode);
  void visit_links(const SSNode *ssnode);

public:
  static void visualize(const SearchSpace *search_space, bool interrupt);
  static void visualize(const SearchSpace *search_space, const EP *highlight, bool interrupt);

  static void dump_to_file(const SearchSpace *search_space, const std::filesystem::path &file_name);
  static void dump_to_file(const SearchSpace *search_space, const EP *highlight, const std::filesystem::path &file_name);
};

} // namespace LibSynapse