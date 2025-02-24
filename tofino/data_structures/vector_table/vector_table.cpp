#include <sycon/sycon.h>

using namespace sycon;

struct state_t {
  VectorTable vector_table;

  state_t() : vector_table("Ingress", {"vector_table_0", "vector_table_1"}) {}
};

std::unique_ptr<state_t> state;

void sycon::nf_init() {
  state = std::make_unique<state_t>();

  LOG("***** Empty vector table *****");
  state->vector_table.dump();

  LOG("******** New entry ********");
  u32 key = 0xdeadbeef;

  buffer_t value(2);
  value.set(0, 2, 0xcafe);

  state->vector_table.put(key, value);
  state->vector_table.dump();

  LOG("******** Entry mod ********");
  value.set(0, 2, 0xbabe);
  state->vector_table.put(key, value);
  state->vector_table.dump();

  LOG("******** Entry del ********");
  state->vector_table.del(key);
  state->vector_table.dump();
}

void sycon::nf_exit() {}

void sycon::nf_user_signal_handler() {}

void sycon::nf_args(CLI::App &app) {}

bool sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) { return true; }

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
