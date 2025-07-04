#include <LibClone/Network.h>

#include <LibCore/Debug.h>

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <filesystem>

namespace LibClone {

namespace {
constexpr char TOKEN_DEVICE[]  = "device";
constexpr char TOKEN_NF[]      = "nf";
constexpr char TOKEN_LINK[]    = "link";
constexpr char TOKEN_PORT[]    = "port";
constexpr char TOKEN_COMMENT[] = "//";

constexpr size_t LENGTH_DEVICE_INPUT = 2;
constexpr size_t LENGTH_NF_INPUT     = 3;
constexpr size_t LENGTH_LINK_INPUT   = 5;
constexpr size_t LENGTH_PORT_INPUT   = 4;

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

  const std::string &id = words[1];
  return std::unique_ptr<Device>(new Device(id));
}

std::unique_ptr<NF> parse_nf(const std::vector<std::string> &words, const std::filesystem::path &network_file,
                             LibCore::SymbolManager *symbol_manager) {
  if (words.size() != LENGTH_NF_INPUT) {
    panic("Invalid network function");
  }

  const std::string &id      = words[1];
  std::filesystem::path path = words[2];

  // Relative to the configuration file, not the current working directory.
  if (path.is_relative()) {
    path = network_file.parent_path() / path;
  }

  return std::unique_ptr<NF>(new NF(id, path, symbol_manager));
}

std::unique_ptr<Link> parse_link(const std::vector<std::string> &words, const std::unordered_map<NFId, std::unique_ptr<NF>> &nfs) {
  if (words.size() != LENGTH_LINK_INPUT) {
    panic("Invalid link");
  }

  const std::string &node1  = words[1];
  const std::string &sport1 = words[2];
  const u32 port1           = std::stoul(sport1);

  if (nfs.find(node1) == nfs.end() && node1 != "port") {
    panic("Could not find node %s", node1.c_str());
  }

  const std::string node2  = words[3];
  const std::string sport2 = words[4];
  const u32 port2          = std::stoul(sport2);

  if (nfs.find(node2) == nfs.end() && node2 != "port") {
    panic("Could not find node %s", node2.c_str());
  }

  return std::unique_ptr<Link>(new Link(node1, port1, node2, port2));
}

std::unique_ptr<Port> parse_port(const std::vector<std::string> &words, const std::unordered_map<DeviceId, std::unique_ptr<Device>> &devices) {
  if (words.size() != LENGTH_PORT_INPUT) {
    panic("Invalid port");
  }

  const u32 global_port         = stoul(words[1]);
  const std::string device_name = words[2];
  const u32 device_port         = stoul(words[3]);

  if (devices.find(device_name) == devices.end()) {
    panic("Could not find device %s", device_name.c_str());
  }

  const std::unique_ptr<Device> &device = devices.at(device_name);
  device->add_port(device_port, global_port);

  return std::unique_ptr<Port>(new Port(device.get(), device_port, global_port));
}
} // namespace

Network Network::parse(const std::filesystem::path &network_file, LibCore::SymbolManager *symbol_manager) {
  std::ifstream fstream = open_file(network_file);

  std::unordered_map<DeviceId, std::unique_ptr<Device>> devices;
  std::unordered_map<NFId, std::unique_ptr<NF>> nfs;
  std::vector<std::unique_ptr<Link>> links;
  std::unordered_map<GlobalPortId, std::unique_ptr<Port>> ports;

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
      std::unique_ptr<LibClone::Device> device = parse_device(words);
      const DeviceId device_id                 = device->get_id();
      devices[device_id]                       = std::move(device);
    } else if (type == TOKEN_NF) {
      std::unique_ptr<NF> nf = parse_nf(words, network_file, symbol_manager);
      const NFId nf_id       = nf->get_id();
      nfs[nf->get_id()]      = std::move(nf);
    } else if (type == TOKEN_LINK) {
      std::unique_ptr<Link> link = parse_link(words, nfs);
      links.push_back(std::move(link));
    } else if (type == TOKEN_PORT) {
      std::unique_ptr<Port> port     = parse_port(words, devices);
      const GlobalPortId global_port = port->get_global_port();
      ports[global_port]             = std::move(port);
    } else if (type == TOKEN_COMMENT) {
      // Ignore comments
      continue;
    } else {
      panic("Invalid line \"%s\"", line.c_str());
    }
  }

  return Network(std::move(devices), std::move(nfs), std::move(links), std::move(ports));
}

} // namespace LibClone