#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  MapSetTable map_set_table_1074044080;
  VectorTable vector_table_1074076488;
  VectorTable vector_table_1074093704;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      map_set_table_1074044080("map_set_table_1074044080",{"Ingress.map_set_table_1074044080_142","Ingress.map_set_table_1074044080_155",}, 1000LL),
      vector_table_1074076488("vector_table_1074076488",{"Ingress.vector_table_1074076488_139",}),
      vector_table_1074093704("vector_table_1074093704",{"Ingress.vector_table_1074093704_149","Ingress.vector_table_1074093704_175",})
    {}
};

state_t *state = nullptr;

void sycon::nf_init() {
  nf_state = std::make_unique<state_t>();
  state    = dynamic_cast<state_t *>(nf_state.get());
  
  state->ingress_port_to_nf_dev.add_recirc_entry(6);
  state->ingress_port_to_nf_dev.add_recirc_entry(128);
  state->ingress_port_to_nf_dev.add_recirc_entry(256);
  state->ingress_port_to_nf_dev.add_recirc_entry(384);

  state->forwarding_tbl.add_fwd_to_cpu_entry();
  state->forwarding_tbl.add_recirc_entry(6);
  state->forwarding_tbl.add_recirc_entry(128);
  state->forwarding_tbl.add_recirc_entry(256);
  state->forwarding_tbl.add_recirc_entry(384);
  state->forwarding_tbl.add_drop_entry();

  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(3), 2);
  state->forwarding_tbl.add_fwd_nf_dev_entry(2, asic_get_dev_port(3));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(4), 3);
  state->forwarding_tbl.add_fwd_nf_dev_entry(3, asic_get_dev_port(4));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(5), 4);
  state->forwarding_tbl.add_fwd_nf_dev_entry(4, asic_get_dev_port(5));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(6), 5);
  state->forwarding_tbl.add_fwd_nf_dev_entry(5, asic_get_dev_port(6));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(7), 6);
  state->forwarding_tbl.add_fwd_nf_dev_entry(6, asic_get_dev_port(7));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(8), 7);
  state->forwarding_tbl.add_fwd_nf_dev_entry(7, asic_get_dev_port(8));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(9), 8);
  state->forwarding_tbl.add_fwd_nf_dev_entry(8, asic_get_dev_port(9));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(10), 9);
  state->forwarding_tbl.add_fwd_nf_dev_entry(9, asic_get_dev_port(10));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(11), 10);
  state->forwarding_tbl.add_fwd_nf_dev_entry(10, asic_get_dev_port(11));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(12), 11);
  state->forwarding_tbl.add_fwd_nf_dev_entry(11, asic_get_dev_port(12));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(13), 12);
  state->forwarding_tbl.add_fwd_nf_dev_entry(12, asic_get_dev_port(13));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(14), 13);
  state->forwarding_tbl.add_fwd_nf_dev_entry(13, asic_get_dev_port(14));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(15), 14);
  state->forwarding_tbl.add_fwd_nf_dev_entry(14, asic_get_dev_port(15));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(16), 15);
  state->forwarding_tbl.add_fwd_nf_dev_entry(15, asic_get_dev_port(16));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(17), 16);
  state->forwarding_tbl.add_fwd_nf_dev_entry(16, asic_get_dev_port(17));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(18), 17);
  state->forwarding_tbl.add_fwd_nf_dev_entry(17, asic_get_dev_port(18));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(19), 18);
  state->forwarding_tbl.add_fwd_nf_dev_entry(18, asic_get_dev_port(19));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(20), 19);
  state->forwarding_tbl.add_fwd_nf_dev_entry(19, asic_get_dev_port(20));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(21), 20);
  state->forwarding_tbl.add_fwd_nf_dev_entry(20, asic_get_dev_port(21));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(22), 21);
  state->forwarding_tbl.add_fwd_nf_dev_entry(21, asic_get_dev_port(22));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(23), 22);
  state->forwarding_tbl.add_fwd_nf_dev_entry(22, asic_get_dev_port(23));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(24), 23);
  state->forwarding_tbl.add_fwd_nf_dev_entry(23, asic_get_dev_port(24));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(25), 24);
  state->forwarding_tbl.add_fwd_nf_dev_entry(24, asic_get_dev_port(25));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(26), 25);
  state->forwarding_tbl.add_fwd_nf_dev_entry(25, asic_get_dev_port(26));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(27), 26);
  state->forwarding_tbl.add_fwd_nf_dev_entry(26, asic_get_dev_port(27));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(28), 27);
  state->forwarding_tbl.add_fwd_nf_dev_entry(27, asic_get_dev_port(28));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(29), 28);
  state->forwarding_tbl.add_fwd_nf_dev_entry(28, asic_get_dev_port(29));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(30), 29);
  state->forwarding_tbl.add_fwd_nf_dev_entry(29, asic_get_dev_port(30));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(31), 30);
  state->forwarding_tbl.add_fwd_nf_dev_entry(30, asic_get_dev_port(31));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(32), 31);
  state->forwarding_tbl.add_fwd_nf_dev_entry(31, asic_get_dev_port(32));
  // BDD node 0:map_allocate(capacity:(w32 65536), key_size:(w32 12), map_out:(w64 1074043816)[(w64 0) -> (w64 1074044080)])
  // Module DataplaneMapSetTableAllocate
  // BDD node 2:dchain_allocate(index_range:(w32 65536), chain_out:(w64 1074043832)[ -> (w64 1074076064)])
  // Module Ignore
  // BDD node 3:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1074043840)[(w64 0) -> (w64 1074076488)])
  // Module DataplaneVectorTableAllocate
  // BDD node 4:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074043848)[(w64 0) -> (w64 1074093704)])
  // Module DataplaneVectorTableAllocate
  // BDD node 5:vector_borrow(vector:(w64 1074076488), index:(w32 0), val_out:(w64 1074043688)[ -> (w64 1074090384)])
  // Module Ignore
  // BDD node 6:vector_return(vector:(w64 1074076488), index:(w32 0), value:(w64 1074090384)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_0(4);
  vector_table_1074076488_value_0[0] = 0;
  vector_table_1074076488_value_0[1] = 0;
  vector_table_1074076488_value_0[2] = 0;
  vector_table_1074076488_value_0[3] = 1;
  state->vector_table_1074076488.write(0, vector_table_1074076488_value_0);
  // BDD node 7:vector_borrow(vector:(w64 1074093704), index:(w32 0), val_out:(w64 1074043752)[ -> (w64 1074107600)])
  // Module Ignore
  // BDD node 8:vector_return(vector:(w64 1074093704), index:(w32 0), value:(w64 1074107600)[(w16 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_0(2);
  vector_table_1074093704_value_0[0] = 0;
  vector_table_1074093704_value_0[1] = 1;
  state->vector_table_1074093704.write(0, vector_table_1074093704_value_0);
  // BDD node 9:vector_borrow(vector:(w64 1074076488), index:(w32 1), val_out:(w64 1074043688)[ -> (w64 1074090408)])
  // Module Ignore
  // BDD node 10:vector_return(vector:(w64 1074076488), index:(w32 1), value:(w64 1074090408)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_1(4);
  vector_table_1074076488_value_1[0] = 0;
  vector_table_1074076488_value_1[1] = 0;
  vector_table_1074076488_value_1[2] = 0;
  vector_table_1074076488_value_1[3] = 0;
  state->vector_table_1074076488.write(1, vector_table_1074076488_value_1);
  // BDD node 11:vector_borrow(vector:(w64 1074093704), index:(w32 1), val_out:(w64 1074043752)[ -> (w64 1074107624)])
  // Module Ignore
  // BDD node 12:vector_return(vector:(w64 1074093704), index:(w32 1), value:(w64 1074107624)[(w16 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_1(2);
  vector_table_1074093704_value_1[0] = 0;
  vector_table_1074093704_value_1[1] = 0;
  state->vector_table_1074093704.write(1, vector_table_1074093704_value_1);
  // BDD node 13:vector_borrow(vector:(w64 1074076488), index:(w32 2), val_out:(w64 1074043688)[ -> (w64 1074090432)])
  // Module Ignore
  // BDD node 14:vector_return(vector:(w64 1074076488), index:(w32 2), value:(w64 1074090432)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_2(4);
  vector_table_1074076488_value_2[0] = 0;
  vector_table_1074076488_value_2[1] = 0;
  vector_table_1074076488_value_2[2] = 0;
  vector_table_1074076488_value_2[3] = 1;
  state->vector_table_1074076488.write(2, vector_table_1074076488_value_2);
  // BDD node 15:vector_borrow(vector:(w64 1074093704), index:(w32 2), val_out:(w64 1074043752)[ -> (w64 1074107648)])
  // Module Ignore
  // BDD node 16:vector_return(vector:(w64 1074093704), index:(w32 2), value:(w64 1074107648)[(w16 3)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_2(2);
  vector_table_1074093704_value_2[0] = 0;
  vector_table_1074093704_value_2[1] = 3;
  state->vector_table_1074093704.write(2, vector_table_1074093704_value_2);
  // BDD node 17:vector_borrow(vector:(w64 1074076488), index:(w32 3), val_out:(w64 1074043688)[ -> (w64 1074090456)])
  // Module Ignore
  // BDD node 18:vector_return(vector:(w64 1074076488), index:(w32 3), value:(w64 1074090456)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_3(4);
  vector_table_1074076488_value_3[0] = 0;
  vector_table_1074076488_value_3[1] = 0;
  vector_table_1074076488_value_3[2] = 0;
  vector_table_1074076488_value_3[3] = 0;
  state->vector_table_1074076488.write(3, vector_table_1074076488_value_3);
  // BDD node 19:vector_borrow(vector:(w64 1074093704), index:(w32 3), val_out:(w64 1074043752)[ -> (w64 1074107672)])
  // Module Ignore
  // BDD node 20:vector_return(vector:(w64 1074093704), index:(w32 3), value:(w64 1074107672)[(w16 2)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_3(2);
  vector_table_1074093704_value_3[0] = 0;
  vector_table_1074093704_value_3[1] = 2;
  state->vector_table_1074093704.write(3, vector_table_1074093704_value_3);
  // BDD node 21:vector_borrow(vector:(w64 1074076488), index:(w32 4), val_out:(w64 1074043688)[ -> (w64 1074090480)])
  // Module Ignore
  // BDD node 22:vector_return(vector:(w64 1074076488), index:(w32 4), value:(w64 1074090480)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_4(4);
  vector_table_1074076488_value_4[0] = 0;
  vector_table_1074076488_value_4[1] = 0;
  vector_table_1074076488_value_4[2] = 0;
  vector_table_1074076488_value_4[3] = 1;
  state->vector_table_1074076488.write(4, vector_table_1074076488_value_4);
  // BDD node 23:vector_borrow(vector:(w64 1074093704), index:(w32 4), val_out:(w64 1074043752)[ -> (w64 1074107696)])
  // Module Ignore
  // BDD node 24:vector_return(vector:(w64 1074093704), index:(w32 4), value:(w64 1074107696)[(w16 5)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_4(2);
  vector_table_1074093704_value_4[0] = 0;
  vector_table_1074093704_value_4[1] = 5;
  state->vector_table_1074093704.write(4, vector_table_1074093704_value_4);
  // BDD node 25:vector_borrow(vector:(w64 1074076488), index:(w32 5), val_out:(w64 1074043688)[ -> (w64 1074090504)])
  // Module Ignore
  // BDD node 26:vector_return(vector:(w64 1074076488), index:(w32 5), value:(w64 1074090504)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_5(4);
  vector_table_1074076488_value_5[0] = 0;
  vector_table_1074076488_value_5[1] = 0;
  vector_table_1074076488_value_5[2] = 0;
  vector_table_1074076488_value_5[3] = 0;
  state->vector_table_1074076488.write(5, vector_table_1074076488_value_5);
  // BDD node 27:vector_borrow(vector:(w64 1074093704), index:(w32 5), val_out:(w64 1074043752)[ -> (w64 1074107720)])
  // Module Ignore
  // BDD node 28:vector_return(vector:(w64 1074093704), index:(w32 5), value:(w64 1074107720)[(w16 4)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_5(2);
  vector_table_1074093704_value_5[0] = 0;
  vector_table_1074093704_value_5[1] = 4;
  state->vector_table_1074093704.write(5, vector_table_1074093704_value_5);
  // BDD node 29:vector_borrow(vector:(w64 1074076488), index:(w32 6), val_out:(w64 1074043688)[ -> (w64 1074090528)])
  // Module Ignore
  // BDD node 30:vector_return(vector:(w64 1074076488), index:(w32 6), value:(w64 1074090528)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_6(4);
  vector_table_1074076488_value_6[0] = 0;
  vector_table_1074076488_value_6[1] = 0;
  vector_table_1074076488_value_6[2] = 0;
  vector_table_1074076488_value_6[3] = 1;
  state->vector_table_1074076488.write(6, vector_table_1074076488_value_6);
  // BDD node 31:vector_borrow(vector:(w64 1074093704), index:(w32 6), val_out:(w64 1074043752)[ -> (w64 1074107744)])
  // Module Ignore
  // BDD node 32:vector_return(vector:(w64 1074093704), index:(w32 6), value:(w64 1074107744)[(w16 7)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_6(2);
  vector_table_1074093704_value_6[0] = 0;
  vector_table_1074093704_value_6[1] = 7;
  state->vector_table_1074093704.write(6, vector_table_1074093704_value_6);
  // BDD node 33:vector_borrow(vector:(w64 1074076488), index:(w32 7), val_out:(w64 1074043688)[ -> (w64 1074090552)])
  // Module Ignore
  // BDD node 34:vector_return(vector:(w64 1074076488), index:(w32 7), value:(w64 1074090552)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_7(4);
  vector_table_1074076488_value_7[0] = 0;
  vector_table_1074076488_value_7[1] = 0;
  vector_table_1074076488_value_7[2] = 0;
  vector_table_1074076488_value_7[3] = 0;
  state->vector_table_1074076488.write(7, vector_table_1074076488_value_7);
  // BDD node 35:vector_borrow(vector:(w64 1074093704), index:(w32 7), val_out:(w64 1074043752)[ -> (w64 1074107768)])
  // Module Ignore
  // BDD node 36:vector_return(vector:(w64 1074093704), index:(w32 7), value:(w64 1074107768)[(w16 6)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_7(2);
  vector_table_1074093704_value_7[0] = 0;
  vector_table_1074093704_value_7[1] = 6;
  state->vector_table_1074093704.write(7, vector_table_1074093704_value_7);
  // BDD node 37:vector_borrow(vector:(w64 1074076488), index:(w32 8), val_out:(w64 1074043688)[ -> (w64 1074090576)])
  // Module Ignore
  // BDD node 38:vector_return(vector:(w64 1074076488), index:(w32 8), value:(w64 1074090576)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_8(4);
  vector_table_1074076488_value_8[0] = 0;
  vector_table_1074076488_value_8[1] = 0;
  vector_table_1074076488_value_8[2] = 0;
  vector_table_1074076488_value_8[3] = 1;
  state->vector_table_1074076488.write(8, vector_table_1074076488_value_8);
  // BDD node 39:vector_borrow(vector:(w64 1074093704), index:(w32 8), val_out:(w64 1074043752)[ -> (w64 1074107792)])
  // Module Ignore
  // BDD node 40:vector_return(vector:(w64 1074093704), index:(w32 8), value:(w64 1074107792)[(w16 9)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_8(2);
  vector_table_1074093704_value_8[0] = 0;
  vector_table_1074093704_value_8[1] = 9;
  state->vector_table_1074093704.write(8, vector_table_1074093704_value_8);
  // BDD node 41:vector_borrow(vector:(w64 1074076488), index:(w32 9), val_out:(w64 1074043688)[ -> (w64 1074090600)])
  // Module Ignore
  // BDD node 42:vector_return(vector:(w64 1074076488), index:(w32 9), value:(w64 1074090600)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_9(4);
  vector_table_1074076488_value_9[0] = 0;
  vector_table_1074076488_value_9[1] = 0;
  vector_table_1074076488_value_9[2] = 0;
  vector_table_1074076488_value_9[3] = 0;
  state->vector_table_1074076488.write(9, vector_table_1074076488_value_9);
  // BDD node 43:vector_borrow(vector:(w64 1074093704), index:(w32 9), val_out:(w64 1074043752)[ -> (w64 1074107816)])
  // Module Ignore
  // BDD node 44:vector_return(vector:(w64 1074093704), index:(w32 9), value:(w64 1074107816)[(w16 8)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_9(2);
  vector_table_1074093704_value_9[0] = 0;
  vector_table_1074093704_value_9[1] = 8;
  state->vector_table_1074093704.write(9, vector_table_1074093704_value_9);
  // BDD node 45:vector_borrow(vector:(w64 1074076488), index:(w32 10), val_out:(w64 1074043688)[ -> (w64 1074090624)])
  // Module Ignore
  // BDD node 46:vector_return(vector:(w64 1074076488), index:(w32 10), value:(w64 1074090624)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_10(4);
  vector_table_1074076488_value_10[0] = 0;
  vector_table_1074076488_value_10[1] = 0;
  vector_table_1074076488_value_10[2] = 0;
  vector_table_1074076488_value_10[3] = 1;
  state->vector_table_1074076488.write(10, vector_table_1074076488_value_10);
  // BDD node 47:vector_borrow(vector:(w64 1074093704), index:(w32 10), val_out:(w64 1074043752)[ -> (w64 1074107840)])
  // Module Ignore
  // BDD node 48:vector_return(vector:(w64 1074093704), index:(w32 10), value:(w64 1074107840)[(w16 11)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_10(2);
  vector_table_1074093704_value_10[0] = 0;
  vector_table_1074093704_value_10[1] = 11;
  state->vector_table_1074093704.write(10, vector_table_1074093704_value_10);
  // BDD node 49:vector_borrow(vector:(w64 1074076488), index:(w32 11), val_out:(w64 1074043688)[ -> (w64 1074090648)])
  // Module Ignore
  // BDD node 50:vector_return(vector:(w64 1074076488), index:(w32 11), value:(w64 1074090648)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_11(4);
  vector_table_1074076488_value_11[0] = 0;
  vector_table_1074076488_value_11[1] = 0;
  vector_table_1074076488_value_11[2] = 0;
  vector_table_1074076488_value_11[3] = 0;
  state->vector_table_1074076488.write(11, vector_table_1074076488_value_11);
  // BDD node 51:vector_borrow(vector:(w64 1074093704), index:(w32 11), val_out:(w64 1074043752)[ -> (w64 1074107864)])
  // Module Ignore
  // BDD node 52:vector_return(vector:(w64 1074093704), index:(w32 11), value:(w64 1074107864)[(w16 10)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_11(2);
  vector_table_1074093704_value_11[0] = 0;
  vector_table_1074093704_value_11[1] = 10;
  state->vector_table_1074093704.write(11, vector_table_1074093704_value_11);
  // BDD node 53:vector_borrow(vector:(w64 1074076488), index:(w32 12), val_out:(w64 1074043688)[ -> (w64 1074090672)])
  // Module Ignore
  // BDD node 54:vector_return(vector:(w64 1074076488), index:(w32 12), value:(w64 1074090672)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_12(4);
  vector_table_1074076488_value_12[0] = 0;
  vector_table_1074076488_value_12[1] = 0;
  vector_table_1074076488_value_12[2] = 0;
  vector_table_1074076488_value_12[3] = 1;
  state->vector_table_1074076488.write(12, vector_table_1074076488_value_12);
  // BDD node 55:vector_borrow(vector:(w64 1074093704), index:(w32 12), val_out:(w64 1074043752)[ -> (w64 1074107888)])
  // Module Ignore
  // BDD node 56:vector_return(vector:(w64 1074093704), index:(w32 12), value:(w64 1074107888)[(w16 13)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_12(2);
  vector_table_1074093704_value_12[0] = 0;
  vector_table_1074093704_value_12[1] = 13;
  state->vector_table_1074093704.write(12, vector_table_1074093704_value_12);
  // BDD node 57:vector_borrow(vector:(w64 1074076488), index:(w32 13), val_out:(w64 1074043688)[ -> (w64 1074090696)])
  // Module Ignore
  // BDD node 58:vector_return(vector:(w64 1074076488), index:(w32 13), value:(w64 1074090696)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_13(4);
  vector_table_1074076488_value_13[0] = 0;
  vector_table_1074076488_value_13[1] = 0;
  vector_table_1074076488_value_13[2] = 0;
  vector_table_1074076488_value_13[3] = 0;
  state->vector_table_1074076488.write(13, vector_table_1074076488_value_13);
  // BDD node 59:vector_borrow(vector:(w64 1074093704), index:(w32 13), val_out:(w64 1074043752)[ -> (w64 1074107912)])
  // Module Ignore
  // BDD node 60:vector_return(vector:(w64 1074093704), index:(w32 13), value:(w64 1074107912)[(w16 12)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_13(2);
  vector_table_1074093704_value_13[0] = 0;
  vector_table_1074093704_value_13[1] = 12;
  state->vector_table_1074093704.write(13, vector_table_1074093704_value_13);
  // BDD node 61:vector_borrow(vector:(w64 1074076488), index:(w32 14), val_out:(w64 1074043688)[ -> (w64 1074090720)])
  // Module Ignore
  // BDD node 62:vector_return(vector:(w64 1074076488), index:(w32 14), value:(w64 1074090720)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_14(4);
  vector_table_1074076488_value_14[0] = 0;
  vector_table_1074076488_value_14[1] = 0;
  vector_table_1074076488_value_14[2] = 0;
  vector_table_1074076488_value_14[3] = 1;
  state->vector_table_1074076488.write(14, vector_table_1074076488_value_14);
  // BDD node 63:vector_borrow(vector:(w64 1074093704), index:(w32 14), val_out:(w64 1074043752)[ -> (w64 1074107936)])
  // Module Ignore
  // BDD node 64:vector_return(vector:(w64 1074093704), index:(w32 14), value:(w64 1074107936)[(w16 15)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_14(2);
  vector_table_1074093704_value_14[0] = 0;
  vector_table_1074093704_value_14[1] = 15;
  state->vector_table_1074093704.write(14, vector_table_1074093704_value_14);
  // BDD node 65:vector_borrow(vector:(w64 1074076488), index:(w32 15), val_out:(w64 1074043688)[ -> (w64 1074090744)])
  // Module Ignore
  // BDD node 66:vector_return(vector:(w64 1074076488), index:(w32 15), value:(w64 1074090744)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_15(4);
  vector_table_1074076488_value_15[0] = 0;
  vector_table_1074076488_value_15[1] = 0;
  vector_table_1074076488_value_15[2] = 0;
  vector_table_1074076488_value_15[3] = 0;
  state->vector_table_1074076488.write(15, vector_table_1074076488_value_15);
  // BDD node 67:vector_borrow(vector:(w64 1074093704), index:(w32 15), val_out:(w64 1074043752)[ -> (w64 1074107960)])
  // Module Ignore
  // BDD node 68:vector_return(vector:(w64 1074093704), index:(w32 15), value:(w64 1074107960)[(w16 14)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_15(2);
  vector_table_1074093704_value_15[0] = 0;
  vector_table_1074093704_value_15[1] = 14;
  state->vector_table_1074093704.write(15, vector_table_1074093704_value_15);
  // BDD node 69:vector_borrow(vector:(w64 1074076488), index:(w32 16), val_out:(w64 1074043688)[ -> (w64 1074090768)])
  // Module Ignore
  // BDD node 70:vector_return(vector:(w64 1074076488), index:(w32 16), value:(w64 1074090768)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_16(4);
  vector_table_1074076488_value_16[0] = 0;
  vector_table_1074076488_value_16[1] = 0;
  vector_table_1074076488_value_16[2] = 0;
  vector_table_1074076488_value_16[3] = 1;
  state->vector_table_1074076488.write(16, vector_table_1074076488_value_16);
  // BDD node 71:vector_borrow(vector:(w64 1074093704), index:(w32 16), val_out:(w64 1074043752)[ -> (w64 1074107984)])
  // Module Ignore
  // BDD node 72:vector_return(vector:(w64 1074093704), index:(w32 16), value:(w64 1074107984)[(w16 17)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_16(2);
  vector_table_1074093704_value_16[0] = 0;
  vector_table_1074093704_value_16[1] = 17;
  state->vector_table_1074093704.write(16, vector_table_1074093704_value_16);
  // BDD node 73:vector_borrow(vector:(w64 1074076488), index:(w32 17), val_out:(w64 1074043688)[ -> (w64 1074090792)])
  // Module Ignore
  // BDD node 74:vector_return(vector:(w64 1074076488), index:(w32 17), value:(w64 1074090792)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_17(4);
  vector_table_1074076488_value_17[0] = 0;
  vector_table_1074076488_value_17[1] = 0;
  vector_table_1074076488_value_17[2] = 0;
  vector_table_1074076488_value_17[3] = 0;
  state->vector_table_1074076488.write(17, vector_table_1074076488_value_17);
  // BDD node 75:vector_borrow(vector:(w64 1074093704), index:(w32 17), val_out:(w64 1074043752)[ -> (w64 1074108008)])
  // Module Ignore
  // BDD node 76:vector_return(vector:(w64 1074093704), index:(w32 17), value:(w64 1074108008)[(w16 16)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_17(2);
  vector_table_1074093704_value_17[0] = 0;
  vector_table_1074093704_value_17[1] = 16;
  state->vector_table_1074093704.write(17, vector_table_1074093704_value_17);
  // BDD node 77:vector_borrow(vector:(w64 1074076488), index:(w32 18), val_out:(w64 1074043688)[ -> (w64 1074090816)])
  // Module Ignore
  // BDD node 78:vector_return(vector:(w64 1074076488), index:(w32 18), value:(w64 1074090816)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_18(4);
  vector_table_1074076488_value_18[0] = 0;
  vector_table_1074076488_value_18[1] = 0;
  vector_table_1074076488_value_18[2] = 0;
  vector_table_1074076488_value_18[3] = 1;
  state->vector_table_1074076488.write(18, vector_table_1074076488_value_18);
  // BDD node 79:vector_borrow(vector:(w64 1074093704), index:(w32 18), val_out:(w64 1074043752)[ -> (w64 1074108032)])
  // Module Ignore
  // BDD node 80:vector_return(vector:(w64 1074093704), index:(w32 18), value:(w64 1074108032)[(w16 19)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_18(2);
  vector_table_1074093704_value_18[0] = 0;
  vector_table_1074093704_value_18[1] = 19;
  state->vector_table_1074093704.write(18, vector_table_1074093704_value_18);
  // BDD node 81:vector_borrow(vector:(w64 1074076488), index:(w32 19), val_out:(w64 1074043688)[ -> (w64 1074090840)])
  // Module Ignore
  // BDD node 82:vector_return(vector:(w64 1074076488), index:(w32 19), value:(w64 1074090840)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_19(4);
  vector_table_1074076488_value_19[0] = 0;
  vector_table_1074076488_value_19[1] = 0;
  vector_table_1074076488_value_19[2] = 0;
  vector_table_1074076488_value_19[3] = 0;
  state->vector_table_1074076488.write(19, vector_table_1074076488_value_19);
  // BDD node 83:vector_borrow(vector:(w64 1074093704), index:(w32 19), val_out:(w64 1074043752)[ -> (w64 1074108056)])
  // Module Ignore
  // BDD node 84:vector_return(vector:(w64 1074093704), index:(w32 19), value:(w64 1074108056)[(w16 18)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_19(2);
  vector_table_1074093704_value_19[0] = 0;
  vector_table_1074093704_value_19[1] = 18;
  state->vector_table_1074093704.write(19, vector_table_1074093704_value_19);
  // BDD node 85:vector_borrow(vector:(w64 1074076488), index:(w32 20), val_out:(w64 1074043688)[ -> (w64 1074090864)])
  // Module Ignore
  // BDD node 86:vector_return(vector:(w64 1074076488), index:(w32 20), value:(w64 1074090864)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_20(4);
  vector_table_1074076488_value_20[0] = 0;
  vector_table_1074076488_value_20[1] = 0;
  vector_table_1074076488_value_20[2] = 0;
  vector_table_1074076488_value_20[3] = 1;
  state->vector_table_1074076488.write(20, vector_table_1074076488_value_20);
  // BDD node 87:vector_borrow(vector:(w64 1074093704), index:(w32 20), val_out:(w64 1074043752)[ -> (w64 1074108080)])
  // Module Ignore
  // BDD node 88:vector_return(vector:(w64 1074093704), index:(w32 20), value:(w64 1074108080)[(w16 21)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_20(2);
  vector_table_1074093704_value_20[0] = 0;
  vector_table_1074093704_value_20[1] = 21;
  state->vector_table_1074093704.write(20, vector_table_1074093704_value_20);
  // BDD node 89:vector_borrow(vector:(w64 1074076488), index:(w32 21), val_out:(w64 1074043688)[ -> (w64 1074090888)])
  // Module Ignore
  // BDD node 90:vector_return(vector:(w64 1074076488), index:(w32 21), value:(w64 1074090888)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_21(4);
  vector_table_1074076488_value_21[0] = 0;
  vector_table_1074076488_value_21[1] = 0;
  vector_table_1074076488_value_21[2] = 0;
  vector_table_1074076488_value_21[3] = 0;
  state->vector_table_1074076488.write(21, vector_table_1074076488_value_21);
  // BDD node 91:vector_borrow(vector:(w64 1074093704), index:(w32 21), val_out:(w64 1074043752)[ -> (w64 1074108104)])
  // Module Ignore
  // BDD node 92:vector_return(vector:(w64 1074093704), index:(w32 21), value:(w64 1074108104)[(w16 20)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_21(2);
  vector_table_1074093704_value_21[0] = 0;
  vector_table_1074093704_value_21[1] = 20;
  state->vector_table_1074093704.write(21, vector_table_1074093704_value_21);
  // BDD node 93:vector_borrow(vector:(w64 1074076488), index:(w32 22), val_out:(w64 1074043688)[ -> (w64 1074090912)])
  // Module Ignore
  // BDD node 94:vector_return(vector:(w64 1074076488), index:(w32 22), value:(w64 1074090912)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_22(4);
  vector_table_1074076488_value_22[0] = 0;
  vector_table_1074076488_value_22[1] = 0;
  vector_table_1074076488_value_22[2] = 0;
  vector_table_1074076488_value_22[3] = 1;
  state->vector_table_1074076488.write(22, vector_table_1074076488_value_22);
  // BDD node 95:vector_borrow(vector:(w64 1074093704), index:(w32 22), val_out:(w64 1074043752)[ -> (w64 1074108128)])
  // Module Ignore
  // BDD node 96:vector_return(vector:(w64 1074093704), index:(w32 22), value:(w64 1074108128)[(w16 23)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_22(2);
  vector_table_1074093704_value_22[0] = 0;
  vector_table_1074093704_value_22[1] = 23;
  state->vector_table_1074093704.write(22, vector_table_1074093704_value_22);
  // BDD node 97:vector_borrow(vector:(w64 1074076488), index:(w32 23), val_out:(w64 1074043688)[ -> (w64 1074090936)])
  // Module Ignore
  // BDD node 98:vector_return(vector:(w64 1074076488), index:(w32 23), value:(w64 1074090936)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_23(4);
  vector_table_1074076488_value_23[0] = 0;
  vector_table_1074076488_value_23[1] = 0;
  vector_table_1074076488_value_23[2] = 0;
  vector_table_1074076488_value_23[3] = 0;
  state->vector_table_1074076488.write(23, vector_table_1074076488_value_23);
  // BDD node 99:vector_borrow(vector:(w64 1074093704), index:(w32 23), val_out:(w64 1074043752)[ -> (w64 1074108152)])
  // Module Ignore
  // BDD node 100:vector_return(vector:(w64 1074093704), index:(w32 23), value:(w64 1074108152)[(w16 22)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_23(2);
  vector_table_1074093704_value_23[0] = 0;
  vector_table_1074093704_value_23[1] = 22;
  state->vector_table_1074093704.write(23, vector_table_1074093704_value_23);
  // BDD node 101:vector_borrow(vector:(w64 1074076488), index:(w32 24), val_out:(w64 1074043688)[ -> (w64 1074090960)])
  // Module Ignore
  // BDD node 102:vector_return(vector:(w64 1074076488), index:(w32 24), value:(w64 1074090960)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_24(4);
  vector_table_1074076488_value_24[0] = 0;
  vector_table_1074076488_value_24[1] = 0;
  vector_table_1074076488_value_24[2] = 0;
  vector_table_1074076488_value_24[3] = 1;
  state->vector_table_1074076488.write(24, vector_table_1074076488_value_24);
  // BDD node 103:vector_borrow(vector:(w64 1074093704), index:(w32 24), val_out:(w64 1074043752)[ -> (w64 1074108176)])
  // Module Ignore
  // BDD node 104:vector_return(vector:(w64 1074093704), index:(w32 24), value:(w64 1074108176)[(w16 25)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_24(2);
  vector_table_1074093704_value_24[0] = 0;
  vector_table_1074093704_value_24[1] = 25;
  state->vector_table_1074093704.write(24, vector_table_1074093704_value_24);
  // BDD node 105:vector_borrow(vector:(w64 1074076488), index:(w32 25), val_out:(w64 1074043688)[ -> (w64 1074090984)])
  // Module Ignore
  // BDD node 106:vector_return(vector:(w64 1074076488), index:(w32 25), value:(w64 1074090984)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_25(4);
  vector_table_1074076488_value_25[0] = 0;
  vector_table_1074076488_value_25[1] = 0;
  vector_table_1074076488_value_25[2] = 0;
  vector_table_1074076488_value_25[3] = 0;
  state->vector_table_1074076488.write(25, vector_table_1074076488_value_25);
  // BDD node 107:vector_borrow(vector:(w64 1074093704), index:(w32 25), val_out:(w64 1074043752)[ -> (w64 1074108200)])
  // Module Ignore
  // BDD node 108:vector_return(vector:(w64 1074093704), index:(w32 25), value:(w64 1074108200)[(w16 24)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_25(2);
  vector_table_1074093704_value_25[0] = 0;
  vector_table_1074093704_value_25[1] = 24;
  state->vector_table_1074093704.write(25, vector_table_1074093704_value_25);
  // BDD node 109:vector_borrow(vector:(w64 1074076488), index:(w32 26), val_out:(w64 1074043688)[ -> (w64 1074091008)])
  // Module Ignore
  // BDD node 110:vector_return(vector:(w64 1074076488), index:(w32 26), value:(w64 1074091008)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_26(4);
  vector_table_1074076488_value_26[0] = 0;
  vector_table_1074076488_value_26[1] = 0;
  vector_table_1074076488_value_26[2] = 0;
  vector_table_1074076488_value_26[3] = 1;
  state->vector_table_1074076488.write(26, vector_table_1074076488_value_26);
  // BDD node 111:vector_borrow(vector:(w64 1074093704), index:(w32 26), val_out:(w64 1074043752)[ -> (w64 1074108224)])
  // Module Ignore
  // BDD node 112:vector_return(vector:(w64 1074093704), index:(w32 26), value:(w64 1074108224)[(w16 27)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_26(2);
  vector_table_1074093704_value_26[0] = 0;
  vector_table_1074093704_value_26[1] = 27;
  state->vector_table_1074093704.write(26, vector_table_1074093704_value_26);
  // BDD node 113:vector_borrow(vector:(w64 1074076488), index:(w32 27), val_out:(w64 1074043688)[ -> (w64 1074091032)])
  // Module Ignore
  // BDD node 114:vector_return(vector:(w64 1074076488), index:(w32 27), value:(w64 1074091032)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_27(4);
  vector_table_1074076488_value_27[0] = 0;
  vector_table_1074076488_value_27[1] = 0;
  vector_table_1074076488_value_27[2] = 0;
  vector_table_1074076488_value_27[3] = 0;
  state->vector_table_1074076488.write(27, vector_table_1074076488_value_27);
  // BDD node 115:vector_borrow(vector:(w64 1074093704), index:(w32 27), val_out:(w64 1074043752)[ -> (w64 1074108248)])
  // Module Ignore
  // BDD node 116:vector_return(vector:(w64 1074093704), index:(w32 27), value:(w64 1074108248)[(w16 26)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_27(2);
  vector_table_1074093704_value_27[0] = 0;
  vector_table_1074093704_value_27[1] = 26;
  state->vector_table_1074093704.write(27, vector_table_1074093704_value_27);
  // BDD node 117:vector_borrow(vector:(w64 1074076488), index:(w32 28), val_out:(w64 1074043688)[ -> (w64 1074091056)])
  // Module Ignore
  // BDD node 118:vector_return(vector:(w64 1074076488), index:(w32 28), value:(w64 1074091056)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_28(4);
  vector_table_1074076488_value_28[0] = 0;
  vector_table_1074076488_value_28[1] = 0;
  vector_table_1074076488_value_28[2] = 0;
  vector_table_1074076488_value_28[3] = 1;
  state->vector_table_1074076488.write(28, vector_table_1074076488_value_28);
  // BDD node 119:vector_borrow(vector:(w64 1074093704), index:(w32 28), val_out:(w64 1074043752)[ -> (w64 1074108272)])
  // Module Ignore
  // BDD node 120:vector_return(vector:(w64 1074093704), index:(w32 28), value:(w64 1074108272)[(w16 29)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_28(2);
  vector_table_1074093704_value_28[0] = 0;
  vector_table_1074093704_value_28[1] = 29;
  state->vector_table_1074093704.write(28, vector_table_1074093704_value_28);
  // BDD node 121:vector_borrow(vector:(w64 1074076488), index:(w32 29), val_out:(w64 1074043688)[ -> (w64 1074091080)])
  // Module Ignore
  // BDD node 122:vector_return(vector:(w64 1074076488), index:(w32 29), value:(w64 1074091080)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_29(4);
  vector_table_1074076488_value_29[0] = 0;
  vector_table_1074076488_value_29[1] = 0;
  vector_table_1074076488_value_29[2] = 0;
  vector_table_1074076488_value_29[3] = 0;
  state->vector_table_1074076488.write(29, vector_table_1074076488_value_29);
  // BDD node 123:vector_borrow(vector:(w64 1074093704), index:(w32 29), val_out:(w64 1074043752)[ -> (w64 1074108296)])
  // Module Ignore
  // BDD node 124:vector_return(vector:(w64 1074093704), index:(w32 29), value:(w64 1074108296)[(w16 28)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_29(2);
  vector_table_1074093704_value_29[0] = 0;
  vector_table_1074093704_value_29[1] = 28;
  state->vector_table_1074093704.write(29, vector_table_1074093704_value_29);
  // BDD node 125:vector_borrow(vector:(w64 1074076488), index:(w32 30), val_out:(w64 1074043688)[ -> (w64 1074091104)])
  // Module Ignore
  // BDD node 126:vector_return(vector:(w64 1074076488), index:(w32 30), value:(w64 1074091104)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_30(4);
  vector_table_1074076488_value_30[0] = 0;
  vector_table_1074076488_value_30[1] = 0;
  vector_table_1074076488_value_30[2] = 0;
  vector_table_1074076488_value_30[3] = 1;
  state->vector_table_1074076488.write(30, vector_table_1074076488_value_30);
  // BDD node 127:vector_borrow(vector:(w64 1074093704), index:(w32 30), val_out:(w64 1074043752)[ -> (w64 1074108320)])
  // Module Ignore
  // BDD node 128:vector_return(vector:(w64 1074093704), index:(w32 30), value:(w64 1074108320)[(w16 31)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_30(2);
  vector_table_1074093704_value_30[0] = 0;
  vector_table_1074093704_value_30[1] = 31;
  state->vector_table_1074093704.write(30, vector_table_1074093704_value_30);
  // BDD node 129:vector_borrow(vector:(w64 1074076488), index:(w32 31), val_out:(w64 1074043688)[ -> (w64 1074091128)])
  // Module Ignore
  // BDD node 130:vector_return(vector:(w64 1074076488), index:(w32 31), value:(w64 1074091128)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076488_value_31(4);
  vector_table_1074076488_value_31[0] = 0;
  vector_table_1074076488_value_31[1] = 0;
  vector_table_1074076488_value_31[2] = 0;
  vector_table_1074076488_value_31[3] = 0;
  state->vector_table_1074076488.write(31, vector_table_1074076488_value_31);
  // BDD node 131:vector_borrow(vector:(w64 1074093704), index:(w32 31), val_out:(w64 1074043752)[ -> (w64 1074108344)])
  // Module Ignore
  // BDD node 132:vector_return(vector:(w64 1074093704), index:(w32 31), value:(w64 1074108344)[(w16 30)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093704_value_31(2);
  vector_table_1074093704_value_31[0] = 0;
  vector_table_1074093704_value_31[1] = 30;
  state->vector_table_1074093704.write(31, vector_table_1074093704_value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 vector_data__139;
  u32 map_has_this_key__155;
  u32 DEVICE;

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

  if (bswap16(cpu_hdr->code_path) == 2374) {
    // EP node  2360
    // BDD node 157:dchain_allocate_new_index(chain:(w64 1074076064), index_out:(w64 1074210936)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__157)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  2361
    // BDD node 157:dchain_allocate_new_index(chain:(w64 1074076064), index_out:(w64 1074210936)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__157)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  2362
    // BDD node 157:dchain_allocate_new_index(chain:(w64 1074076064), index_out:(w64 1074210936)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__157)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_2 = packet_consume(pkt, 4);
    // EP node  2363
    // BDD node 157:dchain_allocate_new_index(chain:(w64 1074076064), index_out:(w64 1074210936)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__157)], time:(ReadLSB w64 (w32 0) next_time))
    buffer_t value_0;
    state->vector_table_1074076488.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_0);
    // EP node  2364
    // BDD node 157:dchain_allocate_new_index(chain:(w64 1074076064), index_out:(w64 1074210936)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__157)], time:(ReadLSB w64 (w32 0) next_time))
    buffer_t map_set_table_1074044080_key_0(12);
    map_set_table_1074044080_key_0[0] = *(u8*)(hdr_1 + 16);
    map_set_table_1074044080_key_0[1] = *(u8*)(hdr_1 + 17);
    map_set_table_1074044080_key_0[2] = *(u8*)(hdr_1 + 18);
    map_set_table_1074044080_key_0[3] = *(u8*)(hdr_1 + 19);
    map_set_table_1074044080_key_0[4] = *(u8*)(hdr_1 + 12);
    map_set_table_1074044080_key_0[5] = *(u8*)(hdr_1 + 13);
    map_set_table_1074044080_key_0[6] = *(u8*)(hdr_1 + 14);
    map_set_table_1074044080_key_0[7] = *(u8*)(hdr_1 + 15);
    map_set_table_1074044080_key_0[8] = *(u8*)(hdr_2 + 2);
    map_set_table_1074044080_key_0[9] = *(u8*)(hdr_2 + 3);
    map_set_table_1074044080_key_0[10] = *(u8*)(hdr_2 + 0);
    map_set_table_1074044080_key_0[11] = *(u8*)(hdr_2 + 1);
    bool found_0 = state->map_set_table_1074044080.get(map_set_table_1074044080_key_0);
    // EP node  2365
    // BDD node 157:dchain_allocate_new_index(chain:(w64 1074076064), index_out:(w64 1074210936)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__157)], time:(ReadLSB w64 (w32 0) next_time))
    buffer_t map_set_table_1074044080_key_1(12);
    map_set_table_1074044080_key_1[0] = *(u8*)(hdr_1 + 12);
    map_set_table_1074044080_key_1[1] = *(u8*)(hdr_1 + 13);
    map_set_table_1074044080_key_1[2] = *(u8*)(hdr_1 + 14);
    map_set_table_1074044080_key_1[3] = *(u8*)(hdr_1 + 15);
    map_set_table_1074044080_key_1[4] = *(u8*)(hdr_1 + 16);
    map_set_table_1074044080_key_1[5] = *(u8*)(hdr_1 + 17);
    map_set_table_1074044080_key_1[6] = *(u8*)(hdr_1 + 18);
    map_set_table_1074044080_key_1[7] = *(u8*)(hdr_1 + 19);
    map_set_table_1074044080_key_1[8] = *(u8*)(hdr_2 + 0);
    map_set_table_1074044080_key_1[9] = *(u8*)(hdr_2 + 1);
    map_set_table_1074044080_key_1[10] = *(u8*)(hdr_2 + 2);
    map_set_table_1074044080_key_1[11] = *(u8*)(hdr_2 + 3);
    bool found_1 = state->map_set_table_1074044080.get(map_set_table_1074044080_key_1);
    // EP node  2366
    // BDD node 157:dchain_allocate_new_index(chain:(w64 1074076064), index_out:(w64 1074210936)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__157)], time:(ReadLSB w64 (w32 0) next_time))
    if ((0) == ((u32)value_0.get(0, 4))) {
      // EP node  2367
      // BDD node 157:dchain_allocate_new_index(chain:(w64 1074076064), index_out:(w64 1074210936)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__157)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  2369
      // BDD node 157:dchain_allocate_new_index(chain:(w64 1074076064), index_out:(w64 1074210936)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__157)], time:(ReadLSB w64 (w32 0) next_time))
      result.abort_transaction = true;
      cpu_hdr->trigger_dataplane_execution = 1;
      return result;
    } else {
      // EP node  2368
      // BDD node 157:dchain_allocate_new_index(chain:(w64 1074076064), index_out:(w64 1074210936)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__157)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  2370
      // BDD node 157:dchain_allocate_new_index(chain:(w64 1074076064), index_out:(w64 1074210936)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__157)], time:(ReadLSB w64 (w32 0) next_time))
      if ((0) == (found_1)) {
        // EP node  2371
        // BDD node 157:dchain_allocate_new_index(chain:(w64 1074076064), index_out:(w64 1074210936)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__157)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  3095
        // BDD node 157:dchain_allocate_new_index(chain:(w64 1074076064), index_out:(w64 1074210936)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__157)], time:(ReadLSB w64 (w32 0) next_time))
        buffer_t map_set_table_1074044080_key_2(12);
        map_set_table_1074044080_key_2[0] = *(u8*)(hdr_1 + 12);
        map_set_table_1074044080_key_2[1] = *(u8*)(hdr_1 + 13);
        map_set_table_1074044080_key_2[2] = *(u8*)(hdr_1 + 14);
        map_set_table_1074044080_key_2[3] = *(u8*)(hdr_1 + 15);
        map_set_table_1074044080_key_2[4] = *(u8*)(hdr_1 + 16);
        map_set_table_1074044080_key_2[5] = *(u8*)(hdr_1 + 17);
        map_set_table_1074044080_key_2[6] = *(u8*)(hdr_1 + 18);
        map_set_table_1074044080_key_2[7] = *(u8*)(hdr_1 + 19);
        map_set_table_1074044080_key_2[8] = *(u8*)(hdr_2 + 0);
        map_set_table_1074044080_key_2[9] = *(u8*)(hdr_2 + 1);
        map_set_table_1074044080_key_2[10] = *(u8*)(hdr_2 + 2);
        map_set_table_1074044080_key_2[11] = *(u8*)(hdr_2 + 3);
        bool success_0 = state->map_set_table_1074044080.put(map_set_table_1074044080_key_2);
        // EP node  3143
        // BDD node 158:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__157))
        if ((0) == (success_0)) {
          // EP node  3144
          // BDD node 158:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__157))
          // EP node  3194
          // BDD node 159:vector_borrow(vector:(w64 1074093704), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074211384)[ -> (w64 1074107600)])
          buffer_t value_1;
          state->vector_table_1074093704.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_1);
          // EP node  3299
          // BDD node 160:vector_return(vector:(w64 1074093704), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074107600)[(ReadLSB w16 (w32 0) vector_data__159)])
          // EP node  3793
          // BDD node 164:FORWARD
          cpu_hdr->egress_dev = bswap16((u16)value_1.get(0, 2));
        } else {
          // EP node  3145
          // BDD node 158:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__157))
          // EP node  3246
          // BDD node 168:vector_borrow(vector:(w64 1074093704), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074213544)[ -> (w64 1074107600)])
          buffer_t value_2;
          state->vector_table_1074093704.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_2);
          // EP node  3353
          // BDD node 169:vector_return(vector:(w64 1074093704), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074107600)[(ReadLSB w16 (w32 0) vector_data__168)])
          // EP node  3850
          // BDD node 173:FORWARD
          cpu_hdr->egress_dev = bswap16((u16)value_2.get(0, 2));
        }
      } else {
        // EP node  2372
        // BDD node 157:dchain_allocate_new_index(chain:(w64 1074076064), index_out:(w64 1074210936)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__157)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  2373
        // BDD node 157:dchain_allocate_new_index(chain:(w64 1074076064), index_out:(w64 1074210936)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__157)], time:(ReadLSB w64 (w32 0) next_time))
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      }
    }
  }


  if (trigger_update_ipv4_tcpudp_checksums) {
    update_ipv4_tcpudp_checksums(l3_hdr, l4_hdr);
  }

  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
