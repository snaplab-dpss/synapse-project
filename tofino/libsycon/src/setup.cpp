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
}

void configure_ports() {
  bf_dev_port_t pcie_cpu_port = bf_pcie_cpu_port_get(cfg.dev_tgt.dev_id);
  DEBUG("PCIe CPU port: %u", pcie_cpu_port);

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
  ports.add_port(args.in_port, DEFAULT_PORT_LANE, DEFAULT_PORT_SPEED,
                 args.wait_for_ports);
  ports.add_port(args.out_port, DEFAULT_PORT_LANE, DEFAULT_PORT_SPEED,
                 args.wait_for_ports);
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