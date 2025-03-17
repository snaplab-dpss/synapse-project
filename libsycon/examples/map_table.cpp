#include <sycon/sycon.h>

using namespace sycon;

struct state_t {
  MapTable map_table;

  state_t() : map_table({"Ingress.map_table_0", "Ingress.map_table_1"}, 1000) {}
};

std::unique_ptr<state_t> state;

void sycon::nf_init() {
  state = std::make_unique<state_t>();

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
  state->map_table.dump();

  LOG("******** New entry ********");
  state->map_table.put(key, value);
  state->map_table.dump();

  value = 0xcafebabe;

  LOG("******** Entry mod ********");
  state->map_table.put(key, value);
  state->map_table.dump();

  LOG("******** Entry del ********");
  state->map_table.del(key);
  state->map_table.dump();
}

void sycon::nf_exit() {}

void sycon::nf_user_signal_handler() {}

void sycon::nf_args(CLI::App &app) {}

bool sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) { return true; }

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
