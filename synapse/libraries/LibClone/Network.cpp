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
  using bdd_vector_direction_t = bool;
  const Branch *node;
  bdd_vector_direction_t direction;

  bdd_vector_t(const Branch *_node, bdd_vector_direction_t _direction) : node(_node), direction(_direction) {
    assert(node);
    if (direction) {
      assert(node->get_on_true() && "OnTrue direction requires a true branch");
    } else {
      assert(node->get_on_false() && "OnFalse direction requires a false branch");
    }
  }

  const BDDNode *get_target() const {
    if (direction) {
      return node->get_on_true();
    } else {
      return node->get_on_false();
    }
    return nullptr;
  }

  const BDDNode *get_non_target() const {
    if (direction) {
      return node->get_on_false();
    } else {
      return node->get_on_true();
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
      sections.emplace_back(branch_node, false);
    } else if (on_false_never_handles_port) {
      sections.emplace_back(branch_node, true);
    }

    return BDDNodeVisitAction::Continue;
  });

  return sections;
}

BDDNode *stitch_bdd_sections(BDD &bdd, const NF *nf, const std::vector<bdd_vector_t> &sections) {
  BDDNodeManager &manager = bdd.get_mutable_manager();

  BDDNode *new_root = nf->get_bdd().get_root()->clone(manager, true);

  for (const bdd_vector_t vector : sections) {
    BDD::delete_branch(new_root->get_mutable_node_by_id(vector.node->get_id()), vector.direction, manager);
  }

  new_root->recursive_update_ids(bdd.get_mutable_id());
  return new_root;
}

void replace_route_with_node(BDD &bdd, Route *route, BDDNode *node) {
  const std::vector<BDDNode *> leaves = route->get_mutable_leaves();

  std::vector<BDDNode *> new_leaves{node};
  while (new_leaves.size() != leaves.size()) {
    new_leaves.push_back(node->clone(bdd.get_mutable_manager(), true));
  }

  for (BDDNode *leaf : leaves) {
    leaf->set_next(node);
    node->set_prev(leaf);
  }

  BDD::delete_non_branch(route, bdd.get_mutable_manager());
}

void replace_route_with_drop(BDD &bdd, Route *route, klee::ConstraintManager constraints) {
  Route *drop_node = new Route(bdd.get_mutable_id(), constraints, bdd.get_mutable_symbol_manager(), RouteOp::Drop);
  bdd.get_mutable_manager().add_node(drop_node);
  bdd.get_mutable_id()++;
  replace_route_with_node(bdd, route, drop_node);
}

BDDNode *build_chain_of_device_checking_branches(BDD &bdd, const klee::ConstraintManager &constraints, klee::ref<klee::Expr> device_expr,
                                                 std::vector<std::pair<Port, BDDNode *>> &per_device_logic) {
  auto build_device_checking_condition = [device_expr](Port port) {
    return solver_toolbox.exprBuilder->Eq(device_expr, solver_toolbox.exprBuilder->Constant(port, device_expr->getWidth()));
  };

  BDDNode *root            = nullptr;
  Branch *leaf_branch_node = nullptr;
  for (const auto &[port, on_true_logic] : per_device_logic) {
    klee::ref<klee::Expr> condition = build_device_checking_condition(port);

    Branch *branch_node = new Branch(bdd.get_mutable_id(), constraints, bdd.get_mutable_symbol_manager(), condition);
    bdd.get_mutable_manager().add_node(branch_node);
    bdd.get_mutable_id()++;

    if (leaf_branch_node) {
      leaf_branch_node->set_on_false(branch_node);
      branch_node->set_prev(leaf_branch_node);
    } else {
      root = branch_node;
    }

    branch_node->set_on_true(on_true_logic);
    on_true_logic->set_prev(branch_node);

    leaf_branch_node = branch_node;
  }

  Route *drop_node = new Route(bdd.get_mutable_id(), constraints, bdd.get_mutable_symbol_manager(), RouteOp::Drop);
  bdd.get_mutable_manager().add_node(drop_node);
  bdd.get_mutable_id()++;

  if (leaf_branch_node) {
    leaf_branch_node->set_on_false(drop_node);
    drop_node->set_prev(leaf_branch_node);
  } else {
    root = drop_node;
  }

  return root;
}

