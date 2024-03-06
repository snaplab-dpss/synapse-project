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

bool sycon::nf_process(time_ns_t now, byte_t *pkt, u16 size) {
  cpu_hdr_t *cpu_hdr = (cpu_hdr_t *)packet_consume(pkt, sizeof(cpu_hdr_t));

  cpu_hdr->out_port = SWAP_ENDIAN_16(nf_config.out_dev_port);

  // packet_hexdump(pkt, size);
  packet_log(cpu_hdr);

  return true;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
