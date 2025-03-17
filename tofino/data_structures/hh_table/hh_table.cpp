#include <sycon/sycon.h>

using namespace sycon;

struct state_t {
  ForwardNFDev forward_nf_dev;
  HHTable hh_table;

  state_t()
      : forward_nf_dev(), hh_table({"Ingress.map_table"}, "Ingress.hh_table_cached_counters", "Ingress.hh_table_threshold",
                                   "IngressDeparser.hh_table_digest", 1000) {}
};

std::unique_ptr<state_t> state;

void sycon::nf_init() {
  state = std::make_unique<state_t>();
  state->forward_nf_dev.add_entry(0, asic_get_dev_port(3));
  state->forward_nf_dev.add_entry(1, asic_get_dev_port(4));
  state->forward_nf_dev.add_entry(2, asic_get_dev_port(5));
  state->forward_nf_dev.add_entry(3, asic_get_dev_port(6));
  state->forward_nf_dev.add_entry(4, asic_get_dev_port(7));
  state->forward_nf_dev.add_entry(5, asic_get_dev_port(8));
  state->forward_nf_dev.add_entry(6, asic_get_dev_port(9));
  state->forward_nf_dev.add_entry(7, asic_get_dev_port(10));
  state->forward_nf_dev.add_entry(8, asic_get_dev_port(11));
  state->forward_nf_dev.add_entry(9, asic_get_dev_port(12));
  state->forward_nf_dev.add_entry(10, asic_get_dev_port(13));
  state->forward_nf_dev.add_entry(11, asic_get_dev_port(14));
  state->forward_nf_dev.add_entry(12, asic_get_dev_port(15));
  state->forward_nf_dev.add_entry(13, asic_get_dev_port(16));
  state->forward_nf_dev.add_entry(14, asic_get_dev_port(17));
  state->forward_nf_dev.add_entry(15, asic_get_dev_port(18));
  state->forward_nf_dev.add_entry(16, asic_get_dev_port(19));
  state->forward_nf_dev.add_entry(17, asic_get_dev_port(20));
  state->forward_nf_dev.add_entry(18, asic_get_dev_port(21));
  state->forward_nf_dev.add_entry(19, asic_get_dev_port(22));
  state->forward_nf_dev.add_entry(20, asic_get_dev_port(23));
  state->forward_nf_dev.add_entry(21, asic_get_dev_port(24));
  state->forward_nf_dev.add_entry(22, asic_get_dev_port(25));
  state->forward_nf_dev.add_entry(23, asic_get_dev_port(26));
  state->forward_nf_dev.add_entry(24, asic_get_dev_port(27));
  state->forward_nf_dev.add_entry(25, asic_get_dev_port(28));
  state->forward_nf_dev.add_entry(26, asic_get_dev_port(29));
  state->forward_nf_dev.add_entry(27, asic_get_dev_port(30));
  state->forward_nf_dev.add_entry(28, asic_get_dev_port(31));
  state->forward_nf_dev.add_entry(29, asic_get_dev_port(32));
}

void sycon::nf_exit() {}

void sycon::nf_user_signal_handler() {}

void sycon::nf_args(CLI::App &app) {}

bool sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) { return true; }

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
