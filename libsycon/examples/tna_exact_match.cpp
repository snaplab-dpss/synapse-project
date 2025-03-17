#include <sycon/sycon.h>

using namespace sycon;

class IpRoute : public Table {
private:
  const std::string route_action_name;
  const std::string nat_action_name;

public:
  IpRoute() : Table("SwitchIngress.ipRoute"), route_action_name("route"), nat_action_name("nat") {}

public:
  void add_entry_nat(u16 vrf, u32 dst_addr, u32 data_src_addr, u32 data_dst_addr, u16 data_dst_port) {
    buffer_t key(6);
    key.set(0, 2, vrf);
    key.set(2, 4, dst_addr);

    buffer_t data(10);
    data.set(0, 4, data_src_addr);
    data.set(4, 4, data_dst_addr);
    data.set(8, 2, data_dst_port);

    Table::add_entry(key, nat_action_name, {data});
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
