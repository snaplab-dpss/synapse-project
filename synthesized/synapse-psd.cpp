#include <sycon/sycon.h>

using namespace sycon;

struct state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardNFDev forward_nf_dev;
  MapTable map_table_1074040440;
  VectorRegister vector_register_1074071480;
  DchainTable dchain_table_1074088616;
  MapTable map_table_1074089000;
  VectorTable vector_table_1074120040;
  VectorRegister vector_register_1074137256;

  state_t()
    : ingress_port_to_nf_dev(),
      forward_nf_dev(),
      map_table_1074040440({"Ingress.map_table_1074040440_145",}, 1000LL),
      vector_register_1074071480({"Ingress.vector_register_1074071480_0",}),
      dchain_table_1074088616({"Ingress.dchain_table_1074088616_174",}, 1000LL),
      map_table_1074089000({"Ingress.map_table_1074089000_176",}),
      vector_table_1074120040({"Ingress.vector_table_1074120040_142",}),
      vector_register_1074137256({"Ingress.vector_register_1074137256_0",})
    {}
};

std::unique_ptr<state_t> state;

void sycon::nf_init() {
  state = std::make_unique<state_t>();
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
  // BDD node 0:map_allocate(capacity:(w32 65536), key_size:(w32 4), map_out:(w64 1074040152)[(w64 0) -> (w64 1074040440)])
  // Module MapTableAllocate
  // BDD node 2:vector_allocate(elem_size:(w32 4), capacity:(w32 65536), vector_out:(w64 1074040168)[(w64 0) -> (w64 1074071480)])
  // Module VectorRegisterAllocate
  // BDD node 3:dchain_allocate(index_range:(w32 65536), chain_out:(w64 1074040176)[ -> (w64 1074088616)])
  // Module DchainTableAllocate
  // BDD node 4:map_allocate(capacity:(w32 1048576), key_size:(w32 6), map_out:(w64 1074040184)[(w64 0) -> (w64 1074089000)])
  // Module MapTableAllocate
  // BDD node 6:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1074040200)[(w64 0) -> (w64 1074120040)])
  // Module VectorTableAllocate
  // BDD node 7:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074040208)[(w64 0) -> (w64 1074137256)])
  // Module VectorRegisterAllocate
  // BDD node 8:vector_borrow(vector:(w64 1074120040), index:(w32 0), val_out:(w64 1074040024)[ -> (w64 1074133936)])
  // Module Ignore
  // BDD node 9:vector_return(vector:(w64 1074120040), index:(w32 0), value:(w64 1074133936)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_0(4);
  vector_table_1074120040_value_0[0] = 0;
  vector_table_1074120040_value_0[1] = 0;
  vector_table_1074120040_value_0[2] = 0;
  vector_table_1074120040_value_0[3] = 1;
  state->vector_table_1074120040.write(0, vector_table_1074120040_value_0);
  // BDD node 10:vector_borrow(vector:(w64 1074137256), index:(w32 0), val_out:(w64 1074040088)[ -> (w64 1074151152)])
  // Module Ignore
  // BDD node 11:vector_return(vector:(w64 1074137256), index:(w32 0), value:(w64 1074151152)[(w16 1)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_0(2);
  vector_register_1074137256_value_0[0] = 0;
  vector_register_1074137256_value_0[1] = 1;
  state->vector_register_1074137256.put(0, vector_register_1074137256_value_0);
  // BDD node 12:vector_borrow(vector:(w64 1074120040), index:(w32 1), val_out:(w64 1074040024)[ -> (w64 1074133960)])
  // Module Ignore
  // BDD node 13:vector_return(vector:(w64 1074120040), index:(w32 1), value:(w64 1074133960)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_1(4);
  vector_table_1074120040_value_1[0] = 0;
  vector_table_1074120040_value_1[1] = 0;
  vector_table_1074120040_value_1[2] = 0;
  vector_table_1074120040_value_1[3] = 0;
  state->vector_table_1074120040.write(1, vector_table_1074120040_value_1);
  // BDD node 14:vector_borrow(vector:(w64 1074137256), index:(w32 1), val_out:(w64 1074040088)[ -> (w64 1074151176)])
  // Module Ignore
  // BDD node 15:vector_return(vector:(w64 1074137256), index:(w32 1), value:(w64 1074151176)[(w16 0)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_1(2);
  vector_register_1074137256_value_1[0] = 0;
  vector_register_1074137256_value_1[1] = 0;
  state->vector_register_1074137256.put(1, vector_register_1074137256_value_1);
  // BDD node 16:vector_borrow(vector:(w64 1074120040), index:(w32 2), val_out:(w64 1074040024)[ -> (w64 1074133984)])
  // Module Ignore
  // BDD node 17:vector_return(vector:(w64 1074120040), index:(w32 2), value:(w64 1074133984)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_2(4);
  vector_table_1074120040_value_2[0] = 0;
  vector_table_1074120040_value_2[1] = 0;
  vector_table_1074120040_value_2[2] = 0;
  vector_table_1074120040_value_2[3] = 1;
  state->vector_table_1074120040.write(2, vector_table_1074120040_value_2);
  // BDD node 18:vector_borrow(vector:(w64 1074137256), index:(w32 2), val_out:(w64 1074040088)[ -> (w64 1074151200)])
  // Module Ignore
  // BDD node 19:vector_return(vector:(w64 1074137256), index:(w32 2), value:(w64 1074151200)[(w16 3)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_2(2);
  vector_register_1074137256_value_2[0] = 0;
  vector_register_1074137256_value_2[1] = 3;
  state->vector_register_1074137256.put(2, vector_register_1074137256_value_2);
  // BDD node 20:vector_borrow(vector:(w64 1074120040), index:(w32 3), val_out:(w64 1074040024)[ -> (w64 1074134008)])
  // Module Ignore
  // BDD node 21:vector_return(vector:(w64 1074120040), index:(w32 3), value:(w64 1074134008)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_3(4);
  vector_table_1074120040_value_3[0] = 0;
  vector_table_1074120040_value_3[1] = 0;
  vector_table_1074120040_value_3[2] = 0;
  vector_table_1074120040_value_3[3] = 0;
  state->vector_table_1074120040.write(3, vector_table_1074120040_value_3);
  // BDD node 22:vector_borrow(vector:(w64 1074137256), index:(w32 3), val_out:(w64 1074040088)[ -> (w64 1074151224)])
  // Module Ignore
  // BDD node 23:vector_return(vector:(w64 1074137256), index:(w32 3), value:(w64 1074151224)[(w16 2)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_3(2);
  vector_register_1074137256_value_3[0] = 0;
  vector_register_1074137256_value_3[1] = 2;
  state->vector_register_1074137256.put(3, vector_register_1074137256_value_3);
  // BDD node 24:vector_borrow(vector:(w64 1074120040), index:(w32 4), val_out:(w64 1074040024)[ -> (w64 1074134032)])
  // Module Ignore
  // BDD node 25:vector_return(vector:(w64 1074120040), index:(w32 4), value:(w64 1074134032)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_4(4);
  vector_table_1074120040_value_4[0] = 0;
  vector_table_1074120040_value_4[1] = 0;
  vector_table_1074120040_value_4[2] = 0;
  vector_table_1074120040_value_4[3] = 1;
  state->vector_table_1074120040.write(4, vector_table_1074120040_value_4);
  // BDD node 26:vector_borrow(vector:(w64 1074137256), index:(w32 4), val_out:(w64 1074040088)[ -> (w64 1074151248)])
  // Module Ignore
  // BDD node 27:vector_return(vector:(w64 1074137256), index:(w32 4), value:(w64 1074151248)[(w16 5)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_4(2);
  vector_register_1074137256_value_4[0] = 0;
  vector_register_1074137256_value_4[1] = 5;
  state->vector_register_1074137256.put(4, vector_register_1074137256_value_4);
  // BDD node 28:vector_borrow(vector:(w64 1074120040), index:(w32 5), val_out:(w64 1074040024)[ -> (w64 1074134056)])
  // Module Ignore
  // BDD node 29:vector_return(vector:(w64 1074120040), index:(w32 5), value:(w64 1074134056)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_5(4);
  vector_table_1074120040_value_5[0] = 0;
  vector_table_1074120040_value_5[1] = 0;
  vector_table_1074120040_value_5[2] = 0;
  vector_table_1074120040_value_5[3] = 0;
  state->vector_table_1074120040.write(5, vector_table_1074120040_value_5);
  // BDD node 30:vector_borrow(vector:(w64 1074137256), index:(w32 5), val_out:(w64 1074040088)[ -> (w64 1074151272)])
  // Module Ignore
  // BDD node 31:vector_return(vector:(w64 1074137256), index:(w32 5), value:(w64 1074151272)[(w16 4)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_5(2);
  vector_register_1074137256_value_5[0] = 0;
  vector_register_1074137256_value_5[1] = 4;
  state->vector_register_1074137256.put(5, vector_register_1074137256_value_5);
  // BDD node 32:vector_borrow(vector:(w64 1074120040), index:(w32 6), val_out:(w64 1074040024)[ -> (w64 1074134080)])
  // Module Ignore
  // BDD node 33:vector_return(vector:(w64 1074120040), index:(w32 6), value:(w64 1074134080)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_6(4);
  vector_table_1074120040_value_6[0] = 0;
  vector_table_1074120040_value_6[1] = 0;
  vector_table_1074120040_value_6[2] = 0;
  vector_table_1074120040_value_6[3] = 1;
  state->vector_table_1074120040.write(6, vector_table_1074120040_value_6);
  // BDD node 34:vector_borrow(vector:(w64 1074137256), index:(w32 6), val_out:(w64 1074040088)[ -> (w64 1074151296)])
  // Module Ignore
  // BDD node 35:vector_return(vector:(w64 1074137256), index:(w32 6), value:(w64 1074151296)[(w16 7)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_6(2);
  vector_register_1074137256_value_6[0] = 0;
  vector_register_1074137256_value_6[1] = 7;
  state->vector_register_1074137256.put(6, vector_register_1074137256_value_6);
  // BDD node 36:vector_borrow(vector:(w64 1074120040), index:(w32 7), val_out:(w64 1074040024)[ -> (w64 1074134104)])
  // Module Ignore
  // BDD node 37:vector_return(vector:(w64 1074120040), index:(w32 7), value:(w64 1074134104)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_7(4);
  vector_table_1074120040_value_7[0] = 0;
  vector_table_1074120040_value_7[1] = 0;
  vector_table_1074120040_value_7[2] = 0;
  vector_table_1074120040_value_7[3] = 0;
  state->vector_table_1074120040.write(7, vector_table_1074120040_value_7);
  // BDD node 38:vector_borrow(vector:(w64 1074137256), index:(w32 7), val_out:(w64 1074040088)[ -> (w64 1074151320)])
  // Module Ignore
  // BDD node 39:vector_return(vector:(w64 1074137256), index:(w32 7), value:(w64 1074151320)[(w16 6)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_7(2);
  vector_register_1074137256_value_7[0] = 0;
  vector_register_1074137256_value_7[1] = 6;
  state->vector_register_1074137256.put(7, vector_register_1074137256_value_7);
  // BDD node 40:vector_borrow(vector:(w64 1074120040), index:(w32 8), val_out:(w64 1074040024)[ -> (w64 1074134128)])
  // Module Ignore
  // BDD node 41:vector_return(vector:(w64 1074120040), index:(w32 8), value:(w64 1074134128)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_8(4);
  vector_table_1074120040_value_8[0] = 0;
  vector_table_1074120040_value_8[1] = 0;
  vector_table_1074120040_value_8[2] = 0;
  vector_table_1074120040_value_8[3] = 1;
  state->vector_table_1074120040.write(8, vector_table_1074120040_value_8);
  // BDD node 42:vector_borrow(vector:(w64 1074137256), index:(w32 8), val_out:(w64 1074040088)[ -> (w64 1074151344)])
  // Module Ignore
  // BDD node 43:vector_return(vector:(w64 1074137256), index:(w32 8), value:(w64 1074151344)[(w16 9)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_8(2);
  vector_register_1074137256_value_8[0] = 0;
  vector_register_1074137256_value_8[1] = 9;
  state->vector_register_1074137256.put(8, vector_register_1074137256_value_8);
  // BDD node 44:vector_borrow(vector:(w64 1074120040), index:(w32 9), val_out:(w64 1074040024)[ -> (w64 1074134152)])
  // Module Ignore
  // BDD node 45:vector_return(vector:(w64 1074120040), index:(w32 9), value:(w64 1074134152)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_9(4);
  vector_table_1074120040_value_9[0] = 0;
  vector_table_1074120040_value_9[1] = 0;
  vector_table_1074120040_value_9[2] = 0;
  vector_table_1074120040_value_9[3] = 0;
  state->vector_table_1074120040.write(9, vector_table_1074120040_value_9);
  // BDD node 46:vector_borrow(vector:(w64 1074137256), index:(w32 9), val_out:(w64 1074040088)[ -> (w64 1074151368)])
  // Module Ignore
  // BDD node 47:vector_return(vector:(w64 1074137256), index:(w32 9), value:(w64 1074151368)[(w16 8)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_9(2);
  vector_register_1074137256_value_9[0] = 0;
  vector_register_1074137256_value_9[1] = 8;
  state->vector_register_1074137256.put(9, vector_register_1074137256_value_9);
  // BDD node 48:vector_borrow(vector:(w64 1074120040), index:(w32 10), val_out:(w64 1074040024)[ -> (w64 1074134176)])
  // Module Ignore
  // BDD node 49:vector_return(vector:(w64 1074120040), index:(w32 10), value:(w64 1074134176)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_10(4);
  vector_table_1074120040_value_10[0] = 0;
  vector_table_1074120040_value_10[1] = 0;
  vector_table_1074120040_value_10[2] = 0;
  vector_table_1074120040_value_10[3] = 1;
  state->vector_table_1074120040.write(10, vector_table_1074120040_value_10);
  // BDD node 50:vector_borrow(vector:(w64 1074137256), index:(w32 10), val_out:(w64 1074040088)[ -> (w64 1074151392)])
  // Module Ignore
  // BDD node 51:vector_return(vector:(w64 1074137256), index:(w32 10), value:(w64 1074151392)[(w16 11)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_10(2);
  vector_register_1074137256_value_10[0] = 0;
  vector_register_1074137256_value_10[1] = 11;
  state->vector_register_1074137256.put(10, vector_register_1074137256_value_10);
  // BDD node 52:vector_borrow(vector:(w64 1074120040), index:(w32 11), val_out:(w64 1074040024)[ -> (w64 1074134200)])
  // Module Ignore
  // BDD node 53:vector_return(vector:(w64 1074120040), index:(w32 11), value:(w64 1074134200)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_11(4);
  vector_table_1074120040_value_11[0] = 0;
  vector_table_1074120040_value_11[1] = 0;
  vector_table_1074120040_value_11[2] = 0;
  vector_table_1074120040_value_11[3] = 0;
  state->vector_table_1074120040.write(11, vector_table_1074120040_value_11);
  // BDD node 54:vector_borrow(vector:(w64 1074137256), index:(w32 11), val_out:(w64 1074040088)[ -> (w64 1074151416)])
  // Module Ignore
  // BDD node 55:vector_return(vector:(w64 1074137256), index:(w32 11), value:(w64 1074151416)[(w16 10)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_11(2);
  vector_register_1074137256_value_11[0] = 0;
  vector_register_1074137256_value_11[1] = 10;
  state->vector_register_1074137256.put(11, vector_register_1074137256_value_11);
  // BDD node 56:vector_borrow(vector:(w64 1074120040), index:(w32 12), val_out:(w64 1074040024)[ -> (w64 1074134224)])
  // Module Ignore
  // BDD node 57:vector_return(vector:(w64 1074120040), index:(w32 12), value:(w64 1074134224)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_12(4);
  vector_table_1074120040_value_12[0] = 0;
  vector_table_1074120040_value_12[1] = 0;
  vector_table_1074120040_value_12[2] = 0;
  vector_table_1074120040_value_12[3] = 1;
  state->vector_table_1074120040.write(12, vector_table_1074120040_value_12);
  // BDD node 58:vector_borrow(vector:(w64 1074137256), index:(w32 12), val_out:(w64 1074040088)[ -> (w64 1074151440)])
  // Module Ignore
  // BDD node 59:vector_return(vector:(w64 1074137256), index:(w32 12), value:(w64 1074151440)[(w16 13)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_12(2);
  vector_register_1074137256_value_12[0] = 0;
  vector_register_1074137256_value_12[1] = 13;
  state->vector_register_1074137256.put(12, vector_register_1074137256_value_12);
  // BDD node 60:vector_borrow(vector:(w64 1074120040), index:(w32 13), val_out:(w64 1074040024)[ -> (w64 1074134248)])
  // Module Ignore
  // BDD node 61:vector_return(vector:(w64 1074120040), index:(w32 13), value:(w64 1074134248)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_13(4);
  vector_table_1074120040_value_13[0] = 0;
  vector_table_1074120040_value_13[1] = 0;
  vector_table_1074120040_value_13[2] = 0;
  vector_table_1074120040_value_13[3] = 0;
  state->vector_table_1074120040.write(13, vector_table_1074120040_value_13);
  // BDD node 62:vector_borrow(vector:(w64 1074137256), index:(w32 13), val_out:(w64 1074040088)[ -> (w64 1074151464)])
  // Module Ignore
  // BDD node 63:vector_return(vector:(w64 1074137256), index:(w32 13), value:(w64 1074151464)[(w16 12)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_13(2);
  vector_register_1074137256_value_13[0] = 0;
  vector_register_1074137256_value_13[1] = 12;
  state->vector_register_1074137256.put(13, vector_register_1074137256_value_13);
  // BDD node 64:vector_borrow(vector:(w64 1074120040), index:(w32 14), val_out:(w64 1074040024)[ -> (w64 1074134272)])
  // Module Ignore
  // BDD node 65:vector_return(vector:(w64 1074120040), index:(w32 14), value:(w64 1074134272)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_14(4);
  vector_table_1074120040_value_14[0] = 0;
  vector_table_1074120040_value_14[1] = 0;
  vector_table_1074120040_value_14[2] = 0;
  vector_table_1074120040_value_14[3] = 1;
  state->vector_table_1074120040.write(14, vector_table_1074120040_value_14);
  // BDD node 66:vector_borrow(vector:(w64 1074137256), index:(w32 14), val_out:(w64 1074040088)[ -> (w64 1074151488)])
  // Module Ignore
  // BDD node 67:vector_return(vector:(w64 1074137256), index:(w32 14), value:(w64 1074151488)[(w16 15)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_14(2);
  vector_register_1074137256_value_14[0] = 0;
  vector_register_1074137256_value_14[1] = 15;
  state->vector_register_1074137256.put(14, vector_register_1074137256_value_14);
  // BDD node 68:vector_borrow(vector:(w64 1074120040), index:(w32 15), val_out:(w64 1074040024)[ -> (w64 1074134296)])
  // Module Ignore
  // BDD node 69:vector_return(vector:(w64 1074120040), index:(w32 15), value:(w64 1074134296)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_15(4);
  vector_table_1074120040_value_15[0] = 0;
  vector_table_1074120040_value_15[1] = 0;
  vector_table_1074120040_value_15[2] = 0;
  vector_table_1074120040_value_15[3] = 0;
  state->vector_table_1074120040.write(15, vector_table_1074120040_value_15);
  // BDD node 70:vector_borrow(vector:(w64 1074137256), index:(w32 15), val_out:(w64 1074040088)[ -> (w64 1074151512)])
  // Module Ignore
  // BDD node 71:vector_return(vector:(w64 1074137256), index:(w32 15), value:(w64 1074151512)[(w16 14)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_15(2);
  vector_register_1074137256_value_15[0] = 0;
  vector_register_1074137256_value_15[1] = 14;
  state->vector_register_1074137256.put(15, vector_register_1074137256_value_15);
  // BDD node 72:vector_borrow(vector:(w64 1074120040), index:(w32 16), val_out:(w64 1074040024)[ -> (w64 1074134320)])
  // Module Ignore
  // BDD node 73:vector_return(vector:(w64 1074120040), index:(w32 16), value:(w64 1074134320)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_16(4);
  vector_table_1074120040_value_16[0] = 0;
  vector_table_1074120040_value_16[1] = 0;
  vector_table_1074120040_value_16[2] = 0;
  vector_table_1074120040_value_16[3] = 1;
  state->vector_table_1074120040.write(16, vector_table_1074120040_value_16);
  // BDD node 74:vector_borrow(vector:(w64 1074137256), index:(w32 16), val_out:(w64 1074040088)[ -> (w64 1074151536)])
  // Module Ignore
  // BDD node 75:vector_return(vector:(w64 1074137256), index:(w32 16), value:(w64 1074151536)[(w16 17)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_16(2);
  vector_register_1074137256_value_16[0] = 0;
  vector_register_1074137256_value_16[1] = 17;
  state->vector_register_1074137256.put(16, vector_register_1074137256_value_16);
  // BDD node 76:vector_borrow(vector:(w64 1074120040), index:(w32 17), val_out:(w64 1074040024)[ -> (w64 1074134344)])
  // Module Ignore
  // BDD node 77:vector_return(vector:(w64 1074120040), index:(w32 17), value:(w64 1074134344)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_17(4);
  vector_table_1074120040_value_17[0] = 0;
  vector_table_1074120040_value_17[1] = 0;
  vector_table_1074120040_value_17[2] = 0;
  vector_table_1074120040_value_17[3] = 0;
  state->vector_table_1074120040.write(17, vector_table_1074120040_value_17);
  // BDD node 78:vector_borrow(vector:(w64 1074137256), index:(w32 17), val_out:(w64 1074040088)[ -> (w64 1074151560)])
  // Module Ignore
  // BDD node 79:vector_return(vector:(w64 1074137256), index:(w32 17), value:(w64 1074151560)[(w16 16)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_17(2);
  vector_register_1074137256_value_17[0] = 0;
  vector_register_1074137256_value_17[1] = 16;
  state->vector_register_1074137256.put(17, vector_register_1074137256_value_17);
  // BDD node 80:vector_borrow(vector:(w64 1074120040), index:(w32 18), val_out:(w64 1074040024)[ -> (w64 1074134368)])
  // Module Ignore
  // BDD node 81:vector_return(vector:(w64 1074120040), index:(w32 18), value:(w64 1074134368)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_18(4);
  vector_table_1074120040_value_18[0] = 0;
  vector_table_1074120040_value_18[1] = 0;
  vector_table_1074120040_value_18[2] = 0;
  vector_table_1074120040_value_18[3] = 1;
  state->vector_table_1074120040.write(18, vector_table_1074120040_value_18);
  // BDD node 82:vector_borrow(vector:(w64 1074137256), index:(w32 18), val_out:(w64 1074040088)[ -> (w64 1074151584)])
  // Module Ignore
  // BDD node 83:vector_return(vector:(w64 1074137256), index:(w32 18), value:(w64 1074151584)[(w16 19)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_18(2);
  vector_register_1074137256_value_18[0] = 0;
  vector_register_1074137256_value_18[1] = 19;
  state->vector_register_1074137256.put(18, vector_register_1074137256_value_18);
  // BDD node 84:vector_borrow(vector:(w64 1074120040), index:(w32 19), val_out:(w64 1074040024)[ -> (w64 1074134392)])
  // Module Ignore
  // BDD node 85:vector_return(vector:(w64 1074120040), index:(w32 19), value:(w64 1074134392)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_19(4);
  vector_table_1074120040_value_19[0] = 0;
  vector_table_1074120040_value_19[1] = 0;
  vector_table_1074120040_value_19[2] = 0;
  vector_table_1074120040_value_19[3] = 0;
  state->vector_table_1074120040.write(19, vector_table_1074120040_value_19);
  // BDD node 86:vector_borrow(vector:(w64 1074137256), index:(w32 19), val_out:(w64 1074040088)[ -> (w64 1074151608)])
  // Module Ignore
  // BDD node 87:vector_return(vector:(w64 1074137256), index:(w32 19), value:(w64 1074151608)[(w16 18)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_19(2);
  vector_register_1074137256_value_19[0] = 0;
  vector_register_1074137256_value_19[1] = 18;
  state->vector_register_1074137256.put(19, vector_register_1074137256_value_19);
  // BDD node 88:vector_borrow(vector:(w64 1074120040), index:(w32 20), val_out:(w64 1074040024)[ -> (w64 1074134416)])
  // Module Ignore
  // BDD node 89:vector_return(vector:(w64 1074120040), index:(w32 20), value:(w64 1074134416)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_20(4);
  vector_table_1074120040_value_20[0] = 0;
  vector_table_1074120040_value_20[1] = 0;
  vector_table_1074120040_value_20[2] = 0;
  vector_table_1074120040_value_20[3] = 1;
  state->vector_table_1074120040.write(20, vector_table_1074120040_value_20);
  // BDD node 90:vector_borrow(vector:(w64 1074137256), index:(w32 20), val_out:(w64 1074040088)[ -> (w64 1074151632)])
  // Module Ignore
  // BDD node 91:vector_return(vector:(w64 1074137256), index:(w32 20), value:(w64 1074151632)[(w16 21)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_20(2);
  vector_register_1074137256_value_20[0] = 0;
  vector_register_1074137256_value_20[1] = 21;
  state->vector_register_1074137256.put(20, vector_register_1074137256_value_20);
  // BDD node 92:vector_borrow(vector:(w64 1074120040), index:(w32 21), val_out:(w64 1074040024)[ -> (w64 1074134440)])
  // Module Ignore
  // BDD node 93:vector_return(vector:(w64 1074120040), index:(w32 21), value:(w64 1074134440)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_21(4);
  vector_table_1074120040_value_21[0] = 0;
  vector_table_1074120040_value_21[1] = 0;
  vector_table_1074120040_value_21[2] = 0;
  vector_table_1074120040_value_21[3] = 0;
  state->vector_table_1074120040.write(21, vector_table_1074120040_value_21);
  // BDD node 94:vector_borrow(vector:(w64 1074137256), index:(w32 21), val_out:(w64 1074040088)[ -> (w64 1074151656)])
  // Module Ignore
  // BDD node 95:vector_return(vector:(w64 1074137256), index:(w32 21), value:(w64 1074151656)[(w16 20)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_21(2);
  vector_register_1074137256_value_21[0] = 0;
  vector_register_1074137256_value_21[1] = 20;
  state->vector_register_1074137256.put(21, vector_register_1074137256_value_21);
  // BDD node 96:vector_borrow(vector:(w64 1074120040), index:(w32 22), val_out:(w64 1074040024)[ -> (w64 1074134464)])
  // Module Ignore
  // BDD node 97:vector_return(vector:(w64 1074120040), index:(w32 22), value:(w64 1074134464)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_22(4);
  vector_table_1074120040_value_22[0] = 0;
  vector_table_1074120040_value_22[1] = 0;
  vector_table_1074120040_value_22[2] = 0;
  vector_table_1074120040_value_22[3] = 1;
  state->vector_table_1074120040.write(22, vector_table_1074120040_value_22);
  // BDD node 98:vector_borrow(vector:(w64 1074137256), index:(w32 22), val_out:(w64 1074040088)[ -> (w64 1074151680)])
  // Module Ignore
  // BDD node 99:vector_return(vector:(w64 1074137256), index:(w32 22), value:(w64 1074151680)[(w16 23)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_22(2);
  vector_register_1074137256_value_22[0] = 0;
  vector_register_1074137256_value_22[1] = 23;
  state->vector_register_1074137256.put(22, vector_register_1074137256_value_22);
  // BDD node 100:vector_borrow(vector:(w64 1074120040), index:(w32 23), val_out:(w64 1074040024)[ -> (w64 1074134488)])
  // Module Ignore
  // BDD node 101:vector_return(vector:(w64 1074120040), index:(w32 23), value:(w64 1074134488)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_23(4);
  vector_table_1074120040_value_23[0] = 0;
  vector_table_1074120040_value_23[1] = 0;
  vector_table_1074120040_value_23[2] = 0;
  vector_table_1074120040_value_23[3] = 0;
  state->vector_table_1074120040.write(23, vector_table_1074120040_value_23);
  // BDD node 102:vector_borrow(vector:(w64 1074137256), index:(w32 23), val_out:(w64 1074040088)[ -> (w64 1074151704)])
  // Module Ignore
  // BDD node 103:vector_return(vector:(w64 1074137256), index:(w32 23), value:(w64 1074151704)[(w16 22)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_23(2);
  vector_register_1074137256_value_23[0] = 0;
  vector_register_1074137256_value_23[1] = 22;
  state->vector_register_1074137256.put(23, vector_register_1074137256_value_23);
  // BDD node 104:vector_borrow(vector:(w64 1074120040), index:(w32 24), val_out:(w64 1074040024)[ -> (w64 1074134512)])
  // Module Ignore
  // BDD node 105:vector_return(vector:(w64 1074120040), index:(w32 24), value:(w64 1074134512)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_24(4);
  vector_table_1074120040_value_24[0] = 0;
  vector_table_1074120040_value_24[1] = 0;
  vector_table_1074120040_value_24[2] = 0;
  vector_table_1074120040_value_24[3] = 1;
  state->vector_table_1074120040.write(24, vector_table_1074120040_value_24);
  // BDD node 106:vector_borrow(vector:(w64 1074137256), index:(w32 24), val_out:(w64 1074040088)[ -> (w64 1074151728)])
  // Module Ignore
  // BDD node 107:vector_return(vector:(w64 1074137256), index:(w32 24), value:(w64 1074151728)[(w16 25)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_24(2);
  vector_register_1074137256_value_24[0] = 0;
  vector_register_1074137256_value_24[1] = 25;
  state->vector_register_1074137256.put(24, vector_register_1074137256_value_24);
  // BDD node 108:vector_borrow(vector:(w64 1074120040), index:(w32 25), val_out:(w64 1074040024)[ -> (w64 1074134536)])
  // Module Ignore
  // BDD node 109:vector_return(vector:(w64 1074120040), index:(w32 25), value:(w64 1074134536)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_25(4);
  vector_table_1074120040_value_25[0] = 0;
  vector_table_1074120040_value_25[1] = 0;
  vector_table_1074120040_value_25[2] = 0;
  vector_table_1074120040_value_25[3] = 0;
  state->vector_table_1074120040.write(25, vector_table_1074120040_value_25);
  // BDD node 110:vector_borrow(vector:(w64 1074137256), index:(w32 25), val_out:(w64 1074040088)[ -> (w64 1074151752)])
  // Module Ignore
  // BDD node 111:vector_return(vector:(w64 1074137256), index:(w32 25), value:(w64 1074151752)[(w16 24)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_25(2);
  vector_register_1074137256_value_25[0] = 0;
  vector_register_1074137256_value_25[1] = 24;
  state->vector_register_1074137256.put(25, vector_register_1074137256_value_25);
  // BDD node 112:vector_borrow(vector:(w64 1074120040), index:(w32 26), val_out:(w64 1074040024)[ -> (w64 1074134560)])
  // Module Ignore
  // BDD node 113:vector_return(vector:(w64 1074120040), index:(w32 26), value:(w64 1074134560)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_26(4);
  vector_table_1074120040_value_26[0] = 0;
  vector_table_1074120040_value_26[1] = 0;
  vector_table_1074120040_value_26[2] = 0;
  vector_table_1074120040_value_26[3] = 1;
  state->vector_table_1074120040.write(26, vector_table_1074120040_value_26);
  // BDD node 114:vector_borrow(vector:(w64 1074137256), index:(w32 26), val_out:(w64 1074040088)[ -> (w64 1074151776)])
  // Module Ignore
  // BDD node 115:vector_return(vector:(w64 1074137256), index:(w32 26), value:(w64 1074151776)[(w16 27)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_26(2);
  vector_register_1074137256_value_26[0] = 0;
  vector_register_1074137256_value_26[1] = 27;
  state->vector_register_1074137256.put(26, vector_register_1074137256_value_26);
  // BDD node 116:vector_borrow(vector:(w64 1074120040), index:(w32 27), val_out:(w64 1074040024)[ -> (w64 1074134584)])
  // Module Ignore
  // BDD node 117:vector_return(vector:(w64 1074120040), index:(w32 27), value:(w64 1074134584)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_27(4);
  vector_table_1074120040_value_27[0] = 0;
  vector_table_1074120040_value_27[1] = 0;
  vector_table_1074120040_value_27[2] = 0;
  vector_table_1074120040_value_27[3] = 0;
  state->vector_table_1074120040.write(27, vector_table_1074120040_value_27);
  // BDD node 118:vector_borrow(vector:(w64 1074137256), index:(w32 27), val_out:(w64 1074040088)[ -> (w64 1074151800)])
  // Module Ignore
  // BDD node 119:vector_return(vector:(w64 1074137256), index:(w32 27), value:(w64 1074151800)[(w16 26)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_27(2);
  vector_register_1074137256_value_27[0] = 0;
  vector_register_1074137256_value_27[1] = 26;
  state->vector_register_1074137256.put(27, vector_register_1074137256_value_27);
  // BDD node 120:vector_borrow(vector:(w64 1074120040), index:(w32 28), val_out:(w64 1074040024)[ -> (w64 1074134608)])
  // Module Ignore
  // BDD node 121:vector_return(vector:(w64 1074120040), index:(w32 28), value:(w64 1074134608)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_28(4);
  vector_table_1074120040_value_28[0] = 0;
  vector_table_1074120040_value_28[1] = 0;
  vector_table_1074120040_value_28[2] = 0;
  vector_table_1074120040_value_28[3] = 1;
  state->vector_table_1074120040.write(28, vector_table_1074120040_value_28);
  // BDD node 122:vector_borrow(vector:(w64 1074137256), index:(w32 28), val_out:(w64 1074040088)[ -> (w64 1074151824)])
  // Module Ignore
  // BDD node 123:vector_return(vector:(w64 1074137256), index:(w32 28), value:(w64 1074151824)[(w16 29)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_28(2);
  vector_register_1074137256_value_28[0] = 0;
  vector_register_1074137256_value_28[1] = 29;
  state->vector_register_1074137256.put(28, vector_register_1074137256_value_28);
  // BDD node 124:vector_borrow(vector:(w64 1074120040), index:(w32 29), val_out:(w64 1074040024)[ -> (w64 1074134632)])
  // Module Ignore
  // BDD node 125:vector_return(vector:(w64 1074120040), index:(w32 29), value:(w64 1074134632)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_29(4);
  vector_table_1074120040_value_29[0] = 0;
  vector_table_1074120040_value_29[1] = 0;
  vector_table_1074120040_value_29[2] = 0;
  vector_table_1074120040_value_29[3] = 0;
  state->vector_table_1074120040.write(29, vector_table_1074120040_value_29);
  // BDD node 126:vector_borrow(vector:(w64 1074137256), index:(w32 29), val_out:(w64 1074040088)[ -> (w64 1074151848)])
  // Module Ignore
  // BDD node 127:vector_return(vector:(w64 1074137256), index:(w32 29), value:(w64 1074151848)[(w16 28)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_29(2);
  vector_register_1074137256_value_29[0] = 0;
  vector_register_1074137256_value_29[1] = 28;
  state->vector_register_1074137256.put(29, vector_register_1074137256_value_29);
  // BDD node 128:vector_borrow(vector:(w64 1074120040), index:(w32 30), val_out:(w64 1074040024)[ -> (w64 1074134656)])
  // Module Ignore
  // BDD node 129:vector_return(vector:(w64 1074120040), index:(w32 30), value:(w64 1074134656)[(w32 1)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_30(4);
  vector_table_1074120040_value_30[0] = 0;
  vector_table_1074120040_value_30[1] = 0;
  vector_table_1074120040_value_30[2] = 0;
  vector_table_1074120040_value_30[3] = 1;
  state->vector_table_1074120040.write(30, vector_table_1074120040_value_30);
  // BDD node 130:vector_borrow(vector:(w64 1074137256), index:(w32 30), val_out:(w64 1074040088)[ -> (w64 1074151872)])
  // Module Ignore
  // BDD node 131:vector_return(vector:(w64 1074137256), index:(w32 30), value:(w64 1074151872)[(w16 31)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_30(2);
  vector_register_1074137256_value_30[0] = 0;
  vector_register_1074137256_value_30[1] = 31;
  state->vector_register_1074137256.put(30, vector_register_1074137256_value_30);
  // BDD node 132:vector_borrow(vector:(w64 1074120040), index:(w32 31), val_out:(w64 1074040024)[ -> (w64 1074134680)])
  // Module Ignore
  // BDD node 133:vector_return(vector:(w64 1074120040), index:(w32 31), value:(w64 1074134680)[(w32 0)])
  // Module VectorTableUpdate
  buffer_t vector_table_1074120040_value_31(4);
  vector_table_1074120040_value_31[0] = 0;
  vector_table_1074120040_value_31[1] = 0;
  vector_table_1074120040_value_31[2] = 0;
  vector_table_1074120040_value_31[3] = 0;
  state->vector_table_1074120040.write(31, vector_table_1074120040_value_31);
  // BDD node 134:vector_borrow(vector:(w64 1074137256), index:(w32 31), val_out:(w64 1074040088)[ -> (w64 1074151896)])
  // Module Ignore
  // BDD node 135:vector_return(vector:(w64 1074137256), index:(w32 31), value:(w64 1074151896)[(w16 30)])
  // Module VectorRegisterUpdate
  buffer_t vector_register_1074137256_value_31(2);
  vector_register_1074137256_value_31[0] = 0;
  vector_register_1074137256_value_31[1] = 30;
  state->vector_register_1074137256.put(31, vector_register_1074137256_value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u16 vector_data_r14;
  u32 DEVICE;
  u32 vector_data_r35;

} __attribute__((packed));

bool sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  bool forward = true;
  bool trigger_update_ipv4_tcpudp_checksums = false;
  void* l3_hdr = nullptr;
  void* l4_hdr = nullptr;

  cpu_hdr_t *cpu_hdr = packet_consume<cpu_hdr_t>(pkt);
  cpu_hdr_extra_t *cpu_hdr_extra = packet_consume<cpu_hdr_extra_t>(pkt);
  LOG_DEBUG("[t=%lu] New packet (size=%u, code_path=%d)\n", now, size, bswap16(cpu_hdr->code_path));

  if (bswap16(cpu_hdr->code_path) == 13105) {
    // EP node  14465
    // BDD node 147:dchain_allocate_new_index(chain:(w64 1074088616), index_out:(w64 1074265272)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index_r1)], time:(ReadLSB w64 (w32 0) next_time))
    u32 allocated_index_0;
    bool success_0 = state->dchain_table_1074088616.allocate_new_index(allocated_index_0);
    // EP node  14669
    // BDD node 313:packet_borrow_next_chunk(p:(w64 1074251832), length:(w32 14), chunk:(w64 1074262496)[ -> (w64 1073761328)])
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  14876
    // BDD node 158:vector_borrow(vector:(w64 1074071480), index:(ReadLSB w32 (w32 0) new_index_r1), val_out:(w64 1074265368)[ -> (w64 1074085376)])
    buffer_t value_0;
    state->vector_register_1074071480.get(allocated_index_0, value_0);
    // EP node  15086
    // BDD node 159:expire_items_single_map_iteratively(vector:(w64 1074102824), map:(w64 1074089000), start:(ReadLSB w32 (w32 0) new_index_r1), n_elems:(ReadLSB w32 (w32 0) vector_data_r37))
    // EP node  15228
    // BDD node 148:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_r1))
    if ((0) == (success_0)) {
      // EP node  15229
      // BDD node 148:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_r1))
      // EP node  15450
      // BDD node 314:packet_borrow_next_chunk(p:(w64 1074251832), length:(w32 20), chunk:(w64 1074263232)[ -> (w64 1073761584)])
      u8* hdr_1 = packet_consume(pkt, 20);
      // EP node  15675
      // BDD node 150:vector_return(vector:(w64 1074137256), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074151152)[(ReadLSB w16 (w32 0) vector_data_r14)])
      // EP node  15827
      // BDD node 315:packet_borrow_next_chunk(p:(w64 1074251832), length:(w32 4), chunk:(w64 1074263888)[ -> (w64 1073761840)])
      u8* hdr_2 = packet_consume(pkt, 4);
      // EP node  15981
      // BDD node 154:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r14)))
      if ((65535) != (bswap16(cpu_hdr_extra->vector_data_r14))) {
        // EP node  15982
        // BDD node 154:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r14)))
        // EP node  18428
        // BDD node 155:FORWARD
        cpu_hdr->egress_dev = bswap16(bswap16(cpu_hdr_extra->vector_data_r14));
      } else {
        // EP node  15983
        // BDD node 154:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r14)))
        // EP node  17294
        // BDD node 156:DROP
        forward = false;
      }
    } else {
      // EP node  15230
      // BDD node 148:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_r1))
      // EP node  16221
      // BDD node 316:packet_borrow_next_chunk(p:(w64 1074251832), length:(w32 20), chunk:(w64 1074263232)[ -> (w64 1073761584)])
      u8* hdr_3 = packet_consume(pkt, 20);
      // EP node  16626
      // BDD node 161:map_put(map:(w64 1074040440), key:(w64 1074068160)[(ReadLSB w32 (w32 268) packet_chunks) -> (ReadLSB w32 (w32 268) packet_chunks)], value:(ReadLSB w32 (w32 0) new_index_r1))
      buffer_t map_table_1074040440_key_0(4);
      map_table_1074040440_key_0[0] = *(u8*)(hdr_3 + 12);
      map_table_1074040440_key_0[1] = *(u8*)(hdr_3 + 13);
      map_table_1074040440_key_0[2] = *(u8*)(hdr_3 + 14);
      map_table_1074040440_key_0[3] = *(u8*)(hdr_3 + 15);
      state->map_table_1074040440.put(map_table_1074040440_key_0, allocated_index_0);
      // EP node  16954
      // BDD node 171:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r14)))
      if ((65535) != (bswap16(cpu_hdr_extra->vector_data_r14))) {
        // EP node  16955
        // BDD node 171:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r14)))
        // EP node  17638
        // BDD node 317:packet_borrow_next_chunk(p:(w64 1074251832), length:(w32 4), chunk:(w64 1074263888)[ -> (w64 1073761840)])
        u8* hdr_4 = packet_consume(pkt, 4);
        // EP node  18073
        // BDD node 164:vector_return(vector:(w64 1074071480), index:(ReadLSB w32 (w32 0) new_index_r1), value:(w64 1074085376)[(w32 1)])
        buffer_t vector_register_1074071480_value_0(4);
        vector_register_1074071480_value_0[0] = 0;
        vector_register_1074071480_value_0[1] = 0;
        vector_register_1074071480_value_0[2] = 0;
        vector_register_1074071480_value_0[3] = 1;
        state->vector_register_1074071480.put(allocated_index_0, vector_register_1074071480_value_0);
        // EP node  18250
        // BDD node 162:map_put(map:(w64 1074089000), key:(w64 1074116720)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))], value:(w32 0))
        buffer_t map_table_1074089000_key_0(6);
        map_table_1074089000_key_0[0] = *(u8*)(hdr_3 + 12);
        map_table_1074089000_key_0[1] = *(u8*)(hdr_3 + 13);
        map_table_1074089000_key_0[2] = *(u8*)(hdr_3 + 14);
        map_table_1074089000_key_0[3] = *(u8*)(hdr_3 + 15);
        map_table_1074089000_key_0[4] = *(u8*)(hdr_4 + 2);
        map_table_1074089000_key_0[5] = *(u8*)(hdr_4 + 3);
        state->map_table_1074089000.put(map_table_1074089000_key_0, 0);
        // EP node  18429
        // BDD node 167:vector_return(vector:(w64 1074137256), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074151152)[(ReadLSB w16 (w32 0) vector_data_r14)])
        // EP node  18884
        // BDD node 172:FORWARD
        cpu_hdr->egress_dev = bswap16(bswap16(cpu_hdr_extra->vector_data_r14));
      } else {
        // EP node  16956
        // BDD node 171:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r14)))
        // EP node  19161
        // BDD node 323:vector_return(vector:(w64 1074071480), index:(ReadLSB w32 (w32 0) new_index_r1), value:(w64 1074085376)[(w32 1)])
        buffer_t vector_register_1074071480_value_1(4);
        vector_register_1074071480_value_1[0] = 0;
        vector_register_1074071480_value_1[1] = 0;
        vector_register_1074071480_value_1[2] = 0;
        vector_register_1074071480_value_1[3] = 1;
        state->vector_register_1074071480.put(allocated_index_0, vector_register_1074071480_value_1);
        // EP node  19255
        // BDD node 324:vector_return(vector:(w64 1074137256), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074151152)[(ReadLSB w16 (w32 0) vector_data_r14)])
        // EP node  19349
        // BDD node 321:packet_borrow_next_chunk(p:(w64 1074251832), length:(w32 4), chunk:(w64 1074263888)[ -> (w64 1073761840)])
        u8* hdr_5 = packet_consume(pkt, 4);
        // EP node  19444
        // BDD node 322:map_put(map:(w64 1074089000), key:(w64 1074116720)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))], value:(w32 0))
        buffer_t map_table_1074089000_key_1(6);
        map_table_1074089000_key_1[0] = *(u8*)(hdr_3 + 12);
        map_table_1074089000_key_1[1] = *(u8*)(hdr_3 + 13);
        map_table_1074089000_key_1[2] = *(u8*)(hdr_3 + 14);
        map_table_1074089000_key_1[3] = *(u8*)(hdr_3 + 15);
        map_table_1074089000_key_1[4] = *(u8*)(hdr_5 + 2);
        map_table_1074089000_key_1[5] = *(u8*)(hdr_5 + 3);
        state->map_table_1074089000.put(map_table_1074089000_key_1, 0);
        // EP node  19828
        // BDD node 173:DROP
        forward = false;
      }
    }
  }
  else if (bswap16(cpu_hdr->code_path) == 10530) {
    // EP node  13392
    // BDD node 307:packet_borrow_next_chunk(p:(w64 1074251832), length:(w32 14), chunk:(w64 1074262496)[ -> (w64 1073761328)])
    u8* hdr_6 = packet_consume(pkt, 14);
    // EP node  13450
    // BDD node 308:packet_borrow_next_chunk(p:(w64 1074251832), length:(w32 20), chunk:(w64 1074263232)[ -> (w64 1073761584)])
    u8* hdr_7 = packet_consume(pkt, 20);
    // EP node  13509
    // BDD node 309:packet_borrow_next_chunk(p:(w64 1074251832), length:(w32 4), chunk:(w64 1074263888)[ -> (w64 1073761840)])
    u8* hdr_8 = packet_consume(pkt, 4);
    // EP node  13569
    // BDD node 180:map_put(map:(w64 1074089000), key:(w64 1074116720)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))], value:(Extract w32 0 (Add w64 (w64 1) (SExt w64 (Extract w32 0 (Add w64 (w64 18446744073709551615) (SExt w64 (ReadLSB w32 (w32 0) vector_data_r35))))))))
    buffer_t map_table_1074089000_key_2(6);
    map_table_1074089000_key_2[0] = *(u8*)(hdr_7 + 12);
    map_table_1074089000_key_2[1] = *(u8*)(hdr_7 + 13);
    map_table_1074089000_key_2[2] = *(u8*)(hdr_7 + 14);
    map_table_1074089000_key_2[3] = *(u8*)(hdr_7 + 15);
    map_table_1074089000_key_2[4] = *(u8*)(hdr_8 + 2);
    map_table_1074089000_key_2[5] = *(u8*)(hdr_8 + 3);
    state->map_table_1074089000.put(map_table_1074089000_key_2, (1) + ((bswap32(cpu_hdr_extra->vector_data_r35)) - (1)));
    // EP node  13874
    // BDD node 189:FORWARD
    cpu_hdr->egress_dev = bswap16(bswap16(cpu_hdr_extra->vector_data_r14));
  }
  else if (bswap16(cpu_hdr->code_path) == 11162) {
    // EP node  13875
    // BDD node 310:packet_borrow_next_chunk(p:(w64 1074251832), length:(w32 14), chunk:(w64 1074262496)[ -> (w64 1073761328)])
    u8* hdr_9 = packet_consume(pkt, 14);
    // EP node  13938
    // BDD node 311:packet_borrow_next_chunk(p:(w64 1074251832), length:(w32 20), chunk:(w64 1074263232)[ -> (w64 1073761584)])
    u8* hdr_10 = packet_consume(pkt, 20);
    // EP node  14002
    // BDD node 312:packet_borrow_next_chunk(p:(w64 1074251832), length:(w32 4), chunk:(w64 1074263888)[ -> (w64 1073761840)])
    u8* hdr_11 = packet_consume(pkt, 4);
    // EP node  14067
    // BDD node 302:map_put(map:(w64 1074089000), key:(w64 1074116720)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))], value:(Extract w32 0 (Add w64 (w64 1) (SExt w64 (Extract w32 0 (Add w64 (w64 18446744073709551615) (SExt w64 (ReadLSB w32 (w32 0) vector_data_r35))))))))
    buffer_t map_table_1074089000_key_3(6);
    map_table_1074089000_key_3[0] = *(u8*)(hdr_10 + 12);
    map_table_1074089000_key_3[1] = *(u8*)(hdr_10 + 13);
    map_table_1074089000_key_3[2] = *(u8*)(hdr_10 + 14);
    map_table_1074089000_key_3[3] = *(u8*)(hdr_10 + 15);
    map_table_1074089000_key_3[4] = *(u8*)(hdr_11 + 2);
    map_table_1074089000_key_3[5] = *(u8*)(hdr_11 + 3);
    state->map_table_1074089000.put(map_table_1074089000_key_3, (1) + ((bswap32(cpu_hdr_extra->vector_data_r35)) - (1)));
    // EP node  14331
    // BDD node 190:DROP
    forward = false;
  }


  if (trigger_update_ipv4_tcpudp_checksums) {
    update_ipv4_tcpudp_checksums(l3_hdr, l4_hdr);
  }

  return forward;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
