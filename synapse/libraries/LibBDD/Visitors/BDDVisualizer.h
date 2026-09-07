#pragma once

#include <LibBDD/Nodes/Nodes.h>
#include <LibBDD/Visitors/Visitor.h>
#include <LibCore/TreeViz.h>

#include <filesystem>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <cmath>
#include <unistd.h>
#include <optional>

#include <unordered_set>
#include <unordered_map>

namespace LibBDD {

using LibCore::TreeViz;
using LibCore::Graphviz::Color;
using LibCore::Graphviz::Node;
using LibCore::Graphviz::Shape;

struct processed_t {
  std::unordered_set<bdd_node_id_t> nodes;
  const BDDNode *next;

  processed_t() : next(nullptr) {}
};

struct bdd_visualizer_opts_t {
  std::filesystem::path fname;
  std::unordered_map<bdd_node_id_t, Color> colors_per_node;
  std::optional<Color> default_color;
  std::unordered_map<bdd_node_id_t, std::string> annotations_per_node;
  processed_t processed;
  // Expressions in labels are elided past this many nested operators and this many characters
  // (0 = unlimited); the .bdd file keeps them in full.
  unsigned expr_max_depth = 4;
  size_t expr_max_len     = 200;
};

class BDDViz : public BDDVisitor {
protected:
  std::string pp(klee::ref<klee::Expr> expr, bool use_signed = true) const;

  const Node call_node;
  const Node branch_node;
  const Node forward_node;
  const Node drop_node;
  const Node broadcast_node;

  bdd_visualizer_opts_t opts;
  TreeViz treeviz;

public:
  BDDViz(const bdd_visualizer_opts_t &_opts);
  BDDViz();

  static void visualize(const BDD *bdd, bool interrupt, bdd_visualizer_opts_t opts = {});
  static void dump_to_file(const BDD *bdd, const std::filesystem::path &fpath);
  static void dump_to_file(const BDD *bdd, const bdd_visualizer_opts_t &opts);

  void visit(const BDD *bdd) override final;
  void visitRoot(const BDDNode *root) override final;
  void write() const;
  void show(bool interrupt) const;

  BDDVisitor::Action visit(const Branch *node) override final;
  BDDVisitor::Action visit(const Call *node) override final;
  BDDVisitor::Action visit(const Route *node) override final;

private:
  std::string get_gv_name(const BDDNode *node) const { return std::to_string(node->get_id()); }
  Color get_color(const BDDNode *node) const;
};

} // namespace LibBDD