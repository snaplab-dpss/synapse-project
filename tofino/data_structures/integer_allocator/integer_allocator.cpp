#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IntegerAllocator integer_allocator;

  state_t()
      : integer_allocator("integer_allocator", {"Ingress.integer_allocator_allocated_indexes"}, 1000, "Ingress.integer_allocator_head_reg",
                          "Ingress.integer_allocator_tail_reg", "Ingress.integer_allocator_indexes_reg", "Ingress.integer_allocator_pending_reg",
                          "IngressDeparser.integer_allocator_digest") {}
};

state_t *state = nullptr;

void sycon::nf_init() {
  nf_state = std::make_unique<state_t>();
  state    = dynamic_cast<state_t *>(nf_state.get());
}

void sycon::nf_exit() {}

void sycon::nf_user_signal_handler() {}

void sycon::nf_args(CLI::App &app) {}

struct cpu_hdr_extra_t {
  u16 ingress_dev;
} __attribute__((packed));

nf_process_result_t sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  nf_process_result_t result;
  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
