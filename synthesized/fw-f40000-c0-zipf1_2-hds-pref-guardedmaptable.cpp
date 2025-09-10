#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  GuardedMapTable guarded_map_table_1074044752;
  DchainTable dchain_table_1074076736;
  VectorTable vector_table_1074077160;
  VectorTable vector_table_1074094376;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      guarded_map_table_1074044752("guarded_map_table_1074044752",{"Ingress.guarded_map_table_1074044752_142","Ingress.guarded_map_table_1074044752_159",},"Ingress.guarded_map_table_1074044752_guard", 1000LL),
      dchain_table_1074076736("dchain_table_1074076736",{"Ingress.dchain_table_1074076736_148",}, 1000LL),
      vector_table_1074077160("vector_table_1074077160",{"Ingress.vector_table_1074077160_139",}),
      vector_table_1074094376("vector_table_1074094376",{"Ingress.vector_table_1074094376_187","Ingress.vector_table_1074094376_149","Ingress.vector_table_1074094376_277",})
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
  // BDD node 0:map_allocate(capacity:(w32 65536), key_size:(w32 13), map_out:(w64 1074044488)[(w64 0) -> (w64 1074044752)])
  // Module DataplaneGuardedMapTableAllocate
  // BDD node 2:dchain_allocate(index_range:(w32 65536), chain_out:(w64 1074044504)[ -> (w64 1074076736)])
  // Module DataplaneDchainTableAllocate
  // BDD node 3:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1074044512)[(w64 0) -> (w64 1074077160)])
  // Module DataplaneVectorTableAllocate
  // BDD node 4:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074044520)[(w64 0) -> (w64 1074094376)])
  // Module DataplaneVectorTableAllocate
  // BDD node 5:vector_borrow(vector:(w64 1074077160), index:(w32 0), val_out:(w64 1074044360)[ -> (w64 1074091056)])
  // Module Ignore
  // BDD node 6:vector_return(vector:(w64 1074077160), index:(w32 0), value:(w64 1074091056)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_0(4);
  vector_table_1074077160_value_0[0] = 0;
  vector_table_1074077160_value_0[1] = 0;
  vector_table_1074077160_value_0[2] = 0;
  vector_table_1074077160_value_0[3] = 1;
  state->vector_table_1074077160.write(0, vector_table_1074077160_value_0);
  // BDD node 7:vector_borrow(vector:(w64 1074094376), index:(w32 0), val_out:(w64 1074044424)[ -> (w64 1074108272)])
  // Module Ignore
  // BDD node 8:vector_return(vector:(w64 1074094376), index:(w32 0), value:(w64 1074108272)[(w16 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_0(2);
  vector_table_1074094376_value_0[0] = 0;
  vector_table_1074094376_value_0[1] = 1;
  state->vector_table_1074094376.write(0, vector_table_1074094376_value_0);
  // BDD node 9:vector_borrow(vector:(w64 1074077160), index:(w32 1), val_out:(w64 1074044360)[ -> (w64 1074091080)])
  // Module Ignore
  // BDD node 10:vector_return(vector:(w64 1074077160), index:(w32 1), value:(w64 1074091080)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_1(4);
  vector_table_1074077160_value_1[0] = 0;
  vector_table_1074077160_value_1[1] = 0;
  vector_table_1074077160_value_1[2] = 0;
  vector_table_1074077160_value_1[3] = 0;
  state->vector_table_1074077160.write(1, vector_table_1074077160_value_1);
  // BDD node 11:vector_borrow(vector:(w64 1074094376), index:(w32 1), val_out:(w64 1074044424)[ -> (w64 1074108296)])
  // Module Ignore
  // BDD node 12:vector_return(vector:(w64 1074094376), index:(w32 1), value:(w64 1074108296)[(w16 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_1(2);
  vector_table_1074094376_value_1[0] = 0;
  vector_table_1074094376_value_1[1] = 0;
  state->vector_table_1074094376.write(1, vector_table_1074094376_value_1);
  // BDD node 13:vector_borrow(vector:(w64 1074077160), index:(w32 2), val_out:(w64 1074044360)[ -> (w64 1074091104)])
  // Module Ignore
  // BDD node 14:vector_return(vector:(w64 1074077160), index:(w32 2), value:(w64 1074091104)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_2(4);
  vector_table_1074077160_value_2[0] = 0;
  vector_table_1074077160_value_2[1] = 0;
  vector_table_1074077160_value_2[2] = 0;
  vector_table_1074077160_value_2[3] = 1;
  state->vector_table_1074077160.write(2, vector_table_1074077160_value_2);
  // BDD node 15:vector_borrow(vector:(w64 1074094376), index:(w32 2), val_out:(w64 1074044424)[ -> (w64 1074108320)])
  // Module Ignore
  // BDD node 16:vector_return(vector:(w64 1074094376), index:(w32 2), value:(w64 1074108320)[(w16 3)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_2(2);
  vector_table_1074094376_value_2[0] = 0;
  vector_table_1074094376_value_2[1] = 3;
  state->vector_table_1074094376.write(2, vector_table_1074094376_value_2);
  // BDD node 17:vector_borrow(vector:(w64 1074077160), index:(w32 3), val_out:(w64 1074044360)[ -> (w64 1074091128)])
  // Module Ignore
  // BDD node 18:vector_return(vector:(w64 1074077160), index:(w32 3), value:(w64 1074091128)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_3(4);
  vector_table_1074077160_value_3[0] = 0;
  vector_table_1074077160_value_3[1] = 0;
  vector_table_1074077160_value_3[2] = 0;
  vector_table_1074077160_value_3[3] = 0;
  state->vector_table_1074077160.write(3, vector_table_1074077160_value_3);
  // BDD node 19:vector_borrow(vector:(w64 1074094376), index:(w32 3), val_out:(w64 1074044424)[ -> (w64 1074108344)])
  // Module Ignore
  // BDD node 20:vector_return(vector:(w64 1074094376), index:(w32 3), value:(w64 1074108344)[(w16 2)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_3(2);
  vector_table_1074094376_value_3[0] = 0;
  vector_table_1074094376_value_3[1] = 2;
  state->vector_table_1074094376.write(3, vector_table_1074094376_value_3);
  // BDD node 21:vector_borrow(vector:(w64 1074077160), index:(w32 4), val_out:(w64 1074044360)[ -> (w64 1074091152)])
  // Module Ignore
  // BDD node 22:vector_return(vector:(w64 1074077160), index:(w32 4), value:(w64 1074091152)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_4(4);
  vector_table_1074077160_value_4[0] = 0;
  vector_table_1074077160_value_4[1] = 0;
  vector_table_1074077160_value_4[2] = 0;
  vector_table_1074077160_value_4[3] = 1;
  state->vector_table_1074077160.write(4, vector_table_1074077160_value_4);
  // BDD node 23:vector_borrow(vector:(w64 1074094376), index:(w32 4), val_out:(w64 1074044424)[ -> (w64 1074108368)])
  // Module Ignore
  // BDD node 24:vector_return(vector:(w64 1074094376), index:(w32 4), value:(w64 1074108368)[(w16 5)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_4(2);
  vector_table_1074094376_value_4[0] = 0;
  vector_table_1074094376_value_4[1] = 5;
  state->vector_table_1074094376.write(4, vector_table_1074094376_value_4);
  // BDD node 25:vector_borrow(vector:(w64 1074077160), index:(w32 5), val_out:(w64 1074044360)[ -> (w64 1074091176)])
  // Module Ignore
  // BDD node 26:vector_return(vector:(w64 1074077160), index:(w32 5), value:(w64 1074091176)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_5(4);
  vector_table_1074077160_value_5[0] = 0;
  vector_table_1074077160_value_5[1] = 0;
  vector_table_1074077160_value_5[2] = 0;
  vector_table_1074077160_value_5[3] = 0;
  state->vector_table_1074077160.write(5, vector_table_1074077160_value_5);
  // BDD node 27:vector_borrow(vector:(w64 1074094376), index:(w32 5), val_out:(w64 1074044424)[ -> (w64 1074108392)])
  // Module Ignore
  // BDD node 28:vector_return(vector:(w64 1074094376), index:(w32 5), value:(w64 1074108392)[(w16 4)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_5(2);
  vector_table_1074094376_value_5[0] = 0;
  vector_table_1074094376_value_5[1] = 4;
  state->vector_table_1074094376.write(5, vector_table_1074094376_value_5);
  // BDD node 29:vector_borrow(vector:(w64 1074077160), index:(w32 6), val_out:(w64 1074044360)[ -> (w64 1074091200)])
  // Module Ignore
  // BDD node 30:vector_return(vector:(w64 1074077160), index:(w32 6), value:(w64 1074091200)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_6(4);
  vector_table_1074077160_value_6[0] = 0;
  vector_table_1074077160_value_6[1] = 0;
  vector_table_1074077160_value_6[2] = 0;
  vector_table_1074077160_value_6[3] = 1;
  state->vector_table_1074077160.write(6, vector_table_1074077160_value_6);
  // BDD node 31:vector_borrow(vector:(w64 1074094376), index:(w32 6), val_out:(w64 1074044424)[ -> (w64 1074108416)])
  // Module Ignore
  // BDD node 32:vector_return(vector:(w64 1074094376), index:(w32 6), value:(w64 1074108416)[(w16 7)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_6(2);
  vector_table_1074094376_value_6[0] = 0;
  vector_table_1074094376_value_6[1] = 7;
  state->vector_table_1074094376.write(6, vector_table_1074094376_value_6);
  // BDD node 33:vector_borrow(vector:(w64 1074077160), index:(w32 7), val_out:(w64 1074044360)[ -> (w64 1074091224)])
  // Module Ignore
  // BDD node 34:vector_return(vector:(w64 1074077160), index:(w32 7), value:(w64 1074091224)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_7(4);
  vector_table_1074077160_value_7[0] = 0;
  vector_table_1074077160_value_7[1] = 0;
  vector_table_1074077160_value_7[2] = 0;
  vector_table_1074077160_value_7[3] = 0;
  state->vector_table_1074077160.write(7, vector_table_1074077160_value_7);
  // BDD node 35:vector_borrow(vector:(w64 1074094376), index:(w32 7), val_out:(w64 1074044424)[ -> (w64 1074108440)])
  // Module Ignore
  // BDD node 36:vector_return(vector:(w64 1074094376), index:(w32 7), value:(w64 1074108440)[(w16 6)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_7(2);
  vector_table_1074094376_value_7[0] = 0;
  vector_table_1074094376_value_7[1] = 6;
  state->vector_table_1074094376.write(7, vector_table_1074094376_value_7);
  // BDD node 37:vector_borrow(vector:(w64 1074077160), index:(w32 8), val_out:(w64 1074044360)[ -> (w64 1074091248)])
  // Module Ignore
  // BDD node 38:vector_return(vector:(w64 1074077160), index:(w32 8), value:(w64 1074091248)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_8(4);
  vector_table_1074077160_value_8[0] = 0;
  vector_table_1074077160_value_8[1] = 0;
  vector_table_1074077160_value_8[2] = 0;
  vector_table_1074077160_value_8[3] = 1;
  state->vector_table_1074077160.write(8, vector_table_1074077160_value_8);
  // BDD node 39:vector_borrow(vector:(w64 1074094376), index:(w32 8), val_out:(w64 1074044424)[ -> (w64 1074108464)])
  // Module Ignore
  // BDD node 40:vector_return(vector:(w64 1074094376), index:(w32 8), value:(w64 1074108464)[(w16 9)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_8(2);
  vector_table_1074094376_value_8[0] = 0;
  vector_table_1074094376_value_8[1] = 9;
  state->vector_table_1074094376.write(8, vector_table_1074094376_value_8);
  // BDD node 41:vector_borrow(vector:(w64 1074077160), index:(w32 9), val_out:(w64 1074044360)[ -> (w64 1074091272)])
  // Module Ignore
  // BDD node 42:vector_return(vector:(w64 1074077160), index:(w32 9), value:(w64 1074091272)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_9(4);
  vector_table_1074077160_value_9[0] = 0;
  vector_table_1074077160_value_9[1] = 0;
  vector_table_1074077160_value_9[2] = 0;
  vector_table_1074077160_value_9[3] = 0;
  state->vector_table_1074077160.write(9, vector_table_1074077160_value_9);
  // BDD node 43:vector_borrow(vector:(w64 1074094376), index:(w32 9), val_out:(w64 1074044424)[ -> (w64 1074108488)])
  // Module Ignore
  // BDD node 44:vector_return(vector:(w64 1074094376), index:(w32 9), value:(w64 1074108488)[(w16 8)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_9(2);
  vector_table_1074094376_value_9[0] = 0;
  vector_table_1074094376_value_9[1] = 8;
  state->vector_table_1074094376.write(9, vector_table_1074094376_value_9);
  // BDD node 45:vector_borrow(vector:(w64 1074077160), index:(w32 10), val_out:(w64 1074044360)[ -> (w64 1074091296)])
  // Module Ignore
  // BDD node 46:vector_return(vector:(w64 1074077160), index:(w32 10), value:(w64 1074091296)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_10(4);
  vector_table_1074077160_value_10[0] = 0;
  vector_table_1074077160_value_10[1] = 0;
  vector_table_1074077160_value_10[2] = 0;
  vector_table_1074077160_value_10[3] = 1;
  state->vector_table_1074077160.write(10, vector_table_1074077160_value_10);
  // BDD node 47:vector_borrow(vector:(w64 1074094376), index:(w32 10), val_out:(w64 1074044424)[ -> (w64 1074108512)])
  // Module Ignore
  // BDD node 48:vector_return(vector:(w64 1074094376), index:(w32 10), value:(w64 1074108512)[(w16 11)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_10(2);
  vector_table_1074094376_value_10[0] = 0;
  vector_table_1074094376_value_10[1] = 11;
  state->vector_table_1074094376.write(10, vector_table_1074094376_value_10);
  // BDD node 49:vector_borrow(vector:(w64 1074077160), index:(w32 11), val_out:(w64 1074044360)[ -> (w64 1074091320)])
  // Module Ignore
  // BDD node 50:vector_return(vector:(w64 1074077160), index:(w32 11), value:(w64 1074091320)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_11(4);
  vector_table_1074077160_value_11[0] = 0;
  vector_table_1074077160_value_11[1] = 0;
  vector_table_1074077160_value_11[2] = 0;
  vector_table_1074077160_value_11[3] = 0;
  state->vector_table_1074077160.write(11, vector_table_1074077160_value_11);
  // BDD node 51:vector_borrow(vector:(w64 1074094376), index:(w32 11), val_out:(w64 1074044424)[ -> (w64 1074108536)])
  // Module Ignore
  // BDD node 52:vector_return(vector:(w64 1074094376), index:(w32 11), value:(w64 1074108536)[(w16 10)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_11(2);
  vector_table_1074094376_value_11[0] = 0;
  vector_table_1074094376_value_11[1] = 10;
  state->vector_table_1074094376.write(11, vector_table_1074094376_value_11);
  // BDD node 53:vector_borrow(vector:(w64 1074077160), index:(w32 12), val_out:(w64 1074044360)[ -> (w64 1074091344)])
  // Module Ignore
  // BDD node 54:vector_return(vector:(w64 1074077160), index:(w32 12), value:(w64 1074091344)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_12(4);
  vector_table_1074077160_value_12[0] = 0;
  vector_table_1074077160_value_12[1] = 0;
  vector_table_1074077160_value_12[2] = 0;
  vector_table_1074077160_value_12[3] = 1;
  state->vector_table_1074077160.write(12, vector_table_1074077160_value_12);
  // BDD node 55:vector_borrow(vector:(w64 1074094376), index:(w32 12), val_out:(w64 1074044424)[ -> (w64 1074108560)])
  // Module Ignore
  // BDD node 56:vector_return(vector:(w64 1074094376), index:(w32 12), value:(w64 1074108560)[(w16 13)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_12(2);
  vector_table_1074094376_value_12[0] = 0;
  vector_table_1074094376_value_12[1] = 13;
  state->vector_table_1074094376.write(12, vector_table_1074094376_value_12);
  // BDD node 57:vector_borrow(vector:(w64 1074077160), index:(w32 13), val_out:(w64 1074044360)[ -> (w64 1074091368)])
  // Module Ignore
  // BDD node 58:vector_return(vector:(w64 1074077160), index:(w32 13), value:(w64 1074091368)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_13(4);
  vector_table_1074077160_value_13[0] = 0;
  vector_table_1074077160_value_13[1] = 0;
  vector_table_1074077160_value_13[2] = 0;
  vector_table_1074077160_value_13[3] = 0;
  state->vector_table_1074077160.write(13, vector_table_1074077160_value_13);
  // BDD node 59:vector_borrow(vector:(w64 1074094376), index:(w32 13), val_out:(w64 1074044424)[ -> (w64 1074108584)])
  // Module Ignore
  // BDD node 60:vector_return(vector:(w64 1074094376), index:(w32 13), value:(w64 1074108584)[(w16 12)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_13(2);
  vector_table_1074094376_value_13[0] = 0;
  vector_table_1074094376_value_13[1] = 12;
  state->vector_table_1074094376.write(13, vector_table_1074094376_value_13);
  // BDD node 61:vector_borrow(vector:(w64 1074077160), index:(w32 14), val_out:(w64 1074044360)[ -> (w64 1074091392)])
  // Module Ignore
  // BDD node 62:vector_return(vector:(w64 1074077160), index:(w32 14), value:(w64 1074091392)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_14(4);
  vector_table_1074077160_value_14[0] = 0;
  vector_table_1074077160_value_14[1] = 0;
  vector_table_1074077160_value_14[2] = 0;
  vector_table_1074077160_value_14[3] = 1;
  state->vector_table_1074077160.write(14, vector_table_1074077160_value_14);
  // BDD node 63:vector_borrow(vector:(w64 1074094376), index:(w32 14), val_out:(w64 1074044424)[ -> (w64 1074108608)])
  // Module Ignore
  // BDD node 64:vector_return(vector:(w64 1074094376), index:(w32 14), value:(w64 1074108608)[(w16 15)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_14(2);
  vector_table_1074094376_value_14[0] = 0;
  vector_table_1074094376_value_14[1] = 15;
  state->vector_table_1074094376.write(14, vector_table_1074094376_value_14);
  // BDD node 65:vector_borrow(vector:(w64 1074077160), index:(w32 15), val_out:(w64 1074044360)[ -> (w64 1074091416)])
  // Module Ignore
  // BDD node 66:vector_return(vector:(w64 1074077160), index:(w32 15), value:(w64 1074091416)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_15(4);
  vector_table_1074077160_value_15[0] = 0;
  vector_table_1074077160_value_15[1] = 0;
  vector_table_1074077160_value_15[2] = 0;
  vector_table_1074077160_value_15[3] = 0;
  state->vector_table_1074077160.write(15, vector_table_1074077160_value_15);
  // BDD node 67:vector_borrow(vector:(w64 1074094376), index:(w32 15), val_out:(w64 1074044424)[ -> (w64 1074108632)])
  // Module Ignore
  // BDD node 68:vector_return(vector:(w64 1074094376), index:(w32 15), value:(w64 1074108632)[(w16 14)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_15(2);
  vector_table_1074094376_value_15[0] = 0;
  vector_table_1074094376_value_15[1] = 14;
  state->vector_table_1074094376.write(15, vector_table_1074094376_value_15);
  // BDD node 69:vector_borrow(vector:(w64 1074077160), index:(w32 16), val_out:(w64 1074044360)[ -> (w64 1074091440)])
  // Module Ignore
  // BDD node 70:vector_return(vector:(w64 1074077160), index:(w32 16), value:(w64 1074091440)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_16(4);
  vector_table_1074077160_value_16[0] = 0;
  vector_table_1074077160_value_16[1] = 0;
  vector_table_1074077160_value_16[2] = 0;
  vector_table_1074077160_value_16[3] = 1;
  state->vector_table_1074077160.write(16, vector_table_1074077160_value_16);
  // BDD node 71:vector_borrow(vector:(w64 1074094376), index:(w32 16), val_out:(w64 1074044424)[ -> (w64 1074108656)])
  // Module Ignore
  // BDD node 72:vector_return(vector:(w64 1074094376), index:(w32 16), value:(w64 1074108656)[(w16 17)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_16(2);
  vector_table_1074094376_value_16[0] = 0;
  vector_table_1074094376_value_16[1] = 17;
  state->vector_table_1074094376.write(16, vector_table_1074094376_value_16);
  // BDD node 73:vector_borrow(vector:(w64 1074077160), index:(w32 17), val_out:(w64 1074044360)[ -> (w64 1074091464)])
  // Module Ignore
  // BDD node 74:vector_return(vector:(w64 1074077160), index:(w32 17), value:(w64 1074091464)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_17(4);
  vector_table_1074077160_value_17[0] = 0;
  vector_table_1074077160_value_17[1] = 0;
  vector_table_1074077160_value_17[2] = 0;
  vector_table_1074077160_value_17[3] = 0;
  state->vector_table_1074077160.write(17, vector_table_1074077160_value_17);
  // BDD node 75:vector_borrow(vector:(w64 1074094376), index:(w32 17), val_out:(w64 1074044424)[ -> (w64 1074108680)])
  // Module Ignore
  // BDD node 76:vector_return(vector:(w64 1074094376), index:(w32 17), value:(w64 1074108680)[(w16 16)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_17(2);
  vector_table_1074094376_value_17[0] = 0;
  vector_table_1074094376_value_17[1] = 16;
  state->vector_table_1074094376.write(17, vector_table_1074094376_value_17);
  // BDD node 77:vector_borrow(vector:(w64 1074077160), index:(w32 18), val_out:(w64 1074044360)[ -> (w64 1074091488)])
  // Module Ignore
  // BDD node 78:vector_return(vector:(w64 1074077160), index:(w32 18), value:(w64 1074091488)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_18(4);
  vector_table_1074077160_value_18[0] = 0;
  vector_table_1074077160_value_18[1] = 0;
  vector_table_1074077160_value_18[2] = 0;
  vector_table_1074077160_value_18[3] = 1;
  state->vector_table_1074077160.write(18, vector_table_1074077160_value_18);
  // BDD node 79:vector_borrow(vector:(w64 1074094376), index:(w32 18), val_out:(w64 1074044424)[ -> (w64 1074108704)])
  // Module Ignore
  // BDD node 80:vector_return(vector:(w64 1074094376), index:(w32 18), value:(w64 1074108704)[(w16 19)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_18(2);
  vector_table_1074094376_value_18[0] = 0;
  vector_table_1074094376_value_18[1] = 19;
  state->vector_table_1074094376.write(18, vector_table_1074094376_value_18);
  // BDD node 81:vector_borrow(vector:(w64 1074077160), index:(w32 19), val_out:(w64 1074044360)[ -> (w64 1074091512)])
  // Module Ignore
  // BDD node 82:vector_return(vector:(w64 1074077160), index:(w32 19), value:(w64 1074091512)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_19(4);
  vector_table_1074077160_value_19[0] = 0;
  vector_table_1074077160_value_19[1] = 0;
  vector_table_1074077160_value_19[2] = 0;
  vector_table_1074077160_value_19[3] = 0;
  state->vector_table_1074077160.write(19, vector_table_1074077160_value_19);
  // BDD node 83:vector_borrow(vector:(w64 1074094376), index:(w32 19), val_out:(w64 1074044424)[ -> (w64 1074108728)])
  // Module Ignore
  // BDD node 84:vector_return(vector:(w64 1074094376), index:(w32 19), value:(w64 1074108728)[(w16 18)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_19(2);
  vector_table_1074094376_value_19[0] = 0;
  vector_table_1074094376_value_19[1] = 18;
  state->vector_table_1074094376.write(19, vector_table_1074094376_value_19);
  // BDD node 85:vector_borrow(vector:(w64 1074077160), index:(w32 20), val_out:(w64 1074044360)[ -> (w64 1074091536)])
  // Module Ignore
  // BDD node 86:vector_return(vector:(w64 1074077160), index:(w32 20), value:(w64 1074091536)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_20(4);
  vector_table_1074077160_value_20[0] = 0;
  vector_table_1074077160_value_20[1] = 0;
  vector_table_1074077160_value_20[2] = 0;
  vector_table_1074077160_value_20[3] = 1;
  state->vector_table_1074077160.write(20, vector_table_1074077160_value_20);
  // BDD node 87:vector_borrow(vector:(w64 1074094376), index:(w32 20), val_out:(w64 1074044424)[ -> (w64 1074108752)])
  // Module Ignore
  // BDD node 88:vector_return(vector:(w64 1074094376), index:(w32 20), value:(w64 1074108752)[(w16 21)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_20(2);
  vector_table_1074094376_value_20[0] = 0;
  vector_table_1074094376_value_20[1] = 21;
  state->vector_table_1074094376.write(20, vector_table_1074094376_value_20);
  // BDD node 89:vector_borrow(vector:(w64 1074077160), index:(w32 21), val_out:(w64 1074044360)[ -> (w64 1074091560)])
  // Module Ignore
  // BDD node 90:vector_return(vector:(w64 1074077160), index:(w32 21), value:(w64 1074091560)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_21(4);
  vector_table_1074077160_value_21[0] = 0;
  vector_table_1074077160_value_21[1] = 0;
  vector_table_1074077160_value_21[2] = 0;
  vector_table_1074077160_value_21[3] = 0;
  state->vector_table_1074077160.write(21, vector_table_1074077160_value_21);
  // BDD node 91:vector_borrow(vector:(w64 1074094376), index:(w32 21), val_out:(w64 1074044424)[ -> (w64 1074108776)])
  // Module Ignore
  // BDD node 92:vector_return(vector:(w64 1074094376), index:(w32 21), value:(w64 1074108776)[(w16 20)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_21(2);
  vector_table_1074094376_value_21[0] = 0;
  vector_table_1074094376_value_21[1] = 20;
  state->vector_table_1074094376.write(21, vector_table_1074094376_value_21);
  // BDD node 93:vector_borrow(vector:(w64 1074077160), index:(w32 22), val_out:(w64 1074044360)[ -> (w64 1074091584)])
  // Module Ignore
  // BDD node 94:vector_return(vector:(w64 1074077160), index:(w32 22), value:(w64 1074091584)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_22(4);
  vector_table_1074077160_value_22[0] = 0;
  vector_table_1074077160_value_22[1] = 0;
  vector_table_1074077160_value_22[2] = 0;
  vector_table_1074077160_value_22[3] = 1;
  state->vector_table_1074077160.write(22, vector_table_1074077160_value_22);
  // BDD node 95:vector_borrow(vector:(w64 1074094376), index:(w32 22), val_out:(w64 1074044424)[ -> (w64 1074108800)])
  // Module Ignore
  // BDD node 96:vector_return(vector:(w64 1074094376), index:(w32 22), value:(w64 1074108800)[(w16 23)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_22(2);
  vector_table_1074094376_value_22[0] = 0;
  vector_table_1074094376_value_22[1] = 23;
  state->vector_table_1074094376.write(22, vector_table_1074094376_value_22);
  // BDD node 97:vector_borrow(vector:(w64 1074077160), index:(w32 23), val_out:(w64 1074044360)[ -> (w64 1074091608)])
  // Module Ignore
  // BDD node 98:vector_return(vector:(w64 1074077160), index:(w32 23), value:(w64 1074091608)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_23(4);
  vector_table_1074077160_value_23[0] = 0;
  vector_table_1074077160_value_23[1] = 0;
  vector_table_1074077160_value_23[2] = 0;
  vector_table_1074077160_value_23[3] = 0;
  state->vector_table_1074077160.write(23, vector_table_1074077160_value_23);
  // BDD node 99:vector_borrow(vector:(w64 1074094376), index:(w32 23), val_out:(w64 1074044424)[ -> (w64 1074108824)])
  // Module Ignore
  // BDD node 100:vector_return(vector:(w64 1074094376), index:(w32 23), value:(w64 1074108824)[(w16 22)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_23(2);
  vector_table_1074094376_value_23[0] = 0;
  vector_table_1074094376_value_23[1] = 22;
  state->vector_table_1074094376.write(23, vector_table_1074094376_value_23);
  // BDD node 101:vector_borrow(vector:(w64 1074077160), index:(w32 24), val_out:(w64 1074044360)[ -> (w64 1074091632)])
  // Module Ignore
  // BDD node 102:vector_return(vector:(w64 1074077160), index:(w32 24), value:(w64 1074091632)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_24(4);
  vector_table_1074077160_value_24[0] = 0;
  vector_table_1074077160_value_24[1] = 0;
  vector_table_1074077160_value_24[2] = 0;
  vector_table_1074077160_value_24[3] = 1;
  state->vector_table_1074077160.write(24, vector_table_1074077160_value_24);
  // BDD node 103:vector_borrow(vector:(w64 1074094376), index:(w32 24), val_out:(w64 1074044424)[ -> (w64 1074108848)])
  // Module Ignore
  // BDD node 104:vector_return(vector:(w64 1074094376), index:(w32 24), value:(w64 1074108848)[(w16 25)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_24(2);
  vector_table_1074094376_value_24[0] = 0;
  vector_table_1074094376_value_24[1] = 25;
  state->vector_table_1074094376.write(24, vector_table_1074094376_value_24);
  // BDD node 105:vector_borrow(vector:(w64 1074077160), index:(w32 25), val_out:(w64 1074044360)[ -> (w64 1074091656)])
  // Module Ignore
  // BDD node 106:vector_return(vector:(w64 1074077160), index:(w32 25), value:(w64 1074091656)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_25(4);
  vector_table_1074077160_value_25[0] = 0;
  vector_table_1074077160_value_25[1] = 0;
  vector_table_1074077160_value_25[2] = 0;
  vector_table_1074077160_value_25[3] = 0;
  state->vector_table_1074077160.write(25, vector_table_1074077160_value_25);
  // BDD node 107:vector_borrow(vector:(w64 1074094376), index:(w32 25), val_out:(w64 1074044424)[ -> (w64 1074108872)])
  // Module Ignore
  // BDD node 108:vector_return(vector:(w64 1074094376), index:(w32 25), value:(w64 1074108872)[(w16 24)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_25(2);
  vector_table_1074094376_value_25[0] = 0;
  vector_table_1074094376_value_25[1] = 24;
  state->vector_table_1074094376.write(25, vector_table_1074094376_value_25);
  // BDD node 109:vector_borrow(vector:(w64 1074077160), index:(w32 26), val_out:(w64 1074044360)[ -> (w64 1074091680)])
  // Module Ignore
  // BDD node 110:vector_return(vector:(w64 1074077160), index:(w32 26), value:(w64 1074091680)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_26(4);
  vector_table_1074077160_value_26[0] = 0;
  vector_table_1074077160_value_26[1] = 0;
  vector_table_1074077160_value_26[2] = 0;
  vector_table_1074077160_value_26[3] = 1;
  state->vector_table_1074077160.write(26, vector_table_1074077160_value_26);
  // BDD node 111:vector_borrow(vector:(w64 1074094376), index:(w32 26), val_out:(w64 1074044424)[ -> (w64 1074108896)])
  // Module Ignore
  // BDD node 112:vector_return(vector:(w64 1074094376), index:(w32 26), value:(w64 1074108896)[(w16 27)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_26(2);
  vector_table_1074094376_value_26[0] = 0;
  vector_table_1074094376_value_26[1] = 27;
  state->vector_table_1074094376.write(26, vector_table_1074094376_value_26);
  // BDD node 113:vector_borrow(vector:(w64 1074077160), index:(w32 27), val_out:(w64 1074044360)[ -> (w64 1074091704)])
  // Module Ignore
  // BDD node 114:vector_return(vector:(w64 1074077160), index:(w32 27), value:(w64 1074091704)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_27(4);
  vector_table_1074077160_value_27[0] = 0;
  vector_table_1074077160_value_27[1] = 0;
  vector_table_1074077160_value_27[2] = 0;
  vector_table_1074077160_value_27[3] = 0;
  state->vector_table_1074077160.write(27, vector_table_1074077160_value_27);
  // BDD node 115:vector_borrow(vector:(w64 1074094376), index:(w32 27), val_out:(w64 1074044424)[ -> (w64 1074108920)])
  // Module Ignore
  // BDD node 116:vector_return(vector:(w64 1074094376), index:(w32 27), value:(w64 1074108920)[(w16 26)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_27(2);
  vector_table_1074094376_value_27[0] = 0;
  vector_table_1074094376_value_27[1] = 26;
  state->vector_table_1074094376.write(27, vector_table_1074094376_value_27);
  // BDD node 117:vector_borrow(vector:(w64 1074077160), index:(w32 28), val_out:(w64 1074044360)[ -> (w64 1074091728)])
  // Module Ignore
  // BDD node 118:vector_return(vector:(w64 1074077160), index:(w32 28), value:(w64 1074091728)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_28(4);
  vector_table_1074077160_value_28[0] = 0;
  vector_table_1074077160_value_28[1] = 0;
  vector_table_1074077160_value_28[2] = 0;
  vector_table_1074077160_value_28[3] = 1;
  state->vector_table_1074077160.write(28, vector_table_1074077160_value_28);
  // BDD node 119:vector_borrow(vector:(w64 1074094376), index:(w32 28), val_out:(w64 1074044424)[ -> (w64 1074108944)])
  // Module Ignore
  // BDD node 120:vector_return(vector:(w64 1074094376), index:(w32 28), value:(w64 1074108944)[(w16 29)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_28(2);
  vector_table_1074094376_value_28[0] = 0;
  vector_table_1074094376_value_28[1] = 29;
  state->vector_table_1074094376.write(28, vector_table_1074094376_value_28);
  // BDD node 121:vector_borrow(vector:(w64 1074077160), index:(w32 29), val_out:(w64 1074044360)[ -> (w64 1074091752)])
  // Module Ignore
  // BDD node 122:vector_return(vector:(w64 1074077160), index:(w32 29), value:(w64 1074091752)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_29(4);
  vector_table_1074077160_value_29[0] = 0;
  vector_table_1074077160_value_29[1] = 0;
  vector_table_1074077160_value_29[2] = 0;
  vector_table_1074077160_value_29[3] = 0;
  state->vector_table_1074077160.write(29, vector_table_1074077160_value_29);
  // BDD node 123:vector_borrow(vector:(w64 1074094376), index:(w32 29), val_out:(w64 1074044424)[ -> (w64 1074108968)])
  // Module Ignore
  // BDD node 124:vector_return(vector:(w64 1074094376), index:(w32 29), value:(w64 1074108968)[(w16 28)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_29(2);
  vector_table_1074094376_value_29[0] = 0;
  vector_table_1074094376_value_29[1] = 28;
  state->vector_table_1074094376.write(29, vector_table_1074094376_value_29);
  // BDD node 125:vector_borrow(vector:(w64 1074077160), index:(w32 30), val_out:(w64 1074044360)[ -> (w64 1074091776)])
  // Module Ignore
  // BDD node 126:vector_return(vector:(w64 1074077160), index:(w32 30), value:(w64 1074091776)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_30(4);
  vector_table_1074077160_value_30[0] = 0;
  vector_table_1074077160_value_30[1] = 0;
  vector_table_1074077160_value_30[2] = 0;
  vector_table_1074077160_value_30[3] = 1;
  state->vector_table_1074077160.write(30, vector_table_1074077160_value_30);
  // BDD node 127:vector_borrow(vector:(w64 1074094376), index:(w32 30), val_out:(w64 1074044424)[ -> (w64 1074108992)])
  // Module Ignore
  // BDD node 128:vector_return(vector:(w64 1074094376), index:(w32 30), value:(w64 1074108992)[(w16 31)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_30(2);
  vector_table_1074094376_value_30[0] = 0;
  vector_table_1074094376_value_30[1] = 31;
  state->vector_table_1074094376.write(30, vector_table_1074094376_value_30);
  // BDD node 129:vector_borrow(vector:(w64 1074077160), index:(w32 31), val_out:(w64 1074044360)[ -> (w64 1074091800)])
  // Module Ignore
  // BDD node 130:vector_return(vector:(w64 1074077160), index:(w32 31), value:(w64 1074091800)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074077160_value_31(4);
  vector_table_1074077160_value_31[0] = 0;
  vector_table_1074077160_value_31[1] = 0;
  vector_table_1074077160_value_31[2] = 0;
  vector_table_1074077160_value_31[3] = 0;
  state->vector_table_1074077160.write(31, vector_table_1074077160_value_31);
  // BDD node 131:vector_borrow(vector:(w64 1074094376), index:(w32 31), val_out:(w64 1074044424)[ -> (w64 1074109016)])
  // Module Ignore
  // BDD node 132:vector_return(vector:(w64 1074094376), index:(w32 31), value:(w64 1074109016)[(w16 30)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074094376_value_31(2);
  vector_table_1074094376_value_31[0] = 0;
  vector_table_1074094376_value_31[1] = 30;
  state->vector_table_1074094376.write(31, vector_table_1074094376_value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 vector_data_r1;
  u32 map_has_this_key;
  u32 allocated_index;
  u32 DEVICE;
  u64 next_time;

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

  if (bswap16(cpu_hdr->code_path) == 6929) {
    // EP node  6911
    // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  6912
    // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  6913
    // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_2 = packet_consume(pkt, 4);
    // EP node  6914
    // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    buffer_t value_0;
    state->vector_table_1074077160.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_0);
    // EP node  6915
    // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    if ((0) == ((u32)value_0.get(0, 4))) {
      // EP node  6916
      // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  6918
      // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      result.abort_transaction = true;
      cpu_hdr->trigger_dataplane_execution = 1;
      return result;
    } else {
      // EP node  6917
      // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  6919
      // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      buffer_t guarded_map_table_1074044752_key_0(13);
      guarded_map_table_1074044752_key_0[0] = *(u8*)(hdr_2 + 0);
      guarded_map_table_1074044752_key_0[1] = *(u8*)(hdr_2 + 1);
      guarded_map_table_1074044752_key_0[2] = *(u8*)(hdr_2 + 2);
      guarded_map_table_1074044752_key_0[3] = *(u8*)(hdr_2 + 3);
      guarded_map_table_1074044752_key_0[4] = *(u8*)(hdr_1 + 12);
      guarded_map_table_1074044752_key_0[5] = *(u8*)(hdr_1 + 13);
      guarded_map_table_1074044752_key_0[6] = *(u8*)(hdr_1 + 14);
      guarded_map_table_1074044752_key_0[7] = *(u8*)(hdr_1 + 15);
      guarded_map_table_1074044752_key_0[8] = *(u8*)(hdr_1 + 16);
      guarded_map_table_1074044752_key_0[9] = *(u8*)(hdr_1 + 17);
      guarded_map_table_1074044752_key_0[10] = *(u8*)(hdr_1 + 18);
      guarded_map_table_1074044752_key_0[11] = *(u8*)(hdr_1 + 19);
      guarded_map_table_1074044752_key_0[12] = *(u8*)(hdr_1 + 9);
      u32 value_1;
      bool found_0 = state->guarded_map_table_1074044752.get(guarded_map_table_1074044752_key_0, value_1);
      // EP node  6920
      // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      if ((0) == (found_0)) {
        // EP node  6921
        // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  6924
        // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        bool guard_allow_0 = state->guarded_map_table_1074044752.guard_check();
        // EP node  6925
        // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        if ((guard_allow_0) != (0)) {
          // EP node  6926
          // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
          // EP node  11137
          // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
          u32 allocated_index_0;
          bool success_0 = state->dchain_table_1074076736.allocate_new_index(allocated_index_0);
          // EP node  11224
          // BDD node 162:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
          if ((0) == (success_0)) {
            // EP node  11225
            // BDD node 162:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
            // EP node  11314
            // BDD node 163:vector_borrow(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074218536)[ -> (w64 1074108272)])
            buffer_t value_2;
            state->vector_table_1074094376.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_2);
            // EP node  11680
            // BDD node 164:vector_return(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074108272)[(ReadLSB w16 (w32 0) vector_data_640)])
            // EP node  12339
            // BDD node 168:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_640)))
            if ((bswap32(cpu_hdr_extra->DEVICE) & 65535) != ((u16)value_2.get(0, 2))) {
              // EP node  12340
              // BDD node 168:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_640)))
              // EP node  12731
              // BDD node 169:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
              if ((65535) != ((u16)value_2.get(0, 2))) {
                // EP node  12732
                // BDD node 169:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                // EP node  14154
                // BDD node 170:FORWARD
                cpu_hdr->egress_dev = bswap16((u16)value_2.get(0, 2));
              } else {
                // EP node  12733
                // BDD node 169:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                // EP node  14155
                // BDD node 171:DROP
                result.forward = false;
              }
            } else {
              // EP node  12341
              // BDD node 168:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_640)))
              // EP node  13842
              // BDD node 172:DROP
              result.forward = false;
            }
          } else {
            // EP node  11226
            // BDD node 162:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
            // EP node  11496
            // BDD node 174:map_put(map:(w64 1074044752), key:(w64 1074072472)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks))))))))))], value:(ReadLSB w32 (w32 0) new_index))
            buffer_t guarded_map_table_1074044752_key_1(13);
            guarded_map_table_1074044752_key_1[0] = *(u8*)(hdr_2 + 0);
            guarded_map_table_1074044752_key_1[1] = *(u8*)(hdr_2 + 1);
            guarded_map_table_1074044752_key_1[2] = *(u8*)(hdr_2 + 2);
            guarded_map_table_1074044752_key_1[3] = *(u8*)(hdr_2 + 3);
            guarded_map_table_1074044752_key_1[4] = *(u8*)(hdr_1 + 12);
            guarded_map_table_1074044752_key_1[5] = *(u8*)(hdr_1 + 13);
            guarded_map_table_1074044752_key_1[6] = *(u8*)(hdr_1 + 14);
            guarded_map_table_1074044752_key_1[7] = *(u8*)(hdr_1 + 15);
            guarded_map_table_1074044752_key_1[8] = *(u8*)(hdr_1 + 16);
            guarded_map_table_1074044752_key_1[9] = *(u8*)(hdr_1 + 17);
            guarded_map_table_1074044752_key_1[10] = *(u8*)(hdr_1 + 18);
            guarded_map_table_1074044752_key_1[11] = *(u8*)(hdr_1 + 19);
            guarded_map_table_1074044752_key_1[12] = *(u8*)(hdr_1 + 9);
            state->guarded_map_table_1074044752.put(guarded_map_table_1074044752_key_1, allocated_index_0);
            // EP node  11773
            // BDD node 176:vector_borrow(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074221408)[ -> (w64 1074108272)])
            buffer_t value_3;
            state->vector_table_1074094376.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_3);
            // EP node  12149
            // BDD node 177:vector_return(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074108272)[(ReadLSB w16 (w32 0) vector_data_640)])
            // EP node  13944
            // BDD node 181:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_640)))
            if ((bswap32(cpu_hdr_extra->DEVICE) & 65535) != ((u16)value_3.get(0, 2))) {
              // EP node  13945
              // BDD node 181:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_640)))
              // EP node  14262
              // BDD node 182:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
              if ((65535) != ((u16)value_3.get(0, 2))) {
                // EP node  14263
                // BDD node 182:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                // EP node  14594
                // BDD node 183:FORWARD
                cpu_hdr->egress_dev = bswap16((u16)value_3.get(0, 2));
              } else {
                // EP node  14264
                // BDD node 182:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                // EP node  14595
                // BDD node 184:DROP
                result.forward = false;
              }
            } else {
              // EP node  13946
              // BDD node 181:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_640)))
              // EP node  14372
              // BDD node 185:DROP
              result.forward = false;
            }
          }
        } else {
          // EP node  6927
          // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
          // EP node  6928
          // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
          result.abort_transaction = true;
          cpu_hdr->trigger_dataplane_execution = 1;
          return result;
        }
      } else {
        // EP node  6922
        // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  6923
        // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
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
