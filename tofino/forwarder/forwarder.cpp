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

    auto bf_status = key->setValue(key_fields.ingress_port, static_cast<uint64_t>(ingress_port));
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
  state                                             = new state_t();
  std::vector<std::pair<u16, u16>> port_connections = {
      {1, 1},   {2, 2},   {3, 3},   {4, 4},   {5, 5},   {6, 6},   {7, 7},   {8, 8},   {9, 9},   {10, 10}, {11, 11},
      {12, 12}, {13, 13}, {14, 14}, {15, 15}, {16, 16}, {17, 17}, {18, 18}, {19, 19}, {20, 20}, {21, 21}, {22, 22},
      {23, 23}, {24, 24}, {25, 25}, {26, 26}, {27, 27}, {28, 28}, {29, 29}, {30, 30}, {31, 31}, {32, 32},
  };

  for (auto &[src_port, dst_port] : port_connections) {
    u16 src_dev_port = get_asic_dev_port(src_port);
    u16 dst_dev_port = get_asic_dev_port(dst_port);
    state->forwarder.add_fwd_entry(src_dev_port, dst_dev_port);
  }
}

void sycon::nf_exit() {
  state->forwarder.dump();
  delete state;
}

void sycon::nf_args(CLI::App &app) {}

void sycon::nf_user_signal_handler() {}

bool sycon::nf_process(time_ns_t now, byte_t *pkt, uint16_t size) { return true; }

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
