#include <sycon/sycon.h>

using namespace sycon;

class Forwarder : public Table {
public:
  Forwarder() : Table("Ingress", "forwarder") {}

public:
  void add_fwd_entry(u16 ingress_port, u16 out_port) {
    buffer_t key(2);
    key.set(0, 2, ingress_port);

    buffer_t param(2);
    param.set(0, 2, out_port);

    add_entry(key, "fwd", {param});
  }
};

struct state_t {
  Forwarder forwarder;
};

std::unique_ptr<state_t> state;

void sycon::nf_init() {
  state = std::make_unique<state_t>();

  const std::vector<std::pair<u16, u16>> port_connections = {
      {1, 1},   {2, 2},   {3, 3},   {4, 4},   {5, 5},   {6, 6},   {7, 7},   {8, 8},   {9, 9},   {10, 10}, {11, 11},
      {12, 12}, {13, 13}, {14, 14}, {15, 15}, {16, 16}, {17, 17}, {18, 18}, {19, 19}, {20, 20}, {21, 21}, {22, 22},
      {23, 23}, {24, 24}, {25, 25}, {26, 26}, {27, 27}, {28, 28}, {29, 29}, {30, 30}, {31, 31}, {32, 32},
  };

  for (auto &[src_port, dst_port] : port_connections) {
    u16 src_dev_port = asic_get_dev_port(src_port);
    u16 dst_dev_port = asic_get_dev_port(dst_port);
    state->forwarder.add_fwd_entry(src_dev_port, dst_dev_port);
  }
}

void sycon::nf_exit() { state->forwarder.dump(); }

void sycon::nf_args(CLI::App &app) {}

void sycon::nf_user_signal_handler() {}

bool sycon::nf_process(time_ns_t now, u8 *pkt, uint16_t size) { return true; }

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
