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
    enum class ColorType { RGB, Literal };
    enum class ColorLiteral { Gray, Cyan, CornflowerBlue, Yellow, Chartreuse2, Brown1, Purple };

    ColorType type;
    struct {
      u8 r;
      u8 g;
      u8 b;
      u8 o;
    } rgb;
    ColorLiteral literal;

    Color(u8 r, u8 g, u8 b, u8 o = 1) : type(ColorType::RGB), rgb{r, g, b, o} {}
    Color(ColorLiteral _literal) : type(ColorType::Literal), literal(_literal) {}

    std::string to_gv_repr() const;
  };

  enum class Shape { Box, MDiamond, Octagon };
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

  void set_default_node_color(Color color) { default_node.color = color; }
  void set_default_node_shape(Shape shape) { default_node.shape = shape; }
  void set_default_node_styles(const std::vector<Style> &styles) { default_node.styles = styles; }

  std::filesystem::path get_file_path() const { return fpath; }

  static void find_and_replace(std::string &str, const std::vector<std::pair<std::string, std::string>> &replacements);
  static void sanitize_html_label(std::string &label);

private:
  std::string color_to_string(Color color) const;
  std::string shape_to_string(Shape shape) const;
  std::string style_to_string(Style style) const;
};

} // namespace LibCore