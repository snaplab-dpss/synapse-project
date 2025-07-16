#include <LibCore/TreeViz.h>
#include <LibCore/ClusterViz.h>

#include <iostream>

using namespace LibCore;
using namespace LibCore::Graphviz;

void test_treeviz() {
  TreeViz treeviz;
  treeviz.add_node("0", "node0");
  treeviz.add_node("1", "node1");
  treeviz.add_node(Node("2", "node2", Color::Literal::Yellow, Shape::Octagon, 0, {Style::Filled}));
  treeviz.add_edge("0", "1");
  treeviz.add_edge("0", "2");
  treeviz.show(false);
  std::cout << "Tree graph written to: " << treeviz.get_file_path() << std::endl;
}

void test_clusterviz() {
  ClusterViz clusterviz;

  Node cluster_a_default_node(Color::Literal::Gray);

  Node cluster_a_node_A  = cluster_a_default_node;
  cluster_a_node_A.id    = "A";
  cluster_a_node_A.label = "Node A";

  Node cluster_a_node_B  = cluster_a_default_node;
  cluster_a_node_B.id    = "B";
  cluster_a_node_B.label = "Node B";

  Cluster cluster_a("cluster_a", "Cluster A", Color::Literal::White);
  cluster_a.nodes.insert(cluster_a_node_A);
  cluster_a.nodes.insert(cluster_a_node_B);

  Node cluster_b_default_node(Color::Literal::Cyan);

  Node cluster_b_node_C  = cluster_b_default_node;
  cluster_b_node_C.id    = "C";
  cluster_b_node_C.label = "Node C";

  Node cluster_b_node_D  = cluster_b_default_node;
  cluster_b_node_D.id    = "D";
  cluster_b_node_D.label = "Node D";

  Cluster cluster_b("cluster_b", "Cluster B", Color::Literal::White);
  cluster_b.nodes.insert(cluster_b_node_C);
  cluster_b.nodes.insert(cluster_b_node_D);

  Node standalone_node("Standalone", "Standalone Node", Color::Literal::Firebrick2, Shape::Ellipse);

  clusterviz.add_cluster(cluster_a);
  clusterviz.add_cluster(cluster_b);
  clusterviz.add_node(standalone_node);

  clusterviz.add_edge(Edge(cluster_a_node_A.id, cluster_b_node_C.id));
  clusterviz.add_edge(Edge(cluster_a_node_B.id, cluster_b_node_D.id));
  clusterviz.add_edge(Edge(standalone_node.id, cluster_a_node_A.id));

  clusterviz.show(false);
  std::cout << "Cluster graph written to: " << clusterviz.get_file_path() << std::endl;
}

int main() {
  test_treeviz();
  test_clusterviz();
  return 0;
}