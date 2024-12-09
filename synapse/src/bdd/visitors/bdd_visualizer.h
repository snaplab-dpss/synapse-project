#pragma once

#include <filesystem>
#include <assert.h>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <math.h>
#include <unistd.h>

#include <unordered_set>
#include <unordered_map>

#include "../visitor.h"
#include "../../graphviz/graphviz.h"
#include "../nodes/node.h"

struct processed_t {
  std::unordered_set<node_id_t> nodes;
  const Node *next;

  processed_t() : next(nullptr) {}
};

struct bdd_visualizer_opts_t {
  std::filesystem::path fname;
  std::unordered_map<node_id_t, std::string> colors_per_node;
  std::pair<bool, std::string> default_color;
  std::unordered_map<node_id_t, std::string> annotations_per_node;
  processed_t processed;
};

class BDDViz : public BDDVisitor, public Graphviz {
protected:
  bdd_visualizer_opts_t opts;

public:
  BDDViz(const bdd_visualizer_opts_t &_opts);
  BDDViz();

  std::string get_color(const Node *node) const;

  static void visualize(const BDD *bdd, bool interrupt,
                        bdd_visualizer_opts_t opts = {});

  void visit(const BDD *bdd) override;
  void visitRoot(const Node *root) override;

  BDDVisitor::Action visit(const Branch *node) override;
  BDDVisitor::Action visit(const Call *node) override;
  BDDVisitor::Action visit(const Route *node) override;

private:
  std::string get_gv_name(const Node *node) const {
    return std::to_string(node->get_id());
  }
};
