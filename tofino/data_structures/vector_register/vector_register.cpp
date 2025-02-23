#include <sycon/sycon.h>

using namespace sycon;

struct state_t {
  VectorRegister vector_register;

  state_t() : vector_register("Ingress", {"reg0", "reg1", "reg2"}) {}
};

std::unique_ptr<state_t> state;

void sycon::nf_init() {
  state = std::make_unique<state_t>();

  buffer_t buffer(7);
  buffer.set(0, 7, 0x01020304050607);

  std::cerr << "In buffer: " << buffer << "(" << buffer.get(0, 7) << ")" << std::endl;
  LOG("Value: 0x%lx", buffer.get(0, 7));

  state->vector_register.put(0, buffer);
  state->vector_register.dump();

  buffer_t out_buffer;
  state->vector_register.get(0, out_buffer);

  std::cerr << "Out buffer: " << out_buffer << "(" << out_buffer.get(0, 7) << ")" << std::endl;
  LOG("Value: 0x%lx", out_buffer.get(0, 7));
}

void sycon::nf_exit() {}

void sycon::nf_user_signal_handler() {}

void sycon::nf_args(CLI::App &app) {}

bool sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) { return true; }

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
