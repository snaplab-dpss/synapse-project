#include "../include/sycon/log.h"
#include "config.h"

namespace sycon {

void setup_transaction() {
  if (pthread_mutex_init(&cfg.transaction_mutex, NULL) != 0) {
    ERROR("pthread_mutex_init failed")
  }
}

void begin_transaction() {
  pthread_mutex_lock(&cfg.transaction_mutex);
  bool atomic = true;
  bf_status_t bf_status = cfg.session->beginTransaction(atomic);
  ASSERT_BF_STATUS(bf_status);
}

void end_transaction() {
  bool hw_sync = true;
  bf_status_t bf_status = cfg.session->commitTransaction(hw_sync);
  ASSERT_BF_STATUS(bf_status);
  pthread_mutex_unlock(&cfg.transaction_mutex);
}

void begin_batch() {
  pthread_mutex_lock(&cfg.transaction_mutex);
  bf_status_t bf_status = cfg.session->beginBatch();
  ASSERT_BF_STATUS(bf_status);
}

void flush_batch() {
  auto bf_status = cfg.session->flushBatch();
  ASSERT_BF_STATUS(bf_status);
}

void end_batch() {
  bool hw_sync = true;
  bf_status_t bf_status = cfg.session->endBatch(hw_sync);
  ASSERT_BF_STATUS(bf_status);
  pthread_mutex_unlock(&cfg.transaction_mutex);
}

}  // namespace sycon