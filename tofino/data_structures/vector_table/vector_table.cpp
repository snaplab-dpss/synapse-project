#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  VectorTable vector_table;

  state_t() : vector_table("vector_table", {"Ingress.vector_table_0", "Ingress.vector_table_1"}) {}

  virtual void rollback() override final { vector_table.rollback(); }

  virtual void commit() override final { vector_table.commit(); }
};

state_t *state = nullptr;

void sycon::nf_init() {
  nf_state = std::make_unique<state_t>();
  state    = dynamic_cast<state_t *>(nf_state.get());

  bool duplicate_request_detected;

  LOG("***** Unmodified vector table *****");
  state->vector_table.dump();

  u32 index = 3;

  buffer_t value(2);
  value.set(0, 2, 0xcafe);

  LOG("******** Entry write ********");
  state->vector_table.write(index, value, duplicate_request_detected);
  state->vector_table.dump();

  LOG("******** Entry mod ********");
  value.set(0, 2, 0xbabe);
  state->vector_table.write(index, value, duplicate_request_detected);
  state->vector_table.dump();
}

void sycon::nf_exit() {}

void sycon::nf_user_signal_handler() {}

void sycon::nf_args(CLI::App &app) {}

nf_process_result_t sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  nf_process_result_t result;
  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
