#include <LibSynapse/Target.h>
#include <LibSynapse/Modules/Module.h>
#include <LibSynapse/Modules/ModuleFactory.h>
#include <LibSynapse/Modules/x86/x86.h>
#include <LibSynapse/Modules/Controller/Controller.h>
#include <LibSynapse/Modules/Tofino/Tofino.h>

#include <cassert>
#include <toml++/toml.hpp>

#include <sstream>

namespace LibSynapse {

targets_config_t::targets_config_t(const std::filesystem::path &targets_config_file) {
  toml::table config;

  try {
    config = toml::parse_file(targets_config_file.string());
  } catch (const toml::parse_error &err) {
    panic("Parsing failed: %s\n", err.what());
  }

  assert(config.contains("switch") && "Switch configuration not found");
  assert(config["switch"].as_table()->contains("arch") && "Arch configuration not found");

  tofino_config.properties = {
      .total_ports                                = static_cast<int>(config["switch"]["front_panel_ports"].as_array()->size()),
      .total_recirc_ports                         = static_cast<int>(config["switch"]["recirculation_ports"].as_array()->size()),
      .max_packet_bytes_in_condition              = *config["switch"]["arch"]["max_packet_bytes_in_condition"].value<bytes_t>(),
      .pipes                                      = *config["switch"]["arch"]["pipes"].value<int>(),
      .stages                                     = *config["switch"]["arch"]["stages"].value<int>(),
      .sram_per_stage                             = *config["switch"]["arch"]["sram_per_stage"].value<bits_t>(),
      .tcam_per_stage                             = *config["switch"]["arch"]["tcam_per_stage"].value<bits_t>(),
      .map_ram_per_stage                          = *config["switch"]["arch"]["map_ram_per_stage"].value<bits_t>(),
      .max_logical_tcam_tables_per_stage          = *config["switch"]["arch"]["max_logical_tcam_tables_per_stage"].value<int>(),
      .max_logical_sram_and_tcam_tables_per_stage = *config["switch"]["arch"]["max_logical_sram_and_tcam_tables_per_stage"].value<int>(),
      .phv_size                                   = *config["switch"]["arch"]["phv_size"].value<bits_t>(),
      .phv_8bit_containers                        = *config["switch"]["arch"]["phv_8bit_containers"].value<int>(),
      .phv_16bit_containers                       = *config["switch"]["arch"]["phv_16bit_containers"].value<int>(),
      .phv_32bit_containers                       = *config["switch"]["arch"]["phv_32bit_containers"].value<int>(),
      .packet_buffer_size                         = *config["switch"]["arch"]["packet_buffer_size"].value<bits_t>(),
      .exact_match_xbar_per_stage                 = *config["switch"]["arch"]["exact_match_xbar_per_stage"].value<bits_t>(),
      .max_exact_match_keys                       = *config["switch"]["arch"]["max_exact_match_keys"].value<int>(),
      .ternary_match_xbar                         = *config["switch"]["arch"]["ternary_match_xbar"].value<bits_t>(),
      .max_ternary_match_keys                     = *config["switch"]["arch"]["max_ternary_match_keys"].value<int>(),
      .max_salu_size                              = *config["switch"]["arch"]["max_salu_size"].value<bits_t>(),
      .max_digests                                = *config["switch"]["arch"]["max_digests"].value<u8>(),
      .min_expiration_time                        = *config["switch"]["arch"]["min_expiration_time_ms"].value<time_ms_t>(),
      .max_capacity                               = *config["switch"]["arch"]["max_capacity_pps"].value<bps_t>(),
  };

  std::unordered_set<u16> nf_devices;
  std::unordered_set<u16> front_panel_ports;

  for (auto &&elem : *config["switch"]["front_panel_ports"].as_array()) {
    int virtual_port  = *elem.at_path("vport").value<int>();
    int physical_port = *elem.at_path("pport").value<int>();
    int pipe          = *elem.at_path("pipe").value<int>();
    bps_t capacity    = *elem.at_path("capacity_bps").value<bps_t>();

    if (virtual_port < 0) {
      continue;
    }

    if (nf_devices.find(virtual_port) != nf_devices.end()) {
      panic("Invalid config file: repeated vport %d", virtual_port);
    }

    if (front_panel_ports.find(physical_port) != front_panel_ports.end()) {
      panic("Invalid config file: repeated pport %d", physical_port);
    }

    nf_devices.insert(virtual_port);
    front_panel_ports.insert(physical_port);

    Tofino::tofino_port_t port{
        .nf_device        = static_cast<u16>(virtual_port),
        .front_panel_port = static_cast<u16>(physical_port),
        .pipe             = static_cast<u16>(pipe),
        .capacity         = capacity,
    };

    tofino_config.ports.push_back(port);
  }

  std::unordered_set<u16> dev_ports;

  for (auto &&elem : *config["switch"]["recirculation_ports"].as_array()) {
    int dev_port   = *elem.at_path("dev_port").value<int>();
    int pipe       = *elem.at_path("pipe").value<int>();
    bps_t capacity = *elem.at_path("capacity_bps").value<bps_t>();

    if (dev_port < 0) {
      continue;
    }

    if (dev_ports.find(dev_port) != dev_ports.end()) {
      panic("Invalid config file: repeated dev port %d", dev_port);
    }

    dev_ports.insert(dev_port);

    Tofino::tofino_recirculation_port_t recirc_port{
        .dev_port = static_cast<u16>(dev_port),
        .pipe     = static_cast<u16>(pipe),
        .capacity = capacity,
    };

    tofino_config.recirculation_ports.push_back(recirc_port);
  }

  controller_capacity = *config["controller"]["capacity_pps"].value<pps_t>();

  if (tofino_config.properties.total_recirc_ports != static_cast<int>(tofino_config.recirculation_ports.size())) {
    panic("Invalid config file: total_recirc_ports size does not match the number of provided recirculation ports.");
  }
}

TargetView::TargetView(TargetType _type, std::vector<const ModuleFactory *> _module_factories, const TargetContext *_base_ctx)
    : type(_type), module_factories(_module_factories), base_ctx(_base_ctx) {}

Target::Target(TargetType _type, std::vector<std::unique_ptr<ModuleFactory>> _module_factories, std::unique_ptr<TargetContext> _base_ctx)
    : type(_type), module_factories(std::move(_module_factories)), base_ctx(std::move(_base_ctx)) {}

TargetView Target::get_view() const {
  std::vector<const ModuleFactory *> factories;
  for (const std::unique_ptr<ModuleFactory> &factory : module_factories) {
    factories.push_back(factory.get());
  }
  return TargetView(type, factories, base_ctx.get());
}

std::ostream &operator<<(std::ostream &os, TargetType target) {
  os << target.instance_id << ": ";
  switch (target.type) {
  case TargetArchitecture::Controller:
    os << "Ctrl";
    break;
  case TargetArchitecture::Tofino:
    os << "Tofino";
    break;
  case TargetArchitecture::x86:
    os << "x86";
    break;
  }
  return os;
}

std::string to_string(TargetType target) {
  std::stringstream ss;
  ss << target;
  return ss.str();
}

TargetsView::TargetsView(const std::unordered_map<TargetView, bool, TargetViewHasher> &_elements) : elements(_elements) {}

TargetView TargetsView::get_initial_target() const {
  for (const std::pair<TargetView, bool> &tv_pair : elements) {
    if (tv_pair.second) {
      return tv_pair.first;
    }
  }
  panic("No initial target set");
}

Targets::Targets(const targets_config_t &targets_config, const std::unordered_map<TargetType, bool> &targets) {
  for (const auto &[target, is_init] : targets) {
    switch (target.type) {
    case TargetArchitecture::Tofino: {
      elements.emplace(std::make_unique<Tofino::TofinoTarget>(targets_config.tofino_config, target.instance_id), is_init);
    } break;
    case TargetArchitecture::Controller: {
      elements.emplace(std::make_unique<Controller::ControllerTarget>(target.instance_id), is_init);
    } break;
    case TargetArchitecture::x86: {
      elements.emplace(std::make_unique<x86::x86Target>(target.instance_id), is_init);
    } break;
    default: {
      panic("Unknown target type in physical network");
    }
    }
  }
}

TargetsView Targets::get_view() const {
  std::unordered_map<TargetView, bool, TargetViewHasher> views;
  for (const std::pair<const std::unique_ptr<Target>, bool> &element : elements) {
    views.emplace(element.first->get_view(), element.second);
  }
  return TargetsView(views);
}

} // namespace LibSynapse
