#include <LibCore/TreeViz.h>
#include <LibCore/Debug.h>
#include <LibCore/System.h>

#include <fstream>

namespace LibCore {

TreeViz::TreeViz(const std::filesystem::path &_fpath)
    : fpath(_fpath.empty() ? create_random_file(".dot") : _fpath), default_node(Color::Gray, Shape::Box, 0, {Style::Filled}) {}

TreeViz::TreeViz() : TreeViz(create_random_file(".dot")) {}

void TreeViz::add_node(const NodeId &id, const Label &label) {
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

void TreeViz::add_edge(const NodeId &from, const NodeId &to, std::optional<Label> label) { edges.emplace_back(from, to, label); }

void TreeViz::write() const {
  std::ofstream file(fpath);
  if (!file.is_open()) {
    panic("Failed to open file %s for writing", fpath.string().c_str());
  }

  file << "digraph G {\n";

  for (const Node &node : nodes) {
    file << "\t";
    file << "\"" << node.id << "\"";
    file << "[";
    file << "label=\"" << node.label << "\"";
    file << ",";
    file << "shape=" << shape_to_string(node.shape);
    file << ",";
    file << "style=\"";
    for (const Style &style : node.styles) {
      file << style_to_string(style) << ",";
    }
    file << "\"";
    file << ",";
    file << "fillcolor=\"" << color_to_string(node.color) << "\"";
    file << ",";
    file << "border=" << static_cast<int>(node.border);
    file << ",";
    file << "];\n";
  }

  for (const Edge &edge : edges) {
    file << "\t";
    file << "\"" << edge.from << "\"";
    file << " -> ";
    file << "\"" << edge.to << "\"";
    if (edge.label.has_value()) {
      file << " [label=\"" << edge.label.value() << "\"]";
    }
    file << ";\n";
  }

  file << "}";
}

void TreeViz::show(bool interrupt) const {
  write();

  const std::string cmd = "xdot " + fpath.string() + " 2>/dev/null &";
  const int status      = system(cmd.c_str());

  if (status != 0) {
    panic("Failed to open graph: %s", std::strerror(errno));
  }

  if (interrupt) {
    std::cout << "Press Enter to continue ";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
}

std::string TreeViz::color_to_string(Color color) const {
  switch (color) {
  case Color::Gray:
    return "gray";
  case Color::Cyan:
    return "cyan";
  case Color::CornflowerBlue:
    return "cornflowerblue";
  case Color::Yellow:
    return "yellow";
  case Color::Chartreuse2:
    return "chartreuse2";
  case Color::Brown1:
    return "brown1";
  case Color::Purple:
    return "purple";
  }
  panic("Unknown color type");
}

std::string TreeViz::shape_to_string(Shape shape) const {
  switch (shape) {
  case Shape::Box:
    return "box";
  case Shape::MDiamond:
    return "mdiamond";
  case Shape::Octagon:
    return "octagon";
  }
  panic("Unknown shape type");
}

std::string TreeViz::style_to_string(Style style) const {
  switch (style) {
  case Style::Rounded:
    return "rounded";
  case Style::Filled:
    return "filled";
  }
  panic("Unknown style type");
}

} // namespace LibCore