#include <ctime>
#include <fstream>
#include <limits>
#include <math.h>
#include <unistd.h>

#include "ep_visualizer.hpp"

#include "../system.hpp"
#include "../execution_plan/execution_plan.hpp"
#include "../targets/targets.hpp"

#define SHOW_MODULE_NAME(M)                                                                                            \
  void EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {                                              \
    function_call(ep_node, node->get_node(), node->get_target(), node->get_name());                                    \
  }

#define VISIT_BRANCH(M)                                                                                                \
  void EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {                                              \
    branch(ep_node, node->get_node(), node->get_target(), node->get_name());                                           \
  }

#define IGNORE_MODULE(M)                                                                                               \
  void EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {}

namespace synapse {
namespace {
std::unordered_map<TargetType, std::string> node_colors = {
    {TargetType::Tofino, "cornflowerblue"},
    {TargetType::Controller, "lightcoral"},
    {TargetType::x86, "orange"},
};

std::unordered_set<ModuleType> modules_to_ignore = {
    ModuleType::x86_Ignore,
    ModuleType::Tofino_Ignore,
    ModuleType::Controller_Ignore,
};

bool should_ignore_node(const EPNode *node) {
  const Module *module = node->get_module();
  ModuleType type      = module->get_type();
  return modules_to_ignore.find(type) != modules_to_ignore.end();
}

void log_visualization(const EP *ep, const std::string &fname) {
  assert(ep && "Invalid EP");
  std::cerr << "Visualizing EP";
  std::cerr << " id=" << ep->get_id();
  std::cerr << " file=" << fname;
  std::cerr << " ancestors=[";
  bool first = true;
  for (ep_id_t ancestor : ep->get_ancestors()) {
    if (!first) {
      std::cerr << " ";
    }
    std::cerr << ancestor;
    first = false;
  }
  std::cerr << "]";
  std::cerr << "\n";
}
} // namespace

EPViz::EPViz() {}

void EPViz::log(const EPNode *ep_node) const {
  // Don't log anything.
}

void EPViz::function_call(const EPNode *ep_node, const Node *node, TargetType target, const std::string &label) {
  std::string nice_label = label;
  find_and_replace(nice_label, {{"\n", "\\n"}});

  assert(node_colors.find(target) != node_colors.end() && "No color for target");
  ss << "[label=\"";

  ss << "[";
  ss << "EPNode=";
  ss << ep_node->get_id();
  ss << ",BDDNode=";
  ss << node->get_id();
  ss << "]";
  ss << "\\n";

  ss << nice_label;
  ss << "\", ";
  ss << "color=" << node_colors[target] << "];";
  ss << "\n";
}

void EPViz::branch(const EPNode *ep_node, const Node *node, TargetType target, const std::string &label) {
  std::string nice_label = label;
  find_and_replace(nice_label, {{"\n", "\\n"}});

  assert(node_colors.find(target) != node_colors.end() && "No color for target");
  ss << "[shape=Mdiamond, label=\"";

  ss << "[";
  ss << "EPNode=";
  ss << ep_node->get_id();
  ss << ",BDDNode=";
  ss << node->get_id();
  ss << "]";
  ss << " ";

  ss << nice_label;
  ss << "\", ";
  ss << "color=" << node_colors[target] << "];";
  ss << "\n";
}

void EPViz::visualize(const EP *ep, bool interrupt) {
  assert(ep && "Invalid EP");
  EPViz visualizer;
  visualizer.visit(ep);
  log_visualization(ep, visualizer.fpath);
  visualizer.show(interrupt);
}

void EPViz::visit(const EP *ep) {
  assert(ep && "Invalid EP");
  ss << "digraph EP {\n";
  ss << "layout=\"dot\";";
  ss << "node [shape=record,style=filled];\n";

  EPVisitor::visit(ep);

  ss << "}\n";
  ss.flush();
}

void EPViz::visit(const EP *ep, const EPNode *node) {
  assert(ep && "Invalid EP");

  bool ignore = should_ignore_node(node);

  if (!ignore) {
    ss << node->get_id() << " ";
  }

  const Module *module     = node->get_module();
  EPVisitor::Action action = module->visit(*this, ep, node);

  if (action == EPVisitor::Action::skipChildren) {
    return;
  }

  const std::vector<EPNode *> &children = node->get_children();
  for (const EPNode *child : children) {
    while (child && should_ignore_node(child)) {
      assert(child->get_children().size() <= 1 && "Invalid child");
      if (!child->get_children().empty()) {
        child = child->get_children().front();
      } else {
        child = nullptr;
      }
    }

    if (!child) {
      continue;
    }

    visit(ep, child);

    if (!ignore) {
      ss << node->get_id() << " -> " << child->get_id() << ";"
         << "\n";
    }
  }
}
} // namespace synapse