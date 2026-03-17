#include <LibClone/PhysicalNetwork.h>

#include <LibCore/Debug.h>

#include <queue>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace LibClone {

namespace {
constexpr char TOKEN_CMD_LINK[] = "link";
constexpr char TOKEN_PORT[]     = "global_port";
constexpr char TOKEN_COMMENT[]  = "//";
constexpr char TOKEN_DEVICE[]   = "device";

constexpr size_t LENGTH_DEVICE_INPUT = 3;
constexpr size_t LENGTH_PORT_INPUT   = 2;
constexpr size_t LENGTH_LINK_INPUT   = 5;

std::ifstream open_file(const std::string &path) {
  std::ifstream fstream;
  fstream.open(path);

  if (!fstream.is_open()) {
    panic("Could not open input file %s", path.c_str());
  }

  return fstream;
}

std::unique_ptr<Device> parse_device(const std::vector<std::string> &words) {
  assert_or_panic(words.size() == LENGTH_DEVICE_INPUT, "Invalid device");

  const DeviceId id = std::stoi(words[1]);
  assert(id >= 0 && "ERROR: Unallowed DeviceId");
  const std::string &arch = words[2];

  return std::make_unique<Device>(id, arch);
}

void parse_link(const std::vector<std::string> &words, const std::unordered_map<DeviceId, std::unique_ptr<Device>> &devices,
                std::unordered_map<InfrastructureNodeId, std::unique_ptr<InfrastructureNode>> &nodes) {
  assert_or_panic(words.size() == LENGTH_LINK_INPUT, "Invalid link");

  const InfrastructureNodeId node1_id = words[1] == TOKEN_PORT ? InfrastructureNodeId(-1) : std::stoi(words[1]);
  const Port sport                    = std::stoul(words[2]);

  const InfrastructureNodeId node2_id = words[3] == TOKEN_PORT ? InfrastructureNodeId(-1) : std::stoi(words[3]);
  const Port dport                    = std::stoul(words[4]);

  const bool node1_is_device = devices.find(node1_id) != devices.end();
  if (!node1_is_device && node1_id != -1) {
    panic("Could not find node %ld", node1_id);
  }

  if (nodes.find(node1_id) == nodes.end()) {
    if (node1_is_device) {
      nodes[node1_id] = std::make_unique<InfrastructureNode>(node1_id, devices.at(node1_id).get());
    } else {
      nodes[node1_id] = std::make_unique<InfrastructureNode>(node1_id);
    }
  }

  const bool node2_is_device = devices.find(node2_id) != devices.end();
  if (!node2_is_device && node2_id != -1) {
    panic("Could not find node %ld", node2_id);
  }

  if (nodes.find(node2_id) == nodes.end()) {
    if (node2_is_device) {
      nodes[node2_id] = std::make_unique<InfrastructureNode>(node2_id, devices.at(node2_id).get());
    } else {
      nodes[node2_id] = std::make_unique<InfrastructureNode>(node2_id);
    }
  }

  nodes.at(node1_id)->add_link(sport, dport, nodes.at(node2_id).get());
}

std::unordered_map<InfrastructureNodeId, LinkInfo>
dijkstra(const std::unordered_map<InfrastructureNodeId, std::vector<std::pair<Port, InfrastructureNodeId>>> &links, const InfrastructureNodeId &src) {
  std::unordered_map<InfrastructureNodeId, u64> dist;
  std::unordered_map<InfrastructureNodeId, Port> first_hop_port;
  std::unordered_set<InfrastructureNodeId> visited;

  using HeapElem = std::pair<u64, InfrastructureNodeId>;
  std::priority_queue<HeapElem, std::vector<HeapElem>, std::greater<HeapElem>> heap;

  dist[src] = 0;
  heap.push({0, src});

  while (!heap.empty()) {
    auto [cur_dist, u] = heap.top();
    heap.pop();
    if (visited.count(u))
      continue;
    visited.insert(u);

    auto it = links.find(u);
    if (it == links.end())
      continue;

    for (const auto &[out_port, v] : it->second) {
      u64 edge_cost = 1; // Default cost, could be from link metadata
      u64 new_dist  = cur_dist + edge_cost;
      if (!dist.count(v) || new_dist < dist[v]) {
        dist[v] = new_dist;
        if (u == src) {
          first_hop_port[v] = out_port;
        } else {
          first_hop_port[v] = first_hop_port[u];
        }
        if (v != -1)
          heap.push({new_dist, v});
      }
    }
  }

  std::unordered_map<InfrastructureNodeId, LinkInfo> result;
  for (const auto &[dst, cost] : dist) {
    if (dst != src && first_hop_port.count(dst)) {
      result[dst] = {first_hop_port[dst], static_cast<u16>(cost)};
    }
  }
  return result;
}

RoutingTable build_routing_table(const std::unordered_map<InfrastructureNodeId, std::unique_ptr<InfrastructureNode>> &nodes) {
  RoutingTable routing_table;

  std::unordered_map<InfrastructureNodeId, std::vector<std::pair<Port, InfrastructureNodeId>>> adjacency;
  for (const auto &[node_id, node] : nodes) {
    for (const auto &[port, link] : node->get_links()) {
      adjacency[node_id].emplace_back(port, link.second->get_id());
    }
  }

  for (const auto &[node_id, _] : nodes) {
    routing_table[node_id] = dijkstra(adjacency, node_id);
  }

  return routing_table;
}

} // namespace

