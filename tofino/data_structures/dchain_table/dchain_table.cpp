#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  DchainTable dchain_table;

  state_t() : dchain_table("dchain_table", {"Ingress.dchain_table_0", "Ingress.dchain_table_1"}, 1000) {}

  virtual void rollback() override final { dchain_table.rollback(); }

  virtual void commit() override final { dchain_table.commit(); }
};

state_t *state = nullptr;

void sycon::nf_init() {
  nf_state = std::make_unique<state_t>();
  state    = dynamic_cast<state_t *>(nf_state.get());

  bool duplicate_request_detected;

  LOG("***** Empty dchain table *****");
  state->dchain_table.dump();

  LOG("******** Allocate new index ********");
  u32 new_index;
  bool success = state->dchain_table.allocate_new_index(new_index);
  LOG("Allocated new index %u success %d", new_index, success);
  state->dchain_table.dump();

  LOG("******** Is index allocated ********");
  bool is_allocated = state->dchain_table.is_index_allocated(new_index);
  LOG("Is index %u allocated %d", new_index, is_allocated);

  LOG("******** Free index ********");
  state->dchain_table.free_index(new_index, duplicate_request_detected);
  state->dchain_table.dump();

  LOG("******** Allocate and expire index ********");
  success = state->dchain_table.allocate_new_index(new_index);
  LOG("Allocated new index %u success %d", new_index, success);
  is_allocated = state->dchain_table.is_index_allocated(new_index);
  LOG("Is index %u allocated %d", new_index, is_allocated);
  state->dchain_table.dump();

  LOG("zzzzzzzzzzz...");
  sleep(5);

  LOG("******** Is index allocated ********");
  is_allocated = state->dchain_table.is_index_allocated(new_index);
  LOG("Is index %u allocated %d", new_index, is_allocated);
  state->dchain_table.dump();

  LOG("******** Continuously refresh index ********");
  success = state->dchain_table.allocate_new_index(new_index);
  LOG("Allocated new index %u success %d", new_index, success);
  is_allocated = state->dchain_table.is_index_allocated(new_index);

  LOG("Continuously refreshing...");
  for (size_t i = 0; i < 1'000; i++) {
    state->dchain_table.refresh_index(new_index);
  }
  LOG("Done! Now we sleep...");
  sleep(5);
}

void sycon::nf_exit() {}

void sycon::nf_user_signal_handler() {}

void sycon::nf_args(CLI::App &app) {}

nf_process_result_t sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  nf_process_result_t result;
  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
