#include <LibSynapse/Visualizers/SSVisualizer.h>
#include <LibCore/TreeViz.h>
#include <LibCore/Strings.h>

#include <unordered_map>

namespace LibSynapse {

using LibCore::sanitize_html_label;
using LibCore::Graphviz::Color;
using LibCore::Graphviz::Node;
using LibCore::Graphviz::Shape;

namespace {
const std::unordered_map<TargetArchitecture, Color> node_colors = {
    {TargetArchitecture::Tofino, Color::Literal::CornflowerBlue},
    {TargetArchitecture::Controller, Color::Literal::LightCoral},
    {TargetArchitecture::x86, Color::Literal::Orange},
};

const Color selected_color = Color::Literal::Chartreuse1;

std::string stringify_score(const Score &score) {
  std::stringstream score_builder;
  score_builder << score;
  return sanitize_html_label(score_builder.str());
}

bool should_highlight(const SSNode *ssnode, const std::set<ep_id_t> &highlight) { return highlight.find(ssnode->ep_id) != highlight.end(); }

std::string bold(const std::string &str) { return "<b>" + str + "</b>"; }

void log_visualization(const SearchSpace *search_space, const std::string &fname, const EP *ep = nullptr) {
  assert(search_space && "Search space is null");
  std::cerr << "Visualizing SS";
  std::cerr << " file=" << fname;
  if (ep)
    std::cerr << " highlight=" << ep->get_id();
  std::cerr << "\n";
}
} // namespace

SSViz::SSViz() {}

SSViz::SSViz(const ss_opts_t &opts) : treeviz(opts.fpath) {
  const std::set<ep_id_t> &ancestors = opts.highlight->get_ancestors();
  highlight.insert(ancestors.begin(), ancestors.end());
  highlight.insert(opts.highlight->get_id());
}

void SSViz::visit(const SearchSpace *search_space) {
  Node default_node  = treeviz.get_default_node();
  default_node.shape = Shape::Html;
  treeviz.set_default_node(default_node);

  const SSNode *root = search_space->get_root();

  if (root) {
    visit_definitions(root);
    visit_links(root);
  }
}

void SSViz::visualize(const SearchSpace *search_space, bool interrupt) {
  assert(search_space && "Search space is null");
  SSViz visualizer;
  visualizer.visit(search_space);
  log_visualization(search_space, visualizer.treeviz.get_file_path());
  visualizer.treeviz.show(interrupt);
}

void SSViz::visualize(const SearchSpace *search_space, const EP *highlight, bool interrupt) {
  assert(search_space && "Search space is null");
  assert(highlight && "EP is null");
  ss_opts_t opts;
  opts.highlight = highlight;
  SSViz visualizer(opts);
  visualizer.visit(search_space);
  log_visualization(search_space, visualizer.treeviz.get_file_path(), highlight);
  visualizer.treeviz.show(interrupt);
}

void SSViz::dump_to_file(const SearchSpace *search_space, const std::filesystem::path &file_name) {
  assert(search_space && "Search space is null");
  ss_opts_t opts;
  opts.fpath = file_name;
  SSViz visualizer(opts);
  visualizer.visit(search_space);
  visualizer.treeviz.write();
}

void SSViz::dump_to_file(const SearchSpace *search_space, const EP *highlight, const std::filesystem::path &file_name) {
  assert(search_space && "Search space is null");
  assert(highlight && "EP is null");
  ss_opts_t opts;
  opts.highlight = highlight;
  opts.fpath     = file_name;
  SSViz visualizer(opts);
  visualizer.visit(search_space);
  visualizer.treeviz.write();
}

void SSViz::visit_definitions(const SSNode *ssnode) {
  std::stringstream label;
  label << "<table bgcolor=\"" << node_colors.at(ssnode->target.type) << "\"";
  if (should_highlight(ssnode, highlight)) {
    label << " border=\"4\"";
    label << " color=\"" << selected_color << "\"";
  } else {
    label << " border=\"2\"";
    label << " color=\"black\"";
  }
  label << ">";

  label << "<tr>";
  label << "<td>" << bold("EP: ") << ssnode->ep_id << "</td>";
  label << "<td>" << bold("HR: ") << (ssnode->module_data ? ssnode->module_data->hit_rate.to_string() : "None") << "</td>";
  label << "</tr>";

  label << "<tr>";
  label << "<td colspan=\"2\">" << bold("Score: ") << stringify_score(ssnode->score) << "</td>";
  label << "</tr>";

  label << "<tr>";
  label << "<td colspan=\"2\">" << bold("Module: ");
  if (ssnode->module_data) {
    label << ssnode->module_data->name;
    if (ssnode->module_data->bdd_reordered) {
      label << " [R]";
    }
    if (!ssnode->module_data->description.empty()) {
      label << " (";
      label << ssnode->module_data->description;
      label << ")";
    }
  } else {
    label << "ROOT";
  }
  label << "</td>";
  label << "</tr>";

  label << "<tr>";
  label << "<td colspan=\"2\">";
  if (ssnode->bdd_node_data) {
    label << bold("Processed: ");
    label << ssnode->bdd_node_data->description;
  }
  label << "</td>";
  label << "</tr>";

  if (ssnode->next_bdd_node_data) {
    label << "<tr>";
    label << "<td colspan=\"2\">" << bold("Next: ") << ssnode->next_bdd_node_data->description << "</td>";
    label << "</tr>";
  }

  for (const heuristic_metadata_t &meta : ssnode->metadata) {
    label << "<tr>";
    label << "<td colspan=\"2\">" << bold(meta.name + ": ") << meta.description << "</td>";
    label << "</tr>";
  }

  label << "</table>";

  Node tree_node  = treeviz.get_default_node();
  tree_node.id    = std::to_string(ssnode->node_id);
  tree_node.label = label.str();
  tree_node.color = node_colors.at(ssnode->target.type);
  treeviz.add_node(tree_node);

  for (const SSNode *next : ssnode->children) {
    visit_definitions(next);
  }
}

void SSViz::visit_links(const SSNode *ssnode) {
  for (const SSNode *child : ssnode->children) {
    treeviz.add_edge(std::to_string(ssnode->node_id), std::to_string(child->node_id));
    visit_links(child);
  }
}

} // namespace LibSynapse