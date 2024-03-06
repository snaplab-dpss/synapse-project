#include <sycon/sycon.h>
#include <unistd.h>

using namespace sycon;

void sycon::nf_init() {
  // No state
}

void sycon::nf_exit() {
  // No state
}

bool sycon::nf_process(time_ns_t now, byte_t *pkt, uint16_t size) {
  cpu_hdr_t *cpu_hdr = (cpu_hdr_t *)packet_consume(pkt, sizeof(cpu_hdr_t));
  cpu_hdr->out_port = SWAP_ENDIAN_16(cfg.out_dev_port);
  return true;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
