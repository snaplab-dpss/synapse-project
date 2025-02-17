#include <sycon/sycon.h>

using namespace sycon;

class IpRoute : public PrimitiveTable {
private:
  struct key_fields_t {
    // Key fields IDs
    bf_rt_id_t vrf;
    bf_rt_id_t dst_addr;
  } key_fields;

  struct {
    // Data field ids
    bf_rt_id_t route_srcMac;
    bf_rt_id_t route_dstMac;
    bf_rt_id_t route_dst_port;

    bf_rt_id_t nat_srcAddr;
    bf_rt_id_t nat_dstAddr;
    bf_rt_id_t nat_dst_port;
  } data_fields;

  struct actions_t {
    // Actions ids
    bf_rt_id_t route;
    bf_rt_id_t nat;
  } actions;

public:
  IpRoute() : PrimitiveTable("SwitchIngress", "ipRoute") {
    init_key({
        {"vrf", &key_fields.vrf},
        {"hdr.ipv4.dst_addr", &key_fields.dst_addr},
    });

    init_actions({
        {"route", &actions.route},
    });

    init_actions({
        {"nat", &actions.nat},
    });

    init_data_with_actions({
        {"srcMac", {actions.route, &data_fields.route_srcMac}},
        {"dstMac", {actions.route, &data_fields.route_dstMac}},
        {"dst_port", {actions.route, &data_fields.route_dst_port}},
    });

    init_data_with_actions({
        {"srcAddr", {actions.nat, &data_fields.nat_srcAddr}},
        {"dstAddr", {actions.nat, &data_fields.nat_dstAddr}},
        {"dst_port", {actions.nat, &data_fields.nat_dst_port}},
    });
  }

public:
  void add_entry_nat(u16 vrf, u32 dst_addr, u32 data_src_addr, u32 data_dst_addr, u16 data_dst_port) {
    key_setup(vrf, dst_addr);
    data_setup_nat(data_src_addr, data_dst_addr, data_dst_port);

    bf_status_t bf_status = table->tableEntryAdd(*session, dev_tgt, *key, *data);
    ASSERT_BF_STATUS(bf_status);
  }

private:
  void key_setup(u16 vrf, u32 dst_addr) {
    table->keyReset(key.get());

    bf_status_t bf_status = key->setValue(key_fields.vrf, static_cast<u64>(vrf));
    ASSERT_BF_STATUS(bf_status);

    bf_status = key->setValue(key_fields.dst_addr, static_cast<u64>(dst_addr));
    ASSERT_BF_STATUS(bf_status);
  }

  void data_setup_nat(u32 src_addr, u32 dst_addr, u16 dst_port) {
    bf_status_t bf_status = table->dataReset(actions.nat, data.get());
    ASSERT_BF_STATUS(bf_status);

    bf_status = data->setValue(data_fields.nat_srcAddr, static_cast<u64>(src_addr));
    ASSERT_BF_STATUS(bf_status);

    bf_status = data->setValue(data_fields.nat_dstAddr, static_cast<u64>(dst_addr));
    ASSERT_BF_STATUS(bf_status);

    bf_status = data->setValue(data_fields.nat_dst_port, static_cast<u64>(dst_port));
    ASSERT_BF_STATUS(bf_status);
  }
};

struct state_t {
  IpRoute ipRoute;
};

state_t *state;

void sycon::nf_init() {
  state = new state_t();

  state->ipRoute.add_entry_nat(0, 1, 2, 3, 4);
  state->ipRoute.dump();
}

void sycon::nf_exit() { delete state; }

void sycon::nf_user_signal_handler() {}

void sycon::nf_args(CLI::App &app) {}

bool sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) { return true; }

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
