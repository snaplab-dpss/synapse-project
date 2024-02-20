#include <cstdlib>

#include "../include/sycon/sycon.h"
#include "config.h"
#include "constants.h"
#include "metatables/ports.h"

extern "C" {
#include <target-utils/clish/thread.h>
}

namespace sycon {

void configure_ports() {
  // No need to configure the ports when running with tofino model.
  if (args.model) {
    return;
  }

  Ports ports;
  ports.add_port(args.in_port, DEFAULT_PORT_LANE, DEFAULT_PORT_SPEED,
                 args.wait_for_ports);
  ports.add_port(args.out_port, DEFAULT_PORT_LANE, DEFAULT_PORT_SPEED,
                 args.wait_for_ports);
}

void run_cli() {
  cli_run_bfshell();

  // If we started a CLI shell, wait to exit.
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
  std::atexit(nf_cleanup);
}

}  // namespace sycon