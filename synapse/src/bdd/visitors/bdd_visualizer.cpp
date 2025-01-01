#include "bdd_visualizer.h"

#include "../tree.h"
#include "../nodes/nodes.h"
#include "../../exprs/exprs.h"
#include "../../exprs/solver.h"

namespace {
const char *COLOR_PROCESSED = "gray";
const char *COLOR_NEXT = "cyan";
const char *COLOR_CALL = "cornflowerblue";
const char *COLOR_BRANCH = "yellow";
const char *COLOR_FORWARD = "chartreuse2";
const char *COLOR_DROP = "brown1";
const char *COLOR_BROADCAST = "purple";

void log_visualization(const BDD *bdd, const std::string &fname) {
  std::cerr << "Visualizing BDD";
  std::cerr << " id=" << bdd->get_id();
  std::cerr << " hash=" << bdd->hash();
  std::cerr << " file=" << fname;
  std::cerr << "\n";
}
} // namespace

BDDViz::BDDViz(const bdd_visualizer_opts_t &_opts) : Graphviz(_opts.fname), opts(_opts) {}

BDDViz::BDDViz() : Graphviz() {}

std::string BDDViz::get_color(const Node *node) const {
  node_id_t id = node->get_id();

  if (opts.colors_per_node.find(id) != opts.colors_per_node.end()) {
    return opts.colors_per_node.at(id);
  }

  if (opts.default_color.first) {
    return opts.default_color.second;
  }

  if (opts.processed.nodes.find(id) != opts.processed.nodes.end()) {
    return COLOR_PROCESSED;
  }

  if (opts.processed.next && id == opts.processed.next->get_id()) {
    return COLOR_NEXT;
  }

  std::string color;

  switch (node->get_type()) {
  case NodeType::Call: {
    color = COLOR_CALL;
  } break;
  case NodeType::Branch: {
    color = COLOR_BRANCH;
  } break;
  case NodeType::Route: {
    const Route *route = static_cast<const Route *>(node);
    RouteOp operation = route->get_operation();
    switch (operation) {
    case RouteOp::Forward:
      color = COLOR_FORWARD;
      break;
    case RouteOp::Drop:
      color = COLOR_DROP;
      break;
    case RouteOp::Broadcast:
      color = COLOR_BROADCAST;
      break;
    }
  } break;
  }

  return color;
}

void BDDViz::visualize(const BDD *bdd, bool interrupt, bdd_visualizer_opts_t opts) {
  BDDViz visualizer(opts);
  visualizer.visit(bdd);
  log_visualization(bdd, visualizer.fpath);
  visualizer.show(interrupt);
}

void BDDViz::visit(const BDD *bdd) {
  const Node *root = bdd->get_root();

  ss << "digraph mygraph {\n";
  ss << "\tnode [shape=box, style=\"rounded,filled\", border=0];\n";
  visitRoot(root);
  ss << "}";
}

BDDVisitor::Action BDDViz::visit(const Branch *node) {
  const Node *on_true = node->get_on_true();
  const Node *on_false = node->get_on_false();

  klee::ref<klee::Expr> condition = node->get_condition();

  if (on_true)
    on_true->visit(*this);
  if (on_false)
    on_false->visit(*this);

  ss << "\t" << get_gv_name(node);
  ss << " [shape=Mdiamond, label=\"";

  ss << node->get_id() << ":";
  ss << pretty_print_expr(condition);

  if (opts.annotations_per_node.find(node->get_id()) != opts.annotations_per_node.end()) {
    ss << "\\n";
    ss << opts.annotations_per_node.at(node->get_id());
  }

  ss << "\"";

  ss << ", fillcolor=\"" << get_color(node) << "\"";
  ss << "];\n";

  if (on_true) {
    ss << "\t" << get_gv_name(node);
    ss << " -> ";
    ss << get_gv_name(on_true);
    ss << " [label=\"True\"];\n";
  }

  if (on_false) {
    ss << "\t" << get_gv_name(node);
    ss << " -> ";
    ss << get_gv_name(on_false);
    ss << " [label=\"False\"];\n";
  }

  return BDDVisitor::Action::Stop;
}

