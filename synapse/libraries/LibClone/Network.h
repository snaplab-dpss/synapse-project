#pragma once

#include <LibBDD/BDD.h>

#include <LibClone/Node.h>
#include <LibClone/NF.h>
#include <LibClone/Port.h>

#include <vector>
#include <string>
#include <memory>
#include <unordered_set>
#include <unordered_map>

namespace LibClone {

class Network {
private:
  std::unordered_map<NFId, std::unique_ptr<NF>> nfs;
  std::unordered_map<NetworkNodeId, std::unique_ptr<NetworkNode>> nodes;
  NetworkNode *source;

public:
  Network(std::unordered_map<NFId, std::unique_ptr<NF>> &&_nfs, std::unordered_map<NetworkNodeId, std::unique_ptr<NetworkNode>> &&_nodes)
      : nfs(std::move(_nfs)), nodes(std::move(_nodes)), source(nullptr) {
    // for (const std::unique_ptr<LibClone::Link> &link : links) {
    //   const std::string &node1_str = link->get_node1();
    //   const std::string &node2_str = link->get_node2();

    //   const NodeType node1_type = nfs.find(node1_str) != nfs.end() ? NodeType::NF : NodeType::GLOBAL_PORT;
    //   const NodeType node2_type = nfs.find(node2_str) != nfs.end() ? NodeType::NF : NodeType::GLOBAL_PORT;

    //   const unsigned port1 = link->get_port1();
    //   const unsigned port2 = link->get_port2();

    //   if (nodes.find(node1_str) == nodes.end()) {
    //     nodes.emplace(node1_str, NodePtr(new Node(node1_str, node1_type)));
    //   }

    //   if (nodes.find(node2_str) == nodes.end()) {
    //     nodes.emplace(node2_str, NodePtr(new Node(node2_str, node2_type)));
    //   }

    //   const NodePtr node1 = nodes.at(node1_str);
    //   const NodePtr node2 = nodes.at(node2_str);

    //   node1->add_child(port1, port2, node2);

    //   if (node1_type == NodeType::GLOBAL_PORT && source == nullptr) {
    //     source = node1;
    //   }
    // }

    // if (source == nullptr) {
    //   danger("Null source");
    // }
  }

  Network(const Network &)            = delete;
  Network &operator=(const Network &) = delete;

  ~Network() = default;

  static Network parse(const std::filesystem::path &file_path, LibCore::SymbolManager *symbol_manager);

  void consolidate() { assert(false && "TODO"); }

  void debug() const {
    std::cerr << "========== Network ==========\n";
    std::cerr << "NFs:\n";
    for (const auto &[_, nf] : nfs) {
      std::cerr << "  ";
      nf->debug();
      std::cerr << "\n";
    }
    std::cerr << "Nodes:\n";
    for (const auto &[_, node] : nodes) {
      std::cerr << "  ";
      node->debug();
      std::cerr << "\n";
    }
    std::cerr << "=============================\n";
  }

private:
  void traverse(u32 global_port, NetworkNode *origin, u32 nf_port) { assert(false && "TODO"); }
  void print_graph() const { assert(false && "TODO"); }
};

} // namespace LibClone
