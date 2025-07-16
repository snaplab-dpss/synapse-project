#pragma once

#include <string>
#include <filesystem>
#include <unordered_set>
#include <optional>

#include <LibCore/Types.h>
#include <LibCore/Graphviz.h>

namespace LibCore {

using Graphviz::Border;
using Graphviz::Cluster;
using Graphviz::Color;
using Graphviz::Edge;
using Graphviz::ID;
using Graphviz::Label;
using Graphviz::Node;
using Graphviz::Shape;
using Graphviz::Style;

class ClusterViz {
private:
  const std::filesystem::path fpath;
  Node default_node;
  std::unordered_set<Cluster, Cluster::Hash> clusters;
  std::unordered_set<Node, Node::Hash> nodes;
  std::vector<Edge> edges;

public:
  ClusterViz(const std::filesystem::path &path);
  ClusterViz();

  void write() const;
  void show(bool interrupt) const;

  void add_node(const Node &node);
  void add_cluster(const Cluster &cluster);
  void add_edge(const Edge &edge);

  std::filesystem::path get_file_path() const { return fpath; }
};

} // namespace LibCore
