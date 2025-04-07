#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  HHTable hh_table;

  state_t()
      : hh_table("hh_table",
                 {
                     "Ingress.hh_table",
                 },
                 "Ingress.hh_table_cached_counters",
                 {
                     "Ingress.hh_table_cms_row0",
                     "Ingress.hh_table_cms_row1",
                     "Ingress.hh_table_cms_row2",
                 },
                 "Ingress.hh_table_threshold", "IngressDeparser.hh_table_digest", 1000) {}
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
