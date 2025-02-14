#include <sycon/sycon.h>

using namespace sycon;

struct state_t {
/*@{STATE}@*/
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

bool sycon::nf_process(time_ns_t now, byte_t *pkt, u16 size) {
  cpu_hdr_t *cpu_hdr = (cpu_hdr_t *)packet_consume(pkt, sizeof(cpu_hdr_t));
/*@{NF_PROCESS}@*/
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
