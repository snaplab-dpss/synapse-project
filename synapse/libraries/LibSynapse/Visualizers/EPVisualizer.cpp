#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Visualizers/EPVisualizer.h>

#include <ctime>
#include <fstream>
#include <limits>
#include <cmath>
#include <unistd.h>

#define SHOW_MODULE_NAME(M)                                                                                                                          \
  void EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {                                                                            \
    function_call(ep_node, node->get_node(), node->get_target(), node->get_name());                                                                  \
  }

#define VISIT_BRANCH(M)                                                                                                                              \
  void EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) { branch(ep_node, node->get_node(), node->get_target(), node->get_name()); }

#define IGNORE_MODULE(M)                                                                                                                             \
  void EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {}

namespace LibSynapse {

namespace {
const std::unordered_map<TargetType, TreeViz::Color> node_colors = {
    {TargetType::Tofino, TreeViz::Color::Literal::CornflowerBlue},
    {TargetType::Controller, TreeViz::Color::Literal::LightCoral},
    {TargetType::x86, TreeViz::Color::Literal::Orange},
};

const std::unordered_set<ModuleType> modules_to_ignore{
    ModuleType::x86_Ignore,
    ModuleType::Tofino_Ignore,
    ModuleType::Controller_Ignore,
};

bool should_ignore_node(const EPNode *node) {
  const Module *module  = node->get_module();
  const ModuleType type = module->get_type();
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

EPViz::EPViz(const std::filesystem::path &fpath) : treeviz(fpath) {}

void EPViz::log(const EPNode *ep_node) const {
  // Don't log anything.
}

void EPViz::function_call(const EPNode *ep_node, const BDDNode *node, TargetType target, const std::string &label) {
  std::stringstream ss;
  ss << "[";
  ss << "EPNode=";
  ss << ep_node->get_id();
  ss << ",BDDNode=";
  ss << node->get_id();
  ss << "]";
  ss << "\\n";
  ss << TreeViz::find_and_replace(label, {{"\n", "\\n"}});

  TreeViz::Node tree_node = treeviz.get_default_node();
  tree_node.id            = std::to_string(ep_node->get_id());
  tree_node.label         = ss.str();
  tree_node.color         = node_colors.at(target);
  treeviz.add_node(tree_node);
}

void EPViz::branch(const EPNode *ep_node, const BDDNode *node, TargetType target, const std::string &label) {
  std::stringstream ss;
  ss << "[";
  ss << "EPNode=";
  ss << ep_node->get_id();
  ss << ",BDDNode=";
  ss << node->get_id();
  ss << "]";
  ss << " ";
  ss << TreeViz::find_and_replace(label, {{"\n", "\\n"}});

  TreeViz::Node tree_node = treeviz.get_default_node();
  tree_node.id            = std::to_string(ep_node->get_id());
  tree_node.label         = ss.str();
  tree_node.color         = node_colors.at(target);
  tree_node.shape         = TreeViz::Shape::Ellipse;
  treeviz.add_node(tree_node);
}

void EPViz::visualize(const EP *ep, bool interrupt) {
  assert(ep && "Invalid EP");
  EPViz visualizer;
  visualizer.visit(ep);
  log_visualization(ep, visualizer.treeviz.get_file_path());
  visualizer.treeviz.show(interrupt);
}

void EPViz::dump_to_file(const EP *ep, const std::filesystem::path &file_name) {
  assert(ep && "Invalid EP");
  EPViz visualizer(file_name);
  visualizer.visit(ep);
  visualizer.treeviz.write();
}

void EPViz::visit(const EP *ep) {
  assert(ep && "Invalid EP");
  EPVisitor::visit(ep);
}

void EPViz::visit(const EP *ep, const EPNode *node) {
  assert(ep && "Invalid EP");

  const Module *module           = node->get_module();
  const EPVisitor::Action action = module->visit(*this, ep, node);
  if (action == EPVisitor::Action::skipChildren) {
    return;
  }

  for (const EPNode *child : node->get_children()) {
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

    if (!should_ignore_node(node)) {
      treeviz.add_edge(std::to_string(node->get_id()), std::to_string(child->get_id()));
    }
  }
}

} // namespace LibSynapse