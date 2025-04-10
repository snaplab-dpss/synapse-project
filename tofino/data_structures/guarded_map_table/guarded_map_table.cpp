#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  GuardedMapTable guarded_map_table;

  state_t()
      : guarded_map_table("guarded_map_table", {"Ingress.guarded_map_table_0", "Ingress.guarded_map_table_1"}, "Ingress.guarded_map_table_guard") {}
};

state_t *state = nullptr;

void sycon::nf_init() {
  nf_state = std::make_unique<state_t>();
  state    = dynamic_cast<state_t *>(nf_state.get());

  buffer_t key(8);
  key[0] = 0x00;
  key[1] = 0x01;
  key[2] = 0x02;
  key[3] = 0x03;
  key[4] = 0x04;
  key[5] = 0x05;
  key[6] = 0x06;
  key[7] = 0x07;

  u32 value = 0xdeadbeef;

  LOG("***** Empty map table *****");
  state->guarded_map_table.dump();

  LOG("******** New entry ********");
  state->guarded_map_table.put(key, value);
  state->guarded_map_table.dump();

  value = 0xcafebabe;

  LOG("******** Entry mod ********");
  state->guarded_map_table.put(key, value);
  state->guarded_map_table.dump();

  // LOG("******** Entry del ********");
  // state->guarded_map_table.del(key);
  // state->guarded_map_table.dump();
}

void sycon::nf_exit() {}

void sycon::nf_user_signal_handler() {}

void sycon::nf_args(CLI::App &app) {}

nf_process_result_t sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  nf_process_result_t result;
  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
