#pragma once

#include <vector>
#include <filesystem>
#include <iostream>

namespace netcache {

struct args_t {
  std::filesystem::path conf_file_path;
  std::vector<uint16_t> client_ports;
  uint16_t server_port;
  bool cache_activated;
  bool run_ucli;
  bool wait_for_ports;
  bool run_tofino_model;
  int tna_version;

  args_t() : cache_activated(true), run_ucli(false), wait_for_ports(false), run_tofino_model(false), tna_version(2) {}

  void dump() const {
    std::cout << "Arguments:\n";
    std::cout << "  Configuration file path: " << conf_file_path << "\n";
    std::cout << "  Frontend client ports: ";
    for (const auto &port : client_ports) {
      std::cout << port << " ";
    }
    std::cout << "\n";
    std::cout << "  Frontend server port: " << server_port << "\n";
    std::cout << "  Cache activated: " << cache_activated << "\n";
    std::cout << "  Run UCLI: " << run_ucli << "\n";
    std::cout << "  Wait for ports: " << wait_for_ports << "\n";
    std::cout << "  Run Tofino model: " << run_tofino_model << "\n";
    std::cout << "  TNA version: " << tna_version << "\n";
  }
};

} // namespace netcache