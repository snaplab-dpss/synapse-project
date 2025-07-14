#include <LibCore/TreeViz.h>

#include <iostream>

using namespace LibCore;

int main() {
  TreeViz treeviz;

  // Example usage of TreeViz
  treeviz.set_default_node_color(TreeViz::Color::Cyan);
  treeviz.set_default_node_shape(TreeViz::Shape::Box);

  // Add nodes and edges as needed
  treeviz.add_node("0", "node0");
  treeviz.add_node("1", "node1");
  treeviz.add_node(TreeViz::Node("2", "node2", TreeViz::Color::Yellow, TreeViz::Shape::Octagon, 0, {TreeViz::Style::Filled}));
  treeviz.add_edge("0", "1");
  treeviz.add_edge("0", "2");

  // Write the graph to a file or display it
  treeviz.show(false);

  std::cout << "Graph written to: " << treeviz.get_file_path() << std::endl;

  return 0;
}