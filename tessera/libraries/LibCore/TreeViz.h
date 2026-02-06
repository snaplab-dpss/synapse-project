#pragma once

#include <string>
#include <filesystem>
#include <unordered_set>
#include <optional>

#include <LibCore/Types.h>
#include <LibCore/Graphviz.h>

namespace LibCore {

using Graphviz::Border;
using Graphviz::Color;
using Graphviz::Edge;
using Graphviz::ID;
using Graphviz::Label;
using Graphviz::Node;
using Graphviz::Shape;
using Graphviz::Style;

class TreeViz {
private:
  const std::filesystem::path fpath;
  Node default_node;
  std::unordered_set<Node, Node::Hash> nodes;
  std::vector<Edge> edges;

public:
  TreeViz(const std::filesystem::path &path);
  TreeViz();

  void write() const;
  void show(bool interrupt) const;

  void add_node(const ID &id, const Label &label);
  void add_node(const Node &node);
  void add_edge(const ID &from, const ID &to, std::optional<Label> label = std::nullopt);

  std::filesystem::path get_file_path() const { return fpath; }

  Node get_default_node() const { return default_node; }
  void set_default_node(const Node &node) { default_node = node; }
};

} // namespace LibCore
