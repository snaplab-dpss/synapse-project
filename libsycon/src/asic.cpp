#include "../include/sycon/asic.hpp"
#include "../include/sycon/args.hpp"
#include "../include/sycon/config.hpp"
#include "../include/sycon/constants.hpp"
#include "../include/sycon/externs.hpp"
#include "metatables/device_configuration.hpp"
#include "metatables/port_stat.hpp"
#include "metatables/ports.hpp"

#include <unistd.h>
#include <filesystem>
#include <string>

namespace sycon {

std::unique_ptr<Port_Stat> port_stat;
std::unique_ptr<Ports> ports;

u64 get_asic_port_rx(u16 dev_port) { return port_stat->get_port_rx(dev_port, true); }

u64 get_asic_port_tx(u16 dev_port) { return port_stat->get_port_tx(dev_port, true); }

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
  auto bf_status = devMgr.bfRtInfoGet(cfg.dev_tgt.dev_id, args.p4_prog_name, &cfg.info);
  ASSERT_BF_STATUS(bf_status)

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
  bf_status_t status =
      bf_port_info_get(cfg.dev_tgt.dev_id, pcie_cpu_port, &pcie_cpu_port_speed, &pcie_cpu_port_lane_number);
  ASSERT_BF_STATUS(status);

  DEBUG("PCIe CPU port:       %u (%s)", pcie_cpu_port, bf_port_speed_str(pcie_cpu_port_speed));
  DEBUG("Eth CPU port:        %u", eth_cpu_port);

  cfg.in_dev_port  = args.in_port;
  cfg.out_dev_port = args.out_port;

  // No need to configure the ports when running with tofino model.
  if (args.model) {
    return;
  }

  DEBUG("Enabling port %u", args.in_port);
  u16 in_dev_port = ports->get_dev_port(args.in_port, DEFAULT_PORT_LANE);
  ports->add_dev_port(in_dev_port, DEFAULT_PORT_SPEED, DEFAULT_PORT_LOOPBACK_MODE, args.wait_for_ports);

  DEBUG("Enabling port %u", args.out_port);
  u16 out_dev_port = ports->get_dev_port(args.out_port, DEFAULT_PORT_LANE);
  ports->add_dev_port(out_dev_port, DEFAULT_PORT_SPEED, DEFAULT_PORT_LOOPBACK_MODE, args.wait_for_ports);

  cfg.in_dev_port  = in_dev_port;
  cfg.out_dev_port = out_dev_port;
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
  }
}

} // namespace sycon