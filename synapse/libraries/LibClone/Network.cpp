#include <LibClone/Network.h>

#include <LibCore/Debug.h>
#include <LibCore/Solver.h>

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <filesystem>

namespace LibClone {

using LibBDD::bdd_node_id_t;
using LibBDD::BDDNode;
using LibBDD::BDDNodeManager;
using LibBDD::BDDNodeType;
using LibBDD::BDDNodeVisitAction;
using LibBDD::BDDViz;
using LibBDD::Branch;
using LibBDD::Call;
using LibBDD::Route;
using LibBDD::RouteOp;

using LibCore::Cluster;
using LibCore::Edge;
using LibCore::Node;
using LibCore::solver_toolbox;
using LibCore::symbol_t;

namespace {
constexpr char TOKEN_CMD_NF[]   = "nf";
constexpr char TOKEN_CMD_LINK[] = "link";
constexpr char TOKEN_PORT[]     = "global_port";
constexpr char TOKEN_COMMENT[]  = "//";

constexpr size_t LENGTH_PORT_INPUT = 2;
constexpr size_t LENGTH_NF_INPUT   = 3;
constexpr size_t LENGTH_LINK_INPUT = 5;

std::ifstream open_file(const std::string &path) {
  std::ifstream fstream;
  fstream.open(path);

  if (!fstream.is_open()) {
    panic("Could not open input file %s", path.c_str());
  }

  return fstream;
}

std::unique_ptr<NF> parse_nf(const std::vector<std::string> &words, const std::filesystem::path &network_file, SymbolManager *symbol_manager) {
  if (words.size() != LENGTH_NF_INPUT) {
    panic("Invalid network function");
  }

  const std::string &id      = words[1];
  std::filesystem::path path = words[2];

  // Relative to the configuration file, not the current working directory.
  if (path.is_relative()) {
    path = network_file.parent_path() / path;
  }

  return std::make_unique<NF>(id, path, symbol_manager);
}

void parse_link(const std::vector<std::string> &words, const std::unordered_map<NFId, std::unique_ptr<NF>> &nfs,
                std::unordered_map<NetworkNodeId, std::unique_ptr<NetworkNode>> &nodes) {
  if (words.size() != LENGTH_LINK_INPUT) {
    panic("Invalid link");
  }

  const NetworkNodeId node1_id = words[1];
  const Port sport             = std::stoul(words[2]);
  const NetworkNodeId node2_id = words[3];
  const Port dport             = std::stoul(words[4]);

  const bool node1_is_nf = nfs.find(node1_id) != nfs.end();
  if (!node1_is_nf && node1_id != TOKEN_PORT) {
    panic("Could not find node %s", node1_id.c_str());
  }

  if (nodes.find(node1_id) == nodes.end()) {
    if (node1_is_nf) {
      nodes[node1_id] = std::make_unique<NetworkNode>(node1_id, nfs.at(node1_id).get());
    } else {
      nodes[node1_id] = std::make_unique<NetworkNode>(node1_id);
    }
  }

  const bool node2_is_nf = nfs.find(node2_id) != nfs.end();
  if (!node2_is_nf && node2_id != TOKEN_PORT) {
    panic("Could not find node %s", node2_id.c_str());
  }

  if (nodes.find(node2_id) == nodes.end()) {
    if (node2_is_nf) {
      nodes[node2_id] = std::make_unique<NetworkNode>(node2_id, nfs.at(node2_id).get());
    } else {
      nodes[node2_id] = std::make_unique<NetworkNode>(node2_id);
    }
  }

  nodes.at(node1_id)->add_link(sport, dport, nodes.at(node2_id).get());
}

struct bdd_vector_t {
  enum class bdd_vector_direction_t { OnTrue, OnFalse };
  const Branch *node;
  bdd_vector_direction_t direction;

  bdd_vector_t(const Branch *_node, bdd_vector_direction_t _direction) : node(_node), direction(_direction) {
    assert(node);
    switch (direction) {
    case bdd_vector_direction_t::OnTrue: {
      assert(node->get_on_true() && "OnTrue direction requires a true branch");
    } break;
    case bdd_vector_direction_t::OnFalse: {
      assert(node->get_on_false() && "OnFalse direction requires a false branch");
    } break;
    }
  }

  const BDDNode *get_target() const {
    switch (direction) {
    case bdd_vector_direction_t::OnTrue: {
      return node->get_on_true();
    }
    case bdd_vector_direction_t::OnFalse: {
      return node->get_on_false();
    }
    }
    return nullptr;
  }

