#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  VectorRegister vector_register;

  state_t() : vector_register("vector_register", {"Ingress.reg0", "Ingress.reg1", "Ingress.reg2"}) {}

  virtual void rollback() override final { vector_register.rollback(); }

  virtual void commit() override final { vector_register.commit(); }
};

state_t *state = nullptr;

void sycon::nf_init() {
  nf_state = std::make_unique<state_t>();
  state    = dynamic_cast<state_t *>(nf_state.get());

  bool duplicate_request_detected;

  buffer_t buffer(9);
  buffer[0] = 0x00;
  buffer[1] = 0x01;
  buffer[2] = 0x02;
  buffer[3] = 0x03;
  buffer[4] = 0x04;
  buffer[5] = 0x05;
  buffer[6] = 0x06;
  buffer[7] = 0x07;
  buffer[8] = 0x08;

  state->vector_register.put(0, buffer, duplicate_request_detected);
  state->vector_register.dump();
}

void sycon::nf_exit() {}

void sycon::nf_user_signal_handler() {}

void sycon::nf_args(CLI::App &app) {}

nf_process_result_t sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  nf_process_result_t result;
  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
