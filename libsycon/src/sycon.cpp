#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "../include/sycon/sycon.hpp"

#include <signal.h>

extern "C" {
#include <target-utils/clish/thread.h>
}

namespace sycon {

config_t cfg;

static void bf_switchd_init_sig_set(sigset_t *set) {
  sigemptyset(set);
  sigaddset(set, SIGINT);
  sigaddset(set, SIGQUIT);
  sigaddset(set, SIGTERM);
  sigaddset(set, SIGUSR1);
}

static void *bf_switchd_nominated_signal_thread(void *arg) {
  (void)arg;
  sigset_t set;
  siginfo_t info;
  int s, signum;
  pthread_t tid;

  bf_switchd_init_sig_set(&set);

  tid = pthread_self();
  s   = pthread_detach(tid);
  if (s != 0) {
    ERROR("pthread_detach");
  }

  for (;;) {
    sigwait(&set, &signum);

    if (signum == -1) {
      continue;
    }

    switch (signum) {
    case SIGINT:
    case SIGQUIT:
    case SIGTERM:
      DEBUG("~~~ NF exit ~~~")
      nf_exit();
      bf_switchd_exit_sighandler(signum);
      exit(0);
    case SIGUSR1:
      LOG("~~~ Received a USR1 signal ~~~")
      nf_user_signal_handler();
      LOG("~~~ USR1 signal processing done ~~~")
      break;
    default:
      DEBUG("~~~ Received unknown signal %d (%s) ~~~", signum, strsignal(signum));
      break;
    }
  }

  pthread_exit(NULL);
}

void setup_async_signal_handling_thread() {
  sigset_t set;
  bf_switchd_init_sig_set(&set);

  // Block the signals in this process by default
  int s = pthread_sigmask(SIG_BLOCK, &set, NULL);
  if (s != 0) {
    ERROR("pthread_sigmask");
  }

  pthread_t thread;
  s = pthread_create(&thread, NULL, &bf_switchd_nominated_signal_thread, NULL);
  if (s != 0) {
    ERROR("pthread_create");
  }

  s = pthread_setname_np(thread, "bf_signal");
  if (s != 0) {
    ERROR("pthread_setname_np");
  }
}

void nf_setup() { nf_init(); }

void run_cli() {
  cli_run_bfshell();

  // Wait for CLI exit.
  pthread_join(cfg.switchd_ctx->tmr_t_id, NULL);
  pthread_join(cfg.switchd_ctx->dma_t_id, NULL);
  pthread_join(cfg.switchd_ctx->int_t_id, NULL);
  pthread_join(cfg.switchd_ctx->pkt_t_id, NULL);
  pthread_join(cfg.switchd_ctx->port_fsm_t_id, NULL);
  pthread_join(cfg.switchd_ctx->drusim_t_id, NULL);

  for (size_t i = 0; i < sizeof(cfg.switchd_ctx->agent_t_id) / sizeof(cfg.switchd_ctx->agent_t_id[0]); ++i) {
    if (cfg.switchd_ctx->agent_t_id[i] != 0) {
      pthread_join(cfg.switchd_ctx->agent_t_id[i], NULL);
    }
  }
}

} // namespace sycon