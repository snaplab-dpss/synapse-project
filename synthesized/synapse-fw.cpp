#include <sycon/sycon.h>

using namespace sycon;

struct state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardNFDev forward_nf_dev;
  MapTable map_table_1074043296;
  DchainTable dchain_table_1074075280;
  VectorTable vector_table_1074075704;
  VectorTable vector_table_1074092920;

  state_t()
    : ingress_port_to_nf_dev(),
      forward_nf_dev(),
      map_table_1074043296("Ingress", {"map_table_1074043296_157","map_table_1074043296_142",}, 1000LL),
      dchain_table_1074075280("Ingress", {"dchain_table_1074075280_180","dchain_table_1074075280_148",}, 1000LL),
      vector_table_1074075704("Ingress", {"vector_table_1074075704_139",}),
      vector_table_1074092920("Ingress", {"vector_table_1074092920_181","vector_table_1074092920_149",})
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
  // BDD node 0:map_allocate(capacity:(w32 65536), key_size:(w32 13), map_out:(w64 1074043032)[(w64 0) -> (w64 1074043296)])
  // Module MapTableAllocate
  // BDD node 2:dchain_allocate(index_range:(w32 65536), chain_out:(w64 1074043048)[ -> (w64 1074075280)])
  // Module DchainTableAllocate
  // BDD node 3:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1074043056)[(w64 0) -> (w64 1074075704)])
  // Module VectorTableAllocate
  // BDD node 4:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074043064)[(w64 0) -> (w64 1074092920)])
  // Module VectorTableAllocate
  // BDD node 5:vector_borrow(vector:(w64 1074075704), index:(w32 0), val_out:(w64 1074042904)[ -> (w64 1074089600)])
  // Module Ignore
  // BDD node 6:vector_return(vector:(w64 1074075704), index:(w32 0), value:(w64 1074089600)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t value_0(4);
  value_0[0] = 0;
  value_0[1] = 0;
  value_0[2] = 0;
  value_0[3] = 1;
  state->vector_table_1074075704.write(0, value_0);
  // BDD node 7:vector_borrow(vector:(w64 1074092920), index:(w32 0), val_out:(w64 1074042968)[ -> (w64 1074106816)])
  // Module Ignore
  // BDD node 8:vector_return(vector:(w64 1074092920), index:(w32 0), value:(w64 1074106816)[(w16 1)])
  // Module VectorTableUpdate
  buffer_t value_1(2);
  value_1[0] = 0;
  value_1[1] = 1;
  state->vector_table_1074092920.write(0, value_1);
  // BDD node 9:vector_borrow(vector:(w64 1074075704), index:(w32 1), val_out:(w64 1074042904)[ -> (w64 1074089624)])
  // Module Ignore
  // BDD node 10:vector_return(vector:(w64 1074075704), index:(w32 1), value:(w64 1074089624)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t value_2(4);
  value_2[0] = 0;
  value_2[1] = 0;
  value_2[2] = 0;
  value_2[3] = 0;
  state->vector_table_1074075704.write(1, value_2);
  // BDD node 11:vector_borrow(vector:(w64 1074092920), index:(w32 1), val_out:(w64 1074042968)[ -> (w64 1074106840)])
  // Module Ignore
  // BDD node 12:vector_return(vector:(w64 1074092920), index:(w32 1), value:(w64 1074106840)[(w16 0)])
  // Module VectorTableUpdate
  buffer_t value_3(2);
  value_3[0] = 0;
  value_3[1] = 0;
  state->vector_table_1074092920.write(1, value_3);
  // BDD node 13:vector_borrow(vector:(w64 1074075704), index:(w32 2), val_out:(w64 1074042904)[ -> (w64 1074089648)])
  // Module Ignore
  // BDD node 14:vector_return(vector:(w64 1074075704), index:(w32 2), value:(w64 1074089648)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t value_4(4);
  value_4[0] = 0;
  value_4[1] = 0;
  value_4[2] = 0;
  value_4[3] = 1;
  state->vector_table_1074075704.write(2, value_4);
  // BDD node 15:vector_borrow(vector:(w64 1074092920), index:(w32 2), val_out:(w64 1074042968)[ -> (w64 1074106864)])
  // Module Ignore
  // BDD node 16:vector_return(vector:(w64 1074092920), index:(w32 2), value:(w64 1074106864)[(w16 3)])
  // Module VectorTableUpdate
  buffer_t value_5(2);
  value_5[0] = 0;
  value_5[1] = 3;
  state->vector_table_1074092920.write(2, value_5);
  // BDD node 17:vector_borrow(vector:(w64 1074075704), index:(w32 3), val_out:(w64 1074042904)[ -> (w64 1074089672)])
  // Module Ignore
  // BDD node 18:vector_return(vector:(w64 1074075704), index:(w32 3), value:(w64 1074089672)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t value_6(4);
  value_6[0] = 0;
  value_6[1] = 0;
  value_6[2] = 0;
  value_6[3] = 0;
  state->vector_table_1074075704.write(3, value_6);
  // BDD node 19:vector_borrow(vector:(w64 1074092920), index:(w32 3), val_out:(w64 1074042968)[ -> (w64 1074106888)])
  // Module Ignore
  // BDD node 20:vector_return(vector:(w64 1074092920), index:(w32 3), value:(w64 1074106888)[(w16 2)])
  // Module VectorTableUpdate
  buffer_t value_7(2);
  value_7[0] = 0;
  value_7[1] = 2;
  state->vector_table_1074092920.write(3, value_7);
  // BDD node 21:vector_borrow(vector:(w64 1074075704), index:(w32 4), val_out:(w64 1074042904)[ -> (w64 1074089696)])
  // Module Ignore
  // BDD node 22:vector_return(vector:(w64 1074075704), index:(w32 4), value:(w64 1074089696)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t value_8(4);
  value_8[0] = 0;
  value_8[1] = 0;
  value_8[2] = 0;
  value_8[3] = 1;
  state->vector_table_1074075704.write(4, value_8);
  // BDD node 23:vector_borrow(vector:(w64 1074092920), index:(w32 4), val_out:(w64 1074042968)[ -> (w64 1074106912)])
  // Module Ignore
  // BDD node 24:vector_return(vector:(w64 1074092920), index:(w32 4), value:(w64 1074106912)[(w16 5)])
  // Module VectorTableUpdate
  buffer_t value_9(2);
  value_9[0] = 0;
  value_9[1] = 5;
  state->vector_table_1074092920.write(4, value_9);
  // BDD node 25:vector_borrow(vector:(w64 1074075704), index:(w32 5), val_out:(w64 1074042904)[ -> (w64 1074089720)])
  // Module Ignore
  // BDD node 26:vector_return(vector:(w64 1074075704), index:(w32 5), value:(w64 1074089720)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t value_10(4);
  value_10[0] = 0;
  value_10[1] = 0;
  value_10[2] = 0;
  value_10[3] = 0;
  state->vector_table_1074075704.write(5, value_10);
  // BDD node 27:vector_borrow(vector:(w64 1074092920), index:(w32 5), val_out:(w64 1074042968)[ -> (w64 1074106936)])
  // Module Ignore
  // BDD node 28:vector_return(vector:(w64 1074092920), index:(w32 5), value:(w64 1074106936)[(w16 4)])
  // Module VectorTableUpdate
  buffer_t value_11(2);
  value_11[0] = 0;
  value_11[1] = 4;
  state->vector_table_1074092920.write(5, value_11);
  // BDD node 29:vector_borrow(vector:(w64 1074075704), index:(w32 6), val_out:(w64 1074042904)[ -> (w64 1074089744)])
  // Module Ignore
  // BDD node 30:vector_return(vector:(w64 1074075704), index:(w32 6), value:(w64 1074089744)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t value_12(4);
  value_12[0] = 0;
  value_12[1] = 0;
  value_12[2] = 0;
  value_12[3] = 1;
  state->vector_table_1074075704.write(6, value_12);
  // BDD node 31:vector_borrow(vector:(w64 1074092920), index:(w32 6), val_out:(w64 1074042968)[ -> (w64 1074106960)])
  // Module Ignore
  // BDD node 32:vector_return(vector:(w64 1074092920), index:(w32 6), value:(w64 1074106960)[(w16 7)])
  // Module VectorTableUpdate
  buffer_t value_13(2);
  value_13[0] = 0;
  value_13[1] = 7;
  state->vector_table_1074092920.write(6, value_13);
  // BDD node 33:vector_borrow(vector:(w64 1074075704), index:(w32 7), val_out:(w64 1074042904)[ -> (w64 1074089768)])
  // Module Ignore
  // BDD node 34:vector_return(vector:(w64 1074075704), index:(w32 7), value:(w64 1074089768)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t value_14(4);
  value_14[0] = 0;
  value_14[1] = 0;
  value_14[2] = 0;
  value_14[3] = 0;
  state->vector_table_1074075704.write(7, value_14);
  // BDD node 35:vector_borrow(vector:(w64 1074092920), index:(w32 7), val_out:(w64 1074042968)[ -> (w64 1074106984)])
  // Module Ignore
  // BDD node 36:vector_return(vector:(w64 1074092920), index:(w32 7), value:(w64 1074106984)[(w16 6)])
  // Module VectorTableUpdate
  buffer_t value_15(2);
  value_15[0] = 0;
  value_15[1] = 6;
  state->vector_table_1074092920.write(7, value_15);
  // BDD node 37:vector_borrow(vector:(w64 1074075704), index:(w32 8), val_out:(w64 1074042904)[ -> (w64 1074089792)])
  // Module Ignore
  // BDD node 38:vector_return(vector:(w64 1074075704), index:(w32 8), value:(w64 1074089792)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t value_16(4);
  value_16[0] = 0;
  value_16[1] = 0;
  value_16[2] = 0;
  value_16[3] = 1;
  state->vector_table_1074075704.write(8, value_16);
  // BDD node 39:vector_borrow(vector:(w64 1074092920), index:(w32 8), val_out:(w64 1074042968)[ -> (w64 1074107008)])
  // Module Ignore
  // BDD node 40:vector_return(vector:(w64 1074092920), index:(w32 8), value:(w64 1074107008)[(w16 9)])
  // Module VectorTableUpdate
  buffer_t value_17(2);
  value_17[0] = 0;
  value_17[1] = 9;
  state->vector_table_1074092920.write(8, value_17);
  // BDD node 41:vector_borrow(vector:(w64 1074075704), index:(w32 9), val_out:(w64 1074042904)[ -> (w64 1074089816)])
  // Module Ignore
  // BDD node 42:vector_return(vector:(w64 1074075704), index:(w32 9), value:(w64 1074089816)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t value_18(4);
  value_18[0] = 0;
  value_18[1] = 0;
  value_18[2] = 0;
  value_18[3] = 0;
  state->vector_table_1074075704.write(9, value_18);
  // BDD node 43:vector_borrow(vector:(w64 1074092920), index:(w32 9), val_out:(w64 1074042968)[ -> (w64 1074107032)])
  // Module Ignore
  // BDD node 44:vector_return(vector:(w64 1074092920), index:(w32 9), value:(w64 1074107032)[(w16 8)])
  // Module VectorTableUpdate
  buffer_t value_19(2);
  value_19[0] = 0;
  value_19[1] = 8;
  state->vector_table_1074092920.write(9, value_19);
  // BDD node 45:vector_borrow(vector:(w64 1074075704), index:(w32 10), val_out:(w64 1074042904)[ -> (w64 1074089840)])
  // Module Ignore
  // BDD node 46:vector_return(vector:(w64 1074075704), index:(w32 10), value:(w64 1074089840)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t value_20(4);
  value_20[0] = 0;
  value_20[1] = 0;
  value_20[2] = 0;
  value_20[3] = 1;
  state->vector_table_1074075704.write(10, value_20);
  // BDD node 47:vector_borrow(vector:(w64 1074092920), index:(w32 10), val_out:(w64 1074042968)[ -> (w64 1074107056)])
  // Module Ignore
  // BDD node 48:vector_return(vector:(w64 1074092920), index:(w32 10), value:(w64 1074107056)[(w16 11)])
  // Module VectorTableUpdate
  buffer_t value_21(2);
  value_21[0] = 0;
  value_21[1] = 11;
  state->vector_table_1074092920.write(10, value_21);
  // BDD node 49:vector_borrow(vector:(w64 1074075704), index:(w32 11), val_out:(w64 1074042904)[ -> (w64 1074089864)])
  // Module Ignore
  // BDD node 50:vector_return(vector:(w64 1074075704), index:(w32 11), value:(w64 1074089864)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t value_22(4);
  value_22[0] = 0;
  value_22[1] = 0;
  value_22[2] = 0;
  value_22[3] = 0;
  state->vector_table_1074075704.write(11, value_22);
  // BDD node 51:vector_borrow(vector:(w64 1074092920), index:(w32 11), val_out:(w64 1074042968)[ -> (w64 1074107080)])
  // Module Ignore
  // BDD node 52:vector_return(vector:(w64 1074092920), index:(w32 11), value:(w64 1074107080)[(w16 10)])
  // Module VectorTableUpdate
  buffer_t value_23(2);
  value_23[0] = 0;
  value_23[1] = 10;
  state->vector_table_1074092920.write(11, value_23);
  // BDD node 53:vector_borrow(vector:(w64 1074075704), index:(w32 12), val_out:(w64 1074042904)[ -> (w64 1074089888)])
  // Module Ignore
  // BDD node 54:vector_return(vector:(w64 1074075704), index:(w32 12), value:(w64 1074089888)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t value_24(4);
  value_24[0] = 0;
  value_24[1] = 0;
  value_24[2] = 0;
  value_24[3] = 1;
  state->vector_table_1074075704.write(12, value_24);
  // BDD node 55:vector_borrow(vector:(w64 1074092920), index:(w32 12), val_out:(w64 1074042968)[ -> (w64 1074107104)])
  // Module Ignore
  // BDD node 56:vector_return(vector:(w64 1074092920), index:(w32 12), value:(w64 1074107104)[(w16 13)])
  // Module VectorTableUpdate
  buffer_t value_25(2);
  value_25[0] = 0;
  value_25[1] = 13;
  state->vector_table_1074092920.write(12, value_25);
  // BDD node 57:vector_borrow(vector:(w64 1074075704), index:(w32 13), val_out:(w64 1074042904)[ -> (w64 1074089912)])
  // Module Ignore
  // BDD node 58:vector_return(vector:(w64 1074075704), index:(w32 13), value:(w64 1074089912)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t value_26(4);
  value_26[0] = 0;
  value_26[1] = 0;
  value_26[2] = 0;
  value_26[3] = 0;
  state->vector_table_1074075704.write(13, value_26);
  // BDD node 59:vector_borrow(vector:(w64 1074092920), index:(w32 13), val_out:(w64 1074042968)[ -> (w64 1074107128)])
  // Module Ignore
  // BDD node 60:vector_return(vector:(w64 1074092920), index:(w32 13), value:(w64 1074107128)[(w16 12)])
  // Module VectorTableUpdate
  buffer_t value_27(2);
  value_27[0] = 0;
  value_27[1] = 12;
  state->vector_table_1074092920.write(13, value_27);
  // BDD node 61:vector_borrow(vector:(w64 1074075704), index:(w32 14), val_out:(w64 1074042904)[ -> (w64 1074089936)])
  // Module Ignore
  // BDD node 62:vector_return(vector:(w64 1074075704), index:(w32 14), value:(w64 1074089936)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t value_28(4);
  value_28[0] = 0;
  value_28[1] = 0;
  value_28[2] = 0;
  value_28[3] = 1;
  state->vector_table_1074075704.write(14, value_28);
  // BDD node 63:vector_borrow(vector:(w64 1074092920), index:(w32 14), val_out:(w64 1074042968)[ -> (w64 1074107152)])
  // Module Ignore
  // BDD node 64:vector_return(vector:(w64 1074092920), index:(w32 14), value:(w64 1074107152)[(w16 15)])
  // Module VectorTableUpdate
  buffer_t value_29(2);
  value_29[0] = 0;
  value_29[1] = 15;
  state->vector_table_1074092920.write(14, value_29);
  // BDD node 65:vector_borrow(vector:(w64 1074075704), index:(w32 15), val_out:(w64 1074042904)[ -> (w64 1074089960)])
  // Module Ignore
  // BDD node 66:vector_return(vector:(w64 1074075704), index:(w32 15), value:(w64 1074089960)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t value_30(4);
  value_30[0] = 0;
  value_30[1] = 0;
  value_30[2] = 0;
  value_30[3] = 0;
  state->vector_table_1074075704.write(15, value_30);
  // BDD node 67:vector_borrow(vector:(w64 1074092920), index:(w32 15), val_out:(w64 1074042968)[ -> (w64 1074107176)])
  // Module Ignore
  // BDD node 68:vector_return(vector:(w64 1074092920), index:(w32 15), value:(w64 1074107176)[(w16 14)])
  // Module VectorTableUpdate
  buffer_t value_31(2);
  value_31[0] = 0;
  value_31[1] = 14;
  state->vector_table_1074092920.write(15, value_31);
  // BDD node 69:vector_borrow(vector:(w64 1074075704), index:(w32 16), val_out:(w64 1074042904)[ -> (w64 1074089984)])
  // Module Ignore
  // BDD node 70:vector_return(vector:(w64 1074075704), index:(w32 16), value:(w64 1074089984)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t value_32(4);
  value_32[0] = 0;
  value_32[1] = 0;
  value_32[2] = 0;
  value_32[3] = 1;
  state->vector_table_1074075704.write(16, value_32);
  // BDD node 71:vector_borrow(vector:(w64 1074092920), index:(w32 16), val_out:(w64 1074042968)[ -> (w64 1074107200)])
  // Module Ignore
  // BDD node 72:vector_return(vector:(w64 1074092920), index:(w32 16), value:(w64 1074107200)[(w16 17)])
  // Module VectorTableUpdate
  buffer_t value_33(2);
  value_33[0] = 0;
  value_33[1] = 17;
  state->vector_table_1074092920.write(16, value_33);
  // BDD node 73:vector_borrow(vector:(w64 1074075704), index:(w32 17), val_out:(w64 1074042904)[ -> (w64 1074090008)])
  // Module Ignore
  // BDD node 74:vector_return(vector:(w64 1074075704), index:(w32 17), value:(w64 1074090008)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t value_34(4);
  value_34[0] = 0;
  value_34[1] = 0;
  value_34[2] = 0;
  value_34[3] = 0;
  state->vector_table_1074075704.write(17, value_34);
  // BDD node 75:vector_borrow(vector:(w64 1074092920), index:(w32 17), val_out:(w64 1074042968)[ -> (w64 1074107224)])
  // Module Ignore
  // BDD node 76:vector_return(vector:(w64 1074092920), index:(w32 17), value:(w64 1074107224)[(w16 16)])
  // Module VectorTableUpdate
  buffer_t value_35(2);
  value_35[0] = 0;
  value_35[1] = 16;
  state->vector_table_1074092920.write(17, value_35);
  // BDD node 77:vector_borrow(vector:(w64 1074075704), index:(w32 18), val_out:(w64 1074042904)[ -> (w64 1074090032)])
  // Module Ignore
  // BDD node 78:vector_return(vector:(w64 1074075704), index:(w32 18), value:(w64 1074090032)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t value_36(4);
  value_36[0] = 0;
  value_36[1] = 0;
  value_36[2] = 0;
  value_36[3] = 1;
  state->vector_table_1074075704.write(18, value_36);
  // BDD node 79:vector_borrow(vector:(w64 1074092920), index:(w32 18), val_out:(w64 1074042968)[ -> (w64 1074107248)])
  // Module Ignore
  // BDD node 80:vector_return(vector:(w64 1074092920), index:(w32 18), value:(w64 1074107248)[(w16 19)])
  // Module VectorTableUpdate
  buffer_t value_37(2);
  value_37[0] = 0;
  value_37[1] = 19;
  state->vector_table_1074092920.write(18, value_37);
  // BDD node 81:vector_borrow(vector:(w64 1074075704), index:(w32 19), val_out:(w64 1074042904)[ -> (w64 1074090056)])
  // Module Ignore
  // BDD node 82:vector_return(vector:(w64 1074075704), index:(w32 19), value:(w64 1074090056)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t value_38(4);
  value_38[0] = 0;
  value_38[1] = 0;
  value_38[2] = 0;
  value_38[3] = 0;
  state->vector_table_1074075704.write(19, value_38);
  // BDD node 83:vector_borrow(vector:(w64 1074092920), index:(w32 19), val_out:(w64 1074042968)[ -> (w64 1074107272)])
  // Module Ignore
  // BDD node 84:vector_return(vector:(w64 1074092920), index:(w32 19), value:(w64 1074107272)[(w16 18)])
  // Module VectorTableUpdate
  buffer_t value_39(2);
  value_39[0] = 0;
  value_39[1] = 18;
  state->vector_table_1074092920.write(19, value_39);
  // BDD node 85:vector_borrow(vector:(w64 1074075704), index:(w32 20), val_out:(w64 1074042904)[ -> (w64 1074090080)])
  // Module Ignore
  // BDD node 86:vector_return(vector:(w64 1074075704), index:(w32 20), value:(w64 1074090080)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t value_40(4);
  value_40[0] = 0;
  value_40[1] = 0;
  value_40[2] = 0;
  value_40[3] = 1;
  state->vector_table_1074075704.write(20, value_40);
  // BDD node 87:vector_borrow(vector:(w64 1074092920), index:(w32 20), val_out:(w64 1074042968)[ -> (w64 1074107296)])
  // Module Ignore
  // BDD node 88:vector_return(vector:(w64 1074092920), index:(w32 20), value:(w64 1074107296)[(w16 21)])
  // Module VectorTableUpdate
  buffer_t value_41(2);
  value_41[0] = 0;
  value_41[1] = 21;
  state->vector_table_1074092920.write(20, value_41);
  // BDD node 89:vector_borrow(vector:(w64 1074075704), index:(w32 21), val_out:(w64 1074042904)[ -> (w64 1074090104)])
  // Module Ignore
  // BDD node 90:vector_return(vector:(w64 1074075704), index:(w32 21), value:(w64 1074090104)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t value_42(4);
  value_42[0] = 0;
  value_42[1] = 0;
  value_42[2] = 0;
  value_42[3] = 0;
  state->vector_table_1074075704.write(21, value_42);
  // BDD node 91:vector_borrow(vector:(w64 1074092920), index:(w32 21), val_out:(w64 1074042968)[ -> (w64 1074107320)])
  // Module Ignore
  // BDD node 92:vector_return(vector:(w64 1074092920), index:(w32 21), value:(w64 1074107320)[(w16 20)])
  // Module VectorTableUpdate
  buffer_t value_43(2);
  value_43[0] = 0;
  value_43[1] = 20;
  state->vector_table_1074092920.write(21, value_43);
  // BDD node 93:vector_borrow(vector:(w64 1074075704), index:(w32 22), val_out:(w64 1074042904)[ -> (w64 1074090128)])
  // Module Ignore
  // BDD node 94:vector_return(vector:(w64 1074075704), index:(w32 22), value:(w64 1074090128)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t value_44(4);
  value_44[0] = 0;
  value_44[1] = 0;
  value_44[2] = 0;
  value_44[3] = 1;
  state->vector_table_1074075704.write(22, value_44);
  // BDD node 95:vector_borrow(vector:(w64 1074092920), index:(w32 22), val_out:(w64 1074042968)[ -> (w64 1074107344)])
  // Module Ignore
  // BDD node 96:vector_return(vector:(w64 1074092920), index:(w32 22), value:(w64 1074107344)[(w16 23)])
  // Module VectorTableUpdate
  buffer_t value_45(2);
  value_45[0] = 0;
  value_45[1] = 23;
  state->vector_table_1074092920.write(22, value_45);
  // BDD node 97:vector_borrow(vector:(w64 1074075704), index:(w32 23), val_out:(w64 1074042904)[ -> (w64 1074090152)])
  // Module Ignore
  // BDD node 98:vector_return(vector:(w64 1074075704), index:(w32 23), value:(w64 1074090152)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t value_46(4);
  value_46[0] = 0;
  value_46[1] = 0;
  value_46[2] = 0;
  value_46[3] = 0;
  state->vector_table_1074075704.write(23, value_46);
  // BDD node 99:vector_borrow(vector:(w64 1074092920), index:(w32 23), val_out:(w64 1074042968)[ -> (w64 1074107368)])
  // Module Ignore
  // BDD node 100:vector_return(vector:(w64 1074092920), index:(w32 23), value:(w64 1074107368)[(w16 22)])
  // Module VectorTableUpdate
  buffer_t value_47(2);
  value_47[0] = 0;
  value_47[1] = 22;
  state->vector_table_1074092920.write(23, value_47);
  // BDD node 101:vector_borrow(vector:(w64 1074075704), index:(w32 24), val_out:(w64 1074042904)[ -> (w64 1074090176)])
  // Module Ignore
  // BDD node 102:vector_return(vector:(w64 1074075704), index:(w32 24), value:(w64 1074090176)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t value_48(4);
  value_48[0] = 0;
  value_48[1] = 0;
  value_48[2] = 0;
  value_48[3] = 1;
  state->vector_table_1074075704.write(24, value_48);
  // BDD node 103:vector_borrow(vector:(w64 1074092920), index:(w32 24), val_out:(w64 1074042968)[ -> (w64 1074107392)])
  // Module Ignore
  // BDD node 104:vector_return(vector:(w64 1074092920), index:(w32 24), value:(w64 1074107392)[(w16 25)])
  // Module VectorTableUpdate
  buffer_t value_49(2);
  value_49[0] = 0;
  value_49[1] = 25;
  state->vector_table_1074092920.write(24, value_49);
  // BDD node 105:vector_borrow(vector:(w64 1074075704), index:(w32 25), val_out:(w64 1074042904)[ -> (w64 1074090200)])
  // Module Ignore
  // BDD node 106:vector_return(vector:(w64 1074075704), index:(w32 25), value:(w64 1074090200)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t value_50(4);
  value_50[0] = 0;
  value_50[1] = 0;
  value_50[2] = 0;
  value_50[3] = 0;
  state->vector_table_1074075704.write(25, value_50);
  // BDD node 107:vector_borrow(vector:(w64 1074092920), index:(w32 25), val_out:(w64 1074042968)[ -> (w64 1074107416)])
  // Module Ignore
  // BDD node 108:vector_return(vector:(w64 1074092920), index:(w32 25), value:(w64 1074107416)[(w16 24)])
  // Module VectorTableUpdate
  buffer_t value_51(2);
  value_51[0] = 0;
  value_51[1] = 24;
  state->vector_table_1074092920.write(25, value_51);
  // BDD node 109:vector_borrow(vector:(w64 1074075704), index:(w32 26), val_out:(w64 1074042904)[ -> (w64 1074090224)])
  // Module Ignore
  // BDD node 110:vector_return(vector:(w64 1074075704), index:(w32 26), value:(w64 1074090224)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t value_52(4);
  value_52[0] = 0;
  value_52[1] = 0;
  value_52[2] = 0;
  value_52[3] = 1;
  state->vector_table_1074075704.write(26, value_52);
  // BDD node 111:vector_borrow(vector:(w64 1074092920), index:(w32 26), val_out:(w64 1074042968)[ -> (w64 1074107440)])
  // Module Ignore
  // BDD node 112:vector_return(vector:(w64 1074092920), index:(w32 26), value:(w64 1074107440)[(w16 27)])
  // Module VectorTableUpdate
  buffer_t value_53(2);
  value_53[0] = 0;
  value_53[1] = 27;
  state->vector_table_1074092920.write(26, value_53);
  // BDD node 113:vector_borrow(vector:(w64 1074075704), index:(w32 27), val_out:(w64 1074042904)[ -> (w64 1074090248)])
  // Module Ignore
  // BDD node 114:vector_return(vector:(w64 1074075704), index:(w32 27), value:(w64 1074090248)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t value_54(4);
  value_54[0] = 0;
  value_54[1] = 0;
  value_54[2] = 0;
  value_54[3] = 0;
  state->vector_table_1074075704.write(27, value_54);
  // BDD node 115:vector_borrow(vector:(w64 1074092920), index:(w32 27), val_out:(w64 1074042968)[ -> (w64 1074107464)])
  // Module Ignore
  // BDD node 116:vector_return(vector:(w64 1074092920), index:(w32 27), value:(w64 1074107464)[(w16 26)])
  // Module VectorTableUpdate
  buffer_t value_55(2);
  value_55[0] = 0;
  value_55[1] = 26;
  state->vector_table_1074092920.write(27, value_55);
  // BDD node 117:vector_borrow(vector:(w64 1074075704), index:(w32 28), val_out:(w64 1074042904)[ -> (w64 1074090272)])
  // Module Ignore
  // BDD node 118:vector_return(vector:(w64 1074075704), index:(w32 28), value:(w64 1074090272)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t value_56(4);
  value_56[0] = 0;
  value_56[1] = 0;
  value_56[2] = 0;
  value_56[3] = 1;
  state->vector_table_1074075704.write(28, value_56);
  // BDD node 119:vector_borrow(vector:(w64 1074092920), index:(w32 28), val_out:(w64 1074042968)[ -> (w64 1074107488)])
  // Module Ignore
  // BDD node 120:vector_return(vector:(w64 1074092920), index:(w32 28), value:(w64 1074107488)[(w16 29)])
  // Module VectorTableUpdate
  buffer_t value_57(2);
  value_57[0] = 0;
  value_57[1] = 29;
  state->vector_table_1074092920.write(28, value_57);
  // BDD node 121:vector_borrow(vector:(w64 1074075704), index:(w32 29), val_out:(w64 1074042904)[ -> (w64 1074090296)])
  // Module Ignore
  // BDD node 122:vector_return(vector:(w64 1074075704), index:(w32 29), value:(w64 1074090296)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t value_58(4);
  value_58[0] = 0;
  value_58[1] = 0;
  value_58[2] = 0;
  value_58[3] = 0;
  state->vector_table_1074075704.write(29, value_58);
  // BDD node 123:vector_borrow(vector:(w64 1074092920), index:(w32 29), val_out:(w64 1074042968)[ -> (w64 1074107512)])
  // Module Ignore
  // BDD node 124:vector_return(vector:(w64 1074092920), index:(w32 29), value:(w64 1074107512)[(w16 28)])
  // Module VectorTableUpdate
  buffer_t value_59(2);
  value_59[0] = 0;
  value_59[1] = 28;
  state->vector_table_1074092920.write(29, value_59);
  // BDD node 125:vector_borrow(vector:(w64 1074075704), index:(w32 30), val_out:(w64 1074042904)[ -> (w64 1074090320)])
  // Module Ignore
  // BDD node 126:vector_return(vector:(w64 1074075704), index:(w32 30), value:(w64 1074090320)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t value_60(4);
  value_60[0] = 0;
  value_60[1] = 0;
  value_60[2] = 0;
  value_60[3] = 1;
  state->vector_table_1074075704.write(30, value_60);
  // BDD node 127:vector_borrow(vector:(w64 1074092920), index:(w32 30), val_out:(w64 1074042968)[ -> (w64 1074107536)])
  // Module Ignore
  // BDD node 128:vector_return(vector:(w64 1074092920), index:(w32 30), value:(w64 1074107536)[(w16 31)])
  // Module VectorTableUpdate
  buffer_t value_61(2);
  value_61[0] = 0;
  value_61[1] = 31;
  state->vector_table_1074092920.write(30, value_61);
  // BDD node 129:vector_borrow(vector:(w64 1074075704), index:(w32 31), val_out:(w64 1074042904)[ -> (w64 1074090344)])
  // Module Ignore
  // BDD node 130:vector_return(vector:(w64 1074075704), index:(w32 31), value:(w64 1074090344)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t value_62(4);
  value_62[0] = 0;
  value_62[1] = 0;
  value_62[2] = 0;
  value_62[3] = 0;
  state->vector_table_1074075704.write(31, value_62);
  // BDD node 131:vector_borrow(vector:(w64 1074092920), index:(w32 31), val_out:(w64 1074042968)[ -> (w64 1074107560)])
  // Module Ignore
  // BDD node 132:vector_return(vector:(w64 1074092920), index:(w32 31), value:(w64 1074107560)[(w16 30)])
  // Module VectorTableUpdate
  buffer_t value_63(2);
  value_63[0] = 0;
  value_63[1] = 30;
  state->vector_table_1074092920.write(31, value_63);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 DEVICE;

} __attribute__((packed));

