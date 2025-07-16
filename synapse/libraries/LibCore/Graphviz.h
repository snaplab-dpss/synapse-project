#pragma once

#include <LibCore/Types.h>

#include <string>
#include <optional>
#include <vector>
#include <unordered_set>
#include <filesystem>

namespace LibCore {
namespace Graphviz {

struct Color {
  enum class Type { RGB, Literal };
  enum class Literal {
    White,
    Gray,
    Cyan,
    CornflowerBlue,
    Yellow,
    Chartreuse2,
    Brown1,
    Purple,
    Firebrick2,
    Orange,
    Green,
    LightCoral,
  };

  Type type;
  struct {
    u8 red;
    u8 green;
    u8 blue;
    u8 opacity;
  } rgb;
  Literal literal;

  Color() : type(Type::Literal), literal(Literal::Gray) {}
  Color(u8 r, u8 g, u8 b, u8 o = 0xff) : type(Type::RGB), rgb{r, g, b, o} {}
  Color(Literal _literal) : type(Type::Literal), literal(_literal) {}
  Color(const Color &other) : type(other.type), rgb(other.rgb), literal(other.literal) {}
  Color &operator=(const Color &other) = default;
};

enum class Shape { Box, MDiamond, Octagon, Ellipse, Html };
enum class Style { Rounded, Filled };

using ID     = std::string;
using Label  = std::string;
using Border = u8;

struct Node {
  ID id;
  Label label;
  Color color;
  Shape shape;
  Border border;
  std::vector<Style> styles;

  Node(const ID &_id, const Label &_label, const Color &_color = Color::Literal::White, const Shape &_shape = Shape::Box, const Border &_border = 0,
       const std::vector<Style> &_styles = {Style::Filled})
      : id(_id), label(_label), color(_color), shape(_shape), border(_border), styles(_styles) {}

  Node(const Color &_color = Color::Literal::White, const Shape &_shape = Shape::Box, const Border &_border = 0,
       const std::vector<Style> &_styles = {Style::Filled})
      : Node("default", "default", _color, _shape, _border, _styles) {}

  Node(const Node &other) : id(other.id), label(other.label), color(other.color), shape(other.shape), border(other.border), styles(other.styles) {}

  Node(Node &&other)
      : id(std::move(other.id)), label(std::move(other.label)), color(std::move(other.color)), shape(other.shape), border(other.border),
        styles(std::move(other.styles)) {}

  Node &operator=(const Node &other) = default;

  bool operator==(const Node &other) const { return id == other.id; }

  struct Hash {
    size_t operator()(const Node &node) const { return std::hash<ID>()(node.id); }
  };
};

struct Edge {
  ID from;
  ID to;
  std::optional<Label> label;

  Edge(const ID &_from, const ID &_to, const std::optional<Label> _label = {}) : from(_from), to(_to), label(_label) {}
  Edge(const Edge &other) : from(other.from), to(other.to), label(other.label) {}
  Edge(Edge &&other) : from(std::move(other.from)), to(std::move(other.to)), label(std::move(other.label)) {}
  Edge &operator=(const Edge &other) = default;
};

struct Cluster {
  ID id;
  Label label;
  Color color;
  std::unordered_set<Node, Node::Hash> nodes;

  Cluster(const ID &_id, const Label &_label, const Color &_color = Color::Literal::White) : id(_id), label(_label), color(_color) {}
  Cluster(const Cluster &other) : id(other.id), label(other.label), color(other.color), nodes(other.nodes) {}
  Cluster(Cluster &&other) : id(std::move(other.id)), label(std::move(other.label)), color(std::move(other.color)), nodes(std::move(other.nodes)) {}
  Cluster &operator=(const Cluster &other) = default;

  bool operator==(const Cluster &other) const { return id == other.id; }

  struct Hash {
    size_t operator()(const Cluster &cluster) const { return std::hash<ID>()(cluster.id); }
  };
};

std::ostream &operator<<(std::ostream &stream, const Color &color);
std::ostream &operator<<(std::ostream &stream, const Style &style);
std::ostream &operator<<(std::ostream &stream, const Shape &shape);
std::ostream &operator<<(std::ostream &stream, const Node &node);
std::ostream &operator<<(std::ostream &stream, const Edge &edge);
std::ostream &operator<<(std::ostream &stream, const Cluster &cluster);

void visualize_dot_file(const std::filesystem::path &dot_file, bool interrupt);

} // namespace Graphviz
} // namespace LibCore