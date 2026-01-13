#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  BloomFilter bloom_filter;

  state_t()
      : bloom_filter("bloom_filter",
                     {
                         "Ingress.bloom_filter_row0",
                         "Ingress.bloom_filter_row1",
                         "Ingress.bloom_filter_row2",
                     },
                     10'000) {}
};

state_t *state = nullptr;

void sycon::nf_init() {
  nf_state = std::make_unique<state_t>();
  state    = dynamic_cast<state_t *>(nf_state.get());
}

void sycon::nf_exit() {}

void sycon::nf_user_signal_handler() {}

void sycon::nf_args(CLI::App &app) {}

nf_process_result_t sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  nf_process_result_t result;
  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
