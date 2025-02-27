#include <sycon/sycon.h>

using namespace sycon;

struct state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardNFDev forward_nf_dev;
  VectorTable vector_table_1074012584;

  state_t()
    : ingress_port_to_nf_dev(),
      forward_nf_dev(),
      vector_table_1074012584("Ingress", {"vector_table_1074012584_65",})
    {}
};

std::unique_ptr<state_t> state;

void sycon::nf_init() {
  state = std::make_unique<state_t>();
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(3), 0);
  state->forward_nf_dev.add_entry(0, asic_get_dev_port(3));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(4), 1);
  state->forward_nf_dev.add_entry(1, asic_get_dev_port(4));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(5), 2);
  state->forward_nf_dev.add_entry(2, asic_get_dev_port(5));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(6), 3);
  state->forward_nf_dev.add_entry(3, asic_get_dev_port(6));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(7), 4);
  state->forward_nf_dev.add_entry(4, asic_get_dev_port(7));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(8), 5);
  state->forward_nf_dev.add_entry(5, asic_get_dev_port(8));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(9), 6);
  state->forward_nf_dev.add_entry(6, asic_get_dev_port(9));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(10), 7);
  state->forward_nf_dev.add_entry(7, asic_get_dev_port(10));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(11), 8);
  state->forward_nf_dev.add_entry(8, asic_get_dev_port(11));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(12), 9);
  state->forward_nf_dev.add_entry(9, asic_get_dev_port(12));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(13), 10);
  state->forward_nf_dev.add_entry(10, asic_get_dev_port(13));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(14), 11);
  state->forward_nf_dev.add_entry(11, asic_get_dev_port(14));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(15), 12);
  state->forward_nf_dev.add_entry(12, asic_get_dev_port(15));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(16), 13);
  state->forward_nf_dev.add_entry(13, asic_get_dev_port(16));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(17), 14);
  state->forward_nf_dev.add_entry(14, asic_get_dev_port(17));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(18), 15);
  state->forward_nf_dev.add_entry(15, asic_get_dev_port(18));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(19), 16);
  state->forward_nf_dev.add_entry(16, asic_get_dev_port(19));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(20), 17);
  state->forward_nf_dev.add_entry(17, asic_get_dev_port(20));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(21), 18);
  state->forward_nf_dev.add_entry(18, asic_get_dev_port(21));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(22), 19);
  state->forward_nf_dev.add_entry(19, asic_get_dev_port(22));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(23), 20);
  state->forward_nf_dev.add_entry(20, asic_get_dev_port(23));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(24), 21);
  state->forward_nf_dev.add_entry(21, asic_get_dev_port(24));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(25), 22);
  state->forward_nf_dev.add_entry(22, asic_get_dev_port(25));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(26), 23);
  state->forward_nf_dev.add_entry(23, asic_get_dev_port(26));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(27), 24);
  state->forward_nf_dev.add_entry(24, asic_get_dev_port(27));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(28), 25);
  state->forward_nf_dev.add_entry(25, asic_get_dev_port(28));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(29), 26);
  state->forward_nf_dev.add_entry(26, asic_get_dev_port(29));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(30), 27);
  state->forward_nf_dev.add_entry(27, asic_get_dev_port(30));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(31), 28);
  state->forward_nf_dev.add_entry(28, asic_get_dev_port(31));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(32), 29);
  state->forward_nf_dev.add_entry(29, asic_get_dev_port(32));
  // BDD node 0:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074012312)[(w64 0) -> (w64 1074012584)])
  // Module VectorTableAllocate
  // BDD node 1:vector_borrow(vector:(w64 1074012584), index:(w32 0), val_out:(w64 1074012248)[ -> (w64 1074026480)])
  // Module Ignore
  // BDD node 2:vector_return(vector:(w64 1074012584), index:(w32 0), value:(w64 1074026480)[(w16 1)])
  // Module VectorTableUpdate
  buffer_t value_0(2);
  value_0[0] = 0;
  value_0[1] = 1;
  state->vector_table_1074012584.write(0, value_0);
  // BDD node 3:vector_borrow(vector:(w64 1074012584), index:(w32 1), val_out:(w64 1074012248)[ -> (w64 1074026504)])
  // Module Ignore
  // BDD node 4:vector_return(vector:(w64 1074012584), index:(w32 1), value:(w64 1074026504)[(w16 0)])
  // Module VectorTableUpdate
  buffer_t value_1(2);
  value_1[0] = 0;
  value_1[1] = 0;
  state->vector_table_1074012584.write(1, value_1);
  // BDD node 5:vector_borrow(vector:(w64 1074012584), index:(w32 2), val_out:(w64 1074012248)[ -> (w64 1074026528)])
  // Module Ignore
  // BDD node 6:vector_return(vector:(w64 1074012584), index:(w32 2), value:(w64 1074026528)[(w16 3)])
  // Module VectorTableUpdate
  buffer_t value_2(2);
  value_2[0] = 0;
  value_2[1] = 3;
  state->vector_table_1074012584.write(2, value_2);
  // BDD node 7:vector_borrow(vector:(w64 1074012584), index:(w32 3), val_out:(w64 1074012248)[ -> (w64 1074026552)])
  // Module Ignore
  // BDD node 8:vector_return(vector:(w64 1074012584), index:(w32 3), value:(w64 1074026552)[(w16 2)])
  // Module VectorTableUpdate
  buffer_t value_3(2);
  value_3[0] = 0;
  value_3[1] = 2;
  state->vector_table_1074012584.write(3, value_3);
  // BDD node 9:vector_borrow(vector:(w64 1074012584), index:(w32 4), val_out:(w64 1074012248)[ -> (w64 1074026576)])
  // Module Ignore
  // BDD node 10:vector_return(vector:(w64 1074012584), index:(w32 4), value:(w64 1074026576)[(w16 5)])
  // Module VectorTableUpdate
  buffer_t value_4(2);
  value_4[0] = 0;
  value_4[1] = 5;
  state->vector_table_1074012584.write(4, value_4);
  // BDD node 11:vector_borrow(vector:(w64 1074012584), index:(w32 5), val_out:(w64 1074012248)[ -> (w64 1074026600)])
  // Module Ignore
  // BDD node 12:vector_return(vector:(w64 1074012584), index:(w32 5), value:(w64 1074026600)[(w16 4)])
  // Module VectorTableUpdate
  buffer_t value_5(2);
  value_5[0] = 0;
  value_5[1] = 4;
  state->vector_table_1074012584.write(5, value_5);
  // BDD node 13:vector_borrow(vector:(w64 1074012584), index:(w32 6), val_out:(w64 1074012248)[ -> (w64 1074026624)])
  // Module Ignore
  // BDD node 14:vector_return(vector:(w64 1074012584), index:(w32 6), value:(w64 1074026624)[(w16 7)])
  // Module VectorTableUpdate
  buffer_t value_6(2);
  value_6[0] = 0;
  value_6[1] = 7;
  state->vector_table_1074012584.write(6, value_6);
  // BDD node 15:vector_borrow(vector:(w64 1074012584), index:(w32 7), val_out:(w64 1074012248)[ -> (w64 1074026648)])
  // Module Ignore
  // BDD node 16:vector_return(vector:(w64 1074012584), index:(w32 7), value:(w64 1074026648)[(w16 6)])
  // Module VectorTableUpdate
  buffer_t value_7(2);
  value_7[0] = 0;
  value_7[1] = 6;
  state->vector_table_1074012584.write(7, value_7);
  // BDD node 17:vector_borrow(vector:(w64 1074012584), index:(w32 8), val_out:(w64 1074012248)[ -> (w64 1074026672)])
  // Module Ignore
  // BDD node 18:vector_return(vector:(w64 1074012584), index:(w32 8), value:(w64 1074026672)[(w16 9)])
  // Module VectorTableUpdate
  buffer_t value_8(2);
  value_8[0] = 0;
  value_8[1] = 9;
  state->vector_table_1074012584.write(8, value_8);
  // BDD node 19:vector_borrow(vector:(w64 1074012584), index:(w32 9), val_out:(w64 1074012248)[ -> (w64 1074026696)])
  // Module Ignore
  // BDD node 20:vector_return(vector:(w64 1074012584), index:(w32 9), value:(w64 1074026696)[(w16 8)])
  // Module VectorTableUpdate
  buffer_t value_9(2);
  value_9[0] = 0;
  value_9[1] = 8;
  state->vector_table_1074012584.write(9, value_9);
  // BDD node 21:vector_borrow(vector:(w64 1074012584), index:(w32 10), val_out:(w64 1074012248)[ -> (w64 1074026720)])
  // Module Ignore
  // BDD node 22:vector_return(vector:(w64 1074012584), index:(w32 10), value:(w64 1074026720)[(w16 11)])
  // Module VectorTableUpdate
  buffer_t value_10(2);
  value_10[0] = 0;
  value_10[1] = 11;
  state->vector_table_1074012584.write(10, value_10);
  // BDD node 23:vector_borrow(vector:(w64 1074012584), index:(w32 11), val_out:(w64 1074012248)[ -> (w64 1074026744)])
  // Module Ignore
  // BDD node 24:vector_return(vector:(w64 1074012584), index:(w32 11), value:(w64 1074026744)[(w16 10)])
  // Module VectorTableUpdate
  buffer_t value_11(2);
  value_11[0] = 0;
  value_11[1] = 10;
  state->vector_table_1074012584.write(11, value_11);
  // BDD node 25:vector_borrow(vector:(w64 1074012584), index:(w32 12), val_out:(w64 1074012248)[ -> (w64 1074026768)])
  // Module Ignore
  // BDD node 26:vector_return(vector:(w64 1074012584), index:(w32 12), value:(w64 1074026768)[(w16 13)])
  // Module VectorTableUpdate
  buffer_t value_12(2);
  value_12[0] = 0;
  value_12[1] = 13;
  state->vector_table_1074012584.write(12, value_12);
  // BDD node 27:vector_borrow(vector:(w64 1074012584), index:(w32 13), val_out:(w64 1074012248)[ -> (w64 1074026792)])
  // Module Ignore
  // BDD node 28:vector_return(vector:(w64 1074012584), index:(w32 13), value:(w64 1074026792)[(w16 12)])
  // Module VectorTableUpdate
  buffer_t value_13(2);
  value_13[0] = 0;
  value_13[1] = 12;
  state->vector_table_1074012584.write(13, value_13);
  // BDD node 29:vector_borrow(vector:(w64 1074012584), index:(w32 14), val_out:(w64 1074012248)[ -> (w64 1074026816)])
  // Module Ignore
  // BDD node 30:vector_return(vector:(w64 1074012584), index:(w32 14), value:(w64 1074026816)[(w16 15)])
  // Module VectorTableUpdate
  buffer_t value_14(2);
  value_14[0] = 0;
  value_14[1] = 15;
  state->vector_table_1074012584.write(14, value_14);
  // BDD node 31:vector_borrow(vector:(w64 1074012584), index:(w32 15), val_out:(w64 1074012248)[ -> (w64 1074026840)])
  // Module Ignore
  // BDD node 32:vector_return(vector:(w64 1074012584), index:(w32 15), value:(w64 1074026840)[(w16 14)])
  // Module VectorTableUpdate
  buffer_t value_15(2);
  value_15[0] = 0;
  value_15[1] = 14;
  state->vector_table_1074012584.write(15, value_15);
  // BDD node 33:vector_borrow(vector:(w64 1074012584), index:(w32 16), val_out:(w64 1074012248)[ -> (w64 1074026864)])
  // Module Ignore
  // BDD node 34:vector_return(vector:(w64 1074012584), index:(w32 16), value:(w64 1074026864)[(w16 17)])
  // Module VectorTableUpdate
  buffer_t value_16(2);
  value_16[0] = 0;
  value_16[1] = 17;
  state->vector_table_1074012584.write(16, value_16);
  // BDD node 35:vector_borrow(vector:(w64 1074012584), index:(w32 17), val_out:(w64 1074012248)[ -> (w64 1074026888)])
  // Module Ignore
  // BDD node 36:vector_return(vector:(w64 1074012584), index:(w32 17), value:(w64 1074026888)[(w16 16)])
  // Module VectorTableUpdate
  buffer_t value_17(2);
  value_17[0] = 0;
  value_17[1] = 16;
  state->vector_table_1074012584.write(17, value_17);
  // BDD node 37:vector_borrow(vector:(w64 1074012584), index:(w32 18), val_out:(w64 1074012248)[ -> (w64 1074026912)])
  // Module Ignore
  // BDD node 38:vector_return(vector:(w64 1074012584), index:(w32 18), value:(w64 1074026912)[(w16 19)])
  // Module VectorTableUpdate
  buffer_t value_18(2);
  value_18[0] = 0;
  value_18[1] = 19;
  state->vector_table_1074012584.write(18, value_18);
  // BDD node 39:vector_borrow(vector:(w64 1074012584), index:(w32 19), val_out:(w64 1074012248)[ -> (w64 1074026936)])
  // Module Ignore
  // BDD node 40:vector_return(vector:(w64 1074012584), index:(w32 19), value:(w64 1074026936)[(w16 18)])
  // Module VectorTableUpdate
  buffer_t value_19(2);
  value_19[0] = 0;
  value_19[1] = 18;
  state->vector_table_1074012584.write(19, value_19);
  // BDD node 41:vector_borrow(vector:(w64 1074012584), index:(w32 20), val_out:(w64 1074012248)[ -> (w64 1074026960)])
  // Module Ignore
  // BDD node 42:vector_return(vector:(w64 1074012584), index:(w32 20), value:(w64 1074026960)[(w16 21)])
  // Module VectorTableUpdate
  buffer_t value_20(2);
  value_20[0] = 0;
  value_20[1] = 21;
  state->vector_table_1074012584.write(20, value_20);
  // BDD node 43:vector_borrow(vector:(w64 1074012584), index:(w32 21), val_out:(w64 1074012248)[ -> (w64 1074026984)])
  // Module Ignore
  // BDD node 44:vector_return(vector:(w64 1074012584), index:(w32 21), value:(w64 1074026984)[(w16 20)])
  // Module VectorTableUpdate
  buffer_t value_21(2);
  value_21[0] = 0;
  value_21[1] = 20;
  state->vector_table_1074012584.write(21, value_21);
  // BDD node 45:vector_borrow(vector:(w64 1074012584), index:(w32 22), val_out:(w64 1074012248)[ -> (w64 1074027008)])
  // Module Ignore
  // BDD node 46:vector_return(vector:(w64 1074012584), index:(w32 22), value:(w64 1074027008)[(w16 23)])
  // Module VectorTableUpdate
  buffer_t value_22(2);
  value_22[0] = 0;
  value_22[1] = 23;
  state->vector_table_1074012584.write(22, value_22);
  // BDD node 47:vector_borrow(vector:(w64 1074012584), index:(w32 23), val_out:(w64 1074012248)[ -> (w64 1074027032)])
  // Module Ignore
  // BDD node 48:vector_return(vector:(w64 1074012584), index:(w32 23), value:(w64 1074027032)[(w16 22)])
  // Module VectorTableUpdate
  buffer_t value_23(2);
  value_23[0] = 0;
  value_23[1] = 22;
  state->vector_table_1074012584.write(23, value_23);
  // BDD node 49:vector_borrow(vector:(w64 1074012584), index:(w32 24), val_out:(w64 1074012248)[ -> (w64 1074027056)])
  // Module Ignore
  // BDD node 50:vector_return(vector:(w64 1074012584), index:(w32 24), value:(w64 1074027056)[(w16 25)])
  // Module VectorTableUpdate
  buffer_t value_24(2);
  value_24[0] = 0;
  value_24[1] = 25;
  state->vector_table_1074012584.write(24, value_24);
  // BDD node 51:vector_borrow(vector:(w64 1074012584), index:(w32 25), val_out:(w64 1074012248)[ -> (w64 1074027080)])
  // Module Ignore
  // BDD node 52:vector_return(vector:(w64 1074012584), index:(w32 25), value:(w64 1074027080)[(w16 24)])
  // Module VectorTableUpdate
  buffer_t value_25(2);
  value_25[0] = 0;
  value_25[1] = 24;
  state->vector_table_1074012584.write(25, value_25);
  // BDD node 53:vector_borrow(vector:(w64 1074012584), index:(w32 26), val_out:(w64 1074012248)[ -> (w64 1074027104)])
  // Module Ignore
  // BDD node 54:vector_return(vector:(w64 1074012584), index:(w32 26), value:(w64 1074027104)[(w16 27)])
  // Module VectorTableUpdate
  buffer_t value_26(2);
  value_26[0] = 0;
  value_26[1] = 27;
  state->vector_table_1074012584.write(26, value_26);
  // BDD node 55:vector_borrow(vector:(w64 1074012584), index:(w32 27), val_out:(w64 1074012248)[ -> (w64 1074027128)])
  // Module Ignore
  // BDD node 56:vector_return(vector:(w64 1074012584), index:(w32 27), value:(w64 1074027128)[(w16 26)])
  // Module VectorTableUpdate
  buffer_t value_27(2);
  value_27[0] = 0;
  value_27[1] = 26;
  state->vector_table_1074012584.write(27, value_27);
  // BDD node 57:vector_borrow(vector:(w64 1074012584), index:(w32 28), val_out:(w64 1074012248)[ -> (w64 1074027152)])
  // Module Ignore
  // BDD node 58:vector_return(vector:(w64 1074012584), index:(w32 28), value:(w64 1074027152)[(w16 29)])
  // Module VectorTableUpdate
  buffer_t value_28(2);
  value_28[0] = 0;
  value_28[1] = 29;
  state->vector_table_1074012584.write(28, value_28);
  // BDD node 59:vector_borrow(vector:(w64 1074012584), index:(w32 29), val_out:(w64 1074012248)[ -> (w64 1074027176)])
  // Module Ignore
  // BDD node 60:vector_return(vector:(w64 1074012584), index:(w32 29), value:(w64 1074027176)[(w16 28)])
  // Module VectorTableUpdate
  buffer_t value_29(2);
  value_29[0] = 0;
  value_29[1] = 28;
  state->vector_table_1074012584.write(29, value_29);
  // BDD node 61:vector_borrow(vector:(w64 1074012584), index:(w32 30), val_out:(w64 1074012248)[ -> (w64 1074027200)])
  // Module Ignore
  // BDD node 62:vector_return(vector:(w64 1074012584), index:(w32 30), value:(w64 1074027200)[(w16 31)])
  // Module VectorTableUpdate
  buffer_t value_30(2);
  value_30[0] = 0;
  value_30[1] = 31;
  state->vector_table_1074012584.write(30, value_30);
  // BDD node 63:vector_borrow(vector:(w64 1074012584), index:(w32 31), val_out:(w64 1074012248)[ -> (w64 1074027224)])
  // Module Ignore
  // BDD node 64:vector_return(vector:(w64 1074012584), index:(w32 31), value:(w64 1074027224)[(w16 30)])
  // Module VectorTableUpdate
  buffer_t value_31(2);
  value_31[0] = 0;
  value_31[1] = 30;
  state->vector_table_1074012584.write(31, value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {

} __attribute__((packed));

bool sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  bool forward = true;
  cpu_hdr_t *cpu_hdr = packet_consume<cpu_hdr_t>(pkt);
  cpu_hdr_extra_t *cpu_hdr_extra = packet_consume<cpu_hdr_extra_t>(pkt);

  return forward;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
