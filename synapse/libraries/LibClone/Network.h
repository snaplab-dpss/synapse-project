#pragma once

#include <LibCore/Debug.h>
#include <LibCore/Symbol.h>
#include <LibCore/ClusterViz.h>

#include <LibBDD/BDD.h>

#include <LibClone/Node.h>
#include <LibClone/NF.h>

#include <vector>
#include <string>
#include <memory>
#include <unordered_set>
#include <unordered_map>

namespace LibClone {

using LibCore::ClusterViz;
using LibCore::SymbolManager;

class Network {
private:
  SymbolManager *symbol_manager;
  std::unordered_map<NFId, std::unique_ptr<NF>> nfs;
  std::unordered_map<NetworkNodeId, std::unique_ptr<NetworkNode>> nodes;

public:
  Network(SymbolManager *_symbol_manager, std::unordered_map<NFId, std::unique_ptr<NF>> &&_nfs,
          std::unordered_map<NetworkNodeId, std::unique_ptr<NetworkNode>> &&_nodes)
      : symbol_manager(_symbol_manager), nfs(std::move(_nfs)), nodes(std::move(_nodes)) {}

  Network(const Network &)            = delete;
  Network &operator=(const Network &) = delete;

  static Network parse(const std::filesystem::path &file_path, SymbolManager *symbol_manager);

  BDD consolidate() const;
  ClusterViz build_clusterviz() const;

  bool has_global_port(const Port port) const;

  void debug() const {
    std::cerr << "========== Network ==========\n";
    std::cerr << "NFs:\n";
    for (const auto &[_, nf] : nfs) {
      std::cerr << "  " << *nf << "\n";
    }
    std::cerr << "Nodes:\n";
    for (const auto &[_, node] : nodes) {
      std::cerr << "  " << *node << "\n";
    }
    std::cerr << "=============================\n";
  }

private:
  BDD build_new_bdd() const;
};

} // namespace LibClone
