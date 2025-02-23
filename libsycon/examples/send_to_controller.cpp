#include <sycon/sycon.h>
#include <unistd.h>

using namespace sycon;

void sycon::nf_init() {
  // No state
}

void sycon::nf_exit() {
  // No state
}

void sycon::nf_user_signal_handler() {}

void sycon::nf_args(CLI::App &app) {}

bool sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  cpu_hdr_t *cpu_hdr = packet_consume<cpu_hdr_t>(pkt);

  cpu_hdr->egress_dev = cpu_hdr->ingress_dev;

  // packet_hexdump(pkt, size);
  packet_log(cpu_hdr);

  return true;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
