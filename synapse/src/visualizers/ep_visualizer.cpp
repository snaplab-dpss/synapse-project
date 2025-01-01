#include <ctime>
#include <fstream>
#include <limits>
#include <math.h>
#include <unistd.h>

#include "ep_visualizer.h"

#include "../log.h"
#include "../execution_plan/execution_plan.h"
#include "../targets/targets.h"

#define SHOW_MODULE_NAME(M)                                                              \
  void EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {                \
    function_call(ep_node, node->get_node(), node->get_target(), node->get_name());      \
  }

#define VISIT_BRANCH(M)                                                                  \
  void EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {                \
    branch(ep_node, node->get_node(), node->get_target(), node->get_name());             \
  }

#define IGNORE_MODULE(M)                                                                 \
  void EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {}

namespace {
std::unordered_map<TargetType, std::string> node_colors = {
    {TargetType::Tofino, "cornflowerblue"},
    {TargetType::TofinoCPU, "lightcoral"},
    {TargetType::x86, "orange"},
};

std::unordered_set<ModuleType> modules_to_ignore = {
    ModuleType::x86_Ignore,
    ModuleType::Tofino_Ignore,
    ModuleType::TofinoCPU_Ignore,
};

bool should_ignore_node(const EPNode *node) {
  const Module *module = node->get_module();
  ModuleType type = module->get_type();
  return modules_to_ignore.find(type) != modules_to_ignore.end();
}

void log_visualization(const EP *ep, const std::string &fname) {
  ASSERT(ep, "Invalid EP");
  Log::log() << "Visualizing EP";
  Log::log() << " id=" << ep->get_id();
  Log::log() << " file=" << fname;
  Log::log() << " ancestors=[";
  bool first = true;
  for (ep_id_t ancestor : ep->get_ancestors()) {
    if (!first) {
      Log::log() << " ";
    }
    Log::log() << ancestor;
    first = false;
  }
  Log::log() << "]";
  Log::log() << "\n";
}
} // namespace

EPViz::EPViz() {}

void EPViz::log(const EPNode *ep_node) const {
  // Don't log anything.
}

void EPViz::function_call(const EPNode *ep_node, const Node *node, TargetType target,
                          const std::string &label) {
  std::string nice_label = label;
  find_and_replace(nice_label, {{"\n", "\\n"}});

  ASSERT(node_colors.find(target) != node_colors.end(), "No color for target");
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

void EPViz::branch(const EPNode *ep_node, const Node *node, TargetType target,
                   const std::string &label) {
  std::string nice_label = label;
  find_and_replace(nice_label, {{"\n", "\\n"}});

  ASSERT(node_colors.find(target) != node_colors.end(), "No color for target");
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
  ASSERT(ep, "Invalid EP");
  EPViz visualizer;
  visualizer.visit(ep);
  log_visualization(ep, visualizer.fpath);
  visualizer.show(interrupt);
}

void EPViz::visit(const EP *ep) {
  ASSERT(ep, "Invalid EP");
  ss << "digraph EP {\n";
  ss << "layout=\"dot\";";
  ss << "node [shape=record,style=filled];\n";

  EPVisitor::visit(ep);

  ss << "}\n";
  ss.flush();
}

void EPViz::visit(const EP *ep, const EPNode *node) {
  ASSERT(ep, "Invalid EP");

  bool ignore = should_ignore_node(node);

  if (!ignore) {
    ss << node->get_id() << " ";
  }

  const Module *module = node->get_module();
  EPVisitor::Action action = module->visit(*this, ep, node);

  if (action == EPVisitor::Action::skipChildren) {
    return;
  }

  const std::vector<EPNode *> &children = node->get_children();
  for (const EPNode *child : children) {
    while (child && should_ignore_node(child)) {
      ASSERT(child->get_children().size() <= 1, "Invalid child");
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
      ss << node->get_id() << " -> " << child->get_id() << ";" << "\n";
    }
  }
}