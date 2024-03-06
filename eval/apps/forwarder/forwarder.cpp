#include <sycon/sycon.h>
#include <unistd.h>

using namespace sycon;

class Forwarder : public Table {
private:
  struct key_fields_t {
    // Key fields IDs
    bf_rt_id_t ingress_port;
  } key_fields;

  struct {
    // Data field ids
    bf_rt_id_t port;
  } data_fields;

  struct actions_t {
    // Actions ids
    bf_rt_id_t fwd;
    bf_rt_id_t drop;
  } actions;

public:
  Forwarder() : Table("Ingress", "forwarder") {
    init_key({
        {"ig_intr_md.ingress_port", &key_fields.ingress_port},
    });

    init_actions({
        {"fwd", &actions.fwd},
    });

    init_actions({
        {"drop", &actions.drop},
    });

    init_data_with_actions({
        {"port", {actions.fwd, &data_fields.port}},
    });
  }

public:
  void add_fwd_entry(uint16_t ingress_port, uint16_t out_port) {
    key_setup(ingress_port);
    data_setup_fwd(out_port);

    auto bf_status = table->tableEntryAdd(*session, dev_tgt, *key, *data);
    ASSERT_BF_STATUS(bf_status);
  }

private:
  void key_setup(uint16_t ingress_port) {
    table->keyReset(key.get());

    auto bf_status = key->setValue(key_fields.ingress_port,
                                   static_cast<uint64_t>(ingress_port));
    ASSERT_BF_STATUS(bf_status);
  }

  void data_setup_fwd(uint16_t port) {
    auto bf_status = table->dataReset(actions.fwd, data.get());
    ASSERT_BF_STATUS(bf_status);

    bf_status = data->setValue(data_fields.port, static_cast<uint64_t>(port));
    ASSERT_BF_STATUS(bf_status);
  }
};

struct state_t {
  Forwarder forwarder;
};

state_t *state;

void sycon::nf_init() {
  state = new state_t();
  state->forwarder.add_fwd_entry(cfg.in_dev_port, cfg.out_dev_port);
}

void sycon::nf_exit() {
  state->forwarder.dump();
  delete state;
}

bool sycon::nf_process(time_ns_t now, byte_t *pkt, uint16_t size) {
  return true;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
