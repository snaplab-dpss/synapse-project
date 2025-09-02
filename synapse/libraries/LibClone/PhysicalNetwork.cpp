#include <LibClone/PhysicalNetwork.h>

#include <LibCore/Debug.h>

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

  const std::string &id   = words[1];
  const std::string &arch = words[2];

  return std::make_unique<Device>(id, arch);
}

void parse_placement(const std::vector<std::string> &words, const std::unordered_map<InstanceId, std::unique_ptr<Device>> &devices,
                     std::unordered_map<ComponentId, NetworkNodeId> &placement) {
  if (words.size() != LENGTH_PLACEMENT_INPUT) {
    panic("Invalid placement");
  }

  const ComponentId component_id = std::stoul(words[2]);
  const InstanceId instance      = words[4];

  if (devices.find(instance) == devices.end()) {
    panic("Could not find device %s", instance.c_str());
  }

  if (placement.find(component_id) != placement.end()) {
    panic("Component %d is already placed", component_id);
  }

  placement[component_id] = instance;
}

void parse_link(const std::vector<std::string> &words, const std::unordered_map<NFId, std::unique_ptr<Device>> &devices,
                std::unordered_map<NetworkNodeId, std::unique_ptr<NetworkNode>> &nodes) {
  if (words.size() != LENGTH_LINK_INPUT) {
    panic("Invalid link");
  }

  const NetworkNodeId node1_id = words[1];
  const Port sport             = std::stoul(words[2]);
  const NetworkNodeId node2_id = words[3];
  const Port dport             = std::stoul(words[4]);

  const bool node1_is_device = devices.find(node1_id) != devices.end();
  if (!node1_is_device && node1_id != TOKEN_PORT) {
    panic("Could not find node %s", node1_id.c_str());
  }

  if (nodes.find(node1_id) == nodes.end()) {
    if (node1_is_device) {
      nodes[node1_id] = std::make_unique<NetworkNode>(node1_id, devices.at(node1_id).get());
    } else {
      nodes[node1_id] = std::make_unique<NetworkNode>(node1_id);
    }
  }

  const bool node2_is_device = devices.find(node2_id) != devices.end();
  if (!node2_is_device && node2_id != TOKEN_PORT) {
    panic("Could not find node %s", node2_id.c_str());
  }

  if (nodes.find(node2_id) == nodes.end()) {
    if (node2_is_device) {
      nodes[node2_id] = std::make_unique<NetworkNode>(node2_id, devices.at(node2_id).get());
    } else {
      nodes[node2_id] = std::make_unique<NetworkNode>(node2_id);
    }
  }

  nodes.at(node1_id)->add_link(sport, dport, nodes.at(node2_id).get());
}
} // namespace

const NetworkNodeId PhysicalNetwork::get_placement(const ComponentId component_id) const {
  if (placement_strategy.find(component_id) == placement_strategy.end()) {
    panic("Component ID %u not found in placement strategy!", component_id);
  }
  return placement_strategy.at(component_id);
}

PhysicalNetwork PhysicalNetwork::parse(const std::filesystem::path &network_file) {
  std::ifstream fstream = open_file(network_file);

  std::unordered_map<InstanceId, std::unique_ptr<Device>> devices;
  std::unordered_map<NetworkNodeId, std::unique_ptr<NetworkNode>> nodes;
  std::unordered_map<ComponentId, NetworkNodeId> placement;

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
      const InstanceId device_id     = device->get_id();
      devices[device->get_id()]      = std::move(device);
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

  return PhysicalNetwork(std::move(devices), std::move(nodes), std::move(placement));
}

} // namespace LibClone
