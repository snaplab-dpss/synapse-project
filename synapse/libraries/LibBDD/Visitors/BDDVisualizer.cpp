#include <LibBDD/Visitors/BDDVisualizer.h>
#include <LibBDD/BDD.h>
#include <LibCore/Expr.h>
#include <LibCore/Solver.h>

namespace LibBDD {

using LibCore::pretty_print_expr;
using LibCore::solver_toolbox;

namespace {

const TreeViz::Color COLOR_PROCESSED(TreeViz::Color::Literal::Gray);
const TreeViz::Color COLOR_NEXT(TreeViz::Color::Literal::Cyan);
const TreeViz::Color COLOR_CALL(TreeViz::Color::Literal::CornflowerBlue);
const TreeViz::Color COLOR_BRANCH(TreeViz::Color::Literal::Yellow);
const TreeViz::Color COLOR_FORWARD(TreeViz::Color::Literal::Chartreuse2);
const TreeViz::Color COLOR_DROP(TreeViz::Color::Literal::Brown1);
const TreeViz::Color COLOR_BROADCAST(TreeViz::Color::Literal::Purple);

const TreeViz::Shape SHAPE_CALL(TreeViz::Shape::Box);
const TreeViz::Shape SHAPE_BRANCH(TreeViz::Shape::Ellipse);
const TreeViz::Shape SHAPE_FORWARD(TreeViz::Shape::Box);
const TreeViz::Shape SHAPE_DROP(TreeViz::Shape::Box);
const TreeViz::Shape SHAPE_BROADCAST(TreeViz::Shape::Box);

void log_visualization(const BDD *bdd, const std::string &fname) {
  std::cerr << "Visualizing BDD";
  std::cerr << " id=" << bdd->get_id();
  std::cerr << " hash=" << bdd->hash();
  std::cerr << " file=" << fname;
  std::cerr << "\n";
}

} // namespace

BDDViz::BDDViz(const bdd_visualizer_opts_t &_opts)
    : call_node(TreeViz::Node(COLOR_CALL, SHAPE_CALL, 0, {TreeViz::Style::Filled})),
      branch_node(TreeViz::Node(COLOR_BRANCH, SHAPE_BRANCH, 0, {TreeViz::Style::Filled})),
      forward_node(TreeViz::Node(COLOR_FORWARD, SHAPE_FORWARD, 0, {TreeViz::Style::Filled})),
      drop_node(TreeViz::Node(COLOR_DROP, SHAPE_DROP, 0, {TreeViz::Style::Filled})),
      broadcast_node(TreeViz::Node(COLOR_BROADCAST, SHAPE_BROADCAST, 0, {TreeViz::Style::Filled})), opts(_opts), treeviz(_opts.fname) {}

BDDViz::BDDViz() : BDDViz(bdd_visualizer_opts_t()) {}

TreeViz::Color BDDViz::get_color(const BDDNode *node) const {
  const bdd_node_id_t id = node->get_id();

  if (opts.colors_per_node.find(id) != opts.colors_per_node.end()) {
    return opts.colors_per_node.at(id);
  }

  if (opts.default_color.has_value()) {
    return opts.default_color.value();
  }

  if (opts.processed.nodes.find(id) != opts.processed.nodes.end()) {
    return COLOR_PROCESSED;
  }

  if (opts.processed.next && id == opts.processed.next->get_id()) {
    return COLOR_NEXT;
  }

  switch (node->get_type()) {
  case BDDNodeType::Call:
    return COLOR_CALL;
  case BDDNodeType::Branch:
    return COLOR_BRANCH;
  case BDDNodeType::Route: {
    const Route *route      = dynamic_cast<const Route *>(node);
    const RouteOp operation = route->get_operation();
    switch (operation) {
    case RouteOp::Forward:
      return COLOR_FORWARD;
    case RouteOp::Drop:
      return COLOR_DROP;
    case RouteOp::Broadcast:
      return COLOR_BROADCAST;
    }
  }
  }

  return TreeViz::Color::Literal::Gray; // Default color if none matched
}

void BDDViz::visualize(const BDD *bdd, bool interrupt, bdd_visualizer_opts_t opts) {
  assert(bdd && "Invalid BDD");
  BDDViz bddviz(opts);
  bddviz.visit(bdd);
  log_visualization(bdd, bddviz.treeviz.get_file_path().string());
  bddviz.show(interrupt);
}

void BDDViz::dump_to_file(const BDD *bdd, const std::filesystem::path &fpath) {
  bdd_visualizer_opts_t opts;
  opts.fname = fpath;
  dump_to_file(bdd, opts);
}

void BDDViz::dump_to_file(const BDD *bdd, const bdd_visualizer_opts_t &opts) {
  assert(bdd && "Invalid BDD");
  BDDViz bddviz(opts);
  bddviz.visit(bdd);
  bddviz.treeviz.write();
}

void BDDViz::visit(const BDD *bdd) {
  assert(bdd && "Invalid BDD");
  visitRoot(bdd->get_root());
}

BDDVisitor::Action BDDViz::visit(const Branch *node) {
  klee::ref<klee::Expr> condition = node->get_condition();
  const BDDNode *on_true          = node->get_on_true();
  const BDDNode *on_false         = node->get_on_false();

  TreeViz::Node tree_node = branch_node;
  tree_node.id            = get_gv_name(node);
  tree_node.color         = get_color(node);

  std::stringstream label;
  label << node->get_id() << ":" << pretty_print_expr(condition);
  if (opts.annotations_per_node.find(node->get_id()) != opts.annotations_per_node.end()) {
    label << "\\n";
    label << opts.annotations_per_node.at(node->get_id());
  }

  tree_node.label = label.str();
  treeviz.add_node(tree_node);

  if (on_true) {
    treeviz.add_edge(get_gv_name(node), get_gv_name(on_true), "True");
  }

  if (on_false) {
    treeviz.add_edge(get_gv_name(node), get_gv_name(on_false), "False");
  }

  return BDDVisitor::Action::Continue;
}

BDDVisitor::Action BDDViz::visit(const Call *node) {
  const call_t &call     = node->get_call();
  const bdd_node_id_t id = node->get_id();
  const BDDNode *next    = node->get_next();

  TreeViz::Node tree_node = call_node;
  tree_node.id            = get_gv_name(node);
  tree_node.color         = get_color(node);

  std::stringstream label;
  label << id << ":";
  label << call.function_name;
  label << "(";

  size_t i = 0;
  for (const auto &[name, arg] : call.args) {
    if (call.args.size() > 1) {
      label << "\\l";
      label << std::string(2, ' ');
    }

    label << name << ":";

    if (arg.fn_ptr_name.first) {
      label << arg.fn_ptr_name.second;
    } else {
      label << pretty_print_expr(arg.expr, false);

      if (!arg.in.isNull() || !arg.out.isNull()) {
        label << "[";

        if (!arg.in.isNull()) {
          label << pretty_print_expr(arg.in, false);
        }

        if (!arg.out.isNull() && (arg.in.isNull() || !solver_toolbox.are_exprs_always_equal(arg.in, arg.out))) {
          label << " -> ";
          label << pretty_print_expr(arg.out, false);
        }

        label << "]";
      }
    }

    if (i != call.args.size() - 1) {
      label << ",";
    }

    i++;
  }

  label << ")";

  if (!call.ret.isNull()) {
    label << " -> " << pretty_print_expr(call.ret);
  }

  const Symbols &symbols = node->get_local_symbols();
  if (symbols.size()) {
    label << "\\l=>{";
    bool first = true;
    for (const symbol_t &s : symbols.get()) {
      if (!first) {
        label << ",";
      } else {
        first = false;
      }
      label << s.name;
    }
    label << "}";
  }

  if (opts.annotations_per_node.find(node->get_id()) != opts.annotations_per_node.end()) {
    label << "\\l";
    label << opts.annotations_per_node.at(node->get_id());
  }

  tree_node.label = label.str();
  treeviz.add_node(tree_node);

  if (next) {
    treeviz.add_edge(get_gv_name(node), get_gv_name(next));
  }

  return BDDVisitor::Action::Continue;
}

BDDVisitor::Action BDDViz::visit(const Route *node) {
  const bdd_node_id_t id           = node->get_id();
  klee::ref<klee::Expr> dst_device = node->get_dst_device();
  const RouteOp operation          = node->get_operation();
  const BDDNode *next              = node->get_next();

  TreeViz::Node tree_node = forward_node;
  switch (operation) {
  case RouteOp::Forward:
    tree_node = forward_node;
    break;
  case RouteOp::Drop:
    tree_node = drop_node;
    break;
  case RouteOp::Broadcast:
    tree_node = broadcast_node;
    break;
  }
  tree_node.id    = get_gv_name(node);
  tree_node.color = get_color(node);

  std::stringstream label;
  label << id << ":";
  switch (operation) {
  case RouteOp::Forward: {
    label << "fwd(" << pretty_print_expr(dst_device, false) << ")";
    break;
  }
  case RouteOp::Drop: {
    label << "drop()";
    break;
  }
  case RouteOp::Broadcast: {
    label << "bcast()";
    break;
  }
  }

  if (opts.annotations_per_node.find(id) != opts.annotations_per_node.end()) {
    label << "\\l";
    label << opts.annotations_per_node.at(id);
  }
  tree_node.label = label.str();
  treeviz.add_node(tree_node);

  if (next) {
    treeviz.add_edge(get_gv_name(node), get_gv_name(next));
  }

  return BDDVisitor::Action::Continue;
}

void BDDViz::visitRoot(const BDDNode *root) { root->visit(*this); }

void BDDViz::show(bool interrupt) const { treeviz.show(interrupt); }
void BDDViz::write() const { treeviz.write(); }

} // namespace LibBDD