#include <signal.h>
#include <unistd.h>

#include <cstdlib>

#include "../include/sycon/sycon.h"
#include "config.h"
#include "constants.h"
#include "metatables/ports.h"

extern "C" {
#include <target-utils/clish/thread.h>
}

namespace sycon {

nf_config_t nf_config;

void interrupt_handler(int s) {
  // This will trigger an nf_cleanup call to cleanup NF state.
  exit(0);
}

void catch_interrupt() {
  struct sigaction sigIntHandler;

  sigIntHandler.sa_handler = interrupt_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  sigaction(SIGINT, &sigIntHandler, NULL);
  sigaction(SIGQUIT, &sigIntHandler, NULL);
}

void configure_ports() {
  bf_dev_port_t pcie_cpu_port = bf_pcie_cpu_port_get(cfg.dev_tgt.dev_id);
  bf_dev_port_t eth_cpu_port = bf_eth_cpu_port_get(cfg.dev_tgt.dev_id);

  bf_port_speed_t pcie_cpu_port_speed;
  uint32_t pcie_cpu_port_lane_number;
  bf_status_t status =
      bf_port_info_get(cfg.dev_tgt.dev_id, pcie_cpu_port, &pcie_cpu_port_speed,
                       &pcie_cpu_port_lane_number);
  ASSERT_BF_STATUS(status);

  DEBUG("PCIe CPU port:       %u (%s)", pcie_cpu_port,
        bf_port_speed_str(pcie_cpu_port_speed));
  DEBUG("Eth CPU port:        %u", eth_cpu_port);

  nf_config.in_dev_port = args.in_port;
  nf_config.out_dev_port = args.out_port;

  // No need to configure the ports when running with tofino model.
  if (args.model) {
    return;
  }

  DEBUG("");
  DEBUG("**********************************************************");
  DEBUG("*                                                        *");
  DEBUG("*                        WARNING                         *");
  DEBUG("* Running in hardware mode but compiled with debug flags *");
  DEBUG("*                                                        *");
  DEBUG("**********************************************************");

  Ports ports;

  DEBUG("Enabling port %u", args.in_port);
  uint16_t in_dev_port = ports.get_dev_port(args.in_port, DEFAULT_PORT_LANE);
  ports.add_dev_port(in_dev_port, DEFAULT_PORT_SPEED,
                     DEFAULT_PORT_LOOPBACK_MODE, args.wait_for_ports);

  DEBUG("Enabling port %u", args.out_port);
  uint16_t out_dev_port = ports.get_dev_port(args.out_port, DEFAULT_PORT_LANE);
  ports.add_dev_port(out_dev_port, DEFAULT_PORT_SPEED,
                     DEFAULT_PORT_LOOPBACK_MODE, args.wait_for_ports);

  nf_config.in_dev_port = in_dev_port;
  nf_config.out_dev_port = out_dev_port;
}

void run_cli() {
  cli_run_bfshell();

  // Wait for CLI exit.
  pthread_join(cfg.switchd_ctx->tmr_t_id, NULL);
  pthread_join(cfg.switchd_ctx->dma_t_id, NULL);
  pthread_join(cfg.switchd_ctx->int_t_id, NULL);
  pthread_join(cfg.switchd_ctx->pkt_t_id, NULL);
  pthread_join(cfg.switchd_ctx->port_fsm_t_id, NULL);
  pthread_join(cfg.switchd_ctx->drusim_t_id, NULL);

  for (size_t i = 0; i < sizeof(cfg.switchd_ctx->agent_t_id) /
                             sizeof(cfg.switchd_ctx->agent_t_id[0]);
       ++i) {
    if (cfg.switchd_ctx->agent_t_id[i] != 0) {
      pthread_join(cfg.switchd_ctx->agent_t_id[i], NULL);
    }
  }
}

void nf_setup() {
  nf_init();

  catch_interrupt();
  std::atexit(nf_cleanup);
}

}  // namespace sycon