BDDNode *build_network_node_bdd_from_local_port(BDD &bdd, const NetworkNode *network_node, Port port) {
  BDDNode *root = nullptr;

  switch (network_node->get_node_type()) {
  case NetworkNodeType::GLOBAL_PORT: {
    Route *route_node =
        new Route(bdd.get_mutable_id(), {}, bdd.get_mutable_symbol_manager(), RouteOp::Forward, solver_toolbox.exprBuilder->Constant(port, 32));
    bdd.get_mutable_manager().add_node(route_node);
    bdd.get_mutable_id()++;
    root = route_node;
    // std::cerr << "Building BDD for global port " << network_node->get_id() << " with port " << port << "\n";
    return root;
  } break;
  case NetworkNodeType::NF: {
    const NF *nf                             = network_node->get_nf();
    const std::vector<bdd_vector_t> sections = get_bdd_sections_handling_port(nf->get_bdd(), port);
    // for (const bdd_vector_t &bdd_vector : sections) {
    //   std::cerr << "Found discriminating vector: " << bdd_vector.node->dump(true) << " with direction: ";
    //   if (bdd_vector.direction) {
    //     std::cerr << "True\n";
    //   } else {
    //     std::cerr << "False\n";
    //   }
    // }
    root = stitch_bdd_sections(bdd, nf, sections);
  } break;
  }

  assert(root && "Root node should not be null");
  for (BDDNode *leaf_node : root->get_mutable_leaves()) {
    const klee::ConstraintManager leaf_constraints = leaf_node->get_constraints();
    Route *route                                   = leaf_node->get_mutable_latest_routing_decision();
    assert(route && "Leaf node should have a routing decision");

    switch (route->get_operation()) {
    case RouteOp::Forward: {
      klee::ref<klee::Expr> dst_device_expr = route->get_dst_device();
      // std::cerr << "dst_device: " << LibCore::expr_to_string(dst_device_expr) << "\n";

      if (LibCore::is_constant(dst_device_expr)) {
        const Port dst_device = solver_toolbox.value_from_expr(dst_device_expr);
        if (network_node->has_link(dst_device)) {
          const std::pair<Port, const NetworkNode *> link = network_node->get_link(dst_device);
          const Port dst_network_node_port                = link.first;
          const NetworkNode *dst_network_node             = link.second;
          // std::cerr << "  * Direct routing to port " << dst_network_node_port << " of node " << dst_network_node->get_id() << "!\n";
          BDDNode *new_node = build_network_node_bdd_from_local_port(bdd, dst_network_node, dst_network_node_port);
          replace_route_with_node(bdd, route, new_node);
          // std::cerr << " [!] Forwarding logic of node " << network_node->get_id() << " to port " << dst_device
          //           << " has a link on the virtual network, forwarding to node " << dst_network_node->get_id() << " with port "
          //           << dst_network_node_port << "\n";
        } else {
          // std::cout << "WARNING: forwarding logic of node " << network_node->get_id() << " to port " << dst_device
          //           << " does not have a link on the virtual network, dropping the packet.\n";
          replace_route_with_drop(bdd, route, leaf_constraints);
        }
      } else {
        std::vector<std::pair<Port, BDDNode *>> per_device_logic;
        for (const auto &[_, destination] : network_node->get_links()) {
          const Port dst_network_node_port    = destination.first;
          const NetworkNode *dst_network_node = destination.second;
          klee::ref<klee::Expr> dst_port      = solver_toolbox.exprBuilder->Constant(dst_network_node_port, dst_device_expr->getWidth());
          // if (solver_toolbox.is_expr_maybe_true(leaf_constraints, solver_toolbox.exprBuilder->Eq(dst_port, dst_device_expr))) {
          //   std::cerr << "  * Could be sending to port " << dst_network_node_port << " of node " << dst_network_node->get_id() << "!\n";
          // }
          BDDNode *new_node = build_network_node_bdd_from_local_port(bdd, dst_network_node, dst_network_node_port);
          per_device_logic.emplace_back(dst_network_node_port, new_node);
        }
        BDDNode *if_else_logic = build_chain_of_device_checking_branches(bdd, leaf_constraints, dst_device_expr, per_device_logic);
        replace_route_with_node(bdd, route, if_else_logic);
      }
    } break;
    case RouteOp::Drop: {
      // Nothing to do here.
    } break;
    case RouteOp::Broadcast: {
      // TODO: Implement broadcast logic.
      // It should implement all logic triggered from all links on this network node, in sequence.
      todo();
    } break;
    }
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

BDD Network::build_new_bdd() const {
  BDD bdd(symbol_manager);

  for (const auto &[node_id, node] : nodes) {
    if (node->get_node_type() != NetworkNodeType::NF) {
      continue;
    }
    const NF *nf      = node->get_nf();
    const BDD &nf_bdd = nf->get_bdd();

    bdd.set_device(nf_bdd.get_device());
    bdd.set_packet_len(nf_bdd.get_packet_len());
    bdd.set_time(nf_bdd.get_time());
    return bdd;
  }

  panic("No NF nodes found in the network, cannot build BDD");
}

BDD Network::consolidate() const {
  BDD bdd = build_new_bdd();

  std::vector<std::pair<Port, BDDNode *>> per_device_logic;
  for (const auto &[node_id, node] : nodes) {
    if (node->get_node_type() != NetworkNodeType::GLOBAL_PORT) {
      continue;
    }

    for (const auto &[global_port, dport_node] : node->get_links()) {
      // std::cerr << "Setting up global port " << node_id << " with sport " << global_port << " and destination " << dport_node.second->get_id()
      //           << " with dport " << dport_node.first << "\n";
      BDDNode *next_node = build_network_node_bdd_from_local_port(bdd, dport_node.second, dport_node.first);
      per_device_logic.emplace_back(global_port, next_node);
    }
  }

  BDDNode *root = build_chain_of_device_checking_branches(bdd, {}, bdd.get_device().expr, per_device_logic);
  bdd.set_root(root);

  // BDDViz::visualize(&bdd, false);

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

bool Network::has_global_port(const Port port) const {
  for (const auto &[_, node] : nodes) {
    if (node->get_node_type() == NetworkNodeType::GLOBAL_PORT) {
      if (node->has_link(port) || node->connects_to_global_port(port)) {
        return true;
      }
    }
  }
  return false;
}

} // namespace LibClone