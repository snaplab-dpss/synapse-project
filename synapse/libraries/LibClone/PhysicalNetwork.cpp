#include <LibClone/PhysicalNetwork.h>

#include <LibCore/Debug.h>

#include <LibSynapse/Target.h>

#include <queue>
#include <fstream>
#include <sstream>

namespace LibClone {

namespace {
constexpr char TOKEN_CMD_LINK[]  = "link";
constexpr char TOKEN_CMD_PLACE[] = "place";
constexpr char TOKEN_PORT[]      = "global_port";
constexpr char TOKEN_COMMENT[]   = "//";
constexpr char TOKEN_DEVICE[]    = "device";
constexpr char TOKEN_COMPONENT[] = "component";

constexpr size_t LENGTH_DEVICE_INPUT    = 3;
constexpr size_t LENGTH_PORT_INPUT      = 2;
constexpr size_t LENGTH_LINK_INPUT      = 5;
constexpr size_t LENGTH_PLACEMENT_INPUT = 5;

std::ifstream open_file(const std::string &path) {
  std::ifstream fstream;
  fstream.open(path);

  if (!fstream.is_open()) {
    panic("Could not open input file %s", path.c_str());
  }

  return fstream;
}

std::unique_ptr<Device> parse_device(const std::vector<std::string> &words) {
  if (words.size() != LENGTH_DEVICE_INPUT) {
    panic("Invalid device");
  }

  const DeviceId id = std::stoi(words[1]);
  assert(id >= 0 && "ERROR: Unallowed DeviceId");
  const std::string &arch = words[2];

  return std::make_unique<Device>(id, arch);
}

void parse_placement(const std::vector<std::string> &words, const std::unordered_map<DeviceId, std::unique_ptr<Device>> &devices,
                     std::unordered_map<ComponentId, DeviceId> &placement) {
  if (words.size() != LENGTH_PLACEMENT_INPUT) {
    panic("Invalid placement");
  }

  const ComponentId component_id = std::stoul(words[2]);
  const DeviceId instance        = std::stoi(words[4]);

  if (devices.find(instance) == devices.end()) {
    panic("Could not find device");
  }

  if (placement.find(component_id) != placement.end()) {
    panic("Component %lu is already placed", component_id);
  }

  const DeviceId device_id = devices.at(instance)->get_id();
  assert(instance == device_id && "Instance ID should match device ID");

  placement[component_id] = device_id;
}

void parse_link(const std::vector<std::string> &words, const std::unordered_map<DeviceId, std::unique_ptr<Device>> &devices,
                std::unordered_map<InfrastructureNodeId, std::unique_ptr<InfrastructureNode>> &nodes) {
  if (words.size() != LENGTH_LINK_INPUT) {
    panic("Invalid link");
  }

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

std::unordered_map<InfrastructureNodeId, Port>
dijkstra(const std::unordered_map<InfrastructureNodeId, std::vector<std::pair<Port, InfrastructureNodeId>>> &links, const InfrastructureNodeId &src) {
  std::unordered_map<InfrastructureNodeId, int> dist;
  std::unordered_map<InfrastructureNodeId, Port> first_hop_port;
  std::unordered_set<InfrastructureNodeId> visited;

  using HeapElem = std::pair<int, InfrastructureNodeId>;
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
      continue; // no outgoing edges

    for (const auto &[out_port, v] : it->second) {
      int new_dist = cur_dist + 1;
      if (!dist.count(v) || new_dist < dist[v]) {
        dist[v] = new_dist;
        if (u == src) {
          first_hop_port[v] = out_port;
        } else {
          first_hop_port[v] = first_hop_port[u];
        }
        // Do not expand paths that go through the global port
        if (v != -1)
          heap.push({new_dist, v});
      }
    }
  }

  return first_hop_port;
}

std::unordered_map<InfrastructureNodeId, std::unordered_map<InfrastructureNodeId, Port>>
build_forwarding_table(std::unordered_map<InfrastructureNodeId, std::unique_ptr<InfrastructureNode>> &nodes) {
  std::unordered_map<InfrastructureNodeId, std::unordered_map<InfrastructureNodeId, Port>> routing;

  // Build adjacency list
  std::unordered_map<InfrastructureNodeId, std::vector<std::pair<Port, InfrastructureNodeId>>> links;
  for (const auto &[node_id, node] : nodes) {
    for (const auto &[port, link] : node->get_links()) {
      links[node_id].emplace_back(port, link.second->get_id());
    }
  }

  for (const auto &[node_id, _] : nodes) {
    routing[node_id] = dijkstra(links, node_id);
  }

  return routing;
}

} // namespace

DeviceId PhysicalNetwork::get_placement(const ComponentId component_id) const {
  if (placement_strategy.find(component_id) == placement_strategy.end()) {
    panic("Component ID %lu not found in placement strategy!", component_id);
  }
  return placement_strategy.at(component_id);
}

bool PhysicalNetwork::has_device(const DeviceId device_id) const { return devices.find(device_id) != devices.end(); }

const Device *PhysicalNetwork::get_device(const DeviceId device_id) const {
  if (devices.find(device_id) == devices.end()) {
    panic("DEvice ID %ld not found in devices!", device_id);
  }
  return devices.at(device_id).get();
}

const InfrastructureNode *PhysicalNetwork::get_node(const InfrastructureNodeId node_id) const {
  if (nodes.find(node_id) == nodes.end()) {
    panic("Node ID %ld not found in nodes!", node_id);
  }
  return nodes.at(node_id).get();
}

Port PhysicalNetwork::get_forwarding_port(const InfrastructureNodeId src, const InfrastructureNodeId dst) const {
  if (forwarding_table.find(src) == forwarding_table.end()) {
    panic("Source Node ID %ld not found in forwarding table!", src);
  }
  if (forwarding_table.at(src).find(dst) == forwarding_table.at(src).end()) {
    panic("Destination Node ID %ld not reachable from Source Node ID %ld!", dst, src);
  }
  return forwarding_table.at(src).at(dst);
}

const std::vector<std::pair<Port, DeviceId>> PhysicalNetwork::find_path(const InfrastructureNodeId src_id, const InfrastructureNodeId dst_id) const {
  std::vector<std::pair<Port, DeviceId>> path;

  InfrastructureNodeId current_node_id = src_id;

  while (current_node_id != dst_id) {
    Port out_port                          = get_forwarding_port(current_node_id, dst_id);
    const InfrastructureNode *current_node = get_node(current_node_id);

    // const Port next_port                = current_node->get_connected_node(out_port).first;
    const InfrastructureNode *next_node = current_node->get_connected_node(out_port).second;

    if (next_node->get_node_type() == InfrastructureNodeType::GLOBAL_PORT) {
      panic("Path goes through global port, which is not allowed!");
    }

    const Device *next_device = next_node->get_device();
    path.emplace_back(out_port, next_device->get_id());

    current_node_id = next_node->get_id();
  }

  return path;
}

const std::unordered_map<DeviceId, bool> PhysicalNetwork::get_target_list(const ComponentId root_node) const {
  std::unordered_map<DeviceId, bool> targets;

  if (placement_strategy.find(root_node) == placement_strategy.end()) {
    panic("Component ID %lu not found in placement strategy!", root_node);
  }

  const DeviceId root_id = placement_strategy.at(root_node);
  targets.emplace(root_id, true);

  for (const auto &[_, device] : devices) {
    const DeviceId &device_id = device->get_id();
    if (device_id == root_id) {
      continue;
    }
    targets.emplace(device_id, false);
  }

  return targets;
}

const std::set<Port> PhysicalNetwork::get_target_ports(const InfrastructureNodeId node_id) const {
  const InfrastructureNode *node = get_node(node_id);
  return node->get_ports();
}

const PhysicalNetwork PhysicalNetwork::parse(const std::filesystem::path &network_file) {
  std::ifstream fstream = open_file(network_file);

  std::unordered_map<DeviceId, std::unique_ptr<Device>> devices;
  std::unordered_map<InfrastructureNodeId, std::unique_ptr<InfrastructureNode>> nodes;
  std::unordered_map<ComponentId, DeviceId> placement;

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
    } else if (type == TOKEN_CMD_PLACE) {
      parse_placement(words, devices, placement);
    } else if (type == TOKEN_COMMENT) {
      // Ignore comments
      continue;
    } else {
      panic("Invalid line \"%s\"", line.c_str());
    }
  }

  std::unordered_map<InfrastructureNodeId, std::unordered_map<InfrastructureNodeId, Port>> forwarding_table = build_forwarding_table(nodes);

  return PhysicalNetwork(std::move(devices), std::move(nodes), std::move(placement), std::move(forwarding_table));
}

void PhysicalNetwork::add_placement(ComponentId component_id, DeviceId device) {
  if (placement_strategy.find(component_id) != placement_strategy.end()) {
    panic("Component %lu is already placed", component_id);
  }
  placement_strategy[component_id] = device;
}

} // namespace LibClone
