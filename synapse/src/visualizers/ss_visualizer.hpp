#pragma once

#include <vector>

#include "ep_visualizer.hpp"
#include "profiler_visualizer.hpp"
#include "../search_space.hpp"

namespace synapse {
class SSVisualizer : public Graphviz {
private:
  std::set<ep_id_t> highlight;

  SSVisualizer();
  SSVisualizer(const EP *highlight);

  void visit(const SearchSpace *search_space);

public:
  static void visualize(const SearchSpace *search_space, bool interrupt);
  static void visualize(const SearchSpace *search_space, const EP *highlight, bool interrupt);
};
} // namespace synapse