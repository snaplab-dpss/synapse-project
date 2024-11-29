#include <pcap.h>

#include <bf_rt/bf_rt.hpp>
#include <iostream>
#include <sstream>
#include <string>

#include "netcache.h"
#include "ports.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <bf_rt/bf_rt_common.h>
#include <bf_switchd/bf_switchd.h>
#include <bfutils/bf_utils.h>  // required for bfshell
#include <pkt_mgr/pkt_mgr_intf.h>
#ifdef __cplusplus
}
#endif

#define THRIFT_PORT_NUM 7777
#define ALL_PIPES 0xffff

#define ENV_VAR_SDE_INSTALL "SDE_INSTALL"
#define ENV_VAR_CONF_FILE "NETCACHE_HW_CONF"
#define PROGRAM_NAME "netcache"

#define SWITCH_STATUS_SUCCESS 0x00000000L
#define SWITCH_STATUS_FAILURE 0x00000001L
#define SWITCH_PACKET_MAX_BUFFER_SIZE 10000
#define SWITCH_MEMCPY memcpy

#define IN_VIRTUAL_IFACE "veth251"

#define CPU_PORT_TOFINO_MODEL 64

#define BFN_T10_032D_CONF_FILE "../../config/BFN-T10-032D.conf"

// BFN-T10-032D
// BFN-T10-032D-024
// BFN-T10-032D-020
// BFN-T10-032D-018
#define CPU_PORT_2_PIPES 192

// BFN-T10-064Q
// BFN-T10-032Q
#define CPU_PORT_4_PIPES 192

#define SWITCH_PKT_ERROR(fmt, arg...)                                 \
	bf_sys_log_and_trace(BF_MOD_SWITCHAPI, BF_LOG_ERR, "%s:%d: " fmt, \
						 __FUNCTION__, __LINE__, ##arg)
#define SWITCH_PKT_DEBUG(fmt, arg...)                                \
	bf_sys_log_and_trace(BF_MOD_SWITCHAPI, BF_LOG_DBG, "%s:%d " fmt, \
						 __FUNCTION__, __LINE__, ##arg)

namespace netcache {

typedef int switch_int32_t;
typedef uint32_t switch_status_t;

struct stats_t {
	uint64_t bytes;
	uint64_t packets;

	stats_t(uint64_t _bytes, uint64_t _packets) : bytes(_bytes), packets(_packets) {}
};

char *get_env_var_value(const char *env_var) {
	auto env_var_value = getenv(env_var);

	if (!env_var_value) {
		std::cerr << env_var << " env var not found.\n";
		exit(1);
	}

	return env_var_value;
}

char *get_install_dir() { return get_env_var_value(ENV_VAR_SDE_INSTALL); }

std::string get_target_conf_file() {
	return get_env_var_value(ENV_VAR_CONF_FILE);
}

void init_bf_switchd(bool use_tofino_model, bool bf_prompt) {
	auto switchd_main_ctx = (bf_switchd_context_t *)calloc(1, sizeof(bf_switchd_context_t));

	/* Allocate memory to hold switchd configuration and state */
	if (switchd_main_ctx == NULL) {
		std::cerr << "ERROR: Failed to allocate memory for switchd context\n";
		exit(1);
	}

	auto target_conf_file = get_target_conf_file();

	memset(switchd_main_ctx, 0, sizeof(bf_switchd_context_t));

	switchd_main_ctx->install_dir = get_install_dir();
	switchd_main_ctx->conf_file = const_cast<char *>(target_conf_file.c_str());
	switchd_main_ctx->skip_p4 = false;
	switchd_main_ctx->skip_port_add = false;
	switchd_main_ctx->running_in_background = !bf_prompt;
	// switchd_main_ctx->shell_set_ucli = true;
	switchd_main_ctx->dev_sts_thread = true;
	switchd_main_ctx->dev_sts_port = THRIFT_PORT_NUM;

	auto bf_status = bf_switchd_lib_init(switchd_main_ctx);
	std::cerr << "Initialized bf_switchd, status = " << bf_status << "\n";

	if (bf_status != BF_SUCCESS) {
		exit(1);
	}
}

void setup_controller(const std::string &conf_file_path, bool use_tofino_model) {
	auto conf = parse_conf_file(conf_file_path);
	setup_controller(conf, use_tofino_model);
}

void setup_controller(const conf_t &conf, bool use_tofino_model) {
	bf_rt_target_t dev_tgt;
	dev_tgt.dev_id = 0;
	dev_tgt.pipe_id = ALL_PIPES;

	// Get devMgr singleton instance
	auto &devMgr = bfrt::BfRtDevMgr::getInstance();

	// Get info object from dev_id and p4 program name
	const bfrt::BfRtInfo *info = nullptr;
	auto bf_status = devMgr.bfRtInfoGet(dev_tgt.dev_id, PROGRAM_NAME, &info);

	// Create a session object
	auto session = bfrt::BfRtSession::sessionCreate();

	Controller::init(info, session, dev_tgt, conf, use_tofino_model);
}

}  // namespace netcache
