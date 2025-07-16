#include <LibCore/ClusterViz.h>
#include <LibCore/Debug.h>
#include <LibCore/System.h>

#include <fstream>

namespace LibCore {

ClusterViz::ClusterViz(const std::filesystem::path &_fpath)
    : fpath(_fpath.empty() ? create_random_file(".dot") : _fpath), default_node(Color::Literal::Gray, Shape::Box, 0, {Style::Filled}) {}

ClusterViz::ClusterViz() : ClusterViz(create_random_file(".dot")) {}

void ClusterViz::add_node(const Node &node) {
  if (nodes.find(node) == nodes.end()) {
    nodes.insert(node);
  } else {
    panic("Node with id %s already exists", node.id.c_str());
  }
}

void ClusterViz::add_cluster(const Cluster &cluster) {
  if (clusters.find(cluster) == clusters.end()) {
    clusters.insert(cluster);
  } else {
    panic("Cluster with id %s already exists", cluster.id.c_str());
  }
}

void ClusterViz::add_edge(const Edge &edge) { edges.push_back(edge); }

void ClusterViz::write() const {
  std::ofstream file(fpath);
  if (!file.is_open()) {
    panic("Failed to open file %s for writing", fpath.string().c_str());
  }

  file << "digraph G {\n";
  for (const Cluster &cluster : clusters) {
    file << cluster << "\n";
  }
  for (const Node &node : nodes) {
    file << "\t" << node << ";\n";
  }
  for (const Edge &edge : edges) {
    file << "\t" << edge << ";\n";
  }
  file << "}";
}

void ClusterViz::show(bool interrupt) const {
  write();
  Graphviz::visualize_dot_file(fpath, interrupt);
}

} // namespace LibCore