bool sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  bool forward = true;
  cpu_hdr_t *cpu_hdr = packet_consume<cpu_hdr_t>(pkt);
  cpu_hdr_extra_t *cpu_hdr_extra = packet_consume<cpu_hdr_extra_t>(pkt);
  if (SWAP_ENDIAN_16(cpu_hdr->code_path) == 573) {
    // EP node  5521
    // BDD node 249:packet_borrow_next_chunk(p:(w64 1074195128), length:(w32 14), chunk:(w64 1074205832)[ -> (w64 1073761968)])
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  5565
    // BDD node 250:packet_borrow_next_chunk(p:(w64 1074195128), length:(w32 20), chunk:(w64 1074206568)[ -> (w64 1073762115)])
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  5610
    // BDD node 251:packet_borrow_next_chunk(p:(w64 1074195128), length:(w32 4), chunk:(w64 1074207224)[ -> (w64 1073762262)])
    u8* hdr_2 = packet_consume(pkt, 4);
    // EP node  5656
    // BDD node 159:dchain_allocate_new_index(chain:(w64 1074075280), index_out:(w64 1074210800)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u32 allocated_index_0;
    bool success_0 = state->dchain_table_1074075280.allocate_new_index(allocated_index_0);
    // EP node  5703
    // BDD node 160:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
    if ((0) == (success_0)) {
      // EP node  5704
      // BDD node 160:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
      // EP node  5753
      // BDD node 161:vector_borrow(vector:(w64 1074092920), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074211248)[ -> (w64 1074106816)])
      buffer_t value_64;
      state->vector_table_1074092920.read((u16)(SWAP_ENDIAN_32(cpu_hdr_extra->DEVICE) & 65535), value_64);
      // EP node  5804
      // BDD node 162:vector_return(vector:(w64 1074092920), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074106816)[(ReadLSB w16 (w32 0) vector_data_640)])
      // EP node  6012
      // BDD node 166:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
      if ((65535) != ((u16)value_64.get(0, 2))) {
        // EP node  6013
        // BDD node 166:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
        // EP node  6122
        // BDD node 167:FORWARD
        cpu_hdr->egress_dev = SWAP_ENDIAN_16((u16)value_64.get(0, 2));
      } else {
        // EP node  6014
        // BDD node 166:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
        // EP node  6599
        // BDD node 168:DROP
        forward = false;
      }
    } else {
      // EP node  5705
      // BDD node 160:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
      // EP node  6123
      // BDD node 170:map_put(map:(w64 1074043296), key:(w64 1074071016)[(Concat w104 (Read w8 (w32 156) packet_chunks) (Concat w96 (Read w8 (w32 166) packet_chunks) (Concat w88 (Read w8 (w32 165) packet_chunks) (Concat w80 (Read w8 (w32 164) packet_chunks) (Concat w72 (Read w8 (w32 163) packet_chunks) (Concat w64 (Read w8 (w32 162) packet_chunks) (Concat w56 (Read w8 (w32 161) packet_chunks) (Concat w48 (Read w8 (w32 160) packet_chunks) (Concat w40 (Read w8 (w32 159) packet_chunks) (ReadLSB w32 (w32 294) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 156) packet_chunks) (Concat w96 (Read w8 (w32 166) packet_chunks) (Concat w88 (Read w8 (w32 165) packet_chunks) (Concat w80 (Read w8 (w32 164) packet_chunks) (Concat w72 (Read w8 (w32 163) packet_chunks) (Concat w64 (Read w8 (w32 162) packet_chunks) (Concat w56 (Read w8 (w32 161) packet_chunks) (Concat w48 (Read w8 (w32 160) packet_chunks) (Concat w40 (Read w8 (w32 159) packet_chunks) (ReadLSB w32 (w32 294) packet_chunks))))))))))], value:(ReadLSB w32 (w32 0) new_index))
      buffer_t key_0(13);
      key_0[0] = *(u8*)(hdr_1 + 9);
      key_0[1] = *(u8*)(hdr_1 + 19);
      key_0[2] = *(u8*)(hdr_1 + 18);
      key_0[3] = *(u8*)(hdr_1 + 17);
      key_0[4] = *(u8*)(hdr_1 + 16);
      key_0[5] = *(u8*)(hdr_1 + 15);
      key_0[6] = *(u8*)(hdr_1 + 14);
      key_0[7] = *(u8*)(hdr_1 + 13);
      key_0[8] = *(u8*)(hdr_1 + 12);
      key_0[9] = *(u8*)(hdr_2 + 3);
      key_0[10] = *(u8*)(hdr_2 + 2);
      key_0[11] = *(u8*)(hdr_2 + 1);
      key_0[12] = *(u8*)(hdr_2 + 0);
      state->map_table_1074043296.put(key_0, allocated_index_0);
      // EP node  6180
      // BDD node 172:vector_borrow(vector:(w64 1074092920), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074213480)[ -> (w64 1074106816)])
      buffer_t value_65;
      state->vector_table_1074092920.read((u16)(SWAP_ENDIAN_32(cpu_hdr_extra->DEVICE) & 65535), value_65);
      // EP node  6238
      // BDD node 173:vector_return(vector:(w64 1074092920), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074106816)[(ReadLSB w16 (w32 0) vector_data_640)])
      // EP node  6474
      // BDD node 177:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
      if ((65535) != ((u16)value_65.get(0, 2))) {
        // EP node  6475
        // BDD node 177:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
        // EP node  6727
        // BDD node 178:FORWARD
        cpu_hdr->egress_dev = SWAP_ENDIAN_16((u16)value_65.get(0, 2));
      } else {
        // EP node  6476
        // BDD node 177:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
        // EP node  6536
        // BDD node 179:DROP
        forward = false;
      }
    }
  }

  return forward;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
