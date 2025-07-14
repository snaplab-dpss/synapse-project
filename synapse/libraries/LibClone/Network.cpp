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
      nodes[node1_id] = std::make_unique<NetworkNode>(node1_id, nfs.at(node1_id).get(), sport);
    } else {
      nodes[node1_id] = std::make_unique<NetworkNode>(node1_id, sport);
    }
  }

  const bool node2_is_nf = nfs.find(node2_id) != nfs.end();
  if (!node2_is_nf && node2_id != TOKEN_PORT) {
    panic("Could not find node %s", node2_id.c_str());
  }

  if (nodes.find(node2_id) == nodes.end()) {
    if (node2_is_nf) {
      nodes[node2_id] = std::make_unique<NetworkNode>(node2_id, nfs.at(node2_id).get(), dport);
    } else {
      nodes[node2_id] = std::make_unique<NetworkNode>(node2_id, dport);
    }
  }

  nodes.at(node1_id)->add_link(sport, dport, nodes.at(node2_id).get());
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

  return Network(std::move(nfs), std::move(nodes));
}

} // namespace LibClone