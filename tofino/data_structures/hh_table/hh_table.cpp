#include <sycon/sycon.h>

using namespace sycon;

struct state_t {
  HHTable hh_table;

  state_t()
      : hh_table(
            {
                "Ingress.hh_table",
            },
            "Ingress.hh_table_cached_counters",
            {
                "Ingress.hh_table_bloom_filter_row0",
                "Ingress.hh_table_bloom_filter_row1",
                "Ingress.hh_table_bloom_filter_row2",
            },
            {
                "Ingress.hh_table_cms_row0",
                "Ingress.hh_table_cms_row1",
                "Ingress.hh_table_cms_row2",
            },
            "Ingress.hh_table_threshold", "IngressDeparser.hh_table_digest", 1000) {}
};

std::unique_ptr<state_t> state;

void sycon::nf_init() { state = std::make_unique<state_t>(); }

void sycon::nf_exit() {}

void sycon::nf_user_signal_handler() {}

void sycon::nf_args(CLI::App &app) {}

bool sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) { return true; }

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
