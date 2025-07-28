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

using LibBDD::arg_t;
using LibBDD::bdd_node_id_t;
using LibBDD::BDDNode;
using LibBDD::BDDNodeManager;
using LibBDD::BDDNodeType;
using LibBDD::BDDNodeVisitAction;
using LibBDD::BDDViz;
using LibBDD::Branch;
using LibBDD::Call;
using LibBDD::call_t;
using LibBDD::Route;
using LibBDD::RouteOp;

using LibCore::Cluster;
using LibCore::Edge;
using LibCore::Node;
using LibCore::solver_toolbox;
using LibCore::symbol_t;
using LibCore::symbol_translation_t;
using LibCore::Symbols;

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

    if (!solver_toolbox.is_expr_maybe_true(bdd.get_constraints(branch_node), handles_port)) {
      return BDDNodeVisitAction::Stop;
    }

    const BDDNode *on_true  = branch_node->get_on_true();
    const BDDNode *on_false = branch_node->get_on_false();

    const bool on_true_never_handles_port  = solver_toolbox.is_expr_always_false(bdd.get_constraints(on_true), handles_port);
    const bool on_false_never_handles_port = solver_toolbox.is_expr_always_false(bdd.get_constraints(on_false), handles_port);

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
  BDDNode *new_root       = nf->get_bdd().get_root()->clone(manager, true);
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

void replace_route_with_drop(BDD &bdd, Route *route) {
  Route *drop_node = new Route(bdd.get_mutable_id(), bdd.get_mutable_symbol_manager(), RouteOp::Drop);
  bdd.get_mutable_manager().add_node(drop_node);
  bdd.get_mutable_id()++;
  replace_route_with_node(bdd, route, drop_node);
}

