#include "conf.h"

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

namespace netcache {

void from_json(const nlohmann::json &j, kv_t &kv) {
  j.at("store_size").get_to(kv.store_size);
  j.at("initial_entries").get_to(kv.initial_entries);
}

void from_json(const nlohmann::json &j, key_cntr_t &key_cntr) {
  j.at("reg_size").get_to(key_cntr.reg_size);
  j.at("reset_timer").get_to(key_cntr.reset_timer);
}

void from_json(const nlohmann::json &j, cm_t &cm) {
  j.at("reg_size").get_to(cm.reg_size);
  j.at("reset_timer").get_to(cm.reset_timer);
}

void from_json(const nlohmann::json &j, bloom_t &bloom) {
  j.at("reg_size").get_to(bloom.reg_size);
  j.at("reset_timer").get_to(bloom.reset_timer);
}

void from_json(const nlohmann::json &j, conf_t &conf) {
  j.at("kv").get_to(conf.kv);
  j.at("key_cntr").get_to(conf.key_cntr);
  j.at("cm").get_to(conf.cm);
  j.at("bloom").get_to(conf.bloom);
}

conf_t parse_conf_file(const std::filesystem::path &conf_file_path) {
  std::cerr << "Parsing " << conf_file_path << "\n";
  auto conf_file = std::ifstream(conf_file_path);

  if (!conf_file.good()) {
    std::cerr << "Error opening \"" << conf_file_path << "\". Exiting.\n";
    exit(1);
  }

  auto data = nlohmann::json::parse(conf_file);
  auto conf = data.get<conf_t>();

  std::cout << "\n";
  std::cout << "K/V Store\n";
  std::cout << "	store_size:       " << conf.kv.store_size << "\n";
  std::cout << "	initial_entries:  " << conf.kv.initial_entries << "\n";
  std::cout << "CM sketch\n";
  std::cout << "	reg size:         " << conf.cm.reg_size << "\n";
  std::cout << "Bloom filter\n";
  std::cout << "	reg size:         " << conf.bloom.reg_size << "\n";

  return conf;
}

} // namespace netcache
