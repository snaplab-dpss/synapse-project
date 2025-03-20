#include <sycon/sycon.h>

using namespace sycon;

struct state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardNFDev forward_nf_dev;
/*@{STATE_FIELDS}@*/
  state_t()
    : ingress_port_to_nf_dev(),
      forward_nf_dev()/*@{STATE_MEMBER_INIT_LIST}@*/
    {}
};

std::unique_ptr<state_t> state;

void sycon::nf_init() {
  state = std::make_unique<state_t>();
/*@{NF_INIT}@*/
}

void sycon::nf_exit() {
/*@{NF_EXIT}@*/
}

void sycon::nf_args(CLI::App &app) {
/*@{NF_ARGS}@*/
}

void sycon::nf_user_signal_handler() {
/*@{NF_USER_SIGNAL_HANDLER}@*/
}

struct cpu_hdr_extra_t {
/*@{CPU_HDR_EXTRA}@*/
} __attribute__((packed));

bool sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  bool forward = true;
  bool trigger_update_ipv4_tcpudp_checksums = false;
  void* l3_hdr = nullptr;
  void* l4_hdr = nullptr;

  cpu_hdr_t *cpu_hdr = packet_consume<cpu_hdr_t>(pkt);
  cpu_hdr_extra_t *cpu_hdr_extra = packet_consume<cpu_hdr_extra_t>(pkt);
  LOG_DEBUG("[t=%lu] New packet (size=%u, code_path=%d)\n", now, size, bswap16(cpu_hdr->code_path));

/*@{NF_PROCESS}@*/

  if (trigger_update_ipv4_tcpudp_checksums) {
    update_ipv4_tcpudp_checksums(l3_hdr, l4_hdr);
  }

  return forward;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
