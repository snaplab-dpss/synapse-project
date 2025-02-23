#include <sycon/sycon.h>

using namespace sycon;

struct state_t {
  DchainTable dchain_table;

  state_t() : dchain_table("Ingress", {"dchain_table_0"}) {}
};

std::unique_ptr<state_t> state;

void sycon::nf_init() {
  state = std::make_unique<state_t>();

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
  state->dchain_table.free_index(new_index);
  state->dchain_table.dump();
}

void sycon::nf_exit() {}

void sycon::nf_user_signal_handler() {}

void sycon::nf_args(CLI::App &app) {}

bool sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) { return true; }

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
