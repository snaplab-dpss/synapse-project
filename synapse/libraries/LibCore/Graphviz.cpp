#include <LibCore/Graphviz.h>
#include <LibCore/Debug.h>
#include <LibCore/System.h>

#include <iomanip>

namespace LibCore {
namespace Graphviz {

std::ostream &operator<<(std::ostream &stream, const Color &color) {
  switch (color.type) {
  case Color::Type::RGB: {
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
  case Color::Type::Literal: {
    switch (color.literal) {
    case Color::Literal::White: {
      stream << "white";
    } break;
    case Color::Literal::Gray: {
      stream << "gray";
    } break;
    case Color::Literal::Cyan: {
      stream << "cyan";
    } break;
    case Color::Literal::CornflowerBlue: {
      stream << "cornflowerblue";
    } break;
    case Color::Literal::Yellow: {
      stream << "yellow";
    } break;
    case Color::Literal::Chartreuse2: {
      stream << "chartreuse2";
    } break;
    case Color::Literal::Brown1: {
      stream << "brown1";
    } break;
    case Color::Literal::Purple: {
      stream << "purple";
    } break;
    case Color::Literal::Firebrick2: {
      stream << "firebrick2";
    } break;
    case Color::Literal::Orange: {
      stream << "orange";
    } break;
    case Color::Literal::Green: {
      stream << "green";
    } break;
    case Color::Literal::LightCoral: {
      stream << "lightcoral";
    } break;
    }
  } break;
  }

  return stream;
}

std::ostream &operator<<(std::ostream &stream, const Style &style) {
  switch (style) {
  case Style::Rounded:
    stream << "rounded";
    break;
  case Style::Filled:
    stream << "filled";
    break;
  }
  return stream;
}

std::ostream &operator<<(std::ostream &stream, const Shape &shape) {
  switch (shape) {
  case Shape::Box:
    stream << "box";
    break;
  case Shape::MDiamond:
    stream << "mdiamond";
    break;
  case Shape::Octagon:
    stream << "octagon";
    break;
  case Shape::Ellipse:
    stream << "ellipse";
    break;
  case Shape::Html:
    stream << "none";
    break;
  }
  return stream;
}

std::ostream &operator<<(std::ostream &stream, const Node &node) {
  stream << "\"" << node.id << "\"";
  stream << "[";
  stream << "label=" << (node.shape == Shape::Html ? "<" : "\"") << node.label << (node.shape == Shape::Html ? ">" : "\"");
  stream << ",";
  stream << "shape=" << node.shape;
  stream << ",";
  stream << "style=\"";
  for (const Style &style : node.styles) {
    stream << style << ",";
  }
  stream << "\"";
  stream << ",";
  stream << "fillcolor=\"" << node.color << "\"";
  stream << ",";
  stream << "border=" << static_cast<int>(node.border);
  stream << ",";
  stream << "]";
  return stream;
}

std::ostream &operator<<(std::ostream &stream, const Edge &edge) {
  stream << "\"" << edge.from << "\"";
  stream << " -> ";
  stream << "\"" << edge.to << "\"";
  if (edge.label.has_value()) {
    stream << " [label=\"" << edge.label.value() << "\"]";
  }
  return stream;
}

std::ostream &operator<<(std::ostream &stream, const Cluster &cluster) {
  stream << "subgraph " << cluster.id << " {\n";
  stream << "\tlabel = \"" << cluster.label << "\";\n";
  stream << "\tcluster = true;\n";
  stream << "\tbgcolor = \"" << cluster.color << "\";\n";
  for (const Node &node : cluster.nodes) {
    stream << "\t" << node << ";\n";
  }
  stream << "}";
  return stream;
}

void visualize_dot_file(const std::filesystem::path &dot_file, bool interrupt) {
  const std::string cmd = "xdot " + dot_file.string() + " 2>/dev/null &";
  const int status      = system(cmd.c_str());

  if (status != 0) {
    panic("Failed to open graph: %s", std::strerror(errno));
  }

  if (interrupt) {
    std::cout << "Press Enter to continue ";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
}

} // namespace Graphviz
} // namespace LibCore