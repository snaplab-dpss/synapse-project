#include <sycon/sycon.h>

using namespace sycon;

struct state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardNFDev forward_nf_dev;
  MapTable map_table_1074053088;
  VectorTable vector_table_1074066912;
  DchainTable dchain_table_1074085072;
  VectorRegister vector_register_1074085496;
  VectorTable vector_table_1074102712;

  state_t()
    : ingress_port_to_nf_dev(),
      forward_nf_dev(),
      map_table_1074053088("Ingress", {"map_table_1074053088_165",}, 1000LL),
      vector_table_1074066912("Ingress", {"vector_table_1074066912_144",}),
      dchain_table_1074085072("Ingress", {"dchain_table_1074085072_142","dchain_table_1074085072_185",}, 1000LL),
      vector_register_1074085496("Ingress", {"vector_register_1074085496_0",}),
      vector_table_1074102712("Ingress", {"vector_table_1074102712_187","vector_table_1074102712_149",})
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
  // BDD node 0:map_allocate(capacity:(w32 65536), key_size:(w32 13), map_out:(w64 1074052824)[(w64 0) -> (w64 1074053088)])
  // Module MapTableAllocate
  // BDD node 1:vector_allocate(elem_size:(w32 13), capacity:(w32 65536), vector_out:(w64 1074052832)[(w64 0) -> (w64 1074066912)])
  // Module VectorTableAllocate
  // BDD node 2:dchain_allocate(index_range:(w32 65536), chain_out:(w64 1074052840)[ -> (w64 1074085072)])
  // Module DchainTableAllocate
  // BDD node 3:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1074052848)[(w64 0) -> (w64 1074085496)])
  // Module VectorRegisterAllocate
  // BDD node 4:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074052856)[(w64 0) -> (w64 1074102712)])
  // Module VectorTableAllocate
  // BDD node 5:vector_borrow(vector:(w64 1074085496), index:(w32 0), val_out:(w64 1074052696)[ -> (w64 1074099392)])
  // Module Ignore
  // BDD node 6:vector_return(vector:(w64 1074085496), index:(w32 0), value:(w64 1074099392)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_0;
  state->vector_register_1074085496.get(0, value_0);
  value_0[3] = 1;
  value_0[2] = 0;
  value_0[1] = 0;
  value_0[0] = 0;
  state->vector_register_1074085496.put(0, value_0);
  // BDD node 7:vector_borrow(vector:(w64 1074102712), index:(w32 0), val_out:(w64 1074052760)[ -> (w64 1074116608)])
  // Module Ignore
  // BDD node 8:vector_return(vector:(w64 1074102712), index:(w32 0), value:(w64 1074116608)[(w16 1)])
  // Module VectorTableUpdate
  buffer_t value_1(2);
  value_1[0] = 0;
  value_1[1] = 1;
  state->vector_table_1074102712.write(0, value_1);
  // BDD node 9:vector_borrow(vector:(w64 1074085496), index:(w32 1), val_out:(w64 1074052696)[ -> (w64 1074099416)])
  // Module Ignore
  // BDD node 10:vector_return(vector:(w64 1074085496), index:(w32 1), value:(w64 1074099416)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_2;
  state->vector_register_1074085496.get(1, value_2);
  value_2[3] = 0;
  value_2[2] = 0;
  value_2[1] = 0;
  value_2[0] = 0;
  state->vector_register_1074085496.put(1, value_2);
  // BDD node 11:vector_borrow(vector:(w64 1074102712), index:(w32 1), val_out:(w64 1074052760)[ -> (w64 1074116632)])
  // Module Ignore
  // BDD node 12:vector_return(vector:(w64 1074102712), index:(w32 1), value:(w64 1074116632)[(w16 0)])
  // Module VectorTableUpdate
  buffer_t value_3(2);
  value_3[0] = 0;
  value_3[1] = 0;
  state->vector_table_1074102712.write(1, value_3);
  // BDD node 13:vector_borrow(vector:(w64 1074085496), index:(w32 2), val_out:(w64 1074052696)[ -> (w64 1074099440)])
  // Module Ignore
  // BDD node 14:vector_return(vector:(w64 1074085496), index:(w32 2), value:(w64 1074099440)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_4;
  state->vector_register_1074085496.get(2, value_4);
  value_4[3] = 1;
  value_4[2] = 0;
  value_4[1] = 0;
  value_4[0] = 0;
  state->vector_register_1074085496.put(2, value_4);
  // BDD node 15:vector_borrow(vector:(w64 1074102712), index:(w32 2), val_out:(w64 1074052760)[ -> (w64 1074116656)])
  // Module Ignore
  // BDD node 16:vector_return(vector:(w64 1074102712), index:(w32 2), value:(w64 1074116656)[(w16 3)])
  // Module VectorTableUpdate
  buffer_t value_5(2);
  value_5[0] = 0;
  value_5[1] = 3;
  state->vector_table_1074102712.write(2, value_5);
  // BDD node 17:vector_borrow(vector:(w64 1074085496), index:(w32 3), val_out:(w64 1074052696)[ -> (w64 1074099464)])
  // Module Ignore
  // BDD node 18:vector_return(vector:(w64 1074085496), index:(w32 3), value:(w64 1074099464)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_6;
  state->vector_register_1074085496.get(3, value_6);
  value_6[3] = 0;
  value_6[2] = 0;
  value_6[1] = 0;
  value_6[0] = 0;
  state->vector_register_1074085496.put(3, value_6);
  // BDD node 19:vector_borrow(vector:(w64 1074102712), index:(w32 3), val_out:(w64 1074052760)[ -> (w64 1074116680)])
  // Module Ignore
  // BDD node 20:vector_return(vector:(w64 1074102712), index:(w32 3), value:(w64 1074116680)[(w16 2)])
  // Module VectorTableUpdate
  buffer_t value_7(2);
  value_7[0] = 0;
  value_7[1] = 2;
  state->vector_table_1074102712.write(3, value_7);
  // BDD node 21:vector_borrow(vector:(w64 1074085496), index:(w32 4), val_out:(w64 1074052696)[ -> (w64 1074099488)])
  // Module Ignore
  // BDD node 22:vector_return(vector:(w64 1074085496), index:(w32 4), value:(w64 1074099488)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_8;
  state->vector_register_1074085496.get(4, value_8);
  value_8[3] = 1;
  value_8[2] = 0;
  value_8[1] = 0;
  value_8[0] = 0;
  state->vector_register_1074085496.put(4, value_8);
  // BDD node 23:vector_borrow(vector:(w64 1074102712), index:(w32 4), val_out:(w64 1074052760)[ -> (w64 1074116704)])
  // Module Ignore
  // BDD node 24:vector_return(vector:(w64 1074102712), index:(w32 4), value:(w64 1074116704)[(w16 5)])
  // Module VectorTableUpdate
  buffer_t value_9(2);
  value_9[0] = 0;
  value_9[1] = 5;
  state->vector_table_1074102712.write(4, value_9);
  // BDD node 25:vector_borrow(vector:(w64 1074085496), index:(w32 5), val_out:(w64 1074052696)[ -> (w64 1074099512)])
  // Module Ignore
  // BDD node 26:vector_return(vector:(w64 1074085496), index:(w32 5), value:(w64 1074099512)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_10;
  state->vector_register_1074085496.get(5, value_10);
  value_10[3] = 0;
  value_10[2] = 0;
  value_10[1] = 0;
  value_10[0] = 0;
  state->vector_register_1074085496.put(5, value_10);
  // BDD node 27:vector_borrow(vector:(w64 1074102712), index:(w32 5), val_out:(w64 1074052760)[ -> (w64 1074116728)])
  // Module Ignore
  // BDD node 28:vector_return(vector:(w64 1074102712), index:(w32 5), value:(w64 1074116728)[(w16 4)])
  // Module VectorTableUpdate
  buffer_t value_11(2);
  value_11[0] = 0;
  value_11[1] = 4;
  state->vector_table_1074102712.write(5, value_11);
  // BDD node 29:vector_borrow(vector:(w64 1074085496), index:(w32 6), val_out:(w64 1074052696)[ -> (w64 1074099536)])
  // Module Ignore
  // BDD node 30:vector_return(vector:(w64 1074085496), index:(w32 6), value:(w64 1074099536)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_12;
  state->vector_register_1074085496.get(6, value_12);
  value_12[3] = 1;
  value_12[2] = 0;
  value_12[1] = 0;
  value_12[0] = 0;
  state->vector_register_1074085496.put(6, value_12);
  // BDD node 31:vector_borrow(vector:(w64 1074102712), index:(w32 6), val_out:(w64 1074052760)[ -> (w64 1074116752)])
  // Module Ignore
  // BDD node 32:vector_return(vector:(w64 1074102712), index:(w32 6), value:(w64 1074116752)[(w16 7)])
  // Module VectorTableUpdate
  buffer_t value_13(2);
  value_13[0] = 0;
  value_13[1] = 7;
  state->vector_table_1074102712.write(6, value_13);
  // BDD node 33:vector_borrow(vector:(w64 1074085496), index:(w32 7), val_out:(w64 1074052696)[ -> (w64 1074099560)])
  // Module Ignore
  // BDD node 34:vector_return(vector:(w64 1074085496), index:(w32 7), value:(w64 1074099560)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_14;
  state->vector_register_1074085496.get(7, value_14);
  value_14[3] = 0;
  value_14[2] = 0;
  value_14[1] = 0;
  value_14[0] = 0;
  state->vector_register_1074085496.put(7, value_14);
  // BDD node 35:vector_borrow(vector:(w64 1074102712), index:(w32 7), val_out:(w64 1074052760)[ -> (w64 1074116776)])
  // Module Ignore
  // BDD node 36:vector_return(vector:(w64 1074102712), index:(w32 7), value:(w64 1074116776)[(w16 6)])
  // Module VectorTableUpdate
  buffer_t value_15(2);
  value_15[0] = 0;
  value_15[1] = 6;
  state->vector_table_1074102712.write(7, value_15);
  // BDD node 37:vector_borrow(vector:(w64 1074085496), index:(w32 8), val_out:(w64 1074052696)[ -> (w64 1074099584)])
  // Module Ignore
  // BDD node 38:vector_return(vector:(w64 1074085496), index:(w32 8), value:(w64 1074099584)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_16;
  state->vector_register_1074085496.get(8, value_16);
  value_16[3] = 1;
  value_16[2] = 0;
  value_16[1] = 0;
  value_16[0] = 0;
  state->vector_register_1074085496.put(8, value_16);
  // BDD node 39:vector_borrow(vector:(w64 1074102712), index:(w32 8), val_out:(w64 1074052760)[ -> (w64 1074116800)])
  // Module Ignore
  // BDD node 40:vector_return(vector:(w64 1074102712), index:(w32 8), value:(w64 1074116800)[(w16 9)])
  // Module VectorTableUpdate
  buffer_t value_17(2);
  value_17[0] = 0;
  value_17[1] = 9;
  state->vector_table_1074102712.write(8, value_17);
  // BDD node 41:vector_borrow(vector:(w64 1074085496), index:(w32 9), val_out:(w64 1074052696)[ -> (w64 1074099608)])
  // Module Ignore
  // BDD node 42:vector_return(vector:(w64 1074085496), index:(w32 9), value:(w64 1074099608)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_18;
  state->vector_register_1074085496.get(9, value_18);
  value_18[3] = 0;
  value_18[2] = 0;
  value_18[1] = 0;
  value_18[0] = 0;
  state->vector_register_1074085496.put(9, value_18);
  // BDD node 43:vector_borrow(vector:(w64 1074102712), index:(w32 9), val_out:(w64 1074052760)[ -> (w64 1074116824)])
  // Module Ignore
  // BDD node 44:vector_return(vector:(w64 1074102712), index:(w32 9), value:(w64 1074116824)[(w16 8)])
  // Module VectorTableUpdate
  buffer_t value_19(2);
  value_19[0] = 0;
  value_19[1] = 8;
  state->vector_table_1074102712.write(9, value_19);
  // BDD node 45:vector_borrow(vector:(w64 1074085496), index:(w32 10), val_out:(w64 1074052696)[ -> (w64 1074099632)])
  // Module Ignore
  // BDD node 46:vector_return(vector:(w64 1074085496), index:(w32 10), value:(w64 1074099632)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_20;
  state->vector_register_1074085496.get(10, value_20);
  value_20[3] = 1;
  value_20[2] = 0;
  value_20[1] = 0;
  value_20[0] = 0;
  state->vector_register_1074085496.put(10, value_20);
  // BDD node 47:vector_borrow(vector:(w64 1074102712), index:(w32 10), val_out:(w64 1074052760)[ -> (w64 1074116848)])
  // Module Ignore
  // BDD node 48:vector_return(vector:(w64 1074102712), index:(w32 10), value:(w64 1074116848)[(w16 11)])
  // Module VectorTableUpdate
  buffer_t value_21(2);
  value_21[0] = 0;
  value_21[1] = 11;
  state->vector_table_1074102712.write(10, value_21);
  // BDD node 49:vector_borrow(vector:(w64 1074085496), index:(w32 11), val_out:(w64 1074052696)[ -> (w64 1074099656)])
  // Module Ignore
  // BDD node 50:vector_return(vector:(w64 1074085496), index:(w32 11), value:(w64 1074099656)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_22;
  state->vector_register_1074085496.get(11, value_22);
  value_22[3] = 0;
  value_22[2] = 0;
  value_22[1] = 0;
  value_22[0] = 0;
  state->vector_register_1074085496.put(11, value_22);
  // BDD node 51:vector_borrow(vector:(w64 1074102712), index:(w32 11), val_out:(w64 1074052760)[ -> (w64 1074116872)])
  // Module Ignore
  // BDD node 52:vector_return(vector:(w64 1074102712), index:(w32 11), value:(w64 1074116872)[(w16 10)])
  // Module VectorTableUpdate
  buffer_t value_23(2);
  value_23[0] = 0;
  value_23[1] = 10;
  state->vector_table_1074102712.write(11, value_23);
  // BDD node 53:vector_borrow(vector:(w64 1074085496), index:(w32 12), val_out:(w64 1074052696)[ -> (w64 1074099680)])
  // Module Ignore
  // BDD node 54:vector_return(vector:(w64 1074085496), index:(w32 12), value:(w64 1074099680)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_24;
  state->vector_register_1074085496.get(12, value_24);
  value_24[3] = 1;
  value_24[2] = 0;
  value_24[1] = 0;
  value_24[0] = 0;
  state->vector_register_1074085496.put(12, value_24);
  // BDD node 55:vector_borrow(vector:(w64 1074102712), index:(w32 12), val_out:(w64 1074052760)[ -> (w64 1074116896)])
  // Module Ignore
  // BDD node 56:vector_return(vector:(w64 1074102712), index:(w32 12), value:(w64 1074116896)[(w16 13)])
  // Module VectorTableUpdate
  buffer_t value_25(2);
  value_25[0] = 0;
  value_25[1] = 13;
  state->vector_table_1074102712.write(12, value_25);
  // BDD node 57:vector_borrow(vector:(w64 1074085496), index:(w32 13), val_out:(w64 1074052696)[ -> (w64 1074099704)])
  // Module Ignore
  // BDD node 58:vector_return(vector:(w64 1074085496), index:(w32 13), value:(w64 1074099704)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_26;
  state->vector_register_1074085496.get(13, value_26);
  value_26[3] = 0;
  value_26[2] = 0;
  value_26[1] = 0;
  value_26[0] = 0;
  state->vector_register_1074085496.put(13, value_26);
  // BDD node 59:vector_borrow(vector:(w64 1074102712), index:(w32 13), val_out:(w64 1074052760)[ -> (w64 1074116920)])
  // Module Ignore
  // BDD node 60:vector_return(vector:(w64 1074102712), index:(w32 13), value:(w64 1074116920)[(w16 12)])
  // Module VectorTableUpdate
  buffer_t value_27(2);
  value_27[0] = 0;
  value_27[1] = 12;
  state->vector_table_1074102712.write(13, value_27);
  // BDD node 61:vector_borrow(vector:(w64 1074085496), index:(w32 14), val_out:(w64 1074052696)[ -> (w64 1074099728)])
  // Module Ignore
  // BDD node 62:vector_return(vector:(w64 1074085496), index:(w32 14), value:(w64 1074099728)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_28;
  state->vector_register_1074085496.get(14, value_28);
  value_28[3] = 1;
  value_28[2] = 0;
  value_28[1] = 0;
  value_28[0] = 0;
  state->vector_register_1074085496.put(14, value_28);
  // BDD node 63:vector_borrow(vector:(w64 1074102712), index:(w32 14), val_out:(w64 1074052760)[ -> (w64 1074116944)])
  // Module Ignore
  // BDD node 64:vector_return(vector:(w64 1074102712), index:(w32 14), value:(w64 1074116944)[(w16 15)])
  // Module VectorTableUpdate
  buffer_t value_29(2);
  value_29[0] = 0;
  value_29[1] = 15;
  state->vector_table_1074102712.write(14, value_29);
  // BDD node 65:vector_borrow(vector:(w64 1074085496), index:(w32 15), val_out:(w64 1074052696)[ -> (w64 1074099752)])
  // Module Ignore
  // BDD node 66:vector_return(vector:(w64 1074085496), index:(w32 15), value:(w64 1074099752)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_30;
  state->vector_register_1074085496.get(15, value_30);
  value_30[3] = 0;
  value_30[2] = 0;
  value_30[1] = 0;
  value_30[0] = 0;
  state->vector_register_1074085496.put(15, value_30);
  // BDD node 67:vector_borrow(vector:(w64 1074102712), index:(w32 15), val_out:(w64 1074052760)[ -> (w64 1074116968)])
  // Module Ignore
  // BDD node 68:vector_return(vector:(w64 1074102712), index:(w32 15), value:(w64 1074116968)[(w16 14)])
  // Module VectorTableUpdate
  buffer_t value_31(2);
  value_31[0] = 0;
  value_31[1] = 14;
  state->vector_table_1074102712.write(15, value_31);
  // BDD node 69:vector_borrow(vector:(w64 1074085496), index:(w32 16), val_out:(w64 1074052696)[ -> (w64 1074099776)])
  // Module Ignore
  // BDD node 70:vector_return(vector:(w64 1074085496), index:(w32 16), value:(w64 1074099776)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_32;
  state->vector_register_1074085496.get(16, value_32);
  value_32[3] = 1;
  value_32[2] = 0;
  value_32[1] = 0;
  value_32[0] = 0;
  state->vector_register_1074085496.put(16, value_32);
  // BDD node 71:vector_borrow(vector:(w64 1074102712), index:(w32 16), val_out:(w64 1074052760)[ -> (w64 1074116992)])
  // Module Ignore
  // BDD node 72:vector_return(vector:(w64 1074102712), index:(w32 16), value:(w64 1074116992)[(w16 17)])
  // Module VectorTableUpdate
  buffer_t value_33(2);
  value_33[0] = 0;
  value_33[1] = 17;
  state->vector_table_1074102712.write(16, value_33);
  // BDD node 73:vector_borrow(vector:(w64 1074085496), index:(w32 17), val_out:(w64 1074052696)[ -> (w64 1074099800)])
  // Module Ignore
  // BDD node 74:vector_return(vector:(w64 1074085496), index:(w32 17), value:(w64 1074099800)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_34;
  state->vector_register_1074085496.get(17, value_34);
  value_34[3] = 0;
  value_34[2] = 0;
  value_34[1] = 0;
  value_34[0] = 0;
  state->vector_register_1074085496.put(17, value_34);
  // BDD node 75:vector_borrow(vector:(w64 1074102712), index:(w32 17), val_out:(w64 1074052760)[ -> (w64 1074117016)])
  // Module Ignore
  // BDD node 76:vector_return(vector:(w64 1074102712), index:(w32 17), value:(w64 1074117016)[(w16 16)])
  // Module VectorTableUpdate
  buffer_t value_35(2);
  value_35[0] = 0;
  value_35[1] = 16;
  state->vector_table_1074102712.write(17, value_35);
  // BDD node 77:vector_borrow(vector:(w64 1074085496), index:(w32 18), val_out:(w64 1074052696)[ -> (w64 1074099824)])
  // Module Ignore
  // BDD node 78:vector_return(vector:(w64 1074085496), index:(w32 18), value:(w64 1074099824)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_36;
  state->vector_register_1074085496.get(18, value_36);
  value_36[3] = 1;
  value_36[2] = 0;
  value_36[1] = 0;
  value_36[0] = 0;
  state->vector_register_1074085496.put(18, value_36);
  // BDD node 79:vector_borrow(vector:(w64 1074102712), index:(w32 18), val_out:(w64 1074052760)[ -> (w64 1074117040)])
  // Module Ignore
  // BDD node 80:vector_return(vector:(w64 1074102712), index:(w32 18), value:(w64 1074117040)[(w16 19)])
  // Module VectorTableUpdate
  buffer_t value_37(2);
  value_37[0] = 0;
  value_37[1] = 19;
  state->vector_table_1074102712.write(18, value_37);
  // BDD node 81:vector_borrow(vector:(w64 1074085496), index:(w32 19), val_out:(w64 1074052696)[ -> (w64 1074099848)])
  // Module Ignore
  // BDD node 82:vector_return(vector:(w64 1074085496), index:(w32 19), value:(w64 1074099848)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_38;
  state->vector_register_1074085496.get(19, value_38);
  value_38[3] = 0;
  value_38[2] = 0;
  value_38[1] = 0;
  value_38[0] = 0;
  state->vector_register_1074085496.put(19, value_38);
  // BDD node 83:vector_borrow(vector:(w64 1074102712), index:(w32 19), val_out:(w64 1074052760)[ -> (w64 1074117064)])
  // Module Ignore
  // BDD node 84:vector_return(vector:(w64 1074102712), index:(w32 19), value:(w64 1074117064)[(w16 18)])
  // Module VectorTableUpdate
  buffer_t value_39(2);
  value_39[0] = 0;
  value_39[1] = 18;
  state->vector_table_1074102712.write(19, value_39);
  // BDD node 85:vector_borrow(vector:(w64 1074085496), index:(w32 20), val_out:(w64 1074052696)[ -> (w64 1074099872)])
  // Module Ignore
  // BDD node 86:vector_return(vector:(w64 1074085496), index:(w32 20), value:(w64 1074099872)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_40;
  state->vector_register_1074085496.get(20, value_40);
  value_40[3] = 1;
  value_40[2] = 0;
  value_40[1] = 0;
  value_40[0] = 0;
  state->vector_register_1074085496.put(20, value_40);
  // BDD node 87:vector_borrow(vector:(w64 1074102712), index:(w32 20), val_out:(w64 1074052760)[ -> (w64 1074117088)])
  // Module Ignore
  // BDD node 88:vector_return(vector:(w64 1074102712), index:(w32 20), value:(w64 1074117088)[(w16 21)])
  // Module VectorTableUpdate
  buffer_t value_41(2);
  value_41[0] = 0;
  value_41[1] = 21;
  state->vector_table_1074102712.write(20, value_41);
  // BDD node 89:vector_borrow(vector:(w64 1074085496), index:(w32 21), val_out:(w64 1074052696)[ -> (w64 1074099896)])
  // Module Ignore
  // BDD node 90:vector_return(vector:(w64 1074085496), index:(w32 21), value:(w64 1074099896)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_42;
  state->vector_register_1074085496.get(21, value_42);
  value_42[3] = 0;
  value_42[2] = 0;
  value_42[1] = 0;
  value_42[0] = 0;
  state->vector_register_1074085496.put(21, value_42);
  // BDD node 91:vector_borrow(vector:(w64 1074102712), index:(w32 21), val_out:(w64 1074052760)[ -> (w64 1074117112)])
  // Module Ignore
  // BDD node 92:vector_return(vector:(w64 1074102712), index:(w32 21), value:(w64 1074117112)[(w16 20)])
  // Module VectorTableUpdate
  buffer_t value_43(2);
  value_43[0] = 0;
  value_43[1] = 20;
  state->vector_table_1074102712.write(21, value_43);
  // BDD node 93:vector_borrow(vector:(w64 1074085496), index:(w32 22), val_out:(w64 1074052696)[ -> (w64 1074099920)])
  // Module Ignore
  // BDD node 94:vector_return(vector:(w64 1074085496), index:(w32 22), value:(w64 1074099920)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_44;
  state->vector_register_1074085496.get(22, value_44);
  value_44[3] = 1;
  value_44[2] = 0;
  value_44[1] = 0;
  value_44[0] = 0;
  state->vector_register_1074085496.put(22, value_44);
  // BDD node 95:vector_borrow(vector:(w64 1074102712), index:(w32 22), val_out:(w64 1074052760)[ -> (w64 1074117136)])
  // Module Ignore
  // BDD node 96:vector_return(vector:(w64 1074102712), index:(w32 22), value:(w64 1074117136)[(w16 23)])
  // Module VectorTableUpdate
  buffer_t value_45(2);
  value_45[0] = 0;
  value_45[1] = 23;
  state->vector_table_1074102712.write(22, value_45);
  // BDD node 97:vector_borrow(vector:(w64 1074085496), index:(w32 23), val_out:(w64 1074052696)[ -> (w64 1074099944)])
  // Module Ignore
  // BDD node 98:vector_return(vector:(w64 1074085496), index:(w32 23), value:(w64 1074099944)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_46;
  state->vector_register_1074085496.get(23, value_46);
  value_46[3] = 0;
  value_46[2] = 0;
  value_46[1] = 0;
  value_46[0] = 0;
  state->vector_register_1074085496.put(23, value_46);
  // BDD node 99:vector_borrow(vector:(w64 1074102712), index:(w32 23), val_out:(w64 1074052760)[ -> (w64 1074117160)])
  // Module Ignore
  // BDD node 100:vector_return(vector:(w64 1074102712), index:(w32 23), value:(w64 1074117160)[(w16 22)])
  // Module VectorTableUpdate
  buffer_t value_47(2);
  value_47[0] = 0;
  value_47[1] = 22;
  state->vector_table_1074102712.write(23, value_47);
  // BDD node 101:vector_borrow(vector:(w64 1074085496), index:(w32 24), val_out:(w64 1074052696)[ -> (w64 1074099968)])
  // Module Ignore
  // BDD node 102:vector_return(vector:(w64 1074085496), index:(w32 24), value:(w64 1074099968)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_48;
  state->vector_register_1074085496.get(24, value_48);
  value_48[3] = 1;
  value_48[2] = 0;
  value_48[1] = 0;
  value_48[0] = 0;
  state->vector_register_1074085496.put(24, value_48);
  // BDD node 103:vector_borrow(vector:(w64 1074102712), index:(w32 24), val_out:(w64 1074052760)[ -> (w64 1074117184)])
  // Module Ignore
  // BDD node 104:vector_return(vector:(w64 1074102712), index:(w32 24), value:(w64 1074117184)[(w16 25)])
  // Module VectorTableUpdate
  buffer_t value_49(2);
  value_49[0] = 0;
  value_49[1] = 25;
  state->vector_table_1074102712.write(24, value_49);
  // BDD node 105:vector_borrow(vector:(w64 1074085496), index:(w32 25), val_out:(w64 1074052696)[ -> (w64 1074099992)])
  // Module Ignore
  // BDD node 106:vector_return(vector:(w64 1074085496), index:(w32 25), value:(w64 1074099992)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_50;
  state->vector_register_1074085496.get(25, value_50);
  value_50[3] = 0;
  value_50[2] = 0;
  value_50[1] = 0;
  value_50[0] = 0;
  state->vector_register_1074085496.put(25, value_50);
  // BDD node 107:vector_borrow(vector:(w64 1074102712), index:(w32 25), val_out:(w64 1074052760)[ -> (w64 1074117208)])
  // Module Ignore
  // BDD node 108:vector_return(vector:(w64 1074102712), index:(w32 25), value:(w64 1074117208)[(w16 24)])
  // Module VectorTableUpdate
  buffer_t value_51(2);
  value_51[0] = 0;
  value_51[1] = 24;
  state->vector_table_1074102712.write(25, value_51);
  // BDD node 109:vector_borrow(vector:(w64 1074085496), index:(w32 26), val_out:(w64 1074052696)[ -> (w64 1074100016)])
  // Module Ignore
  // BDD node 110:vector_return(vector:(w64 1074085496), index:(w32 26), value:(w64 1074100016)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_52;
  state->vector_register_1074085496.get(26, value_52);
  value_52[3] = 1;
  value_52[2] = 0;
  value_52[1] = 0;
  value_52[0] = 0;
  state->vector_register_1074085496.put(26, value_52);
  // BDD node 111:vector_borrow(vector:(w64 1074102712), index:(w32 26), val_out:(w64 1074052760)[ -> (w64 1074117232)])
  // Module Ignore
  // BDD node 112:vector_return(vector:(w64 1074102712), index:(w32 26), value:(w64 1074117232)[(w16 27)])
  // Module VectorTableUpdate
  buffer_t value_53(2);
  value_53[0] = 0;
  value_53[1] = 27;
  state->vector_table_1074102712.write(26, value_53);
  // BDD node 113:vector_borrow(vector:(w64 1074085496), index:(w32 27), val_out:(w64 1074052696)[ -> (w64 1074100040)])
  // Module Ignore
  // BDD node 114:vector_return(vector:(w64 1074085496), index:(w32 27), value:(w64 1074100040)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_54;
  state->vector_register_1074085496.get(27, value_54);
  value_54[3] = 0;
  value_54[2] = 0;
  value_54[1] = 0;
  value_54[0] = 0;
  state->vector_register_1074085496.put(27, value_54);
  // BDD node 115:vector_borrow(vector:(w64 1074102712), index:(w32 27), val_out:(w64 1074052760)[ -> (w64 1074117256)])
  // Module Ignore
  // BDD node 116:vector_return(vector:(w64 1074102712), index:(w32 27), value:(w64 1074117256)[(w16 26)])
  // Module VectorTableUpdate
  buffer_t value_55(2);
  value_55[0] = 0;
  value_55[1] = 26;
  state->vector_table_1074102712.write(27, value_55);
  // BDD node 117:vector_borrow(vector:(w64 1074085496), index:(w32 28), val_out:(w64 1074052696)[ -> (w64 1074100064)])
  // Module Ignore
  // BDD node 118:vector_return(vector:(w64 1074085496), index:(w32 28), value:(w64 1074100064)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_56;
  state->vector_register_1074085496.get(28, value_56);
  value_56[3] = 1;
  value_56[2] = 0;
  value_56[1] = 0;
  value_56[0] = 0;
  state->vector_register_1074085496.put(28, value_56);
  // BDD node 119:vector_borrow(vector:(w64 1074102712), index:(w32 28), val_out:(w64 1074052760)[ -> (w64 1074117280)])
  // Module Ignore
  // BDD node 120:vector_return(vector:(w64 1074102712), index:(w32 28), value:(w64 1074117280)[(w16 29)])
  // Module VectorTableUpdate
  buffer_t value_57(2);
  value_57[0] = 0;
  value_57[1] = 29;
  state->vector_table_1074102712.write(28, value_57);
  // BDD node 121:vector_borrow(vector:(w64 1074085496), index:(w32 29), val_out:(w64 1074052696)[ -> (w64 1074100088)])
  // Module Ignore
  // BDD node 122:vector_return(vector:(w64 1074085496), index:(w32 29), value:(w64 1074100088)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_58;
  state->vector_register_1074085496.get(29, value_58);
  value_58[3] = 0;
  value_58[2] = 0;
  value_58[1] = 0;
  value_58[0] = 0;
  state->vector_register_1074085496.put(29, value_58);
  // BDD node 123:vector_borrow(vector:(w64 1074102712), index:(w32 29), val_out:(w64 1074052760)[ -> (w64 1074117304)])
  // Module Ignore
  // BDD node 124:vector_return(vector:(w64 1074102712), index:(w32 29), value:(w64 1074117304)[(w16 28)])
  // Module VectorTableUpdate
  buffer_t value_59(2);
  value_59[0] = 0;
  value_59[1] = 28;
  state->vector_table_1074102712.write(29, value_59);
  // BDD node 125:vector_borrow(vector:(w64 1074085496), index:(w32 30), val_out:(w64 1074052696)[ -> (w64 1074100112)])
  // Module Ignore
  // BDD node 126:vector_return(vector:(w64 1074085496), index:(w32 30), value:(w64 1074100112)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_60;
  state->vector_register_1074085496.get(30, value_60);
  value_60[3] = 1;
  value_60[2] = 0;
  value_60[1] = 0;
  value_60[0] = 0;
  state->vector_register_1074085496.put(30, value_60);
  // BDD node 127:vector_borrow(vector:(w64 1074102712), index:(w32 30), val_out:(w64 1074052760)[ -> (w64 1074117328)])
  // Module Ignore
  // BDD node 128:vector_return(vector:(w64 1074102712), index:(w32 30), value:(w64 1074117328)[(w16 31)])
  // Module VectorTableUpdate
  buffer_t value_61(2);
  value_61[0] = 0;
  value_61[1] = 31;
  state->vector_table_1074102712.write(30, value_61);
  // BDD node 129:vector_borrow(vector:(w64 1074085496), index:(w32 31), val_out:(w64 1074052696)[ -> (w64 1074100136)])
  // Module Ignore
  // BDD node 130:vector_return(vector:(w64 1074085496), index:(w32 31), value:(w64 1074100136)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_62;
  state->vector_register_1074085496.get(31, value_62);
  value_62[3] = 0;
  value_62[2] = 0;
  value_62[1] = 0;
  value_62[0] = 0;
  state->vector_register_1074085496.put(31, value_62);
  // BDD node 131:vector_borrow(vector:(w64 1074102712), index:(w32 31), val_out:(w64 1074052760)[ -> (w64 1074117352)])
  // Module Ignore
  // BDD node 132:vector_return(vector:(w64 1074102712), index:(w32 31), value:(w64 1074117352)[(w16 30)])
  // Module VectorTableUpdate
  buffer_t value_63(2);
  value_63[0] = 0;
  value_63[1] = 30;
  state->vector_table_1074102712.write(31, value_63);

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
  bool trigger_update_ipv4_tcpudp_checksums = false;
  void* l3_hdr = nullptr;
  void* l4_hdr = nullptr;

  cpu_hdr_t *cpu_hdr = packet_consume<cpu_hdr_t>(pkt);
  cpu_hdr_extra_t *cpu_hdr_extra = packet_consume<cpu_hdr_extra_t>(pkt);
  DEBUG("[t=%lu] New packet (size=%u, code_path=%d)\n", now, size, bswap16(cpu_hdr->code_path));

  if (bswap16(cpu_hdr->code_path) == 573) {
    // EP node  8470
    // BDD node 261:packet_borrow_next_chunk(p:(w64 1074206600), length:(w32 14), chunk:(w64 1074217352)[ -> (w64 1073763264)])
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  8526
    // BDD node 262:packet_borrow_next_chunk(p:(w64 1074206600), length:(w32 20), chunk:(w64 1074218088)[ -> (w64 1073763520)])
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  8583
    // BDD node 263:packet_borrow_next_chunk(p:(w64 1074206600), length:(w32 4), chunk:(w64 1074218744)[ -> (w64 1073763776)])
    u8* hdr_2 = packet_consume(pkt, 4);
    // EP node  8641
    // BDD node 167:dchain_allocate_new_index(chain:(w64 1074085072), index_out:(w64 1074223792)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u32 allocated_index_0;
    bool success_0 = state->dchain_table_1074085072.allocate_new_index(allocated_index_0);
    // EP node  8700
    // BDD node 168:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
    if ((0) == (success_0)) {
      // EP node  8701
      // BDD node 168:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
      // EP node  9733
      // BDD node 172:DROP
      forward = false;
    } else {
      // EP node  8702
      // BDD node 168:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
      // EP node  8762
      // BDD node 173:vector_borrow(vector:(w64 1074066912), index:(ReadLSB w32 (w32 0) new_index), val_out:(w64 1074223816)[ -> (w64 1074080808)])
      // EP node  8825
      // BDD node 174:map_put(map:(w64 1074053088), key:(w64 1074080808)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks))))))))))], value:(ReadLSB w32 (w32 0) new_index))
      buffer_t key_0(13);
      key_0[0] = *(u8*)(hdr_2 + 0);
      key_0[1] = *(u8*)(hdr_2 + 1);
      key_0[2] = *(u8*)(hdr_2 + 2);
      key_0[3] = *(u8*)(hdr_2 + 3);
      key_0[4] = *(u8*)(hdr_1 + 12);
      key_0[5] = *(u8*)(hdr_1 + 13);
      key_0[6] = *(u8*)(hdr_1 + 14);
      key_0[7] = *(u8*)(hdr_1 + 15);
      key_0[8] = *(u8*)(hdr_1 + 16);
      key_0[9] = *(u8*)(hdr_1 + 17);
      key_0[10] = *(u8*)(hdr_1 + 18);
      key_0[11] = *(u8*)(hdr_1 + 19);
      key_0[12] = *(u8*)(hdr_1 + 9);
      state->map_table_1074053088.put(key_0, allocated_index_0);
      // EP node  8889
      // BDD node 175:vector_return(vector:(w64 1074066912), index:(ReadLSB w32 (w32 0) new_index), value:(w64 1074080808)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks))))))))))])
      buffer_t value_64(13);
      value_64[0] = (u8)key_0.get(0, 1);
      value_64[1] = (u8)key_0.get(1, 1);
      value_64[2] = (u8)key_0.get(2, 1);
      value_64[3] = (u8)key_0.get(3, 1);
      value_64[4] = (u8)key_0.get(4, 1);
      value_64[5] = (u8)key_0.get(5, 1);
      value_64[6] = (u8)key_0.get(6, 1);
      value_64[7] = (u8)key_0.get(7, 1);
      value_64[8] = (u8)key_0.get(8, 1);
      value_64[9] = (u8)key_0.get(9, 1);
      value_64[10] = (u8)key_0.get(10, 1);
      value_64[11] = (u8)key_0.get(11, 1);
      value_64[12] = (u8)key_0.get(12, 1);
      state->vector_table_1074066912.write(allocated_index_0, value_64);
      // EP node  9019
      // BDD node 176:nf_set_rte_ipv4_udptcp_checksum(ip_header:(w64 1073763520), l4_header:(w64 1073763776), packet:(w64 1074155224))
      trigger_update_ipv4_tcpudp_checksums = true;
      l3_hdr = (void *)hdr_1;
      l4_hdr = (void *)hdr_2;
      // EP node  9020
      // BDD node 177:vector_borrow(vector:(w64 1074102712), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074225608)[ -> (w64 1074116608)])
      buffer_t value_65;
      state->vector_table_1074102712.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_65);
      // EP node  9087
      // BDD node 178:vector_return(vector:(w64 1074102712), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074116608)[(ReadLSB w16 (w32 0) vector_data_640)])
      // EP node  9223
      // BDD node 179:packet_return_chunk(p:(w64 1074206600), the_chunk:(w64 1073763776)[(Concat w32 (Read w8 (w32 515) packet_chunks) (Concat w24 (Read w8 (w32 514) packet_chunks) (ReadLSB w16 (w32 0) new_index)))])
      hdr_2[0] = allocated_index_0 & 255;
      hdr_2[1] = (allocated_index_0>>8) & 255;
      // EP node  9293
      // BDD node 180:packet_return_chunk(p:(w64 1074206600), the_chunk:(w64 1073763520)[(Concat w160 (Read w8 (w32 275) packet_chunks) (Concat w152 (Read w8 (w32 274) packet_chunks) (Concat w144 (Read w8 (w32 273) packet_chunks) (Concat w136 (Read w8 (w32 272) packet_chunks) (Concat w128 (w8 4) (Concat w120 (w8 3) (Concat w112 (w8 2) (Concat w104 (w8 1) (Concat w96 (Read w8 (w32 1) checksum) (Concat w88 (Read w8 (w32 0) checksum) (ReadLSB w80 (w32 256) packet_chunks)))))))))))])
      hdr_1[12] = 1;
      hdr_1[13] = 2;
      hdr_1[14] = 3;
      hdr_1[15] = 4;
      // EP node  9364
      // BDD node 182:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
      if ((65535) != ((u16)value_65.get(0, 2))) {
        // EP node  9365
        // BDD node 182:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
        // EP node  9510
        // BDD node 183:FORWARD
        cpu_hdr->egress_dev = bswap16((u16)value_65.get(0, 2));
      } else {
        // EP node  9366
        // BDD node 182:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
        // EP node  9808
        // BDD node 184:DROP
        forward = false;
      }
    }
  }


  if (trigger_update_ipv4_tcpudp_checksums) {
    update_ipv4_tcpudp_checksums(l3_hdr, l4_hdr);
  }

  return forward;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