BDDVisitor::Action BDDViz::visit(const Call *node) {
  const call_t &call = node->get_call();
  node_id_t id = node->get_id();
  const Node *next = node->get_next();

  if (next) {
    next->visit(*this);
  }

  ss << "\t" << get_gv_name(node);
  ss << " [label=\"";
  ss << id << ":";
  ss << call.function_name;
  ss << "(";

  size_t i = 0;
  for (const std::pair<std::string, arg_t> &pair : call.args) {
    if (call.args.size() > 1) {
      ss << "\\l";
      ss << std::string(2, ' ');
    }

    ss << pair.first << ":";
    const arg_t &arg = pair.second;

    if (arg.fn_ptr_name.first) {
      ss << arg.fn_ptr_name.second;
    } else {
      ss << pretty_print_expr(arg.expr, false);

      if (!arg.in.isNull() || !arg.out.isNull()) {
        ss << "[";

        if (!arg.in.isNull()) {
          ss << pretty_print_expr(arg.in, false);
        }

        if (!arg.out.isNull() &&
            (arg.in.isNull() ||
             !solver_toolbox.are_exprs_always_equal(arg.in, arg.out))) {
          ss << " -> ";
          ss << pretty_print_expr(arg.out, false);
        }

        ss << "]";
      }
    }

    if (i != call.args.size() - 1) {
      ss << ",";
    }

    i++;
  }

  ss << ")";

  if (!call.ret.isNull()) {
    ss << " -> " << pretty_print_expr(call.ret);
  }

  const symbols_t &symbols = node->get_locally_generated_symbols();
  if (symbols.size()) {
    ss << "\\l=>{";
    bool first = true;
    for (const symbol_t &s : symbols) {
      if (!first) {
        ss << ",";
      } else {
        first = false;
      }
      ss << s.array->name;
    }
    ss << "}";
  }

  if (opts.annotations_per_node.find(node->get_id()) != opts.annotations_per_node.end()) {
    ss << "\\l";
    ss << opts.annotations_per_node.at(node->get_id());
  }

  ss << "\"";

  ss << ", fillcolor=\"" << get_color(node) << "\"";
  ss << "];\n";

  if (next) {
    ss << "\t" << get_gv_name(node);
    ss << " -> ";
    ss << get_gv_name(next);
    ss << ";\n";
  }

  return BDDVisitor::Action::Stop;
}

BDDVisitor::Action BDDViz::visit(const Route *node) {
  node_id_t id = node->get_id();
  int dst_device = node->get_dst_device();
  RouteOp operation = node->get_operation();
  const Node *next = node->get_next();

  if (next) {
    next->visit(*this);
  }

  ss << "\t" << get_gv_name(node);
  ss << " [label=\"";
  ss << id << ":";

  switch (operation) {
  case RouteOp::Forward: {
    ss << "fwd(" << dst_device << ")";
    break;
  }
  case RouteOp::Drop: {
    ss << "drop()";
    break;
  }
  case RouteOp::Broadcast: {
    ss << "bcast()";
    break;
  }
  }

  if (opts.annotations_per_node.find(id) != opts.annotations_per_node.end()) {
    ss << "\\l";
    ss << opts.annotations_per_node.at(id);
  }

  ss << "\"";
  ss << ", fillcolor=\"" << get_color(node) << "\"";
  ss << "];\n";

  if (next) {
    ss << "\t" << get_gv_name(node);
    ss << " -> ";
    ss << get_gv_name(next);
    ss << ";\n";
  }

  return BDDVisitor::Action::Stop;
}

void BDDViz::visitRoot(const Node *root) { root->visit(*this); }