#include <sycon/sycon.h>
#include <unistd.h>

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
}

void sycon::nf_cleanup() {
  state->table_with_timeout->dump();
  delete state;
}

bool sycon::nf_process(time_ns_t now, byte_t *pkt, uint16_t size) {
  return true;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
