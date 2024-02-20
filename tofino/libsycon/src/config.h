#pragma once

#include <bf_rt/bf_rt.hpp>

extern "C" {
#include <bf_switchd/bf_switchd.h>
}

namespace sycon {

struct config_t {
  bf_switchd_context_t *switchd_ctx;
  bf_rt_target_t dev_tgt;
  const bfrt::BfRtInfo *info;
  std::shared_ptr<bfrt::BfRtSession> session;
  pthread_mutex_t transaction_mutex;

  ~config_t();
};

extern config_t cfg;

}  // namespace sycon