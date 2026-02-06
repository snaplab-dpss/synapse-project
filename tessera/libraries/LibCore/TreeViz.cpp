#include <LibCore/TreeViz.h>
#include <LibCore/Debug.h>
#include <LibCore/System.h>

#include <fstream>

namespace LibCore {

TreeViz::TreeViz(const std::filesystem::path &_fpath)
    : fpath(_fpath.empty() ? create_random_file(".dot") : _fpath), default_node(Color::Literal::Gray, Shape::Box, 0, {Style::Filled}) {}

TreeViz::TreeViz() : TreeViz(create_random_file(".dot")) {}

void TreeViz::add_node(const ID &id, const Label &label) {
  Node node  = default_node;
  node.id    = id;
  node.label = label;
  add_node(node);
}

void TreeViz::add_node(const Node &node) {
  if (nodes.find(node) == nodes.end()) {
    nodes.insert(node);
  } else {
    panic("Node with id %s already exists", node.id.c_str());
  }
}

void TreeViz::add_edge(const ID &from, const ID &to, std::optional<Label> label) { edges.emplace_back(from, to, label); }

void TreeViz::write() const {
  std::ofstream file(fpath);
  if (!file.is_open()) {
    panic("Failed to open file %s for writing", fpath.string().c_str());
  }

  file << "digraph G {\n";
  for (const Node &node : nodes) {
    file << "\t" << node << ";\n";
  }
  for (const Edge &edge : edges) {
    file << "\t" << edge << ";\n";
  }
  file << "}";
}

void TreeViz::show(bool interrupt) const {
  write();
  Graphviz::visualize_dot_file(fpath, interrupt);
}

} // namespace LibCore
