#pragma once

#include <iomanip>
#include <sstream>
#include <string>
#include <filesystem>
#include <unordered_set>
#include <optional>

#include <LibCore/Types.h>

namespace LibCore {

class TreeViz {
public:
  struct Color {
    enum class Type { RGB, Literal };
    enum class Literal {
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

  using NodeId = std::string;
  using Label  = std::string;
  using Border = u8;

  struct Node {
    NodeId id;
    Label label;
    Color color;
    Shape shape;
    Border border;
    std::vector<Style> styles;

    Node(const NodeId &_id, const Label &_label, Color _color, Shape _shape, Border _border, const std::vector<Style> &_styles)
        : id(_id), label(_label), color(_color), shape(_shape), border(_border), styles(_styles) {}

    Node(Color _color, Shape _shape, Border _border, const std::vector<Style> &_styles)
        : Node("default", "default", _color, _shape, _border, _styles) {}

    bool operator==(const Node &other) const { return id == other.id; }

    struct Hash {
      size_t operator()(const Node &node) const { return std::hash<NodeId>()(node.id); }
    };
  };

  struct Edge {
    NodeId from;
    NodeId to;
    std::optional<Label> label;

    Edge(const NodeId &_from, const NodeId &_to, const std::optional<Label> _label) : from(_from), to(_to), label(_label) {}
  };

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

  void add_node(const NodeId &id, const Label &label);
  void add_node(const Node &node);

  void add_edge(const NodeId &from, const NodeId &to, std::optional<Label> label = std::nullopt);

  std::filesystem::path get_file_path() const { return fpath; }

  Node get_default_node() const { return default_node; }
  void set_default_node(const Node &node) { default_node = node; }

  static std::string find_and_replace(const std::string &str, const std::vector<std::pair<std::string, std::string>> &replacements);
  static std::string sanitize_html_label(const std::string &label);
};

std::ostream &operator<<(std::ostream &stream, const TreeViz::Color &color);
std::ostream &operator<<(std::ostream &stream, TreeViz::TreeViz::Style style);
std::ostream &operator<<(std::ostream &stream, TreeViz::TreeViz::Shape shape);

} // namespace LibCore