#pragma once

#include <LibBDD/BDD.h>

#include <LibClone/Node.h>
#include <LibClone/Device.h>
#include <LibClone/NF.h>
#include <LibClone/Link.h>
#include <LibClone/Port.h>

#include <vector>
#include <string>
#include <memory>
#include <unordered_set>
#include <unordered_map>

namespace LibClone {

struct NodeTransition {
  u32 input_port;
  const NetworkNode *node;
  const NetworkNode *tail;

  NodeTransition(u32 _input_port, const NetworkNode *_node, const NetworkNode *_tail) : input_port(_input_port), node(_node), tail(_tail) {}
};

class Network {
private:
  const std::unordered_map<DeviceId, std::unique_ptr<Device>> devices;
  const std::unordered_map<NFId, std::unique_ptr<NF>> nfs;
  const std::vector<std::unique_ptr<Link>> links;
  const std::unordered_map<GlobalPortId, std::unique_ptr<Port>> ports;

  std::unordered_map<NetworkNodeId, NetworkNode *> nodes;
  NetworkNode *source;

public:
  Network(std::unordered_map<DeviceId, std::unique_ptr<Device>> &&_devices, std::unordered_map<NFId, std::unique_ptr<NF>> &&_nfs,
          std::vector<std::unique_ptr<Link>> &&_links, std::unordered_map<GlobalPortId, std::unique_ptr<Port>> &&_ports)
      : devices(std::move(_devices)), nfs(std::move(_nfs)), links(std::move(_links)), ports(std::move(_ports)), nodes(), source(nullptr) {}

  ~Network() = default;

  static Network parse(const std::filesystem::path &file_path, LibCore::SymbolManager *symbol_manager);

  void consolidate() { assert(false && "TODO"); }

  void debug() const {
    std::cerr << "========== Network ==========\n";
    std::cerr << "Devices:\n";
    for (const auto &[_, device] : devices) {
      std::cerr << "  ";
      device->debug();
      std::cerr << "\n";
    }
    std::cerr << "NFs:\n";
    for (const auto &[_, nf] : nfs) {
      std::cerr << "  ";
      nf->debug();
      std::cerr << "\n";
    }
    std::cerr << "Links:\n";
    for (const auto &link : links) {
      std::cerr << "  ";
      link->debug();
      std::cerr << "\n";
    }
    std::cerr << "Ports:\n";
    for (const auto &[_, port] : ports) {
      std::cerr << "  ";
      port->debug();
      std::cerr << "\n";
    }
    std::cerr << "=============================\n";
  }

private:
  void build_graph() { assert(false && "TODO"); }
  void traverse(u32 global_port, NetworkNode *origin, u32 nf_port) { assert(false && "TODO"); }
  void print_graph() const { assert(false && "TODO"); }
};

} // namespace LibClone
