#include <sycon/sycon.h>

using namespace sycon;

struct state_t {
  // Creating table table_1074013032_65...
};

std::unique_ptr<state_t> state;

void sycon::nf_init() {
  state = std::make_unique<state_t>();
  // BDD node 0:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074012760)[(w64 0) -> (w64 1074013032)])
  // Module TableAllocate
  // BDD node 1:vector_borrow(vector:(w64 1074013032), index:(w32 0), val_out:(w64 1074012696)[ -> (w64 1074026928)])
  // Module TableLookup
  // TableLookup
  // BDD node 2:vector_return(vector:(w64 1074013032), index:(w32 0), value:(w64 1074026928)[(w16 1)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 3:vector_borrow(vector:(w64 1074013032), index:(w32 1), val_out:(w64 1074012696)[ -> (w64 1074026952)])
  // Module TableLookup
  // TableLookup
  // BDD node 4:vector_return(vector:(w64 1074013032), index:(w32 1), value:(w64 1074026952)[(w16 0)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 5:vector_borrow(vector:(w64 1074013032), index:(w32 2), val_out:(w64 1074012696)[ -> (w64 1074026976)])
  // Module TableLookup
  // TableLookup
  // BDD node 6:vector_return(vector:(w64 1074013032), index:(w32 2), value:(w64 1074026976)[(w16 3)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 7:vector_borrow(vector:(w64 1074013032), index:(w32 3), val_out:(w64 1074012696)[ -> (w64 1074027000)])
  // Module TableLookup
  // TableLookup
  // BDD node 8:vector_return(vector:(w64 1074013032), index:(w32 3), value:(w64 1074027000)[(w16 2)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 9:vector_borrow(vector:(w64 1074013032), index:(w32 4), val_out:(w64 1074012696)[ -> (w64 1074027024)])
  // Module TableLookup
  // TableLookup
  // BDD node 10:vector_return(vector:(w64 1074013032), index:(w32 4), value:(w64 1074027024)[(w16 5)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 11:vector_borrow(vector:(w64 1074013032), index:(w32 5), val_out:(w64 1074012696)[ -> (w64 1074027048)])
  // Module TableLookup
  // TableLookup
  // BDD node 12:vector_return(vector:(w64 1074013032), index:(w32 5), value:(w64 1074027048)[(w16 4)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 13:vector_borrow(vector:(w64 1074013032), index:(w32 6), val_out:(w64 1074012696)[ -> (w64 1074027072)])
  // Module TableLookup
  // TableLookup
  // BDD node 14:vector_return(vector:(w64 1074013032), index:(w32 6), value:(w64 1074027072)[(w16 7)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 15:vector_borrow(vector:(w64 1074013032), index:(w32 7), val_out:(w64 1074012696)[ -> (w64 1074027096)])
  // Module TableLookup
  // TableLookup
  // BDD node 16:vector_return(vector:(w64 1074013032), index:(w32 7), value:(w64 1074027096)[(w16 6)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 17:vector_borrow(vector:(w64 1074013032), index:(w32 8), val_out:(w64 1074012696)[ -> (w64 1074027120)])
  // Module TableLookup
  // TableLookup
  // BDD node 18:vector_return(vector:(w64 1074013032), index:(w32 8), value:(w64 1074027120)[(w16 9)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 19:vector_borrow(vector:(w64 1074013032), index:(w32 9), val_out:(w64 1074012696)[ -> (w64 1074027144)])
  // Module TableLookup
  // TableLookup
  // BDD node 20:vector_return(vector:(w64 1074013032), index:(w32 9), value:(w64 1074027144)[(w16 8)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 21:vector_borrow(vector:(w64 1074013032), index:(w32 10), val_out:(w64 1074012696)[ -> (w64 1074027168)])
  // Module TableLookup
  // TableLookup
  // BDD node 22:vector_return(vector:(w64 1074013032), index:(w32 10), value:(w64 1074027168)[(w16 11)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 23:vector_borrow(vector:(w64 1074013032), index:(w32 11), val_out:(w64 1074012696)[ -> (w64 1074027192)])
  // Module TableLookup
  // TableLookup
  // BDD node 24:vector_return(vector:(w64 1074013032), index:(w32 11), value:(w64 1074027192)[(w16 10)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 25:vector_borrow(vector:(w64 1074013032), index:(w32 12), val_out:(w64 1074012696)[ -> (w64 1074027216)])
  // Module TableLookup
  // TableLookup
  // BDD node 26:vector_return(vector:(w64 1074013032), index:(w32 12), value:(w64 1074027216)[(w16 13)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 27:vector_borrow(vector:(w64 1074013032), index:(w32 13), val_out:(w64 1074012696)[ -> (w64 1074027240)])
  // Module TableLookup
  // TableLookup
  // BDD node 28:vector_return(vector:(w64 1074013032), index:(w32 13), value:(w64 1074027240)[(w16 12)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 29:vector_borrow(vector:(w64 1074013032), index:(w32 14), val_out:(w64 1074012696)[ -> (w64 1074027264)])
  // Module TableLookup
  // TableLookup
  // BDD node 30:vector_return(vector:(w64 1074013032), index:(w32 14), value:(w64 1074027264)[(w16 15)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 31:vector_borrow(vector:(w64 1074013032), index:(w32 15), val_out:(w64 1074012696)[ -> (w64 1074027288)])
  // Module TableLookup
  // TableLookup
  // BDD node 32:vector_return(vector:(w64 1074013032), index:(w32 15), value:(w64 1074027288)[(w16 14)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 33:vector_borrow(vector:(w64 1074013032), index:(w32 16), val_out:(w64 1074012696)[ -> (w64 1074027312)])
  // Module TableLookup
  // TableLookup
  // BDD node 34:vector_return(vector:(w64 1074013032), index:(w32 16), value:(w64 1074027312)[(w16 17)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 35:vector_borrow(vector:(w64 1074013032), index:(w32 17), val_out:(w64 1074012696)[ -> (w64 1074027336)])
  // Module TableLookup
  // TableLookup
  // BDD node 36:vector_return(vector:(w64 1074013032), index:(w32 17), value:(w64 1074027336)[(w16 16)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 37:vector_borrow(vector:(w64 1074013032), index:(w32 18), val_out:(w64 1074012696)[ -> (w64 1074027360)])
  // Module TableLookup
  // TableLookup
  // BDD node 38:vector_return(vector:(w64 1074013032), index:(w32 18), value:(w64 1074027360)[(w16 19)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 39:vector_borrow(vector:(w64 1074013032), index:(w32 19), val_out:(w64 1074012696)[ -> (w64 1074027384)])
  // Module TableLookup
  // TableLookup
  // BDD node 40:vector_return(vector:(w64 1074013032), index:(w32 19), value:(w64 1074027384)[(w16 18)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 41:vector_borrow(vector:(w64 1074013032), index:(w32 20), val_out:(w64 1074012696)[ -> (w64 1074027408)])
  // Module TableLookup
  // TableLookup
  // BDD node 42:vector_return(vector:(w64 1074013032), index:(w32 20), value:(w64 1074027408)[(w16 21)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 43:vector_borrow(vector:(w64 1074013032), index:(w32 21), val_out:(w64 1074012696)[ -> (w64 1074027432)])
  // Module TableLookup
  // TableLookup
  // BDD node 44:vector_return(vector:(w64 1074013032), index:(w32 21), value:(w64 1074027432)[(w16 20)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 45:vector_borrow(vector:(w64 1074013032), index:(w32 22), val_out:(w64 1074012696)[ -> (w64 1074027456)])
  // Module TableLookup
  // TableLookup
  // BDD node 46:vector_return(vector:(w64 1074013032), index:(w32 22), value:(w64 1074027456)[(w16 23)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 47:vector_borrow(vector:(w64 1074013032), index:(w32 23), val_out:(w64 1074012696)[ -> (w64 1074027480)])
  // Module TableLookup
  // TableLookup
  // BDD node 48:vector_return(vector:(w64 1074013032), index:(w32 23), value:(w64 1074027480)[(w16 22)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 49:vector_borrow(vector:(w64 1074013032), index:(w32 24), val_out:(w64 1074012696)[ -> (w64 1074027504)])
  // Module TableLookup
  // TableLookup
  // BDD node 50:vector_return(vector:(w64 1074013032), index:(w32 24), value:(w64 1074027504)[(w16 25)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 51:vector_borrow(vector:(w64 1074013032), index:(w32 25), val_out:(w64 1074012696)[ -> (w64 1074027528)])
  // Module TableLookup
  // TableLookup
  // BDD node 52:vector_return(vector:(w64 1074013032), index:(w32 25), value:(w64 1074027528)[(w16 24)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 53:vector_borrow(vector:(w64 1074013032), index:(w32 26), val_out:(w64 1074012696)[ -> (w64 1074027552)])
  // Module TableLookup
  // TableLookup
  // BDD node 54:vector_return(vector:(w64 1074013032), index:(w32 26), value:(w64 1074027552)[(w16 27)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 55:vector_borrow(vector:(w64 1074013032), index:(w32 27), val_out:(w64 1074012696)[ -> (w64 1074027576)])
  // Module TableLookup
  // TableLookup
  // BDD node 56:vector_return(vector:(w64 1074013032), index:(w32 27), value:(w64 1074027576)[(w16 26)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 57:vector_borrow(vector:(w64 1074013032), index:(w32 28), val_out:(w64 1074012696)[ -> (w64 1074027600)])
  // Module TableLookup
  // TableLookup
  // BDD node 58:vector_return(vector:(w64 1074013032), index:(w32 28), value:(w64 1074027600)[(w16 29)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 59:vector_borrow(vector:(w64 1074013032), index:(w32 29), val_out:(w64 1074012696)[ -> (w64 1074027624)])
  // Module TableLookup
  // TableLookup
  // BDD node 60:vector_return(vector:(w64 1074013032), index:(w32 29), value:(w64 1074027624)[(w16 28)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 61:vector_borrow(vector:(w64 1074013032), index:(w32 30), val_out:(w64 1074012696)[ -> (w64 1074027648)])
  // Module TableLookup
  // TableLookup
  // BDD node 62:vector_return(vector:(w64 1074013032), index:(w32 30), value:(w64 1074027648)[(w16 31)])
  // Module TableUpdate
  // TableUpdate
  // BDD node 63:vector_borrow(vector:(w64 1074013032), index:(w32 31), val_out:(w64 1074012696)[ -> (w64 1074027672)])
  // Module TableLookup
  // TableLookup
  // BDD node 64:vector_return(vector:(w64 1074013032), index:(w32 31), value:(w64 1074027672)[(w16 30)])
  // Module TableUpdate
  // TableUpdate

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

bool sycon::nf_process(time_ns_t now, byte_t *pkt, u16 size) {
  cpu_hdr_t *cpu_hdr = (cpu_hdr_t *)packet_consume(pkt, sizeof(cpu_hdr_t));

}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
