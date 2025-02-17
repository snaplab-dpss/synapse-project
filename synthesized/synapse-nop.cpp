#include <sycon/sycon.h>

using namespace sycon;

struct state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardNFDev forward_nf_dev;
  Table<1,1> table_1074013032_65;

  state_t()
    : ingress_port_to_nf_dev(),
      forward_nf_dev(),
      table_1074013032_65("Ingress", "table_1074013032_65")
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
  // BDD node 0:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074012760)[(w64 0) -> (w64 1074013032)])
  // Module TableAllocate
  // BDD node 1:vector_borrow(vector:(w64 1074013032), index:(w32 0), val_out:(w64 1074012696)[ -> (w64 1074026928)])
  // Module Ignore
  // BDD node 2:vector_return(vector:(w64 1074013032), index:(w32 0), value:(w64 1074026928)[(w16 1)])
  // Module TableUpdate
  fields_t<1> table_key_0;
  table_key_0[0] = 0;
  fields_t<1> table_value_0;
  table_value_0[0] = 1;
  state->table_1074013032_65.put(table_key_0, table_value_0);
  // BDD node 3:vector_borrow(vector:(w64 1074013032), index:(w32 1), val_out:(w64 1074012696)[ -> (w64 1074026952)])
  // Module Ignore
  // BDD node 4:vector_return(vector:(w64 1074013032), index:(w32 1), value:(w64 1074026952)[(w16 0)])
  // Module TableUpdate
  fields_t<1> table_key_1;
  table_key_1[0] = 1;
  fields_t<1> table_value_1;
  table_value_1[0] = 0;
  state->table_1074013032_65.put(table_key_1, table_value_1);
  // BDD node 5:vector_borrow(vector:(w64 1074013032), index:(w32 2), val_out:(w64 1074012696)[ -> (w64 1074026976)])
  // Module Ignore
  // BDD node 6:vector_return(vector:(w64 1074013032), index:(w32 2), value:(w64 1074026976)[(w16 3)])
  // Module TableUpdate
  fields_t<1> table_key_2;
  table_key_2[0] = 2;
  fields_t<1> table_value_2;
  table_value_2[0] = 3;
  state->table_1074013032_65.put(table_key_2, table_value_2);
  // BDD node 7:vector_borrow(vector:(w64 1074013032), index:(w32 3), val_out:(w64 1074012696)[ -> (w64 1074027000)])
  // Module Ignore
  // BDD node 8:vector_return(vector:(w64 1074013032), index:(w32 3), value:(w64 1074027000)[(w16 2)])
  // Module TableUpdate
  fields_t<1> table_key_3;
  table_key_3[0] = 3;
  fields_t<1> table_value_3;
  table_value_3[0] = 2;
  state->table_1074013032_65.put(table_key_3, table_value_3);
  // BDD node 9:vector_borrow(vector:(w64 1074013032), index:(w32 4), val_out:(w64 1074012696)[ -> (w64 1074027024)])
  // Module Ignore
  // BDD node 10:vector_return(vector:(w64 1074013032), index:(w32 4), value:(w64 1074027024)[(w16 5)])
  // Module TableUpdate
  fields_t<1> table_key_4;
  table_key_4[0] = 4;
  fields_t<1> table_value_4;
  table_value_4[0] = 5;
  state->table_1074013032_65.put(table_key_4, table_value_4);
  // BDD node 11:vector_borrow(vector:(w64 1074013032), index:(w32 5), val_out:(w64 1074012696)[ -> (w64 1074027048)])
  // Module Ignore
  // BDD node 12:vector_return(vector:(w64 1074013032), index:(w32 5), value:(w64 1074027048)[(w16 4)])
  // Module TableUpdate
  fields_t<1> table_key_5;
  table_key_5[0] = 5;
  fields_t<1> table_value_5;
  table_value_5[0] = 4;
  state->table_1074013032_65.put(table_key_5, table_value_5);
  // BDD node 13:vector_borrow(vector:(w64 1074013032), index:(w32 6), val_out:(w64 1074012696)[ -> (w64 1074027072)])
  // Module Ignore
  // BDD node 14:vector_return(vector:(w64 1074013032), index:(w32 6), value:(w64 1074027072)[(w16 7)])
  // Module TableUpdate
  fields_t<1> table_key_6;
  table_key_6[0] = 6;
  fields_t<1> table_value_6;
  table_value_6[0] = 7;
  state->table_1074013032_65.put(table_key_6, table_value_6);
  // BDD node 15:vector_borrow(vector:(w64 1074013032), index:(w32 7), val_out:(w64 1074012696)[ -> (w64 1074027096)])
  // Module Ignore
  // BDD node 16:vector_return(vector:(w64 1074013032), index:(w32 7), value:(w64 1074027096)[(w16 6)])
  // Module TableUpdate
  fields_t<1> table_key_7;
  table_key_7[0] = 7;
  fields_t<1> table_value_7;
  table_value_7[0] = 6;
  state->table_1074013032_65.put(table_key_7, table_value_7);
  // BDD node 17:vector_borrow(vector:(w64 1074013032), index:(w32 8), val_out:(w64 1074012696)[ -> (w64 1074027120)])
  // Module Ignore
  // BDD node 18:vector_return(vector:(w64 1074013032), index:(w32 8), value:(w64 1074027120)[(w16 9)])
  // Module TableUpdate
  fields_t<1> table_key_8;
  table_key_8[0] = 8;
  fields_t<1> table_value_8;
  table_value_8[0] = 9;
  state->table_1074013032_65.put(table_key_8, table_value_8);
  // BDD node 19:vector_borrow(vector:(w64 1074013032), index:(w32 9), val_out:(w64 1074012696)[ -> (w64 1074027144)])
  // Module Ignore
  // BDD node 20:vector_return(vector:(w64 1074013032), index:(w32 9), value:(w64 1074027144)[(w16 8)])
  // Module TableUpdate
  fields_t<1> table_key_9;
  table_key_9[0] = 9;
  fields_t<1> table_value_9;
  table_value_9[0] = 8;
  state->table_1074013032_65.put(table_key_9, table_value_9);
  // BDD node 21:vector_borrow(vector:(w64 1074013032), index:(w32 10), val_out:(w64 1074012696)[ -> (w64 1074027168)])
  // Module Ignore
  // BDD node 22:vector_return(vector:(w64 1074013032), index:(w32 10), value:(w64 1074027168)[(w16 11)])
  // Module TableUpdate
  fields_t<1> table_key_10;
  table_key_10[0] = 10;
  fields_t<1> table_value_10;
  table_value_10[0] = 11;
  state->table_1074013032_65.put(table_key_10, table_value_10);
  // BDD node 23:vector_borrow(vector:(w64 1074013032), index:(w32 11), val_out:(w64 1074012696)[ -> (w64 1074027192)])
  // Module Ignore
  // BDD node 24:vector_return(vector:(w64 1074013032), index:(w32 11), value:(w64 1074027192)[(w16 10)])
  // Module TableUpdate
  fields_t<1> table_key_11;
  table_key_11[0] = 11;
  fields_t<1> table_value_11;
  table_value_11[0] = 10;
  state->table_1074013032_65.put(table_key_11, table_value_11);
  // BDD node 25:vector_borrow(vector:(w64 1074013032), index:(w32 12), val_out:(w64 1074012696)[ -> (w64 1074027216)])
  // Module Ignore
  // BDD node 26:vector_return(vector:(w64 1074013032), index:(w32 12), value:(w64 1074027216)[(w16 13)])
  // Module TableUpdate
  fields_t<1> table_key_12;
  table_key_12[0] = 12;
  fields_t<1> table_value_12;
  table_value_12[0] = 13;
  state->table_1074013032_65.put(table_key_12, table_value_12);
  // BDD node 27:vector_borrow(vector:(w64 1074013032), index:(w32 13), val_out:(w64 1074012696)[ -> (w64 1074027240)])
  // Module Ignore
  // BDD node 28:vector_return(vector:(w64 1074013032), index:(w32 13), value:(w64 1074027240)[(w16 12)])
  // Module TableUpdate
  fields_t<1> table_key_13;
  table_key_13[0] = 13;
  fields_t<1> table_value_13;
  table_value_13[0] = 12;
  state->table_1074013032_65.put(table_key_13, table_value_13);
  // BDD node 29:vector_borrow(vector:(w64 1074013032), index:(w32 14), val_out:(w64 1074012696)[ -> (w64 1074027264)])
  // Module Ignore
  // BDD node 30:vector_return(vector:(w64 1074013032), index:(w32 14), value:(w64 1074027264)[(w16 15)])
  // Module TableUpdate
  fields_t<1> table_key_14;
  table_key_14[0] = 14;
  fields_t<1> table_value_14;
  table_value_14[0] = 15;
  state->table_1074013032_65.put(table_key_14, table_value_14);
  // BDD node 31:vector_borrow(vector:(w64 1074013032), index:(w32 15), val_out:(w64 1074012696)[ -> (w64 1074027288)])
  // Module Ignore
  // BDD node 32:vector_return(vector:(w64 1074013032), index:(w32 15), value:(w64 1074027288)[(w16 14)])
  // Module TableUpdate
  fields_t<1> table_key_15;
  table_key_15[0] = 15;
  fields_t<1> table_value_15;
  table_value_15[0] = 14;
  state->table_1074013032_65.put(table_key_15, table_value_15);
  // BDD node 33:vector_borrow(vector:(w64 1074013032), index:(w32 16), val_out:(w64 1074012696)[ -> (w64 1074027312)])
  // Module Ignore
  // BDD node 34:vector_return(vector:(w64 1074013032), index:(w32 16), value:(w64 1074027312)[(w16 17)])
  // Module TableUpdate
  fields_t<1> table_key_16;
  table_key_16[0] = 16;
  fields_t<1> table_value_16;
  table_value_16[0] = 17;
  state->table_1074013032_65.put(table_key_16, table_value_16);
  // BDD node 35:vector_borrow(vector:(w64 1074013032), index:(w32 17), val_out:(w64 1074012696)[ -> (w64 1074027336)])
  // Module Ignore
  // BDD node 36:vector_return(vector:(w64 1074013032), index:(w32 17), value:(w64 1074027336)[(w16 16)])
  // Module TableUpdate
  fields_t<1> table_key_17;
  table_key_17[0] = 17;
  fields_t<1> table_value_17;
  table_value_17[0] = 16;
  state->table_1074013032_65.put(table_key_17, table_value_17);
  // BDD node 37:vector_borrow(vector:(w64 1074013032), index:(w32 18), val_out:(w64 1074012696)[ -> (w64 1074027360)])
  // Module Ignore
  // BDD node 38:vector_return(vector:(w64 1074013032), index:(w32 18), value:(w64 1074027360)[(w16 19)])
  // Module TableUpdate
  fields_t<1> table_key_18;
  table_key_18[0] = 18;
  fields_t<1> table_value_18;
  table_value_18[0] = 19;
  state->table_1074013032_65.put(table_key_18, table_value_18);
  // BDD node 39:vector_borrow(vector:(w64 1074013032), index:(w32 19), val_out:(w64 1074012696)[ -> (w64 1074027384)])
  // Module Ignore
  // BDD node 40:vector_return(vector:(w64 1074013032), index:(w32 19), value:(w64 1074027384)[(w16 18)])
  // Module TableUpdate
  fields_t<1> table_key_19;
  table_key_19[0] = 19;
  fields_t<1> table_value_19;
  table_value_19[0] = 18;
  state->table_1074013032_65.put(table_key_19, table_value_19);
  // BDD node 41:vector_borrow(vector:(w64 1074013032), index:(w32 20), val_out:(w64 1074012696)[ -> (w64 1074027408)])
  // Module Ignore
  // BDD node 42:vector_return(vector:(w64 1074013032), index:(w32 20), value:(w64 1074027408)[(w16 21)])
  // Module TableUpdate
  fields_t<1> table_key_20;
  table_key_20[0] = 20;
  fields_t<1> table_value_20;
  table_value_20[0] = 21;
  state->table_1074013032_65.put(table_key_20, table_value_20);
  // BDD node 43:vector_borrow(vector:(w64 1074013032), index:(w32 21), val_out:(w64 1074012696)[ -> (w64 1074027432)])
  // Module Ignore
  // BDD node 44:vector_return(vector:(w64 1074013032), index:(w32 21), value:(w64 1074027432)[(w16 20)])
  // Module TableUpdate
  fields_t<1> table_key_21;
  table_key_21[0] = 21;
  fields_t<1> table_value_21;
  table_value_21[0] = 20;
  state->table_1074013032_65.put(table_key_21, table_value_21);
  // BDD node 45:vector_borrow(vector:(w64 1074013032), index:(w32 22), val_out:(w64 1074012696)[ -> (w64 1074027456)])
  // Module Ignore
  // BDD node 46:vector_return(vector:(w64 1074013032), index:(w32 22), value:(w64 1074027456)[(w16 23)])
  // Module TableUpdate
  fields_t<1> table_key_22;
  table_key_22[0] = 22;
  fields_t<1> table_value_22;
  table_value_22[0] = 23;
  state->table_1074013032_65.put(table_key_22, table_value_22);
  // BDD node 47:vector_borrow(vector:(w64 1074013032), index:(w32 23), val_out:(w64 1074012696)[ -> (w64 1074027480)])
  // Module Ignore
  // BDD node 48:vector_return(vector:(w64 1074013032), index:(w32 23), value:(w64 1074027480)[(w16 22)])
  // Module TableUpdate
  fields_t<1> table_key_23;
  table_key_23[0] = 23;
  fields_t<1> table_value_23;
  table_value_23[0] = 22;
  state->table_1074013032_65.put(table_key_23, table_value_23);
  // BDD node 49:vector_borrow(vector:(w64 1074013032), index:(w32 24), val_out:(w64 1074012696)[ -> (w64 1074027504)])
  // Module Ignore
  // BDD node 50:vector_return(vector:(w64 1074013032), index:(w32 24), value:(w64 1074027504)[(w16 25)])
  // Module TableUpdate
  fields_t<1> table_key_24;
  table_key_24[0] = 24;
  fields_t<1> table_value_24;
  table_value_24[0] = 25;
  state->table_1074013032_65.put(table_key_24, table_value_24);
  // BDD node 51:vector_borrow(vector:(w64 1074013032), index:(w32 25), val_out:(w64 1074012696)[ -> (w64 1074027528)])
  // Module Ignore
  // BDD node 52:vector_return(vector:(w64 1074013032), index:(w32 25), value:(w64 1074027528)[(w16 24)])
  // Module TableUpdate
  fields_t<1> table_key_25;
  table_key_25[0] = 25;
  fields_t<1> table_value_25;
  table_value_25[0] = 24;
  state->table_1074013032_65.put(table_key_25, table_value_25);
  // BDD node 53:vector_borrow(vector:(w64 1074013032), index:(w32 26), val_out:(w64 1074012696)[ -> (w64 1074027552)])
  // Module Ignore
  // BDD node 54:vector_return(vector:(w64 1074013032), index:(w32 26), value:(w64 1074027552)[(w16 27)])
  // Module TableUpdate
  fields_t<1> table_key_26;
  table_key_26[0] = 26;
  fields_t<1> table_value_26;
  table_value_26[0] = 27;
  state->table_1074013032_65.put(table_key_26, table_value_26);
  // BDD node 55:vector_borrow(vector:(w64 1074013032), index:(w32 27), val_out:(w64 1074012696)[ -> (w64 1074027576)])
  // Module Ignore
  // BDD node 56:vector_return(vector:(w64 1074013032), index:(w32 27), value:(w64 1074027576)[(w16 26)])
  // Module TableUpdate
  fields_t<1> table_key_27;
  table_key_27[0] = 27;
  fields_t<1> table_value_27;
  table_value_27[0] = 26;
  state->table_1074013032_65.put(table_key_27, table_value_27);
  // BDD node 57:vector_borrow(vector:(w64 1074013032), index:(w32 28), val_out:(w64 1074012696)[ -> (w64 1074027600)])
  // Module Ignore
  // BDD node 58:vector_return(vector:(w64 1074013032), index:(w32 28), value:(w64 1074027600)[(w16 29)])
  // Module TableUpdate
  fields_t<1> table_key_28;
  table_key_28[0] = 28;
  fields_t<1> table_value_28;
  table_value_28[0] = 29;
  state->table_1074013032_65.put(table_key_28, table_value_28);
  // BDD node 59:vector_borrow(vector:(w64 1074013032), index:(w32 29), val_out:(w64 1074012696)[ -> (w64 1074027624)])
  // Module Ignore
  // BDD node 60:vector_return(vector:(w64 1074013032), index:(w32 29), value:(w64 1074027624)[(w16 28)])
  // Module TableUpdate
  fields_t<1> table_key_29;
  table_key_29[0] = 29;
  fields_t<1> table_value_29;
  table_value_29[0] = 28;
  state->table_1074013032_65.put(table_key_29, table_value_29);
  // BDD node 61:vector_borrow(vector:(w64 1074013032), index:(w32 30), val_out:(w64 1074012696)[ -> (w64 1074027648)])
  // Module Ignore
  // BDD node 62:vector_return(vector:(w64 1074013032), index:(w32 30), value:(w64 1074027648)[(w16 31)])
  // Module TableUpdate
  fields_t<1> table_key_30;
  table_key_30[0] = 30;
  fields_t<1> table_value_30;
  table_value_30[0] = 31;
  state->table_1074013032_65.put(table_key_30, table_value_30);
  // BDD node 63:vector_borrow(vector:(w64 1074013032), index:(w32 31), val_out:(w64 1074012696)[ -> (w64 1074027672)])
  // Module Ignore
  // BDD node 64:vector_return(vector:(w64 1074013032), index:(w32 31), value:(w64 1074027672)[(w16 30)])
  // Module TableUpdate
  fields_t<1> table_key_31;
  table_key_31[0] = 31;
  fields_t<1> table_value_31;
  table_value_31[0] = 30;
  state->table_1074013032_65.put(table_key_31, table_value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

bool sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  cpu_hdr_t *cpu_hdr = (cpu_hdr_t *)packet_consume(pkt, sizeof(cpu_hdr_t));

}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
