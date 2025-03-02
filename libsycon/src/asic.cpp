#include "../include/sycon/asic.h"
#include "../include/sycon/args.h"
#include "../include/sycon/config.h"
#include "../include/sycon/constants.h"
#include "../include/sycon/externs.h"
#include "metatables/device_configuration.h"
#include "metatables/port_stat.h"
#include "metatables/ports.h"

#include <unistd.h>
#include <filesystem>
#include <string>

namespace sycon {

std::unique_ptr<Port_Stat> port_stat;
std::unique_ptr<Ports> ports;

u16 asic_get_dev_port(u16 front_panel_port) { return ports->get_dev_port(front_panel_port, DEFAULT_PORT_LANE); }
u16 asic_get_front_panel_port_from_dev_port(u16 dev_port) { return ports->get_front_panel_port(dev_port); }
u64 asic_get_port_rx(u16 dev_port) { return port_stat->get_port_rx(dev_port, true); }
u64 asic_get_port_tx(u16 dev_port) { return port_stat->get_port_tx(dev_port, true); }
void asic_reset_port_stats() { port_stat->reset_stats(); }

static std::string get_conf_file() {
  std::filesystem::path conf_file = read_env(ENV_SDE_INSTALL);

  switch (args.tna_version) {
  case 1:
    conf_file /= SDE_CONF_RELATIVE_PATH_TOFINO1;
    break;
  case 2:
    conf_file /= SDE_CONF_RELATIVE_PATH_TOFINO2;
    break;
  default:
    ERROR("We are not compatible with TNA %d", args.tna_version);
  }

  conf_file /= args.p4_prog_name + ".conf";

  if (!std::filesystem::exists(conf_file)) {
    ERROR("Configuration file %s not found", conf_file.c_str());
  }

  return std::string(conf_file);
}

// This function does the initial setUp of getting bfrtInfo object associated
// with the P4 program from which all other required objects are obtained
static void setup_bf_session() {
  cfg.dev_tgt.dev_id  = 0;
  cfg.dev_tgt.pipe_id = ALL_PIPES;

  // Get devMgr singleton instance
  auto &devMgr = bfrt::BfRtDevMgr::getInstance();

  // Get info object from dev_id and p4 program name
  bf_status_t bf_status = devMgr.bfRtInfoGet(cfg.dev_tgt.dev_id, args.p4_prog_name, &cfg.info);
  ASSERT_BF_STATUS(bf_status);

  // Create the sessions objects
  cfg.session            = bfrt::BfRtSession::sessionCreate();
  cfg.usr_signal_session = bfrt::BfRtSession::sessionCreate();
}

void init_switchd() {
  // Check if root privileges exist or not, exit if not.
  if (geteuid() != 0) {
    ERROR("Requires sudo. Exiting.");
  }

  // Allocate memory for the libbf_switchd context.
  cfg.switchd_ctx = (bf_switchd_context_t *)calloc(1, sizeof(bf_switchd_context_t));

  if (!cfg.switchd_ctx) {
    ERROR("Cannot allocate switchd context");
  }

  // Always set "background" because we do not want bf_switchd_lib_init to start
  // a CLI session.  That can be done afterward by the caller if requested
  // through command line options.
  cfg.switchd_ctx->running_in_background = true;

  cfg.switchd_ctx->skip_port_add = false;

  std::string install_dir = read_env(ENV_SDE_INSTALL);
  std::string conf_file   = get_conf_file();

  cfg.switchd_ctx->install_dir = const_cast<char *>(install_dir.c_str());
  cfg.switchd_ctx->conf_file   = const_cast<char *>(conf_file.c_str());

  LOG("Install dir: %s", cfg.switchd_ctx->install_dir);
  LOG("Config file: %s", cfg.switchd_ctx->conf_file);

  // Initialize libbf_switchd.
  bf_status_t status = bf_switchd_lib_init(cfg.switchd_ctx);

  if (status != BF_SUCCESS) {
    free(cfg.switchd_ctx);
    ERROR("Failed to initialize libbf_switchd (%s)", bf_err_str(status));
  }

  setup_bf_session();
}

static void configure_ports() {
  ports     = std::make_unique<Ports>();
  port_stat = std::make_unique<Port_Stat>();

  bf_dev_port_t pcie_cpu_port = bf_pcie_cpu_port_get(cfg.dev_tgt.dev_id);
  bf_dev_port_t eth_cpu_port  = bf_eth_cpu_port_get(cfg.dev_tgt.dev_id);

  bf_port_speed_t pcie_cpu_port_speed;
  u32 pcie_cpu_port_lane_number;
  bf_status_t status = bf_port_info_get(cfg.dev_tgt.dev_id, pcie_cpu_port, &pcie_cpu_port_speed, &pcie_cpu_port_lane_number);
  ASSERT_BF_STATUS(status);

  LOG("PCIe CPU port:       %u (%s)", pcie_cpu_port, bf_port_speed_str(pcie_cpu_port_speed));
  LOG("Eth CPU port:        %u", eth_cpu_port);

  // No need to configure the ports when running with tofino model.
  for (u16 port : args.ports) {
    if (args.model) {
      cfg.dev_ports.push_back(port);
    } else {
      LOG("Enabling port %u", port);
      u16 dev_port = ports->get_dev_port(port, DEFAULT_PORT_LANE);
      ports->add_dev_port(dev_port, DEFAULT_PORT_SPEED, DEFAULT_PORT_LOOPBACK_MODE, false);
      cfg.dev_ports.push_back(dev_port);
    }
  }

  if (args.wait_for_ports) {
    bool all_ports_up = false;
    while (!all_ports_up) {
      sleep(1);
      all_ports_up = true;
      for (u16 port : args.ports) {
        u16 dev_port = ports->get_dev_port(port, DEFAULT_PORT_LANE);
        if (!ports->is_port_up(dev_port)) {
          LOG("Port %u is not ready yet...", port);
          all_ports_up = false;
          break;
        }
      }
    }
    LOG("All ports are ready!");
  }
}

static void update_dev_configuration() {
  Device_Configuration dev_cfg;
  dev_cfg.set_digest_timout(TOFINO_DIGEST_TIMEOUT);
}

void configure_dev() {
  configure_ports();
  update_dev_configuration();

  if (!args.model) {
    DEBUG("");
    DEBUG("**********************************************************");
    DEBUG("*                                                        *");
    DEBUG("*                        WARNING                         *");
    DEBUG("* Running in hardware mode but compiled with debug flags *");
    DEBUG("*                                                        *");
    DEBUG("**********************************************************");
    DEBUG("");
  }
}

} // namespace sycon