bool PhysicalNetwork::has_device(const DeviceId device_id) const { return devices.find(device_id) != devices.end(); }

const Device *PhysicalNetwork::get_device(const DeviceId device_id) const {
  assert_or_panic(devices.find(device_id) != devices.end(), "Device ID %ld not found in devices!", device_id);
  return devices.at(device_id).get();
}

const InfrastructureNode *PhysicalNetwork::get_node(const InfrastructureNodeId node_id) const {
  assert_or_panic(nodes.find(node_id) != nodes.end(), "Node ID %ld not found in nodes!", node_id);
  return nodes.at(node_id).get();
}

Port PhysicalNetwork::get_forwarding_port(const InfrastructureNodeId src, const InfrastructureNodeId dst) const {
  assert_or_panic(routing_table.find(src) != routing_table.end(), "Source Node ID %ld not found in forwarding table!", src);
  assert_or_panic(routing_table.at(src).find(dst) != routing_table.at(src).end(), "Destination Node ID %ld not reachable from Source Node ID %ld!",
                  dst, src);
  return routing_table.at(src).at(dst).port;
}

const std::vector<std::pair<Port, DeviceId>> PhysicalNetwork::find_path(const InfrastructureNodeId src_id, const InfrastructureNodeId dst_id) const {
  std::vector<std::pair<Port, DeviceId>> path;

  InfrastructureNodeId current_node_id = src_id;

  while (current_node_id != dst_id) {
    Port out_port                          = get_forwarding_port(current_node_id, dst_id);
    const InfrastructureNode *current_node = get_node(current_node_id);

    // const Port next_port                = current_node->get_connected_node(out_port).first;
    const InfrastructureNode *next_node = current_node->get_connected_node(out_port).second;

    assert_or_panic(next_node->get_node_type() != InfrastructureNodeType::GLOBAL_PORT, "Path goes through global port, which is not allowed!");

    const Device *next_device = next_node->get_device();
    path.emplace_back(out_port, next_device->get_id());

    current_node_id = next_node->get_id();
  }

  return path;
}

const std::set<Port> PhysicalNetwork::get_target_ports(const InfrastructureNodeId node_id) const {
  const InfrastructureNode *node = get_node(node_id);
  return node->get_ports();
}

CostMatrix PhysicalNetwork::get_cost_matrix() const {
  CostMatrix cost_matrix;

  for (const auto &[src_id, links] : routing_table) {
    cost_matrix[src_id][src_id] = 0;
    for (const auto &[dst_id, link_info] : links) {
      cost_matrix[src_id][dst_id] = link_info.cost;
    }
  }

  return cost_matrix;
}

const PhysicalNetwork PhysicalNetwork::parse(const std::filesystem::path &network_file) {
  std::ifstream fstream = open_file(network_file);

  std::unordered_map<DeviceId, std::unique_ptr<Device>> devices;
  std::unordered_map<InfrastructureNodeId, std::unique_ptr<InfrastructureNode>> nodes;

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

    if (type == TOKEN_DEVICE) {
      std::unique_ptr<Device> device = parse_device(words);
      assert(devices.find(device->get_id()) == devices.end());
      devices[device->get_id()] = std::move(device);
    } else if (type == TOKEN_CMD_LINK) {
      parse_link(words, devices, nodes);
    } else if (type == TOKEN_COMMENT) {
      // Ignore comments
      continue;
    } else {
      panic("Invalid line \"%s\"", line.c_str());
    }
  }

  RoutingTable forwarding_table = build_routing_table(nodes);

  return PhysicalNetwork(std::move(devices), std::move(nodes), std::move(forwarding_table));
}

void PhysicalNetwork::debug() const {
  std::cerr << "========== Physical Network ==========\n";
  std::cerr << "Devices:\n";
  for (const auto &[_, device] : devices) {
    std::cerr << "  " << *device << "\n";
  }
  std::cerr << "Nodes:\n";
  for (const auto &[_, node] : nodes) {
    std::cerr << "  " << *node << "\n";
  }

  std::cerr << "Forwarding Table:\n";
  for (const auto &[src_node_id, table] : routing_table) {
    for (const auto &[dst_node_id, link_info] : table) {
      std::cerr << "{ Src Node ID: " << src_node_id << ", Dst Node ID: " << dst_node_id << ", Port: " << link_info.port
                << ", Cost: " << link_info.cost << " }\n";
    }
  }
  std::cerr << "=======================================\n";
}

} // namespace LibClone
