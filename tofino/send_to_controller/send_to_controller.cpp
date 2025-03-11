#include <sycon/sycon.h>

using namespace sycon;

struct state_t {
  std::unordered_map<u16, u16> port_connections;
};

state_t *state;

void sycon::nf_init() {
  state                   = new state_t();
  state->port_connections = {
      {1, 1},   {2, 2},   {3, 3},   {4, 4},   {5, 5},   {6, 6},   {7, 7},   {8, 8},   {9, 9},   {10, 10}, {11, 11},
      {12, 12}, {13, 13}, {14, 14}, {15, 15}, {16, 16}, {17, 17}, {18, 18}, {19, 19}, {20, 20}, {21, 21}, {22, 22},
      {23, 23}, {24, 24}, {25, 25}, {26, 26}, {27, 27}, {28, 28}, {29, 29}, {30, 30}, {31, 31}, {32, 32},
  };
}

void sycon::nf_exit() { delete state; }

void sycon::nf_args(CLI::App &app) {}

void sycon::nf_user_signal_handler() {}

struct cpu_hdr_extra_t {
  u16 ingress_dev;
} __attribute__((packed));

bool sycon::nf_process(time_ns_t now, u8 *pkt, uint16_t size) {
  cpu_hdr_t *cpu_hdr             = packet_consume<cpu_hdr_t>(pkt);
  cpu_hdr_extra_t *cpu_hdr_extra = packet_consume<cpu_hdr_extra_t>(pkt);

  u16 in_dev_port  = bswap16(cpu_hdr_extra->ingress_dev);
  u16 in_port      = asic_get_front_panel_port_from_dev_port(in_dev_port);
  u16 out_port     = state->port_connections.at(in_port);
  u16 out_dev_port = asic_get_dev_port(out_port);

  DEBUG("In dev port %u, in port %u, out port %u, out dev port %u", in_dev_port, in_port, out_port, out_dev_port);

  cpu_hdr->egress_dev = bswap16(out_dev_port);
  return true;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
