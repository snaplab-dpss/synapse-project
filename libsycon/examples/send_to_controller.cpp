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

struct cpu_hdr_extra_t {
  u32 ingress_dev;
} __attribute__((packed));

nf_process_result_t sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  nf_process_result_t result;
  cpu_hdr_t *cpu_hdr             = packet_consume<cpu_hdr_t>(pkt);
  cpu_hdr_extra_t *cpu_hdr_extra = packet_consume<cpu_hdr_extra_t>(pkt);

  cpu_hdr->egress_dev = cpu_hdr_extra->ingress_dev;

  // packet_hexdump(pkt, size);
  packet_log(cpu_hdr);

  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
