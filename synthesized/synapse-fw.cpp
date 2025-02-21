#include <sycon/sycon.h>

using namespace sycon;

struct state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardNFDev forward_nf_dev;
  Table<5,1> table_1074043808_142;
  Table<3,1> table_1074043808_157;
  Table<1,0> table_1074075792_180;
  VectorRegister vector_register_1074076216_139;
  VectorRegister vector_register_1074093432_181;

  state_t()
    : ingress_port_to_nf_dev(),
      forward_nf_dev(),
      table_1074043808_142("Ingress", "table_1074043808_142"),
      table_1074043808_157("Ingress", "table_1074043808_157"),
      table_1074075792_180("Ingress", "table_1074075792_180"),
      vector_register_1074076216_139("Ingress", {"vector_register_1074076216_139_0",}),
      vector_register_1074093432_181("Ingress", {"vector_register_1074093432_181_0",})
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
  // BDD node 0:map_allocate(capacity:(w32 65536), key_size:(w32 13), map_out:(w64 1074043544)[(w64 0) -> (w64 1074043808)])
  // Module TableAllocate
  // BDD node 2:dchain_allocate(index_range:(w32 65536), chain_out:(w64 1074043560)[ -> (w64 1074075792)])
  // Module TableAllocate
  // BDD node 3:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1074043568)[(w64 0) -> (w64 1074076216)])
  // Module VectorRegisterAllocate
  // BDD node 4:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074043576)[(w64 0) -> (w64 1074093432)])
  // Module VectorRegisterAllocate
  // BDD node 5:vector_borrow(vector:(w64 1074076216), index:(w32 0), val_out:(w64 1074043416)[ -> (w64 1074090112)])
  // Module Ignore
  // BDD node 6:vector_return(vector:(w64 1074076216), index:(w32 0), value:(w64 1074090112)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_0;
  state->vector_register_1074076216_139.get(0, value_0);
  value_0[0] = 1;
  value_0[1] = 0;
  value_0[2] = 0;
  value_0[3] = 0;
  state->vector_register_1074076216_139.put(0, value_0);
  // BDD node 7:vector_borrow(vector:(w64 1074093432), index:(w32 0), val_out:(w64 1074043480)[ -> (w64 1074107328)])
  // Module Ignore
  // BDD node 8:vector_return(vector:(w64 1074093432), index:(w32 0), value:(w64 1074107328)[(w16 1)])
  // Module VectorRegisterUpdate
  buffer_t value_1;
  state->vector_register_1074093432_181.get(0, value_1);
  value_1[0] = 1;
  value_1[1] = 0;
  state->vector_register_1074093432_181.put(0, value_1);
  // BDD node 9:vector_borrow(vector:(w64 1074076216), index:(w32 1), val_out:(w64 1074043416)[ -> (w64 1074090136)])
  // Module Ignore
  // BDD node 10:vector_return(vector:(w64 1074076216), index:(w32 1), value:(w64 1074090136)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_2;
  state->vector_register_1074076216_139.get(1, value_2);
  value_2[0] = 0;
  value_2[1] = 0;
  value_2[2] = 0;
  value_2[3] = 0;
  state->vector_register_1074076216_139.put(1, value_2);
  // BDD node 11:vector_borrow(vector:(w64 1074093432), index:(w32 1), val_out:(w64 1074043480)[ -> (w64 1074107352)])
  // Module Ignore
  // BDD node 12:vector_return(vector:(w64 1074093432), index:(w32 1), value:(w64 1074107352)[(w16 0)])
  // Module VectorRegisterUpdate
  buffer_t value_3;
  state->vector_register_1074093432_181.get(1, value_3);
  value_3[0] = 0;
  value_3[1] = 0;
  state->vector_register_1074093432_181.put(1, value_3);
  // BDD node 13:vector_borrow(vector:(w64 1074076216), index:(w32 2), val_out:(w64 1074043416)[ -> (w64 1074090160)])
  // Module Ignore
  // BDD node 14:vector_return(vector:(w64 1074076216), index:(w32 2), value:(w64 1074090160)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_4;
  state->vector_register_1074076216_139.get(2, value_4);
  value_4[0] = 1;
  value_4[1] = 0;
  value_4[2] = 0;
  value_4[3] = 0;
  state->vector_register_1074076216_139.put(2, value_4);
  // BDD node 15:vector_borrow(vector:(w64 1074093432), index:(w32 2), val_out:(w64 1074043480)[ -> (w64 1074107376)])
  // Module Ignore
  // BDD node 16:vector_return(vector:(w64 1074093432), index:(w32 2), value:(w64 1074107376)[(w16 3)])
  // Module VectorRegisterUpdate
  buffer_t value_5;
  state->vector_register_1074093432_181.get(2, value_5);
  value_5[0] = 3;
  value_5[1] = 0;
  state->vector_register_1074093432_181.put(2, value_5);
  // BDD node 17:vector_borrow(vector:(w64 1074076216), index:(w32 3), val_out:(w64 1074043416)[ -> (w64 1074090184)])
  // Module Ignore
  // BDD node 18:vector_return(vector:(w64 1074076216), index:(w32 3), value:(w64 1074090184)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_6;
  state->vector_register_1074076216_139.get(3, value_6);
  value_6[0] = 0;
  value_6[1] = 0;
  value_6[2] = 0;
  value_6[3] = 0;
  state->vector_register_1074076216_139.put(3, value_6);
  // BDD node 19:vector_borrow(vector:(w64 1074093432), index:(w32 3), val_out:(w64 1074043480)[ -> (w64 1074107400)])
  // Module Ignore
  // BDD node 20:vector_return(vector:(w64 1074093432), index:(w32 3), value:(w64 1074107400)[(w16 2)])
  // Module VectorRegisterUpdate
  buffer_t value_7;
  state->vector_register_1074093432_181.get(3, value_7);
  value_7[0] = 2;
  value_7[1] = 0;
  state->vector_register_1074093432_181.put(3, value_7);
  // BDD node 21:vector_borrow(vector:(w64 1074076216), index:(w32 4), val_out:(w64 1074043416)[ -> (w64 1074090208)])
  // Module Ignore
  // BDD node 22:vector_return(vector:(w64 1074076216), index:(w32 4), value:(w64 1074090208)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_8;
  state->vector_register_1074076216_139.get(4, value_8);
  value_8[0] = 1;
  value_8[1] = 0;
  value_8[2] = 0;
  value_8[3] = 0;
  state->vector_register_1074076216_139.put(4, value_8);
  // BDD node 23:vector_borrow(vector:(w64 1074093432), index:(w32 4), val_out:(w64 1074043480)[ -> (w64 1074107424)])
  // Module Ignore
  // BDD node 24:vector_return(vector:(w64 1074093432), index:(w32 4), value:(w64 1074107424)[(w16 5)])
  // Module VectorRegisterUpdate
  buffer_t value_9;
  state->vector_register_1074093432_181.get(4, value_9);
  value_9[0] = 5;
  value_9[1] = 0;
  state->vector_register_1074093432_181.put(4, value_9);
  // BDD node 25:vector_borrow(vector:(w64 1074076216), index:(w32 5), val_out:(w64 1074043416)[ -> (w64 1074090232)])
  // Module Ignore
  // BDD node 26:vector_return(vector:(w64 1074076216), index:(w32 5), value:(w64 1074090232)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_10;
  state->vector_register_1074076216_139.get(5, value_10);
  value_10[0] = 0;
  value_10[1] = 0;
  value_10[2] = 0;
  value_10[3] = 0;
  state->vector_register_1074076216_139.put(5, value_10);
  // BDD node 27:vector_borrow(vector:(w64 1074093432), index:(w32 5), val_out:(w64 1074043480)[ -> (w64 1074107448)])
  // Module Ignore
  // BDD node 28:vector_return(vector:(w64 1074093432), index:(w32 5), value:(w64 1074107448)[(w16 4)])
  // Module VectorRegisterUpdate
  buffer_t value_11;
  state->vector_register_1074093432_181.get(5, value_11);
  value_11[0] = 4;
  value_11[1] = 0;
  state->vector_register_1074093432_181.put(5, value_11);
  // BDD node 29:vector_borrow(vector:(w64 1074076216), index:(w32 6), val_out:(w64 1074043416)[ -> (w64 1074090256)])
  // Module Ignore
  // BDD node 30:vector_return(vector:(w64 1074076216), index:(w32 6), value:(w64 1074090256)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_12;
  state->vector_register_1074076216_139.get(6, value_12);
  value_12[0] = 1;
  value_12[1] = 0;
  value_12[2] = 0;
  value_12[3] = 0;
  state->vector_register_1074076216_139.put(6, value_12);
  // BDD node 31:vector_borrow(vector:(w64 1074093432), index:(w32 6), val_out:(w64 1074043480)[ -> (w64 1074107472)])
  // Module Ignore
  // BDD node 32:vector_return(vector:(w64 1074093432), index:(w32 6), value:(w64 1074107472)[(w16 7)])
  // Module VectorRegisterUpdate
  buffer_t value_13;
  state->vector_register_1074093432_181.get(6, value_13);
  value_13[0] = 7;
  value_13[1] = 0;
  state->vector_register_1074093432_181.put(6, value_13);
  // BDD node 33:vector_borrow(vector:(w64 1074076216), index:(w32 7), val_out:(w64 1074043416)[ -> (w64 1074090280)])
  // Module Ignore
  // BDD node 34:vector_return(vector:(w64 1074076216), index:(w32 7), value:(w64 1074090280)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_14;
  state->vector_register_1074076216_139.get(7, value_14);
  value_14[0] = 0;
  value_14[1] = 0;
  value_14[2] = 0;
  value_14[3] = 0;
  state->vector_register_1074076216_139.put(7, value_14);
  // BDD node 35:vector_borrow(vector:(w64 1074093432), index:(w32 7), val_out:(w64 1074043480)[ -> (w64 1074107496)])
  // Module Ignore
  // BDD node 36:vector_return(vector:(w64 1074093432), index:(w32 7), value:(w64 1074107496)[(w16 6)])
  // Module VectorRegisterUpdate
  buffer_t value_15;
  state->vector_register_1074093432_181.get(7, value_15);
  value_15[0] = 6;
  value_15[1] = 0;
  state->vector_register_1074093432_181.put(7, value_15);
  // BDD node 37:vector_borrow(vector:(w64 1074076216), index:(w32 8), val_out:(w64 1074043416)[ -> (w64 1074090304)])
  // Module Ignore
  // BDD node 38:vector_return(vector:(w64 1074076216), index:(w32 8), value:(w64 1074090304)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_16;
  state->vector_register_1074076216_139.get(8, value_16);
  value_16[0] = 1;
  value_16[1] = 0;
  value_16[2] = 0;
  value_16[3] = 0;
  state->vector_register_1074076216_139.put(8, value_16);
  // BDD node 39:vector_borrow(vector:(w64 1074093432), index:(w32 8), val_out:(w64 1074043480)[ -> (w64 1074107520)])
  // Module Ignore
  // BDD node 40:vector_return(vector:(w64 1074093432), index:(w32 8), value:(w64 1074107520)[(w16 9)])
  // Module VectorRegisterUpdate
  buffer_t value_17;
  state->vector_register_1074093432_181.get(8, value_17);
  value_17[0] = 9;
  value_17[1] = 0;
  state->vector_register_1074093432_181.put(8, value_17);
  // BDD node 41:vector_borrow(vector:(w64 1074076216), index:(w32 9), val_out:(w64 1074043416)[ -> (w64 1074090328)])
  // Module Ignore
  // BDD node 42:vector_return(vector:(w64 1074076216), index:(w32 9), value:(w64 1074090328)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_18;
  state->vector_register_1074076216_139.get(9, value_18);
  value_18[0] = 0;
  value_18[1] = 0;
  value_18[2] = 0;
  value_18[3] = 0;
  state->vector_register_1074076216_139.put(9, value_18);
  // BDD node 43:vector_borrow(vector:(w64 1074093432), index:(w32 9), val_out:(w64 1074043480)[ -> (w64 1074107544)])
  // Module Ignore
  // BDD node 44:vector_return(vector:(w64 1074093432), index:(w32 9), value:(w64 1074107544)[(w16 8)])
  // Module VectorRegisterUpdate
  buffer_t value_19;
  state->vector_register_1074093432_181.get(9, value_19);
  value_19[0] = 8;
  value_19[1] = 0;
  state->vector_register_1074093432_181.put(9, value_19);
  // BDD node 45:vector_borrow(vector:(w64 1074076216), index:(w32 10), val_out:(w64 1074043416)[ -> (w64 1074090352)])
  // Module Ignore
  // BDD node 46:vector_return(vector:(w64 1074076216), index:(w32 10), value:(w64 1074090352)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_20;
  state->vector_register_1074076216_139.get(10, value_20);
  value_20[0] = 1;
  value_20[1] = 0;
  value_20[2] = 0;
  value_20[3] = 0;
  state->vector_register_1074076216_139.put(10, value_20);
  // BDD node 47:vector_borrow(vector:(w64 1074093432), index:(w32 10), val_out:(w64 1074043480)[ -> (w64 1074107568)])
  // Module Ignore
  // BDD node 48:vector_return(vector:(w64 1074093432), index:(w32 10), value:(w64 1074107568)[(w16 11)])
  // Module VectorRegisterUpdate
  buffer_t value_21;
  state->vector_register_1074093432_181.get(10, value_21);
  value_21[0] = 11;
  value_21[1] = 0;
  state->vector_register_1074093432_181.put(10, value_21);
  // BDD node 49:vector_borrow(vector:(w64 1074076216), index:(w32 11), val_out:(w64 1074043416)[ -> (w64 1074090376)])
  // Module Ignore
  // BDD node 50:vector_return(vector:(w64 1074076216), index:(w32 11), value:(w64 1074090376)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_22;
  state->vector_register_1074076216_139.get(11, value_22);
  value_22[0] = 0;
  value_22[1] = 0;
  value_22[2] = 0;
  value_22[3] = 0;
  state->vector_register_1074076216_139.put(11, value_22);
  // BDD node 51:vector_borrow(vector:(w64 1074093432), index:(w32 11), val_out:(w64 1074043480)[ -> (w64 1074107592)])
  // Module Ignore
  // BDD node 52:vector_return(vector:(w64 1074093432), index:(w32 11), value:(w64 1074107592)[(w16 10)])
  // Module VectorRegisterUpdate
  buffer_t value_23;
  state->vector_register_1074093432_181.get(11, value_23);
  value_23[0] = 10;
  value_23[1] = 0;
  state->vector_register_1074093432_181.put(11, value_23);
  // BDD node 53:vector_borrow(vector:(w64 1074076216), index:(w32 12), val_out:(w64 1074043416)[ -> (w64 1074090400)])
  // Module Ignore
  // BDD node 54:vector_return(vector:(w64 1074076216), index:(w32 12), value:(w64 1074090400)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_24;
  state->vector_register_1074076216_139.get(12, value_24);
  value_24[0] = 1;
  value_24[1] = 0;
  value_24[2] = 0;
  value_24[3] = 0;
  state->vector_register_1074076216_139.put(12, value_24);
  // BDD node 55:vector_borrow(vector:(w64 1074093432), index:(w32 12), val_out:(w64 1074043480)[ -> (w64 1074107616)])
  // Module Ignore
  // BDD node 56:vector_return(vector:(w64 1074093432), index:(w32 12), value:(w64 1074107616)[(w16 13)])
  // Module VectorRegisterUpdate
  buffer_t value_25;
  state->vector_register_1074093432_181.get(12, value_25);
  value_25[0] = 13;
  value_25[1] = 0;
  state->vector_register_1074093432_181.put(12, value_25);
  // BDD node 57:vector_borrow(vector:(w64 1074076216), index:(w32 13), val_out:(w64 1074043416)[ -> (w64 1074090424)])
  // Module Ignore
  // BDD node 58:vector_return(vector:(w64 1074076216), index:(w32 13), value:(w64 1074090424)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_26;
  state->vector_register_1074076216_139.get(13, value_26);
  value_26[0] = 0;
  value_26[1] = 0;
  value_26[2] = 0;
  value_26[3] = 0;
  state->vector_register_1074076216_139.put(13, value_26);
  // BDD node 59:vector_borrow(vector:(w64 1074093432), index:(w32 13), val_out:(w64 1074043480)[ -> (w64 1074107640)])
  // Module Ignore
  // BDD node 60:vector_return(vector:(w64 1074093432), index:(w32 13), value:(w64 1074107640)[(w16 12)])
  // Module VectorRegisterUpdate
  buffer_t value_27;
  state->vector_register_1074093432_181.get(13, value_27);
  value_27[0] = 12;
  value_27[1] = 0;
  state->vector_register_1074093432_181.put(13, value_27);
  // BDD node 61:vector_borrow(vector:(w64 1074076216), index:(w32 14), val_out:(w64 1074043416)[ -> (w64 1074090448)])
  // Module Ignore
  // BDD node 62:vector_return(vector:(w64 1074076216), index:(w32 14), value:(w64 1074090448)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_28;
  state->vector_register_1074076216_139.get(14, value_28);
  value_28[0] = 1;
  value_28[1] = 0;
  value_28[2] = 0;
  value_28[3] = 0;
  state->vector_register_1074076216_139.put(14, value_28);
  // BDD node 63:vector_borrow(vector:(w64 1074093432), index:(w32 14), val_out:(w64 1074043480)[ -> (w64 1074107664)])
  // Module Ignore
  // BDD node 64:vector_return(vector:(w64 1074093432), index:(w32 14), value:(w64 1074107664)[(w16 15)])
  // Module VectorRegisterUpdate
  buffer_t value_29;
  state->vector_register_1074093432_181.get(14, value_29);
  value_29[0] = 15;
  value_29[1] = 0;
  state->vector_register_1074093432_181.put(14, value_29);
  // BDD node 65:vector_borrow(vector:(w64 1074076216), index:(w32 15), val_out:(w64 1074043416)[ -> (w64 1074090472)])
  // Module Ignore
  // BDD node 66:vector_return(vector:(w64 1074076216), index:(w32 15), value:(w64 1074090472)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_30;
  state->vector_register_1074076216_139.get(15, value_30);
  value_30[0] = 0;
  value_30[1] = 0;
  value_30[2] = 0;
  value_30[3] = 0;
  state->vector_register_1074076216_139.put(15, value_30);
  // BDD node 67:vector_borrow(vector:(w64 1074093432), index:(w32 15), val_out:(w64 1074043480)[ -> (w64 1074107688)])
  // Module Ignore
  // BDD node 68:vector_return(vector:(w64 1074093432), index:(w32 15), value:(w64 1074107688)[(w16 14)])
  // Module VectorRegisterUpdate
  buffer_t value_31;
  state->vector_register_1074093432_181.get(15, value_31);
  value_31[0] = 14;
  value_31[1] = 0;
  state->vector_register_1074093432_181.put(15, value_31);
  // BDD node 69:vector_borrow(vector:(w64 1074076216), index:(w32 16), val_out:(w64 1074043416)[ -> (w64 1074090496)])
  // Module Ignore
  // BDD node 70:vector_return(vector:(w64 1074076216), index:(w32 16), value:(w64 1074090496)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_32;
  state->vector_register_1074076216_139.get(16, value_32);
  value_32[0] = 1;
  value_32[1] = 0;
  value_32[2] = 0;
  value_32[3] = 0;
  state->vector_register_1074076216_139.put(16, value_32);
  // BDD node 71:vector_borrow(vector:(w64 1074093432), index:(w32 16), val_out:(w64 1074043480)[ -> (w64 1074107712)])
  // Module Ignore
  // BDD node 72:vector_return(vector:(w64 1074093432), index:(w32 16), value:(w64 1074107712)[(w16 17)])
  // Module VectorRegisterUpdate
  buffer_t value_33;
  state->vector_register_1074093432_181.get(16, value_33);
  value_33[0] = 17;
  value_33[1] = 0;
  state->vector_register_1074093432_181.put(16, value_33);
  // BDD node 73:vector_borrow(vector:(w64 1074076216), index:(w32 17), val_out:(w64 1074043416)[ -> (w64 1074090520)])
  // Module Ignore
  // BDD node 74:vector_return(vector:(w64 1074076216), index:(w32 17), value:(w64 1074090520)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_34;
  state->vector_register_1074076216_139.get(17, value_34);
  value_34[0] = 0;
  value_34[1] = 0;
  value_34[2] = 0;
  value_34[3] = 0;
  state->vector_register_1074076216_139.put(17, value_34);
  // BDD node 75:vector_borrow(vector:(w64 1074093432), index:(w32 17), val_out:(w64 1074043480)[ -> (w64 1074107736)])
  // Module Ignore
  // BDD node 76:vector_return(vector:(w64 1074093432), index:(w32 17), value:(w64 1074107736)[(w16 16)])
  // Module VectorRegisterUpdate
  buffer_t value_35;
  state->vector_register_1074093432_181.get(17, value_35);
  value_35[0] = 16;
  value_35[1] = 0;
  state->vector_register_1074093432_181.put(17, value_35);
  // BDD node 77:vector_borrow(vector:(w64 1074076216), index:(w32 18), val_out:(w64 1074043416)[ -> (w64 1074090544)])
  // Module Ignore
  // BDD node 78:vector_return(vector:(w64 1074076216), index:(w32 18), value:(w64 1074090544)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_36;
  state->vector_register_1074076216_139.get(18, value_36);
  value_36[0] = 1;
  value_36[1] = 0;
  value_36[2] = 0;
  value_36[3] = 0;
  state->vector_register_1074076216_139.put(18, value_36);
  // BDD node 79:vector_borrow(vector:(w64 1074093432), index:(w32 18), val_out:(w64 1074043480)[ -> (w64 1074107760)])
  // Module Ignore
  // BDD node 80:vector_return(vector:(w64 1074093432), index:(w32 18), value:(w64 1074107760)[(w16 19)])
  // Module VectorRegisterUpdate
  buffer_t value_37;
  state->vector_register_1074093432_181.get(18, value_37);
  value_37[0] = 19;
  value_37[1] = 0;
  state->vector_register_1074093432_181.put(18, value_37);
  // BDD node 81:vector_borrow(vector:(w64 1074076216), index:(w32 19), val_out:(w64 1074043416)[ -> (w64 1074090568)])
  // Module Ignore
  // BDD node 82:vector_return(vector:(w64 1074076216), index:(w32 19), value:(w64 1074090568)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_38;
  state->vector_register_1074076216_139.get(19, value_38);
  value_38[0] = 0;
  value_38[1] = 0;
  value_38[2] = 0;
  value_38[3] = 0;
  state->vector_register_1074076216_139.put(19, value_38);
  // BDD node 83:vector_borrow(vector:(w64 1074093432), index:(w32 19), val_out:(w64 1074043480)[ -> (w64 1074107784)])
  // Module Ignore
  // BDD node 84:vector_return(vector:(w64 1074093432), index:(w32 19), value:(w64 1074107784)[(w16 18)])
  // Module VectorRegisterUpdate
  buffer_t value_39;
  state->vector_register_1074093432_181.get(19, value_39);
  value_39[0] = 18;
  value_39[1] = 0;
  state->vector_register_1074093432_181.put(19, value_39);
  // BDD node 85:vector_borrow(vector:(w64 1074076216), index:(w32 20), val_out:(w64 1074043416)[ -> (w64 1074090592)])
  // Module Ignore
  // BDD node 86:vector_return(vector:(w64 1074076216), index:(w32 20), value:(w64 1074090592)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_40;
  state->vector_register_1074076216_139.get(20, value_40);
  value_40[0] = 1;
  value_40[1] = 0;
  value_40[2] = 0;
  value_40[3] = 0;
  state->vector_register_1074076216_139.put(20, value_40);
  // BDD node 87:vector_borrow(vector:(w64 1074093432), index:(w32 20), val_out:(w64 1074043480)[ -> (w64 1074107808)])
  // Module Ignore
  // BDD node 88:vector_return(vector:(w64 1074093432), index:(w32 20), value:(w64 1074107808)[(w16 21)])
  // Module VectorRegisterUpdate
  buffer_t value_41;
  state->vector_register_1074093432_181.get(20, value_41);
  value_41[0] = 21;
  value_41[1] = 0;
  state->vector_register_1074093432_181.put(20, value_41);
  // BDD node 89:vector_borrow(vector:(w64 1074076216), index:(w32 21), val_out:(w64 1074043416)[ -> (w64 1074090616)])
  // Module Ignore
  // BDD node 90:vector_return(vector:(w64 1074076216), index:(w32 21), value:(w64 1074090616)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_42;
  state->vector_register_1074076216_139.get(21, value_42);
  value_42[0] = 0;
  value_42[1] = 0;
  value_42[2] = 0;
  value_42[3] = 0;
  state->vector_register_1074076216_139.put(21, value_42);
  // BDD node 91:vector_borrow(vector:(w64 1074093432), index:(w32 21), val_out:(w64 1074043480)[ -> (w64 1074107832)])
  // Module Ignore
  // BDD node 92:vector_return(vector:(w64 1074093432), index:(w32 21), value:(w64 1074107832)[(w16 20)])
  // Module VectorRegisterUpdate
  buffer_t value_43;
  state->vector_register_1074093432_181.get(21, value_43);
  value_43[0] = 20;
  value_43[1] = 0;
  state->vector_register_1074093432_181.put(21, value_43);
  // BDD node 93:vector_borrow(vector:(w64 1074076216), index:(w32 22), val_out:(w64 1074043416)[ -> (w64 1074090640)])
  // Module Ignore
  // BDD node 94:vector_return(vector:(w64 1074076216), index:(w32 22), value:(w64 1074090640)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_44;
  state->vector_register_1074076216_139.get(22, value_44);
  value_44[0] = 1;
  value_44[1] = 0;
  value_44[2] = 0;
  value_44[3] = 0;
  state->vector_register_1074076216_139.put(22, value_44);
  // BDD node 95:vector_borrow(vector:(w64 1074093432), index:(w32 22), val_out:(w64 1074043480)[ -> (w64 1074107856)])
  // Module Ignore
  // BDD node 96:vector_return(vector:(w64 1074093432), index:(w32 22), value:(w64 1074107856)[(w16 23)])
  // Module VectorRegisterUpdate
  buffer_t value_45;
  state->vector_register_1074093432_181.get(22, value_45);
  value_45[0] = 23;
  value_45[1] = 0;
  state->vector_register_1074093432_181.put(22, value_45);
  // BDD node 97:vector_borrow(vector:(w64 1074076216), index:(w32 23), val_out:(w64 1074043416)[ -> (w64 1074090664)])
  // Module Ignore
  // BDD node 98:vector_return(vector:(w64 1074076216), index:(w32 23), value:(w64 1074090664)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_46;
  state->vector_register_1074076216_139.get(23, value_46);
  value_46[0] = 0;
  value_46[1] = 0;
  value_46[2] = 0;
  value_46[3] = 0;
  state->vector_register_1074076216_139.put(23, value_46);
  // BDD node 99:vector_borrow(vector:(w64 1074093432), index:(w32 23), val_out:(w64 1074043480)[ -> (w64 1074107880)])
  // Module Ignore
  // BDD node 100:vector_return(vector:(w64 1074093432), index:(w32 23), value:(w64 1074107880)[(w16 22)])
  // Module VectorRegisterUpdate
  buffer_t value_47;
  state->vector_register_1074093432_181.get(23, value_47);
  value_47[0] = 22;
  value_47[1] = 0;
  state->vector_register_1074093432_181.put(23, value_47);
  // BDD node 101:vector_borrow(vector:(w64 1074076216), index:(w32 24), val_out:(w64 1074043416)[ -> (w64 1074090688)])
  // Module Ignore
  // BDD node 102:vector_return(vector:(w64 1074076216), index:(w32 24), value:(w64 1074090688)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_48;
  state->vector_register_1074076216_139.get(24, value_48);
  value_48[0] = 1;
  value_48[1] = 0;
  value_48[2] = 0;
  value_48[3] = 0;
  state->vector_register_1074076216_139.put(24, value_48);
  // BDD node 103:vector_borrow(vector:(w64 1074093432), index:(w32 24), val_out:(w64 1074043480)[ -> (w64 1074107904)])
  // Module Ignore
  // BDD node 104:vector_return(vector:(w64 1074093432), index:(w32 24), value:(w64 1074107904)[(w16 25)])
  // Module VectorRegisterUpdate
  buffer_t value_49;
  state->vector_register_1074093432_181.get(24, value_49);
  value_49[0] = 25;
  value_49[1] = 0;
  state->vector_register_1074093432_181.put(24, value_49);
  // BDD node 105:vector_borrow(vector:(w64 1074076216), index:(w32 25), val_out:(w64 1074043416)[ -> (w64 1074090712)])
  // Module Ignore
  // BDD node 106:vector_return(vector:(w64 1074076216), index:(w32 25), value:(w64 1074090712)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_50;
  state->vector_register_1074076216_139.get(25, value_50);
  value_50[0] = 0;
  value_50[1] = 0;
  value_50[2] = 0;
  value_50[3] = 0;
  state->vector_register_1074076216_139.put(25, value_50);
  // BDD node 107:vector_borrow(vector:(w64 1074093432), index:(w32 25), val_out:(w64 1074043480)[ -> (w64 1074107928)])
  // Module Ignore
  // BDD node 108:vector_return(vector:(w64 1074093432), index:(w32 25), value:(w64 1074107928)[(w16 24)])
  // Module VectorRegisterUpdate
  buffer_t value_51;
  state->vector_register_1074093432_181.get(25, value_51);
  value_51[0] = 24;
  value_51[1] = 0;
  state->vector_register_1074093432_181.put(25, value_51);
  // BDD node 109:vector_borrow(vector:(w64 1074076216), index:(w32 26), val_out:(w64 1074043416)[ -> (w64 1074090736)])
  // Module Ignore
  // BDD node 110:vector_return(vector:(w64 1074076216), index:(w32 26), value:(w64 1074090736)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_52;
  state->vector_register_1074076216_139.get(26, value_52);
  value_52[0] = 1;
  value_52[1] = 0;
  value_52[2] = 0;
  value_52[3] = 0;
  state->vector_register_1074076216_139.put(26, value_52);
  // BDD node 111:vector_borrow(vector:(w64 1074093432), index:(w32 26), val_out:(w64 1074043480)[ -> (w64 1074107952)])
  // Module Ignore
  // BDD node 112:vector_return(vector:(w64 1074093432), index:(w32 26), value:(w64 1074107952)[(w16 27)])
  // Module VectorRegisterUpdate
  buffer_t value_53;
  state->vector_register_1074093432_181.get(26, value_53);
  value_53[0] = 27;
  value_53[1] = 0;
  state->vector_register_1074093432_181.put(26, value_53);
  // BDD node 113:vector_borrow(vector:(w64 1074076216), index:(w32 27), val_out:(w64 1074043416)[ -> (w64 1074090760)])
  // Module Ignore
  // BDD node 114:vector_return(vector:(w64 1074076216), index:(w32 27), value:(w64 1074090760)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_54;
  state->vector_register_1074076216_139.get(27, value_54);
  value_54[0] = 0;
  value_54[1] = 0;
  value_54[2] = 0;
  value_54[3] = 0;
  state->vector_register_1074076216_139.put(27, value_54);
  // BDD node 115:vector_borrow(vector:(w64 1074093432), index:(w32 27), val_out:(w64 1074043480)[ -> (w64 1074107976)])
  // Module Ignore
  // BDD node 116:vector_return(vector:(w64 1074093432), index:(w32 27), value:(w64 1074107976)[(w16 26)])
  // Module VectorRegisterUpdate
  buffer_t value_55;
  state->vector_register_1074093432_181.get(27, value_55);
  value_55[0] = 26;
  value_55[1] = 0;
  state->vector_register_1074093432_181.put(27, value_55);
  // BDD node 117:vector_borrow(vector:(w64 1074076216), index:(w32 28), val_out:(w64 1074043416)[ -> (w64 1074090784)])
  // Module Ignore
  // BDD node 118:vector_return(vector:(w64 1074076216), index:(w32 28), value:(w64 1074090784)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_56;
  state->vector_register_1074076216_139.get(28, value_56);
  value_56[0] = 1;
  value_56[1] = 0;
  value_56[2] = 0;
  value_56[3] = 0;
  state->vector_register_1074076216_139.put(28, value_56);
  // BDD node 119:vector_borrow(vector:(w64 1074093432), index:(w32 28), val_out:(w64 1074043480)[ -> (w64 1074108000)])
  // Module Ignore
  // BDD node 120:vector_return(vector:(w64 1074093432), index:(w32 28), value:(w64 1074108000)[(w16 29)])
  // Module VectorRegisterUpdate
  buffer_t value_57;
  state->vector_register_1074093432_181.get(28, value_57);
  value_57[0] = 29;
  value_57[1] = 0;
  state->vector_register_1074093432_181.put(28, value_57);
  // BDD node 121:vector_borrow(vector:(w64 1074076216), index:(w32 29), val_out:(w64 1074043416)[ -> (w64 1074090808)])
  // Module Ignore
  // BDD node 122:vector_return(vector:(w64 1074076216), index:(w32 29), value:(w64 1074090808)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_58;
  state->vector_register_1074076216_139.get(29, value_58);
  value_58[0] = 0;
  value_58[1] = 0;
  value_58[2] = 0;
  value_58[3] = 0;
  state->vector_register_1074076216_139.put(29, value_58);
  // BDD node 123:vector_borrow(vector:(w64 1074093432), index:(w32 29), val_out:(w64 1074043480)[ -> (w64 1074108024)])
  // Module Ignore
  // BDD node 124:vector_return(vector:(w64 1074093432), index:(w32 29), value:(w64 1074108024)[(w16 28)])
  // Module VectorRegisterUpdate
  buffer_t value_59;
  state->vector_register_1074093432_181.get(29, value_59);
  value_59[0] = 28;
  value_59[1] = 0;
  state->vector_register_1074093432_181.put(29, value_59);
  // BDD node 125:vector_borrow(vector:(w64 1074076216), index:(w32 30), val_out:(w64 1074043416)[ -> (w64 1074090832)])
  // Module Ignore
  // BDD node 126:vector_return(vector:(w64 1074076216), index:(w32 30), value:(w64 1074090832)[(w32 1)])
  // Module VectorRegisterUpdate
  buffer_t value_60;
  state->vector_register_1074076216_139.get(30, value_60);
  value_60[0] = 1;
  value_60[1] = 0;
  value_60[2] = 0;
  value_60[3] = 0;
  state->vector_register_1074076216_139.put(30, value_60);
  // BDD node 127:vector_borrow(vector:(w64 1074093432), index:(w32 30), val_out:(w64 1074043480)[ -> (w64 1074108048)])
  // Module Ignore
  // BDD node 128:vector_return(vector:(w64 1074093432), index:(w32 30), value:(w64 1074108048)[(w16 31)])
  // Module VectorRegisterUpdate
  buffer_t value_61;
  state->vector_register_1074093432_181.get(30, value_61);
  value_61[0] = 31;
  value_61[1] = 0;
  state->vector_register_1074093432_181.put(30, value_61);
  // BDD node 129:vector_borrow(vector:(w64 1074076216), index:(w32 31), val_out:(w64 1074043416)[ -> (w64 1074090856)])
  // Module Ignore
  // BDD node 130:vector_return(vector:(w64 1074076216), index:(w32 31), value:(w64 1074090856)[(w32 0)])
  // Module VectorRegisterUpdate
  buffer_t value_62;
  state->vector_register_1074076216_139.get(31, value_62);
  value_62[0] = 0;
  value_62[1] = 0;
  value_62[2] = 0;
  value_62[3] = 0;
  state->vector_register_1074076216_139.put(31, value_62);
  // BDD node 131:vector_borrow(vector:(w64 1074093432), index:(w32 31), val_out:(w64 1074043480)[ -> (w64 1074108072)])
  // Module Ignore
  // BDD node 132:vector_return(vector:(w64 1074093432), index:(w32 31), value:(w64 1074108072)[(w16 30)])
  // Module VectorRegisterUpdate
  buffer_t value_63;
  state->vector_register_1074093432_181.get(31, value_63);
  value_63[0] = 30;
  value_63[1] = 0;
  state->vector_register_1074093432_181.put(31, value_63);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u64 next_time_0;
  u32 DEVICE_0;
  u32 vector_data_512_0;
  u32 map_has_this_key_0;
  u32 allocated_index_0;

} __attribute__((packed));

