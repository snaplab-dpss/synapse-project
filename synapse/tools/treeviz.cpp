#include <LibCore/TreeViz.h>

#include <iostream>

using namespace LibCore;

int main() {
  TreeViz treeviz;

  treeviz.add_node("0", "node0");
  treeviz.add_node("1", "node1");
  treeviz.add_node(TreeViz::Node("2", "node2", TreeViz::Color::Literal::Yellow, TreeViz::Shape::Octagon, 0, {TreeViz::Style::Filled}));
  treeviz.add_edge("0", "1");
  treeviz.add_edge("0", "2");

  treeviz.show(false);

  std::cout << "Graph written to: " << treeviz.get_file_path() << std::endl;

  return 0;
}