#include <LibCore/TreeViz.h>
#include <LibCore/Debug.h>
#include <LibCore/System.h>

#include <fstream>

namespace LibCore {

TreeViz::TreeViz(const std::filesystem::path &_fpath)
    : fpath(_fpath.empty() ? create_random_file(".dot") : _fpath), default_node(Color::Literal::Gray, Shape::Box, 0, {Style::Filled}) {}

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
    file << "label=" << (node.shape == Shape::Html ? "<" : "\"") << node.label << (node.shape == Shape::Html ? ">" : "\"");
    file << ",";
    file << "shape=" << node.shape;
    file << ",";
    file << "style=\"";
    for (const Style &style : node.styles) {
      file << style << ",";
    }
    file << "\"";
    file << ",";
    file << "fillcolor=\"" << node.color << "\"";
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

std::string TreeViz::find_and_replace(const std::string &str, const std::vector<std::pair<std::string, std::string>> &replacements) {
  std::string result = str;
  for (const std::pair<std::string, std::string> &replacement : replacements) {
    const std::string &before = replacement.first;
    const std::string &after  = replacement.second;

    std::string::size_type n = 0;
    while ((n = result.find(before, n)) != std::string::npos) {
      result.replace(n, before.size(), after);
      n += after.size();
    }
  }
  return result;
}

std::string TreeViz::sanitize_html_label(const std::string &label) {
  return find_and_replace(label, {
                                     {"&", "&amp;"}, // Careful, this needs to be the first
                                                     // one, otherwise we mess everything up. Notice
                                                     // that all the others use ampersands.
                                     {"{", "&#123;"},
                                     {"}", "&#125;"},
                                     {"[", "&#91;"},
                                     {"]", "&#93;"},
                                     {"<", "&lt;"},
                                     {">", "&gt;"},
                                     {"\n", "<br/>"},
                                 });
}

std::ostream &operator<<(std::ostream &stream, const TreeViz::Color &color) {
  switch (color.type) {
  case TreeViz::Color::Type::RGB: {
    stream << "#";
    stream << std::hex;

    stream << std::setw(2);
    stream << std::setfill('0');
    stream << static_cast<int>(color.rgb.red);

    stream << std::setw(2);
    stream << std::setfill('0');
    stream << static_cast<int>(color.rgb.green);

    stream << std::setw(2);
    stream << std::setfill('0');
    stream << static_cast<int>(color.rgb.blue);

    stream << std::setw(2);
    stream << std::setfill('0');
    stream << static_cast<int>(color.rgb.opacity);

    stream << std::dec;
  } break;
  case TreeViz::Color::Type::Literal: {
    switch (color.literal) {
    case TreeViz::Color::Literal::Gray: {
      stream << "gray";
    } break;
    case TreeViz::Color::Literal::Cyan: {
      stream << "cyan";
    } break;
    case TreeViz::Color::Literal::CornflowerBlue: {
      stream << "cornflowerblue";
    } break;
    case TreeViz::Color::Literal::Yellow: {
      stream << "yellow";
    } break;
    case TreeViz::Color::Literal::Chartreuse2: {
      stream << "chartreuse2";
    } break;
    case TreeViz::Color::Literal::Brown1: {
      stream << "brown1";
    } break;
    case TreeViz::Color::Literal::Purple: {
      stream << "purple";
    } break;
    case TreeViz::Color::Literal::Firebrick2: {
      stream << "firebrick2";
    } break;
    case TreeViz::Color::Literal::Orange: {
      stream << "orange";
    } break;
    case TreeViz::Color::Literal::Green: {
      stream << "green";
    } break;
    case TreeViz::Color::Literal::LightCoral: {
      stream << "lightcoral";
    } break;
    }
  } break;
  }

  return stream;
}

std::ostream &operator<<(std::ostream &stream, TreeViz::Style style) {
  switch (style) {
  case TreeViz::Style::Rounded:
    stream << "rounded";
    break;
  case TreeViz::Style::Filled:
    stream << "filled";
    break;
  }
  return stream;
}

std::ostream &operator<<(std::ostream &stream, TreeViz::Shape shape) {
  switch (shape) {
  case TreeViz::Shape::Box:
    stream << "box";
    break;
  case TreeViz::Shape::MDiamond:
    stream << "mdiamond";
    break;
  case TreeViz::Shape::Octagon:
    stream << "octagon";
    break;
  case TreeViz::Shape::Ellipse:
    stream << "ellipse";
    break;
  case TreeViz::Shape::Html:
    stream << "none";
    break;
  }
  return stream;
}

} // namespace LibCore