  const BDDNode *get_non_target() const {
    switch (direction) {
    case bdd_vector_direction_t::OnTrue: {
      return node->get_on_false();
    }
    case bdd_vector_direction_t::OnFalse: {
      return node->get_on_true();
    }
    }
    return nullptr;
  }
};

std::vector<bdd_vector_t> get_bdd_sections_handling_port(const BDD &bdd, Port port) {
  klee::ref<klee::Expr> handles_port =
      solver_toolbox.exprBuilder->Eq(bdd.get_device().expr, solver_toolbox.exprBuilder->Constant(port, bdd.get_device().expr->getWidth()));

  std::vector<bdd_vector_t> sections;
  bdd.get_root()->visit_nodes([&](const BDDNode *node) {
    if (node->get_type() != BDDNodeType::Branch) {
      return BDDNodeVisitAction::Continue;
    }

    const Branch *branch_node = dynamic_cast<const Branch *>(node);

    if (!solver_toolbox.is_expr_maybe_true(branch_node->get_constraints(), handles_port)) {
      return BDDNodeVisitAction::Stop;
    }

    const BDDNode *on_true  = branch_node->get_on_true();
    const BDDNode *on_false = branch_node->get_on_false();

    const bool on_true_never_handles_port  = solver_toolbox.is_expr_always_false(on_true->get_constraints(), handles_port);
    const bool on_false_never_handles_port = solver_toolbox.is_expr_always_false(on_false->get_constraints(), handles_port);

    assert(!on_true_never_handles_port || !on_false_never_handles_port);

    if (on_true_never_handles_port) {
      sections.emplace_back(branch_node, bdd_vector_t::bdd_vector_direction_t::OnFalse);
    } else if (on_false_never_handles_port) {
      sections.emplace_back(branch_node, bdd_vector_t::bdd_vector_direction_t::OnTrue);
    }

    return BDDNodeVisitAction::Continue;
  });

  return sections;
}

void disconnect_parent(BDDNode *node) {
  assert(node);
  assert(node->get_prev());

  BDDNode *parent      = node->get_mutable_prev();
  BDDNode *grandparent = parent->get_mutable_prev();

  node->set_prev(nullptr);

  if (!grandparent) {
    if (node->get_type() == BDDNodeType::Branch) {
      Branch *parent_branch = dynamic_cast<Branch *>(parent);
      if (parent_branch->get_on_true() == node) {
        parent_branch->set_on_true(nullptr);
      } else {
        assert(parent_branch->get_on_false() == node && "Node is not a child of the parent branch");
        parent_branch->set_on_false(nullptr);
      }
    } else {
      parent->set_next(nullptr);
    }
    return;
  }

  if (grandparent->get_type() != BDDNodeType::Branch) {
    grandparent->set_next(node);
  } else {
    Branch *grandparent_branch = dynamic_cast<Branch *>(grandparent);
    if (grandparent_branch->get_on_true() == parent) {
      grandparent_branch->set_on_true(node);
    } else {
      assert(grandparent_branch->get_on_false() == parent && "Parent is not a child of the grandparent branch");
      grandparent_branch->set_on_false(node);
    }
  }

  node->set_prev(grandparent);
}

BDDNode *stitch_bdd_sections(BDD &bdd, const NF *nf, const std::vector<bdd_vector_t> &sections) {
  BDDNodeManager &manager = bdd.get_mutable_manager();

  BDDNode *new_root = nf->get_bdd().get_root()->clone(manager, true);

  for (const bdd_vector_t vector : sections) {
    BDDNode *discriminating_node = new_root->get_mutable_node_by_id(vector.node->get_id());
    BDDNode *target_node         = new_root->get_mutable_node_by_id(vector.get_target()->get_id());
    BDDNode *non_target_node     = new_root->get_mutable_node_by_id(vector.get_non_target()->get_id());

    disconnect_parent(target_node);

    manager.free_node(non_target_node, true);
    manager.free_node(discriminating_node, false);
  }

  new_root->recursive_update_ids(bdd.get_mutable_id());
  return new_root;
}

BDDNode *build_bdd_from_network_node(BDD &bdd, const NetworkNode *node, Port port) {
  BDDNode *root = nullptr;

  switch (node->get_node_type()) {
  case NetworkNodeType::GLOBAL_PORT: {
    Route *route_node =
        new Route(bdd.get_mutable_id(), {}, bdd.get_mutable_symbol_manager(), RouteOp::Forward, solver_toolbox.exprBuilder->Constant(port, 32));
    bdd.get_mutable_manager().add_node(route_node);
    bdd.get_mutable_id()++;
    root = route_node;
  } break;
  case NetworkNodeType::NF: {
    const NF *nf = node->get_nf();

    const std::vector<bdd_vector_t> sections = get_bdd_sections_handling_port(nf->get_bdd(), port);
    for (const bdd_vector_t &bdd_vector : sections) {
      std::cerr << "Found discriminating vector: " << bdd_vector.node->dump(true) << " with direction: ";
      switch (bdd_vector.direction) {
      case bdd_vector_t::bdd_vector_direction_t::OnTrue: {
        std::cerr << "OnTrue\n";
      } break;
      case bdd_vector_t::bdd_vector_direction_t::OnFalse: {
        std::cerr << "OnFalse\n";
      } break;
      }
    }

    root = stitch_bdd_sections(bdd, nf, sections);

  } break;
  }

  return root;
}

} // namespace

Network Network::parse(const std::filesystem::path &network_file, SymbolManager *symbol_manager) {
  std::ifstream fstream = open_file(network_file);

  std::unordered_map<NFId, std::unique_ptr<NF>> nfs;
  std::unordered_map<NetworkNodeId, std::unique_ptr<NetworkNode>> nodes;

  std::string line;
  while (getline(fstream, line)) {
    std::stringstream ss(line);
    std::vector<std::string> words;
    std::string word;

    while (ss >> word) {
      words.push_back(word);
    }

    if (words.size() == 0) {
      continue;
    }

    const std::string type = words[0];

    if (type == TOKEN_CMD_NF) {
      std::unique_ptr<NF> nf = parse_nf(words, network_file, symbol_manager);
      const NFId nf_id       = nf->get_id();
      nfs[nf->get_id()]      = std::move(nf);
    } else if (type == TOKEN_CMD_LINK) {
      parse_link(words, nfs, nodes);
    } else if (type == TOKEN_COMMENT) {
      // Ignore comments
      continue;
    } else {
      panic("Invalid line \"%s\"", line.c_str());
    }
  }

  return Network(symbol_manager, std::move(nfs), std::move(nodes));
}

BDD Network::consolidate() const {
  BDD bdd(symbol_manager);

  bdd_node_id_t &bdd_node_id = bdd.get_mutable_id();
  Branch *leaf_branch_node   = nullptr;

  for (const auto &[node_id, node] : nodes) {
    if (node->get_node_type() != NetworkNodeType::GLOBAL_PORT) {
      continue;
    }

    for (const auto &[global_port, dport_node] : node->get_links()) {
      std::cerr << "Setting up global port " << node_id << " with sport " << global_port << " and destination " << dport_node.second->get_id()
                << " with dport " << dport_node.first << "\n";

      const symbol_t device = symbol_manager->create_symbol("DEVICE", 32);
      klee::ref<klee::Expr> global_port_check_expr =
          solver_toolbox.exprBuilder->Eq(device.expr, solver_toolbox.exprBuilder->Constant(global_port, device.expr->getWidth()));

      Branch *branch_node = new Branch(bdd_node_id, {}, symbol_manager, global_port_check_expr);
      bdd.get_mutable_manager().add_node(branch_node);
      bdd_node_id++;

      BDDNode *next_node = build_bdd_from_network_node(bdd, dport_node.second, dport_node.first);
      assert(next_node && "Next node should not be null");

      branch_node->set_on_true(next_node);
      next_node->set_prev(branch_node);

      if (leaf_branch_node == nullptr) {
        leaf_branch_node = branch_node;
        bdd.set_root(branch_node);
      } else {
        leaf_branch_node->set_on_false(branch_node);
        branch_node->set_prev(leaf_branch_node);
      }

      leaf_branch_node = branch_node;

      BDDViz::visualize(&bdd, true);
    }
  }

  Route *final_drop_node = new Route(bdd_node_id, {}, symbol_manager, RouteOp::Drop);
  bdd.get_mutable_manager().add_node(final_drop_node);
  bdd_node_id++;

  assert(leaf_branch_node && "Leaf branch node should not be null");
  leaf_branch_node->set_on_false(final_drop_node);
  final_drop_node->set_prev(leaf_branch_node);

  return bdd;
}

ClusterViz Network::build_clusterviz() const {
  ClusterViz clusterviz;

  Cluster global_ports("global_ports", "Global Ports");

  std::unordered_map<NFId, Cluster> nf_clusters;
  for (const auto &[nf_id, nf] : nfs) {
    Cluster nf_cluster(nf_id, nf_id);
    nf_clusters.insert({nf_id, nf_cluster});
  }

  auto build_network_node_id = [](const NetworkNodeId &id, Port port) { return id + "_" + std::to_string(port); };

  for (const auto &[_, node] : nodes) {
    for (const auto &[port, destination] : node->get_links()) {
      Node source_cluster_node(build_network_node_id(node->get_id(), port), std::to_string(port));
      switch (node->get_node_type()) {
      case NetworkNodeType::GLOBAL_PORT: {
        global_ports.nodes.insert(source_cluster_node);
      } break;
      case NetworkNodeType::NF: {
        nf_clusters.at(node->get_nf()->get_id()).nodes.insert(source_cluster_node);
      } break;
      }

      const Port destination_port         = destination.first;
      const NetworkNode *destination_node = destination.second;

      Node destination_cluster_node(build_network_node_id(destination_node->get_id(), destination_port), std::to_string(destination_port));
      switch (destination_node->get_node_type()) {
      case NetworkNodeType::GLOBAL_PORT: {
        global_ports.nodes.insert(destination_cluster_node);
      } break;
      case NetworkNodeType::NF: {
        nf_clusters.at(destination_node->get_nf()->get_id()).nodes.insert(destination_cluster_node);
      } break;
      }

      Edge edge(source_cluster_node.id, destination_cluster_node.id);
      clusterviz.add_edge(edge);
    }
  }

  clusterviz.add_cluster(global_ports);
  for (const auto &[_, cluster] : nf_clusters) {
    clusterviz.add_cluster(cluster);
  }

  return clusterviz;
}

} // namespace LibClone