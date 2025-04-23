#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardNFDev forward_nf_dev;
  VectorTable vector_table_1074013320;

  state_t()
    : ingress_port_to_nf_dev(),
      forward_nf_dev(),
      vector_table_1074013320("vector_table_1074013320",{"Ingress.vector_table_1074013320_65",})
    {}
};

state_t *state = nullptr;

void sycon::nf_init() {
  nf_state = std::make_unique<state_t>();
  state    = dynamic_cast<state_t *>(nf_state.get());
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(3), 2);
  state->forward_nf_dev.add_entry(2, asic_get_dev_port(3));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(4), 3);
  state->forward_nf_dev.add_entry(3, asic_get_dev_port(4));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(5), 4);
  state->forward_nf_dev.add_entry(4, asic_get_dev_port(5));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(6), 5);
  state->forward_nf_dev.add_entry(5, asic_get_dev_port(6));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(7), 6);
  state->forward_nf_dev.add_entry(6, asic_get_dev_port(7));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(8), 7);
  state->forward_nf_dev.add_entry(7, asic_get_dev_port(8));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(9), 8);
  state->forward_nf_dev.add_entry(8, asic_get_dev_port(9));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(10), 9);
  state->forward_nf_dev.add_entry(9, asic_get_dev_port(10));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(11), 10);
  state->forward_nf_dev.add_entry(10, asic_get_dev_port(11));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(12), 11);
  state->forward_nf_dev.add_entry(11, asic_get_dev_port(12));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(13), 12);
  state->forward_nf_dev.add_entry(12, asic_get_dev_port(13));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(14), 13);
  state->forward_nf_dev.add_entry(13, asic_get_dev_port(14));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(15), 14);
  state->forward_nf_dev.add_entry(14, asic_get_dev_port(15));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(16), 15);
  state->forward_nf_dev.add_entry(15, asic_get_dev_port(16));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(17), 16);
  state->forward_nf_dev.add_entry(16, asic_get_dev_port(17));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(18), 17);
  state->forward_nf_dev.add_entry(17, asic_get_dev_port(18));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(19), 18);
  state->forward_nf_dev.add_entry(18, asic_get_dev_port(19));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(20), 19);
  state->forward_nf_dev.add_entry(19, asic_get_dev_port(20));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(21), 20);
  state->forward_nf_dev.add_entry(20, asic_get_dev_port(21));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(22), 21);
  state->forward_nf_dev.add_entry(21, asic_get_dev_port(22));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(23), 22);
  state->forward_nf_dev.add_entry(22, asic_get_dev_port(23));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(24), 23);
  state->forward_nf_dev.add_entry(23, asic_get_dev_port(24));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(25), 24);
  state->forward_nf_dev.add_entry(24, asic_get_dev_port(25));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(26), 25);
  state->forward_nf_dev.add_entry(25, asic_get_dev_port(26));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(27), 26);
  state->forward_nf_dev.add_entry(26, asic_get_dev_port(27));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(28), 27);
  state->forward_nf_dev.add_entry(27, asic_get_dev_port(28));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(29), 28);
  state->forward_nf_dev.add_entry(28, asic_get_dev_port(29));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(30), 29);
  state->forward_nf_dev.add_entry(29, asic_get_dev_port(30));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(31), 30);
  state->forward_nf_dev.add_entry(30, asic_get_dev_port(31));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(32), 31);
  state->forward_nf_dev.add_entry(31, asic_get_dev_port(32));
  // BDD node 0:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074013048)[(w64 0) -> (w64 1074013320)])
  // Module DataplaneVectorTableAllocate
  // BDD node 1:vector_borrow(vector:(w64 1074013320), index:(w32 0), val_out:(w64 1074012984)[ -> (w64 1074027216)])
  // Module Ignore
  // BDD node 2:vector_return(vector:(w64 1074013320), index:(w32 0), value:(w64 1074027216)[(w16 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_0(2);
  vector_table_1074013320_value_0[0] = 0;
  vector_table_1074013320_value_0[1] = 1;
  state->vector_table_1074013320.write(0, vector_table_1074013320_value_0);
  // BDD node 3:vector_borrow(vector:(w64 1074013320), index:(w32 1), val_out:(w64 1074012984)[ -> (w64 1074027240)])
  // Module Ignore
  // BDD node 4:vector_return(vector:(w64 1074013320), index:(w32 1), value:(w64 1074027240)[(w16 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_1(2);
  vector_table_1074013320_value_1[0] = 0;
  vector_table_1074013320_value_1[1] = 0;
  state->vector_table_1074013320.write(1, vector_table_1074013320_value_1);
  // BDD node 5:vector_borrow(vector:(w64 1074013320), index:(w32 2), val_out:(w64 1074012984)[ -> (w64 1074027264)])
  // Module Ignore
  // BDD node 6:vector_return(vector:(w64 1074013320), index:(w32 2), value:(w64 1074027264)[(w16 3)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_2(2);
  vector_table_1074013320_value_2[0] = 0;
  vector_table_1074013320_value_2[1] = 3;
  state->vector_table_1074013320.write(2, vector_table_1074013320_value_2);
  // BDD node 7:vector_borrow(vector:(w64 1074013320), index:(w32 3), val_out:(w64 1074012984)[ -> (w64 1074027288)])
  // Module Ignore
  // BDD node 8:vector_return(vector:(w64 1074013320), index:(w32 3), value:(w64 1074027288)[(w16 2)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_3(2);
  vector_table_1074013320_value_3[0] = 0;
  vector_table_1074013320_value_3[1] = 2;
  state->vector_table_1074013320.write(3, vector_table_1074013320_value_3);
  // BDD node 9:vector_borrow(vector:(w64 1074013320), index:(w32 4), val_out:(w64 1074012984)[ -> (w64 1074027312)])
  // Module Ignore
  // BDD node 10:vector_return(vector:(w64 1074013320), index:(w32 4), value:(w64 1074027312)[(w16 5)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_4(2);
  vector_table_1074013320_value_4[0] = 0;
  vector_table_1074013320_value_4[1] = 5;
  state->vector_table_1074013320.write(4, vector_table_1074013320_value_4);
  // BDD node 11:vector_borrow(vector:(w64 1074013320), index:(w32 5), val_out:(w64 1074012984)[ -> (w64 1074027336)])
  // Module Ignore
  // BDD node 12:vector_return(vector:(w64 1074013320), index:(w32 5), value:(w64 1074027336)[(w16 4)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_5(2);
  vector_table_1074013320_value_5[0] = 0;
  vector_table_1074013320_value_5[1] = 4;
  state->vector_table_1074013320.write(5, vector_table_1074013320_value_5);
  // BDD node 13:vector_borrow(vector:(w64 1074013320), index:(w32 6), val_out:(w64 1074012984)[ -> (w64 1074027360)])
  // Module Ignore
  // BDD node 14:vector_return(vector:(w64 1074013320), index:(w32 6), value:(w64 1074027360)[(w16 7)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_6(2);
  vector_table_1074013320_value_6[0] = 0;
  vector_table_1074013320_value_6[1] = 7;
  state->vector_table_1074013320.write(6, vector_table_1074013320_value_6);
  // BDD node 15:vector_borrow(vector:(w64 1074013320), index:(w32 7), val_out:(w64 1074012984)[ -> (w64 1074027384)])
  // Module Ignore
  // BDD node 16:vector_return(vector:(w64 1074013320), index:(w32 7), value:(w64 1074027384)[(w16 6)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_7(2);
  vector_table_1074013320_value_7[0] = 0;
  vector_table_1074013320_value_7[1] = 6;
  state->vector_table_1074013320.write(7, vector_table_1074013320_value_7);
  // BDD node 17:vector_borrow(vector:(w64 1074013320), index:(w32 8), val_out:(w64 1074012984)[ -> (w64 1074027408)])
  // Module Ignore
  // BDD node 18:vector_return(vector:(w64 1074013320), index:(w32 8), value:(w64 1074027408)[(w16 9)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_8(2);
  vector_table_1074013320_value_8[0] = 0;
  vector_table_1074013320_value_8[1] = 9;
  state->vector_table_1074013320.write(8, vector_table_1074013320_value_8);
  // BDD node 19:vector_borrow(vector:(w64 1074013320), index:(w32 9), val_out:(w64 1074012984)[ -> (w64 1074027432)])
  // Module Ignore
  // BDD node 20:vector_return(vector:(w64 1074013320), index:(w32 9), value:(w64 1074027432)[(w16 8)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_9(2);
  vector_table_1074013320_value_9[0] = 0;
  vector_table_1074013320_value_9[1] = 8;
  state->vector_table_1074013320.write(9, vector_table_1074013320_value_9);
  // BDD node 21:vector_borrow(vector:(w64 1074013320), index:(w32 10), val_out:(w64 1074012984)[ -> (w64 1074027456)])
  // Module Ignore
  // BDD node 22:vector_return(vector:(w64 1074013320), index:(w32 10), value:(w64 1074027456)[(w16 11)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_10(2);
  vector_table_1074013320_value_10[0] = 0;
  vector_table_1074013320_value_10[1] = 11;
  state->vector_table_1074013320.write(10, vector_table_1074013320_value_10);
  // BDD node 23:vector_borrow(vector:(w64 1074013320), index:(w32 11), val_out:(w64 1074012984)[ -> (w64 1074027480)])
  // Module Ignore
  // BDD node 24:vector_return(vector:(w64 1074013320), index:(w32 11), value:(w64 1074027480)[(w16 10)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_11(2);
  vector_table_1074013320_value_11[0] = 0;
  vector_table_1074013320_value_11[1] = 10;
  state->vector_table_1074013320.write(11, vector_table_1074013320_value_11);
  // BDD node 25:vector_borrow(vector:(w64 1074013320), index:(w32 12), val_out:(w64 1074012984)[ -> (w64 1074027504)])
  // Module Ignore
  // BDD node 26:vector_return(vector:(w64 1074013320), index:(w32 12), value:(w64 1074027504)[(w16 13)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_12(2);
  vector_table_1074013320_value_12[0] = 0;
  vector_table_1074013320_value_12[1] = 13;
  state->vector_table_1074013320.write(12, vector_table_1074013320_value_12);
  // BDD node 27:vector_borrow(vector:(w64 1074013320), index:(w32 13), val_out:(w64 1074012984)[ -> (w64 1074027528)])
  // Module Ignore
  // BDD node 28:vector_return(vector:(w64 1074013320), index:(w32 13), value:(w64 1074027528)[(w16 12)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_13(2);
  vector_table_1074013320_value_13[0] = 0;
  vector_table_1074013320_value_13[1] = 12;
  state->vector_table_1074013320.write(13, vector_table_1074013320_value_13);
  // BDD node 29:vector_borrow(vector:(w64 1074013320), index:(w32 14), val_out:(w64 1074012984)[ -> (w64 1074027552)])
  // Module Ignore
  // BDD node 30:vector_return(vector:(w64 1074013320), index:(w32 14), value:(w64 1074027552)[(w16 15)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_14(2);
  vector_table_1074013320_value_14[0] = 0;
  vector_table_1074013320_value_14[1] = 15;
  state->vector_table_1074013320.write(14, vector_table_1074013320_value_14);
  // BDD node 31:vector_borrow(vector:(w64 1074013320), index:(w32 15), val_out:(w64 1074012984)[ -> (w64 1074027576)])
  // Module Ignore
  // BDD node 32:vector_return(vector:(w64 1074013320), index:(w32 15), value:(w64 1074027576)[(w16 14)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_15(2);
  vector_table_1074013320_value_15[0] = 0;
  vector_table_1074013320_value_15[1] = 14;
  state->vector_table_1074013320.write(15, vector_table_1074013320_value_15);
  // BDD node 33:vector_borrow(vector:(w64 1074013320), index:(w32 16), val_out:(w64 1074012984)[ -> (w64 1074027600)])
  // Module Ignore
  // BDD node 34:vector_return(vector:(w64 1074013320), index:(w32 16), value:(w64 1074027600)[(w16 17)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_16(2);
  vector_table_1074013320_value_16[0] = 0;
  vector_table_1074013320_value_16[1] = 17;
  state->vector_table_1074013320.write(16, vector_table_1074013320_value_16);
  // BDD node 35:vector_borrow(vector:(w64 1074013320), index:(w32 17), val_out:(w64 1074012984)[ -> (w64 1074027624)])
  // Module Ignore
  // BDD node 36:vector_return(vector:(w64 1074013320), index:(w32 17), value:(w64 1074027624)[(w16 16)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_17(2);
  vector_table_1074013320_value_17[0] = 0;
  vector_table_1074013320_value_17[1] = 16;
  state->vector_table_1074013320.write(17, vector_table_1074013320_value_17);
  // BDD node 37:vector_borrow(vector:(w64 1074013320), index:(w32 18), val_out:(w64 1074012984)[ -> (w64 1074027648)])
  // Module Ignore
  // BDD node 38:vector_return(vector:(w64 1074013320), index:(w32 18), value:(w64 1074027648)[(w16 19)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_18(2);
  vector_table_1074013320_value_18[0] = 0;
  vector_table_1074013320_value_18[1] = 19;
  state->vector_table_1074013320.write(18, vector_table_1074013320_value_18);
  // BDD node 39:vector_borrow(vector:(w64 1074013320), index:(w32 19), val_out:(w64 1074012984)[ -> (w64 1074027672)])
  // Module Ignore
  // BDD node 40:vector_return(vector:(w64 1074013320), index:(w32 19), value:(w64 1074027672)[(w16 18)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_19(2);
  vector_table_1074013320_value_19[0] = 0;
  vector_table_1074013320_value_19[1] = 18;
  state->vector_table_1074013320.write(19, vector_table_1074013320_value_19);
  // BDD node 41:vector_borrow(vector:(w64 1074013320), index:(w32 20), val_out:(w64 1074012984)[ -> (w64 1074027696)])
  // Module Ignore
  // BDD node 42:vector_return(vector:(w64 1074013320), index:(w32 20), value:(w64 1074027696)[(w16 21)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_20(2);
  vector_table_1074013320_value_20[0] = 0;
  vector_table_1074013320_value_20[1] = 21;
  state->vector_table_1074013320.write(20, vector_table_1074013320_value_20);
  // BDD node 43:vector_borrow(vector:(w64 1074013320), index:(w32 21), val_out:(w64 1074012984)[ -> (w64 1074027720)])
  // Module Ignore
  // BDD node 44:vector_return(vector:(w64 1074013320), index:(w32 21), value:(w64 1074027720)[(w16 20)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_21(2);
  vector_table_1074013320_value_21[0] = 0;
  vector_table_1074013320_value_21[1] = 20;
  state->vector_table_1074013320.write(21, vector_table_1074013320_value_21);
  // BDD node 45:vector_borrow(vector:(w64 1074013320), index:(w32 22), val_out:(w64 1074012984)[ -> (w64 1074027744)])
  // Module Ignore
  // BDD node 46:vector_return(vector:(w64 1074013320), index:(w32 22), value:(w64 1074027744)[(w16 23)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_22(2);
  vector_table_1074013320_value_22[0] = 0;
  vector_table_1074013320_value_22[1] = 23;
  state->vector_table_1074013320.write(22, vector_table_1074013320_value_22);
  // BDD node 47:vector_borrow(vector:(w64 1074013320), index:(w32 23), val_out:(w64 1074012984)[ -> (w64 1074027768)])
  // Module Ignore
  // BDD node 48:vector_return(vector:(w64 1074013320), index:(w32 23), value:(w64 1074027768)[(w16 22)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_23(2);
  vector_table_1074013320_value_23[0] = 0;
  vector_table_1074013320_value_23[1] = 22;
  state->vector_table_1074013320.write(23, vector_table_1074013320_value_23);
  // BDD node 49:vector_borrow(vector:(w64 1074013320), index:(w32 24), val_out:(w64 1074012984)[ -> (w64 1074027792)])
  // Module Ignore
  // BDD node 50:vector_return(vector:(w64 1074013320), index:(w32 24), value:(w64 1074027792)[(w16 25)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_24(2);
  vector_table_1074013320_value_24[0] = 0;
  vector_table_1074013320_value_24[1] = 25;
  state->vector_table_1074013320.write(24, vector_table_1074013320_value_24);
  // BDD node 51:vector_borrow(vector:(w64 1074013320), index:(w32 25), val_out:(w64 1074012984)[ -> (w64 1074027816)])
  // Module Ignore
  // BDD node 52:vector_return(vector:(w64 1074013320), index:(w32 25), value:(w64 1074027816)[(w16 24)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_25(2);
  vector_table_1074013320_value_25[0] = 0;
  vector_table_1074013320_value_25[1] = 24;
  state->vector_table_1074013320.write(25, vector_table_1074013320_value_25);
  // BDD node 53:vector_borrow(vector:(w64 1074013320), index:(w32 26), val_out:(w64 1074012984)[ -> (w64 1074027840)])
  // Module Ignore
  // BDD node 54:vector_return(vector:(w64 1074013320), index:(w32 26), value:(w64 1074027840)[(w16 27)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_26(2);
  vector_table_1074013320_value_26[0] = 0;
  vector_table_1074013320_value_26[1] = 27;
  state->vector_table_1074013320.write(26, vector_table_1074013320_value_26);
  // BDD node 55:vector_borrow(vector:(w64 1074013320), index:(w32 27), val_out:(w64 1074012984)[ -> (w64 1074027864)])
  // Module Ignore
  // BDD node 56:vector_return(vector:(w64 1074013320), index:(w32 27), value:(w64 1074027864)[(w16 26)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_27(2);
  vector_table_1074013320_value_27[0] = 0;
  vector_table_1074013320_value_27[1] = 26;
  state->vector_table_1074013320.write(27, vector_table_1074013320_value_27);
  // BDD node 57:vector_borrow(vector:(w64 1074013320), index:(w32 28), val_out:(w64 1074012984)[ -> (w64 1074027888)])
  // Module Ignore
  // BDD node 58:vector_return(vector:(w64 1074013320), index:(w32 28), value:(w64 1074027888)[(w16 29)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_28(2);
  vector_table_1074013320_value_28[0] = 0;
  vector_table_1074013320_value_28[1] = 29;
  state->vector_table_1074013320.write(28, vector_table_1074013320_value_28);
  // BDD node 59:vector_borrow(vector:(w64 1074013320), index:(w32 29), val_out:(w64 1074012984)[ -> (w64 1074027912)])
  // Module Ignore
  // BDD node 60:vector_return(vector:(w64 1074013320), index:(w32 29), value:(w64 1074027912)[(w16 28)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_29(2);
  vector_table_1074013320_value_29[0] = 0;
  vector_table_1074013320_value_29[1] = 28;
  state->vector_table_1074013320.write(29, vector_table_1074013320_value_29);
  // BDD node 61:vector_borrow(vector:(w64 1074013320), index:(w32 30), val_out:(w64 1074012984)[ -> (w64 1074027936)])
  // Module Ignore
  // BDD node 62:vector_return(vector:(w64 1074013320), index:(w32 30), value:(w64 1074027936)[(w16 31)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_30(2);
  vector_table_1074013320_value_30[0] = 0;
  vector_table_1074013320_value_30[1] = 31;
  state->vector_table_1074013320.write(30, vector_table_1074013320_value_30);
  // BDD node 63:vector_borrow(vector:(w64 1074013320), index:(w32 31), val_out:(w64 1074012984)[ -> (w64 1074027960)])
  // Module Ignore
  // BDD node 64:vector_return(vector:(w64 1074013320), index:(w32 31), value:(w64 1074027960)[(w16 30)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074013320_value_31(2);
  vector_table_1074013320_value_31[0] = 0;
  vector_table_1074013320_value_31[1] = 30;
  state->vector_table_1074013320.write(31, vector_table_1074013320_value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {

} __attribute__((packed));

nf_process_result_t sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  nf_process_result_t result;
  result.forward = true;
  bool trigger_update_ipv4_tcpudp_checksums = false;
  void* l3_hdr = nullptr;
  void* l4_hdr = nullptr;

  cpu_hdr_t *cpu_hdr = packet_consume<cpu_hdr_t>(pkt);
  cpu_hdr_extra_t *cpu_hdr_extra = packet_consume<cpu_hdr_extra_t>(pkt);
  LOG_DEBUG("[t=%lu] New packet (size=%u, code_path=%d)\n", now, size, bswap16(cpu_hdr->code_path));

  cpu_hdr->egress_dev = 0;
  cpu_hdr->trigger_dataplane_execution = 0;



  if (trigger_update_ipv4_tcpudp_checksums) {
    update_ipv4_tcpudp_checksums(l3_hdr, l4_hdr);
  }

  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