bool sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  cpu_hdr_t *cpu_hdr = packet_consume<cpu_hdr_t>(pkt);
  cpu_hdr_extra_t *cpu_hdr_extra = packet_consume<cpu_hdr_extra_t>(pkt);
  if (SWAP_ENDIAN_16(cpu_hdr->code_path) == 3716) {
    // EP node  5597
    // BDD node 249:packet_borrow_next_chunk(p:(w64 1074195640), length:(w32 14), chunk:(w64 1074206344)[ -> (w64 1073762480)])
    // TODO: Controller::ParseHeader
    // EP node  5641
    // BDD node 250:packet_borrow_next_chunk(p:(w64 1074195640), length:(w32 20), chunk:(w64 1074207080)[ -> (w64 1073762627)])
    // TODO: Controller::ParseHeader
    // EP node  5686
    // BDD node 251:packet_borrow_next_chunk(p:(w64 1074195640), length:(w32 4), chunk:(w64 1074207736)[ -> (w64 1073762774)])
    // TODO: Controller::ParseHeader
    // EP node  5732
    // BDD node 159:dchain_allocate_new_index(chain:(w64 1074075792), index_out:(w64 1074211312)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    fields_t<1> table_key_0;
    table_key_0[0] = table_key_0[0];
    fields_t<0> table_value_0;
    state->table_1074075792_180.put(table_key_0, table_value_0);
    // EP node  5779
    // BDD node 160:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
    // TODO: Controller::If
    // EP node  5780
    // BDD node 160:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
    // TODO: Controller::Then
    // EP node  5829
    // BDD node 161:vector_borrow(vector:(w64 1074093432), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074211760)[ -> (w64 1074107328)])
    // TODO: Controller::VectorRegisterLookup
    // EP node  5932
    // BDD node 162:vector_return(vector:(w64 1074093432), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074107328)[(ReadLSB w16 (w32 0) vector_data_640)])
    // EP node  6368
    // BDD node 166:if ((Eq false (Eq (w16 65535) (Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) vector_data_640)))))
    // TODO: Controller::If
    // EP node  6369
    // BDD node 166:if ((Eq false (Eq (w16 65535) (Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) vector_data_640)))))
    // TODO: Controller::Then
    // EP node  6542
    // BDD node 167:FORWARD
    // TODO: Controller::Forward
    // EP node  6370
    // BDD node 166:if ((Eq false (Eq (w16 65535) (Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) vector_data_640)))))
    // TODO: Controller::Else
    // EP node  6543
    // BDD node 168:DROP
    // TODO: Controller::Drop
    // EP node  5781
    // BDD node 160:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
    // TODO: Controller::Else
    // EP node  5880
    // BDD node 170:map_put(map:(w64 1074043808), key:(w64 1074071528)[(Concat w104 (Read w8 (w32 156) packet_chunks) (Concat w96 (Read w8 (w32 166) packet_chunks) (Concat w88 (Read w8 (w32 165) packet_chunks) (Concat w80 (Read w8 (w32 164) packet_chunks) (Concat w72 (Read w8 (w32 163) packet_chunks) (Concat w64 (Read w8 (w32 162) packet_chunks) (Concat w56 (Read w8 (w32 161) packet_chunks) (Concat w48 (Read w8 (w32 160) packet_chunks) (Concat w40 (Read w8 (w32 159) packet_chunks) (ReadLSB w32 (w32 294) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 156) packet_chunks) (Concat w96 (Read w8 (w32 166) packet_chunks) (Concat w88 (Read w8 (w32 165) packet_chunks) (Concat w80 (Read w8 (w32 164) packet_chunks) (Concat w72 (Read w8 (w32 163) packet_chunks) (Concat w64 (Read w8 (w32 162) packet_chunks) (Concat w56 (Read w8 (w32 161) packet_chunks) (Concat w48 (Read w8 (w32 160) packet_chunks) (Concat w40 (Read w8 (w32 159) packet_chunks) (ReadLSB w32 (w32 294) packet_chunks))))))))))], value:(ReadLSB w32 (w32 0) new_index))
    fields_t<3> table_key_1;
    table_key_1[0] = table_key_1[0];
    table_key_1[1] = table_key_1[1];
    table_key_1[2] = table_key_1[2];
    fields_t<1> table_value_1;
    table_value_1[0] = table_value_1[0];
    state->table_1074043808_142.put(table_key_1, table_value_1);
    state->table_1074043808_157.put(table_key_1, table_value_1);
    // EP node  5985
    // BDD node 172:vector_borrow(vector:(w64 1074093432), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074213992)[ -> (w64 1074107328)])
    // TODO: Controller::VectorRegisterLookup
    // EP node  6093
    // BDD node 173:vector_return(vector:(w64 1074093432), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074107328)[(ReadLSB w16 (w32 0) vector_data_640)])
    // EP node  6603
    // BDD node 177:if ((Eq false (Eq (w16 65535) (Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) vector_data_640)))))
    // TODO: Controller::If
    // EP node  6604
    // BDD node 177:if ((Eq false (Eq (w16 65535) (Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) vector_data_640)))))
    // TODO: Controller::Then
    // EP node  6729
    // BDD node 178:FORWARD
    // TODO: Controller::Forward
    // EP node  6605
    // BDD node 177:if ((Eq false (Eq (w16 65535) (Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) vector_data_640)))))
    // TODO: Controller::Else
    // EP node  6730
    // BDD node 179:DROP
    // TODO: Controller::Drop
  }

}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
