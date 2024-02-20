#include <sycon/sycon.h>

using namespace sycon;

bf_status_t table_with_timeout_expiration_callback(
    const bf_rt_target_t &target, const bfrt::BfRtTableKey *key,
    const void *cookie) {
  const cookie_t *c = static_cast<const cookie_t *>(cookie);
  auto table = c->table;

  begin_transaction();
  table->del(key);
  end_transaction();

  return BF_SUCCESS;
}

struct state_t {
  std::unique_ptr<SimpleTable> table_with_timeout;
};

state_t *state;

void sycon::nf_init() {
  state = new state_t();
  state->table_with_timeout =
      SimpleTable::build("Ingress", "table_with_timeout", args.expiration_time,
                         table_with_timeout_expiration_callback);
  state->table_with_timeout->enable_expirations();
}

void sycon::nf_cleanup() {
  printf("Running cleanup!\n");

  // state->table_with_timeout->dump();

  delete state;
}

bool sycon::nf_process(time_ns_t now, byte_t *pkt, uint16_t size) {
  return true;
}

int main(int argc, char **argv) {
  parse_args(argc, argv);
  init_switchd();
  configure_ports();
  register_pcie_pkt_ops();
  nf_setup();

  if (args.run_ucli) {
    run_cli();
  }

  return 0;
}
