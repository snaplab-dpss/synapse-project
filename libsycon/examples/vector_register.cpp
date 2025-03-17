#include <sycon/sycon.h>

using namespace sycon;

struct state_t {
  VectorRegister vector_register;

  state_t() : vector_register({"Ingress.reg0", "Ingress.reg1", "Ingress.reg2"}) {}
};

std::unique_ptr<state_t> state;

void sycon::nf_init() {
  state = std::make_unique<state_t>();

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

  state->vector_register.put(0, buffer);
  state->vector_register.dump();
}

void sycon::nf_exit() {}

void sycon::nf_user_signal_handler() {}

void sycon::nf_args(CLI::App &app) {}

bool sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) { return true; }

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