BDDNode *build_chain_of_device_checking_branches(BDD &bdd, klee::ref<klee::Expr> device_expr,
                                                 std::vector<std::pair<Port, BDDNode *>> &per_device_logic) {
  auto build_device_checking_condition = [device_expr](Port port) {
    return solver_toolbox.exprBuilder->Eq(device_expr, solver_toolbox.exprBuilder->Constant(port, device_expr->getWidth()));
  };

  BDDNode *root            = nullptr;
  Branch *leaf_branch_node = nullptr;
  for (const auto &[port, on_true_logic] : per_device_logic) {
    klee::ref<klee::Expr> condition = build_device_checking_condition(port);

    Branch *branch_node = new Branch(bdd.get_mutable_id(), bdd.get_mutable_symbol_manager(), condition);
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

  Route *drop_node = new Route(bdd.get_mutable_id(), bdd.get_mutable_symbol_manager(), RouteOp::Drop);
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

symbol_t create_translation_symbol(SymbolManager *symbol_manager, const symbol_t &symbol, const BDDNode *node_creating_symbol) {
  // We have to make sure this new symbol name does not collide with any existing symbol.
  // In the BDD reordering logic, we append "_r" to the symbol name to ensure this, where "r" comes from "reordering".
  // Here, we append "_c", where "c" comes from "consolidate".
  const std::string new_name = symbol.base + "_c" + std::to_string(node_creating_symbol->get_id());
  assert(!symbol_manager->has_symbol(new_name) && "Symbol should not exist in the symbol manager");
  const symbol_t new_symbol = symbol_manager->create_symbol(new_name, symbol.expr->getWidth());
  return new_symbol;
}

void translate_symbols(SymbolManager *symbol_manager, BDDNode *root) {
  const std::unordered_set<std::string> symbols_never_translated{"device", "port", "packet_chunks"};
  root->visit_mutable_nodes([symbol_manager, &symbols_never_translated](BDDNode *node) {
    if (node->get_type() != BDDNodeType::Call) {
      return BDDNodeVisitAction::Continue;
    }

    const Call *call       = dynamic_cast<Call *>(node);
    const Symbols &symbols = call->get_local_symbols();

    for (const symbol_t &symbol : symbols.get()) {
      if (symbols_never_translated.find(symbol.name) != symbols_never_translated.end()) {
        continue;
      }
      const symbol_t new_symbol = create_translation_symbol(symbol_manager, symbol, node);
      node->recursive_translate_symbol(symbol, new_symbol);
    }

    return BDDNodeVisitAction::Continue;
  });
}

void reset_data_structure_address(BDD &bdd, Call *init, BDDNode *root) {
  using func_name_t = std::string;
  using arg_name_t  = std::string;

  struct produced_consumed_args_t {
    std::string produced;
    std::unordered_map<func_name_t, arg_name_t> target_functions;
  };

  const std::unordered_map<std::string, produced_consumed_args_t> init_functions{
      {"map_allocate",
       {"map_out",
        {
            {"map_get", "map"},
            {"map_put", "map"},
            {"map_erase", "map"},
            {"map_size", "map"},
            {"expire_items_single_map", "map"},
            {"expire_items_single_map_iteratively", "map"},
        }}},
      {"vector_allocate",
       {"vector_out",
        {
            {"vector_borrow", "vector"},
            {"vector_return", "vector"},
            {"vector_clear", "vector"},
            {"vector_sample_lt", "vector"},
            {"expire_items_single_map", "vector"},
            {"expire_items_single_map_iteratively", "vector"},
        }}},
      {"dchain_allocate",
       {"chain_out",
        {
            {"dchain_allocate_new_index", "chain"},
            {"dchain_rejuvenate_index", "chain"},
            {"dchain_expire_one_index", "chain"},
            {"dchain_is_index_allocated", "chain"},
            {"dchain_free_index", "chain"},
            {"expire_items_single_map", "chain"},
        }}},
      {"cms_allocate",
       {"cms_out",
        {
            {"cms_increment", "cms"},
            {"cms_count_min", "cms"},
            {"cms_periodic_cleanup", "cms"},
        }}},
      {"lpm_allocate",
       {"lpm_out",
        {
            {"lpm_free", "lpm"},
            {"lpm_from_file", "lpm"},
            {"lpm_update", "lpm"},
            {"lpm_lookup", "lpm"},
        }}},
      {"tb_allocate",
       {"tb_out",
        {
            {"tb_is_tracing", "tb"},
            {"tb_trace", "tb"},
            {"tb_update_and_check", "tb"},
            {"tb_expire", "tb"},
        }}},
  };

  call_t init_call = init->get_call();
  auto found_it    = init_functions.find(init_call.function_name);
  if (found_it == init_functions.end()) {
    return;
  }

  const produced_consumed_args_t &produced_consumed_args = found_it->second;

  arg_t &produced = init_call.args.at(produced_consumed_args.produced);
  assert(!produced.out.isNull() && "Produced argument should not be null");
  assert(LibCore::is_constant(produced.out) && "Produced argument should be a constant");

  klee::ref<klee::Expr> old_addr_expr = produced.out;

  const u64 new_addr                  = init->get_id();
  klee::ref<klee::Expr> new_addr_expr = solver_toolbox.exprBuilder->Constant(new_addr, produced.out->getWidth());

  produced.out = new_addr_expr;
  init->set_call(init_call);

  auto addr_resetter = [&produced_consumed_args, old_addr_expr, new_addr_expr](BDDNode *node) {
    if (node->get_type() != BDDNodeType::Call) {
      return BDDNodeVisitAction::Continue;
    }

    Call *call_node = dynamic_cast<Call *>(node);
    call_t call     = call_node->get_call();

    auto found_arg_it = produced_consumed_args.target_functions.find(call.function_name);
    if (found_arg_it == produced_consumed_args.target_functions.end()) {
      return BDDNodeVisitAction::Continue;
    }

    const arg_name_t &arg_name = found_arg_it->second;
    assert_or_panic(call.args.find(arg_name) != call.args.end(), "Target function should have the argument");

    arg_t &target_arg = call.args.at(arg_name);
    assert_or_panic(LibCore::is_constant(target_arg.expr), "Target function argument should be a constant");

    if (!solver_toolbox.are_exprs_always_equal(target_arg.expr, old_addr_expr)) {
      return BDDNodeVisitAction::Continue;
    }

    target_arg.expr = new_addr_expr;
    call_node->set_call(call);

    return BDDNodeVisitAction::Continue;
  };

  root->visit_mutable_nodes(addr_resetter);
  init->visit_mutable_nodes(addr_resetter);
}

void store_cloned_and_translated_init_symbols(BDD &bdd, BDDNode *root, const std::vector<Call *> &init) {
  std::vector<Call *> new_init;
  std::vector<symbol_translation_t> translations;
  for (const Call *call : init) {
    Call *new_call = dynamic_cast<Call *>(call->clone(bdd.get_mutable_manager(), false));
    new_call->set_local_symbols(call->get_local_symbols());

    for (const symbol_t &symbol : call->get_local_symbols().get()) {
      const symbol_t new_symbol = create_translation_symbol(bdd.get_mutable_symbol_manager(), symbol, new_call);
      translations.emplace_back(symbol, new_symbol);
    }

    if (!new_init.empty()) {
      new_init.back()->set_next(new_call);
      new_call->set_prev(new_init.back());
    }

    new_init.push_back(new_call);
  }

  if (!new_init.empty()) {
    new_init.front()->recursive_update_ids(bdd.get_mutable_id());
  }

  for (Call *init_call : new_init) {
    reset_data_structure_address(bdd, init_call, root);
  }

  std::vector<Call *> current_init = bdd.get_init();
  if (!current_init.empty()) {
    current_init.back()->set_next(new_init.front());
    new_init.front()->set_prev(current_init.back());
  }

  current_init.insert(current_init.end(), new_init.begin(), new_init.end());
  bdd.set_init(current_init);

  if (!current_init.empty()) {
    for (const symbol_translation_t &translation : translations) {
      current_init.front()->recursive_translate_symbol(translation.old_symbol, translation.new_symbol);
      root->recursive_translate_symbol(translation.old_symbol, translation.new_symbol);
    }
  }
}

BDDNode *build_network_node_bdd_from_local_port(BDD &bdd, const NetworkNode *network_node, Port port) {
  const std::string ctx = network_node->get_id() + ":" + std::to_string(port);

  BDDNode *root = nullptr;

  switch (network_node->get_node_type()) {
  case NetworkNodeType::GLOBAL_PORT: {
    Route *route_node =
        new Route(bdd.get_mutable_id(), bdd.get_mutable_symbol_manager(), RouteOp::Forward, solver_toolbox.exprBuilder->Constant(port, 32));
    bdd.get_mutable_manager().add_node(route_node);
    bdd.get_mutable_id()++;
    root = route_node;
    return root;
  } break;
  case NetworkNodeType::NF: {
    const NF *nf                             = network_node->get_nf();
    const std::vector<bdd_vector_t> sections = get_bdd_sections_handling_port(nf->get_bdd(), port);

    root = stitch_bdd_sections(bdd, nf, sections);
    store_cloned_and_translated_init_symbols(bdd, root, nf->get_bdd().get_init());
    translate_symbols(bdd.get_mutable_symbol_manager(), root);
  } break;
  }

  assert(root && "Root node should not be null");
  for (BDDNode *leaf_node : root->get_mutable_leaves()) {
    Route *route = leaf_node->get_mutable_latest_routing_decision();
    assert(route && "Leaf node should have a routing decision");

    switch (route->get_operation()) {
    case RouteOp::Forward: {
      klee::ref<klee::Expr> dst_device_expr = route->get_dst_device();
      if (LibCore::is_constant(dst_device_expr)) {
        const Port dst_device = solver_toolbox.value_from_expr(dst_device_expr);
        if (network_node->has_link(dst_device)) {
          const std::pair<Port, const NetworkNode *> link = network_node->get_link(dst_device);
          const Port dst_network_node_port                = link.first;
          const NetworkNode *dst_network_node             = link.second;
          std::cerr << "  [" << ctx << "] Forwards to " << dst_device << " (" << dst_network_node->get_id() << ":" << dst_network_node_port << ")\n";
          BDDNode *new_node = build_network_node_bdd_from_local_port(bdd, dst_network_node, dst_network_node_port);
          replace_route_with_node(bdd, route, new_node);
        } else {
          std::cerr << "  [" << ctx << "] Forwards to local port " << dst_device << " without virtual link (dropping)\n";
          replace_route_with_drop(bdd, route);
        }
      } else {
        std::vector<std::pair<Port, BDDNode *>> per_device_logic;
        for (const auto &[local_port, destination] : network_node->get_links()) {
          klee::ConstraintManager leaf_constraints = bdd.get_constraints(leaf_node);
          leaf_constraints.addConstraint(
              solver_toolbox.exprBuilder->Eq(bdd.get_device().expr, solver_toolbox.exprBuilder->Constant(port, bdd.get_device().expr->getWidth())));

          const Port dst_network_node_port      = destination.first;
          const NetworkNode *dst_network_node   = destination.second;
          klee::ref<klee::Expr> local_port_expr = solver_toolbox.exprBuilder->Constant(local_port, dst_device_expr->getWidth());
          klee::ref<klee::Expr> handles_port    = solver_toolbox.exprBuilder->Eq(dst_device_expr, local_port_expr);
          if (solver_toolbox.is_expr_maybe_true(leaf_constraints, handles_port)) {
            std::cerr << "  [" << ctx << "] May forward to " << local_port << " (" << dst_network_node->get_id() << ":" << dst_network_node_port
                      << ")\n";
            BDDNode *new_node = build_network_node_bdd_from_local_port(bdd, dst_network_node, dst_network_node_port);
            per_device_logic.emplace_back(dst_network_node_port, new_node);
          }
        }
        BDDNode *if_else_logic = build_chain_of_device_checking_branches(bdd, dst_device_expr, per_device_logic);
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
      std::cerr << "Global port " << global_port << " -> " << dport_node.second->get_id() << ":" << dport_node.first << "\n";
      BDDNode *next_node = build_network_node_bdd_from_local_port(bdd, dport_node.second, dport_node.first);
      per_device_logic.emplace_back(global_port, next_node);
    }
  }

  BDDNode *root = build_chain_of_device_checking_branches(bdd, bdd.get_device().expr, per_device_logic);
  bdd.set_root(root);

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