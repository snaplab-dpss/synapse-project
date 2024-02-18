#include "../include/sycon/config.h"

#include <unistd.h>

#include <filesystem>
#include <string>

#include "../include/sycon/args.h"
#include "../include/sycon/config.h"
#include "../include/sycon/log.h"
#include "../include/sycon/util.h"
#include "constants.h"

namespace sycon {

config_t cfg;

// This function does the initial setUp of getting bfrtInfo object associated
// with the P4 program from which all other required objects are obtained
static void setup_bf_session() {
  cfg.dev_tgt.dev_id = 0;
  cfg.dev_tgt.pipe_id = ALL_PIPES;

  // Get devMgr singleton instance
  auto &devMgr = bfrt::BfRtDevMgr::getInstance();

  // Get info object from dev_id and p4 program name
  auto bf_status =
      devMgr.bfRtInfoGet(cfg.dev_tgt.dev_id, args.p4_prog_name, &cfg.info);
  ASSERT_BF_STATUS(bf_status)

  // Create a session object
  cfg.session = bfrt::BfRtSession::sessionCreate();
}

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

void init_switchd() {
  // Check if root privileges exist or not, exit if not.
  if (geteuid() != 0) {
    ERROR("Requires sudo. Exiting.");
  }

  // Allocate memory for the libbf_switchd context.
  cfg.switchd_ctx =
      (bf_switchd_context_t *)calloc(1, sizeof(bf_switchd_context_t));

  if (!cfg.switchd_ctx) {
    ERROR("Cannot allocate switchd context");
  }

  // Always set "background" because we do not want bf_switchd_lib_init to start
  // a CLI session.  That can be done afterward by the caller if requested
  // through command line options.
  cfg.switchd_ctx->running_in_background = true;

  // Always set "skip port add" so that ports are not automatically created when
  // running on either model or HW.
  cfg.switchd_ctx->skip_port_add = true;

  std::string install_dir = read_env(ENV_SDE_INSTALL);
  std::string conf_file = get_conf_file();

  cfg.switchd_ctx->install_dir = const_cast<char *>(install_dir.c_str());
  cfg.switchd_ctx->conf_file = const_cast<char *>(conf_file.c_str());

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

config_t::~config_t() {
  if (switchd_ctx) {
    free(switchd_ctx);
  }
}

}  // namespace sycon