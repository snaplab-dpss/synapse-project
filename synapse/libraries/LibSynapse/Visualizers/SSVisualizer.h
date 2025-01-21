#pragma once

#include <LibSynapse/SearchSpace.h>
#include <LibCore/Graphviz.h>

#include <set>

namespace LibSynapse {

class SSVisualizer : public LibCore::Graphviz {
private:
  std::set<ep_id_t> highlight;

  SSVisualizer();
  SSVisualizer(const EP *highlight);

  void visit(const SearchSpace *search_space);

public:
  static void visualize(const SearchSpace *search_space, bool interrupt);
  static void visualize(const SearchSpace *search_space, const EP *highlight, bool interrupt);
};

} // namespace LibSynapse