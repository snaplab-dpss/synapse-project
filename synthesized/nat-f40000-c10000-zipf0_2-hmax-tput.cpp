#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  FCFSCachedTable fcfs_ct_1074053136;
  VectorRegister vector_register_1074066960;
  VectorTable vector_table_1074085544;
  VectorTable vector_table_1074102760;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      fcfs_ct_1074053136("fcfs_ct_1074053136", {"Ingress.fcfs_ct_1074053136_table_163", }, "Ingress.fcfs_ct_1074053136_reg_liveness", {"Ingress.fcfs_ct_1074053136_reg_key_0", "Ingress.fcfs_ct_1074053136_reg_key_1", "Ingress.fcfs_ct_1074053136_reg_key_2", "Ingress.fcfs_ct_1074053136_reg_key_3", }, 1000LL),
      vector_register_1074066960("vector_register_1074066960",{"Ingress.vector_register_1074066960_0","Ingress.vector_register_1074066960_1","Ingress.vector_register_1074066960_2","Ingress.vector_register_1074066960_3",}),
      vector_table_1074085544("vector_table_1074085544",{"Ingress.vector_table_1074085544_139",}),
      vector_table_1074102760("vector_table_1074102760",{"Ingress.vector_table_1074102760_183","Ingress.vector_table_1074102760_149","Ingress.vector_table_1074102760_175",})
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
  // BDD node 0:map_allocate(capacity:(w32 65536), key_size:(w32 12), map_out:(w64 1074052872)[(w64 0) -> (w64 1074053136)])
  // Module DataplaneFCFSCachedTableAllocate
  // BDD node 1:vector_allocate(elem_size:(w32 12), capacity:(w32 65536), vector_out:(w64 1074052880)[(w64 0) -> (w64 1074066960)])
  // Module DataplaneVectorRegisterAllocate
  // BDD node 2:dchain_allocate(index_range:(w32 65536), chain_out:(w64 1074052888)[ -> (w64 1074085120)])
  // Module Ignore
  // BDD node 3:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1074052896)[(w64 0) -> (w64 1074085544)])
  // Module DataplaneVectorTableAllocate
  // BDD node 4:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074052904)[(w64 0) -> (w64 1074102760)])
  // Module DataplaneVectorTableAllocate
  // BDD node 5:vector_borrow(vector:(w64 1074085544), index:(w32 0), val_out:(w64 1074052744)[ -> (w64 1074099440)])
  // Module Ignore
  // BDD node 6:vector_return(vector:(w64 1074085544), index:(w32 0), value:(w64 1074099440)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_0(4);
  vector_table_1074085544_value_0[0] = 0;
  vector_table_1074085544_value_0[1] = 0;
  vector_table_1074085544_value_0[2] = 0;
  vector_table_1074085544_value_0[3] = 1;
  state->vector_table_1074085544.write(0, vector_table_1074085544_value_0);
  // BDD node 7:vector_borrow(vector:(w64 1074102760), index:(w32 0), val_out:(w64 1074052808)[ -> (w64 1074116656)])
  // Module Ignore
  // BDD node 8:vector_return(vector:(w64 1074102760), index:(w32 0), value:(w64 1074116656)[(w16 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_0(2);
  vector_table_1074102760_value_0[0] = 0;
  vector_table_1074102760_value_0[1] = 1;
  state->vector_table_1074102760.write(0, vector_table_1074102760_value_0);
  // BDD node 9:vector_borrow(vector:(w64 1074085544), index:(w32 1), val_out:(w64 1074052744)[ -> (w64 1074099464)])
  // Module Ignore
  // BDD node 10:vector_return(vector:(w64 1074085544), index:(w32 1), value:(w64 1074099464)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_1(4);
  vector_table_1074085544_value_1[0] = 0;
  vector_table_1074085544_value_1[1] = 0;
  vector_table_1074085544_value_1[2] = 0;
  vector_table_1074085544_value_1[3] = 0;
  state->vector_table_1074085544.write(1, vector_table_1074085544_value_1);
  // BDD node 11:vector_borrow(vector:(w64 1074102760), index:(w32 1), val_out:(w64 1074052808)[ -> (w64 1074116680)])
  // Module Ignore
  // BDD node 12:vector_return(vector:(w64 1074102760), index:(w32 1), value:(w64 1074116680)[(w16 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_1(2);
  vector_table_1074102760_value_1[0] = 0;
  vector_table_1074102760_value_1[1] = 0;
  state->vector_table_1074102760.write(1, vector_table_1074102760_value_1);
  // BDD node 13:vector_borrow(vector:(w64 1074085544), index:(w32 2), val_out:(w64 1074052744)[ -> (w64 1074099488)])
  // Module Ignore
  // BDD node 14:vector_return(vector:(w64 1074085544), index:(w32 2), value:(w64 1074099488)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_2(4);
  vector_table_1074085544_value_2[0] = 0;
  vector_table_1074085544_value_2[1] = 0;
  vector_table_1074085544_value_2[2] = 0;
  vector_table_1074085544_value_2[3] = 1;
  state->vector_table_1074085544.write(2, vector_table_1074085544_value_2);
  // BDD node 15:vector_borrow(vector:(w64 1074102760), index:(w32 2), val_out:(w64 1074052808)[ -> (w64 1074116704)])
  // Module Ignore
  // BDD node 16:vector_return(vector:(w64 1074102760), index:(w32 2), value:(w64 1074116704)[(w16 3)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_2(2);
  vector_table_1074102760_value_2[0] = 0;
  vector_table_1074102760_value_2[1] = 3;
  state->vector_table_1074102760.write(2, vector_table_1074102760_value_2);
  // BDD node 17:vector_borrow(vector:(w64 1074085544), index:(w32 3), val_out:(w64 1074052744)[ -> (w64 1074099512)])
  // Module Ignore
  // BDD node 18:vector_return(vector:(w64 1074085544), index:(w32 3), value:(w64 1074099512)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_3(4);
  vector_table_1074085544_value_3[0] = 0;
  vector_table_1074085544_value_3[1] = 0;
  vector_table_1074085544_value_3[2] = 0;
  vector_table_1074085544_value_3[3] = 0;
  state->vector_table_1074085544.write(3, vector_table_1074085544_value_3);
  // BDD node 19:vector_borrow(vector:(w64 1074102760), index:(w32 3), val_out:(w64 1074052808)[ -> (w64 1074116728)])
  // Module Ignore
  // BDD node 20:vector_return(vector:(w64 1074102760), index:(w32 3), value:(w64 1074116728)[(w16 2)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_3(2);
  vector_table_1074102760_value_3[0] = 0;
  vector_table_1074102760_value_3[1] = 2;
  state->vector_table_1074102760.write(3, vector_table_1074102760_value_3);
  // BDD node 21:vector_borrow(vector:(w64 1074085544), index:(w32 4), val_out:(w64 1074052744)[ -> (w64 1074099536)])
  // Module Ignore
  // BDD node 22:vector_return(vector:(w64 1074085544), index:(w32 4), value:(w64 1074099536)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_4(4);
  vector_table_1074085544_value_4[0] = 0;
  vector_table_1074085544_value_4[1] = 0;
  vector_table_1074085544_value_4[2] = 0;
  vector_table_1074085544_value_4[3] = 1;
  state->vector_table_1074085544.write(4, vector_table_1074085544_value_4);
  // BDD node 23:vector_borrow(vector:(w64 1074102760), index:(w32 4), val_out:(w64 1074052808)[ -> (w64 1074116752)])
  // Module Ignore
  // BDD node 24:vector_return(vector:(w64 1074102760), index:(w32 4), value:(w64 1074116752)[(w16 5)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_4(2);
  vector_table_1074102760_value_4[0] = 0;
  vector_table_1074102760_value_4[1] = 5;
  state->vector_table_1074102760.write(4, vector_table_1074102760_value_4);
  // BDD node 25:vector_borrow(vector:(w64 1074085544), index:(w32 5), val_out:(w64 1074052744)[ -> (w64 1074099560)])
  // Module Ignore
  // BDD node 26:vector_return(vector:(w64 1074085544), index:(w32 5), value:(w64 1074099560)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_5(4);
  vector_table_1074085544_value_5[0] = 0;
  vector_table_1074085544_value_5[1] = 0;
  vector_table_1074085544_value_5[2] = 0;
  vector_table_1074085544_value_5[3] = 0;
  state->vector_table_1074085544.write(5, vector_table_1074085544_value_5);
  // BDD node 27:vector_borrow(vector:(w64 1074102760), index:(w32 5), val_out:(w64 1074052808)[ -> (w64 1074116776)])
  // Module Ignore
  // BDD node 28:vector_return(vector:(w64 1074102760), index:(w32 5), value:(w64 1074116776)[(w16 4)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_5(2);
  vector_table_1074102760_value_5[0] = 0;
  vector_table_1074102760_value_5[1] = 4;
  state->vector_table_1074102760.write(5, vector_table_1074102760_value_5);
  // BDD node 29:vector_borrow(vector:(w64 1074085544), index:(w32 6), val_out:(w64 1074052744)[ -> (w64 1074099584)])
  // Module Ignore
  // BDD node 30:vector_return(vector:(w64 1074085544), index:(w32 6), value:(w64 1074099584)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_6(4);
  vector_table_1074085544_value_6[0] = 0;
  vector_table_1074085544_value_6[1] = 0;
  vector_table_1074085544_value_6[2] = 0;
  vector_table_1074085544_value_6[3] = 1;
  state->vector_table_1074085544.write(6, vector_table_1074085544_value_6);
  // BDD node 31:vector_borrow(vector:(w64 1074102760), index:(w32 6), val_out:(w64 1074052808)[ -> (w64 1074116800)])
  // Module Ignore
  // BDD node 32:vector_return(vector:(w64 1074102760), index:(w32 6), value:(w64 1074116800)[(w16 7)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_6(2);
  vector_table_1074102760_value_6[0] = 0;
  vector_table_1074102760_value_6[1] = 7;
  state->vector_table_1074102760.write(6, vector_table_1074102760_value_6);
  // BDD node 33:vector_borrow(vector:(w64 1074085544), index:(w32 7), val_out:(w64 1074052744)[ -> (w64 1074099608)])
  // Module Ignore
  // BDD node 34:vector_return(vector:(w64 1074085544), index:(w32 7), value:(w64 1074099608)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_7(4);
  vector_table_1074085544_value_7[0] = 0;
  vector_table_1074085544_value_7[1] = 0;
  vector_table_1074085544_value_7[2] = 0;
  vector_table_1074085544_value_7[3] = 0;
  state->vector_table_1074085544.write(7, vector_table_1074085544_value_7);
  // BDD node 35:vector_borrow(vector:(w64 1074102760), index:(w32 7), val_out:(w64 1074052808)[ -> (w64 1074116824)])
  // Module Ignore
  // BDD node 36:vector_return(vector:(w64 1074102760), index:(w32 7), value:(w64 1074116824)[(w16 6)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_7(2);
  vector_table_1074102760_value_7[0] = 0;
  vector_table_1074102760_value_7[1] = 6;
  state->vector_table_1074102760.write(7, vector_table_1074102760_value_7);
  // BDD node 37:vector_borrow(vector:(w64 1074085544), index:(w32 8), val_out:(w64 1074052744)[ -> (w64 1074099632)])
  // Module Ignore
  // BDD node 38:vector_return(vector:(w64 1074085544), index:(w32 8), value:(w64 1074099632)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_8(4);
  vector_table_1074085544_value_8[0] = 0;
  vector_table_1074085544_value_8[1] = 0;
  vector_table_1074085544_value_8[2] = 0;
  vector_table_1074085544_value_8[3] = 1;
  state->vector_table_1074085544.write(8, vector_table_1074085544_value_8);
  // BDD node 39:vector_borrow(vector:(w64 1074102760), index:(w32 8), val_out:(w64 1074052808)[ -> (w64 1074116848)])
  // Module Ignore
  // BDD node 40:vector_return(vector:(w64 1074102760), index:(w32 8), value:(w64 1074116848)[(w16 9)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_8(2);
  vector_table_1074102760_value_8[0] = 0;
  vector_table_1074102760_value_8[1] = 9;
  state->vector_table_1074102760.write(8, vector_table_1074102760_value_8);
  // BDD node 41:vector_borrow(vector:(w64 1074085544), index:(w32 9), val_out:(w64 1074052744)[ -> (w64 1074099656)])
  // Module Ignore
  // BDD node 42:vector_return(vector:(w64 1074085544), index:(w32 9), value:(w64 1074099656)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_9(4);
  vector_table_1074085544_value_9[0] = 0;
  vector_table_1074085544_value_9[1] = 0;
  vector_table_1074085544_value_9[2] = 0;
  vector_table_1074085544_value_9[3] = 0;
  state->vector_table_1074085544.write(9, vector_table_1074085544_value_9);
  // BDD node 43:vector_borrow(vector:(w64 1074102760), index:(w32 9), val_out:(w64 1074052808)[ -> (w64 1074116872)])
  // Module Ignore
  // BDD node 44:vector_return(vector:(w64 1074102760), index:(w32 9), value:(w64 1074116872)[(w16 8)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_9(2);
  vector_table_1074102760_value_9[0] = 0;
  vector_table_1074102760_value_9[1] = 8;
  state->vector_table_1074102760.write(9, vector_table_1074102760_value_9);
  // BDD node 45:vector_borrow(vector:(w64 1074085544), index:(w32 10), val_out:(w64 1074052744)[ -> (w64 1074099680)])
  // Module Ignore
  // BDD node 46:vector_return(vector:(w64 1074085544), index:(w32 10), value:(w64 1074099680)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_10(4);
  vector_table_1074085544_value_10[0] = 0;
  vector_table_1074085544_value_10[1] = 0;
  vector_table_1074085544_value_10[2] = 0;
  vector_table_1074085544_value_10[3] = 1;
  state->vector_table_1074085544.write(10, vector_table_1074085544_value_10);
  // BDD node 47:vector_borrow(vector:(w64 1074102760), index:(w32 10), val_out:(w64 1074052808)[ -> (w64 1074116896)])
  // Module Ignore
  // BDD node 48:vector_return(vector:(w64 1074102760), index:(w32 10), value:(w64 1074116896)[(w16 11)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_10(2);
  vector_table_1074102760_value_10[0] = 0;
  vector_table_1074102760_value_10[1] = 11;
  state->vector_table_1074102760.write(10, vector_table_1074102760_value_10);
  // BDD node 49:vector_borrow(vector:(w64 1074085544), index:(w32 11), val_out:(w64 1074052744)[ -> (w64 1074099704)])
  // Module Ignore
  // BDD node 50:vector_return(vector:(w64 1074085544), index:(w32 11), value:(w64 1074099704)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_11(4);
  vector_table_1074085544_value_11[0] = 0;
  vector_table_1074085544_value_11[1] = 0;
  vector_table_1074085544_value_11[2] = 0;
  vector_table_1074085544_value_11[3] = 0;
  state->vector_table_1074085544.write(11, vector_table_1074085544_value_11);
  // BDD node 51:vector_borrow(vector:(w64 1074102760), index:(w32 11), val_out:(w64 1074052808)[ -> (w64 1074116920)])
  // Module Ignore
  // BDD node 52:vector_return(vector:(w64 1074102760), index:(w32 11), value:(w64 1074116920)[(w16 10)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_11(2);
  vector_table_1074102760_value_11[0] = 0;
  vector_table_1074102760_value_11[1] = 10;
  state->vector_table_1074102760.write(11, vector_table_1074102760_value_11);
  // BDD node 53:vector_borrow(vector:(w64 1074085544), index:(w32 12), val_out:(w64 1074052744)[ -> (w64 1074099728)])
  // Module Ignore
  // BDD node 54:vector_return(vector:(w64 1074085544), index:(w32 12), value:(w64 1074099728)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_12(4);
  vector_table_1074085544_value_12[0] = 0;
  vector_table_1074085544_value_12[1] = 0;
  vector_table_1074085544_value_12[2] = 0;
  vector_table_1074085544_value_12[3] = 1;
  state->vector_table_1074085544.write(12, vector_table_1074085544_value_12);
  // BDD node 55:vector_borrow(vector:(w64 1074102760), index:(w32 12), val_out:(w64 1074052808)[ -> (w64 1074116944)])
  // Module Ignore
  // BDD node 56:vector_return(vector:(w64 1074102760), index:(w32 12), value:(w64 1074116944)[(w16 13)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_12(2);
  vector_table_1074102760_value_12[0] = 0;
  vector_table_1074102760_value_12[1] = 13;
  state->vector_table_1074102760.write(12, vector_table_1074102760_value_12);
  // BDD node 57:vector_borrow(vector:(w64 1074085544), index:(w32 13), val_out:(w64 1074052744)[ -> (w64 1074099752)])
  // Module Ignore
  // BDD node 58:vector_return(vector:(w64 1074085544), index:(w32 13), value:(w64 1074099752)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_13(4);
  vector_table_1074085544_value_13[0] = 0;
  vector_table_1074085544_value_13[1] = 0;
  vector_table_1074085544_value_13[2] = 0;
  vector_table_1074085544_value_13[3] = 0;
  state->vector_table_1074085544.write(13, vector_table_1074085544_value_13);
  // BDD node 59:vector_borrow(vector:(w64 1074102760), index:(w32 13), val_out:(w64 1074052808)[ -> (w64 1074116968)])
  // Module Ignore
  // BDD node 60:vector_return(vector:(w64 1074102760), index:(w32 13), value:(w64 1074116968)[(w16 12)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_13(2);
  vector_table_1074102760_value_13[0] = 0;
  vector_table_1074102760_value_13[1] = 12;
  state->vector_table_1074102760.write(13, vector_table_1074102760_value_13);
  // BDD node 61:vector_borrow(vector:(w64 1074085544), index:(w32 14), val_out:(w64 1074052744)[ -> (w64 1074099776)])
  // Module Ignore
  // BDD node 62:vector_return(vector:(w64 1074085544), index:(w32 14), value:(w64 1074099776)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_14(4);
  vector_table_1074085544_value_14[0] = 0;
  vector_table_1074085544_value_14[1] = 0;
  vector_table_1074085544_value_14[2] = 0;
  vector_table_1074085544_value_14[3] = 1;
  state->vector_table_1074085544.write(14, vector_table_1074085544_value_14);
  // BDD node 63:vector_borrow(vector:(w64 1074102760), index:(w32 14), val_out:(w64 1074052808)[ -> (w64 1074116992)])
  // Module Ignore
  // BDD node 64:vector_return(vector:(w64 1074102760), index:(w32 14), value:(w64 1074116992)[(w16 15)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_14(2);
  vector_table_1074102760_value_14[0] = 0;
  vector_table_1074102760_value_14[1] = 15;
  state->vector_table_1074102760.write(14, vector_table_1074102760_value_14);
  // BDD node 65:vector_borrow(vector:(w64 1074085544), index:(w32 15), val_out:(w64 1074052744)[ -> (w64 1074099800)])
  // Module Ignore
  // BDD node 66:vector_return(vector:(w64 1074085544), index:(w32 15), value:(w64 1074099800)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_15(4);
  vector_table_1074085544_value_15[0] = 0;
  vector_table_1074085544_value_15[1] = 0;
  vector_table_1074085544_value_15[2] = 0;
  vector_table_1074085544_value_15[3] = 0;
  state->vector_table_1074085544.write(15, vector_table_1074085544_value_15);
  // BDD node 67:vector_borrow(vector:(w64 1074102760), index:(w32 15), val_out:(w64 1074052808)[ -> (w64 1074117016)])
  // Module Ignore
  // BDD node 68:vector_return(vector:(w64 1074102760), index:(w32 15), value:(w64 1074117016)[(w16 14)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_15(2);
  vector_table_1074102760_value_15[0] = 0;
  vector_table_1074102760_value_15[1] = 14;
  state->vector_table_1074102760.write(15, vector_table_1074102760_value_15);
  // BDD node 69:vector_borrow(vector:(w64 1074085544), index:(w32 16), val_out:(w64 1074052744)[ -> (w64 1074099824)])
  // Module Ignore
  // BDD node 70:vector_return(vector:(w64 1074085544), index:(w32 16), value:(w64 1074099824)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_16(4);
  vector_table_1074085544_value_16[0] = 0;
  vector_table_1074085544_value_16[1] = 0;
  vector_table_1074085544_value_16[2] = 0;
  vector_table_1074085544_value_16[3] = 1;
  state->vector_table_1074085544.write(16, vector_table_1074085544_value_16);
  // BDD node 71:vector_borrow(vector:(w64 1074102760), index:(w32 16), val_out:(w64 1074052808)[ -> (w64 1074117040)])
  // Module Ignore
  // BDD node 72:vector_return(vector:(w64 1074102760), index:(w32 16), value:(w64 1074117040)[(w16 17)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_16(2);
  vector_table_1074102760_value_16[0] = 0;
  vector_table_1074102760_value_16[1] = 17;
  state->vector_table_1074102760.write(16, vector_table_1074102760_value_16);
  // BDD node 73:vector_borrow(vector:(w64 1074085544), index:(w32 17), val_out:(w64 1074052744)[ -> (w64 1074099848)])
  // Module Ignore
  // BDD node 74:vector_return(vector:(w64 1074085544), index:(w32 17), value:(w64 1074099848)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_17(4);
  vector_table_1074085544_value_17[0] = 0;
  vector_table_1074085544_value_17[1] = 0;
  vector_table_1074085544_value_17[2] = 0;
  vector_table_1074085544_value_17[3] = 0;
  state->vector_table_1074085544.write(17, vector_table_1074085544_value_17);
  // BDD node 75:vector_borrow(vector:(w64 1074102760), index:(w32 17), val_out:(w64 1074052808)[ -> (w64 1074117064)])
  // Module Ignore
  // BDD node 76:vector_return(vector:(w64 1074102760), index:(w32 17), value:(w64 1074117064)[(w16 16)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_17(2);
  vector_table_1074102760_value_17[0] = 0;
  vector_table_1074102760_value_17[1] = 16;
  state->vector_table_1074102760.write(17, vector_table_1074102760_value_17);
  // BDD node 77:vector_borrow(vector:(w64 1074085544), index:(w32 18), val_out:(w64 1074052744)[ -> (w64 1074099872)])
  // Module Ignore
  // BDD node 78:vector_return(vector:(w64 1074085544), index:(w32 18), value:(w64 1074099872)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_18(4);
  vector_table_1074085544_value_18[0] = 0;
  vector_table_1074085544_value_18[1] = 0;
  vector_table_1074085544_value_18[2] = 0;
  vector_table_1074085544_value_18[3] = 1;
  state->vector_table_1074085544.write(18, vector_table_1074085544_value_18);
  // BDD node 79:vector_borrow(vector:(w64 1074102760), index:(w32 18), val_out:(w64 1074052808)[ -> (w64 1074117088)])
  // Module Ignore
  // BDD node 80:vector_return(vector:(w64 1074102760), index:(w32 18), value:(w64 1074117088)[(w16 19)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_18(2);
  vector_table_1074102760_value_18[0] = 0;
  vector_table_1074102760_value_18[1] = 19;
  state->vector_table_1074102760.write(18, vector_table_1074102760_value_18);
  // BDD node 81:vector_borrow(vector:(w64 1074085544), index:(w32 19), val_out:(w64 1074052744)[ -> (w64 1074099896)])
  // Module Ignore
  // BDD node 82:vector_return(vector:(w64 1074085544), index:(w32 19), value:(w64 1074099896)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_19(4);
  vector_table_1074085544_value_19[0] = 0;
  vector_table_1074085544_value_19[1] = 0;
  vector_table_1074085544_value_19[2] = 0;
  vector_table_1074085544_value_19[3] = 0;
  state->vector_table_1074085544.write(19, vector_table_1074085544_value_19);
  // BDD node 83:vector_borrow(vector:(w64 1074102760), index:(w32 19), val_out:(w64 1074052808)[ -> (w64 1074117112)])
  // Module Ignore
  // BDD node 84:vector_return(vector:(w64 1074102760), index:(w32 19), value:(w64 1074117112)[(w16 18)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_19(2);
  vector_table_1074102760_value_19[0] = 0;
  vector_table_1074102760_value_19[1] = 18;
  state->vector_table_1074102760.write(19, vector_table_1074102760_value_19);
  // BDD node 85:vector_borrow(vector:(w64 1074085544), index:(w32 20), val_out:(w64 1074052744)[ -> (w64 1074099920)])
  // Module Ignore
  // BDD node 86:vector_return(vector:(w64 1074085544), index:(w32 20), value:(w64 1074099920)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_20(4);
  vector_table_1074085544_value_20[0] = 0;
  vector_table_1074085544_value_20[1] = 0;
  vector_table_1074085544_value_20[2] = 0;
  vector_table_1074085544_value_20[3] = 1;
  state->vector_table_1074085544.write(20, vector_table_1074085544_value_20);
  // BDD node 87:vector_borrow(vector:(w64 1074102760), index:(w32 20), val_out:(w64 1074052808)[ -> (w64 1074117136)])
  // Module Ignore
  // BDD node 88:vector_return(vector:(w64 1074102760), index:(w32 20), value:(w64 1074117136)[(w16 21)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_20(2);
  vector_table_1074102760_value_20[0] = 0;
  vector_table_1074102760_value_20[1] = 21;
  state->vector_table_1074102760.write(20, vector_table_1074102760_value_20);
  // BDD node 89:vector_borrow(vector:(w64 1074085544), index:(w32 21), val_out:(w64 1074052744)[ -> (w64 1074099944)])
  // Module Ignore
  // BDD node 90:vector_return(vector:(w64 1074085544), index:(w32 21), value:(w64 1074099944)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_21(4);
  vector_table_1074085544_value_21[0] = 0;
  vector_table_1074085544_value_21[1] = 0;
  vector_table_1074085544_value_21[2] = 0;
  vector_table_1074085544_value_21[3] = 0;
  state->vector_table_1074085544.write(21, vector_table_1074085544_value_21);
  // BDD node 91:vector_borrow(vector:(w64 1074102760), index:(w32 21), val_out:(w64 1074052808)[ -> (w64 1074117160)])
  // Module Ignore
  // BDD node 92:vector_return(vector:(w64 1074102760), index:(w32 21), value:(w64 1074117160)[(w16 20)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_21(2);
  vector_table_1074102760_value_21[0] = 0;
  vector_table_1074102760_value_21[1] = 20;
  state->vector_table_1074102760.write(21, vector_table_1074102760_value_21);
  // BDD node 93:vector_borrow(vector:(w64 1074085544), index:(w32 22), val_out:(w64 1074052744)[ -> (w64 1074099968)])
  // Module Ignore
  // BDD node 94:vector_return(vector:(w64 1074085544), index:(w32 22), value:(w64 1074099968)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_22(4);
  vector_table_1074085544_value_22[0] = 0;
  vector_table_1074085544_value_22[1] = 0;
  vector_table_1074085544_value_22[2] = 0;
  vector_table_1074085544_value_22[3] = 1;
  state->vector_table_1074085544.write(22, vector_table_1074085544_value_22);
  // BDD node 95:vector_borrow(vector:(w64 1074102760), index:(w32 22), val_out:(w64 1074052808)[ -> (w64 1074117184)])
  // Module Ignore
  // BDD node 96:vector_return(vector:(w64 1074102760), index:(w32 22), value:(w64 1074117184)[(w16 23)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_22(2);
  vector_table_1074102760_value_22[0] = 0;
  vector_table_1074102760_value_22[1] = 23;
  state->vector_table_1074102760.write(22, vector_table_1074102760_value_22);
  // BDD node 97:vector_borrow(vector:(w64 1074085544), index:(w32 23), val_out:(w64 1074052744)[ -> (w64 1074099992)])
  // Module Ignore
  // BDD node 98:vector_return(vector:(w64 1074085544), index:(w32 23), value:(w64 1074099992)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_23(4);
  vector_table_1074085544_value_23[0] = 0;
  vector_table_1074085544_value_23[1] = 0;
  vector_table_1074085544_value_23[2] = 0;
  vector_table_1074085544_value_23[3] = 0;
  state->vector_table_1074085544.write(23, vector_table_1074085544_value_23);
  // BDD node 99:vector_borrow(vector:(w64 1074102760), index:(w32 23), val_out:(w64 1074052808)[ -> (w64 1074117208)])
  // Module Ignore
  // BDD node 100:vector_return(vector:(w64 1074102760), index:(w32 23), value:(w64 1074117208)[(w16 22)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_23(2);
  vector_table_1074102760_value_23[0] = 0;
  vector_table_1074102760_value_23[1] = 22;
  state->vector_table_1074102760.write(23, vector_table_1074102760_value_23);
  // BDD node 101:vector_borrow(vector:(w64 1074085544), index:(w32 24), val_out:(w64 1074052744)[ -> (w64 1074100016)])
  // Module Ignore
  // BDD node 102:vector_return(vector:(w64 1074085544), index:(w32 24), value:(w64 1074100016)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_24(4);
  vector_table_1074085544_value_24[0] = 0;
  vector_table_1074085544_value_24[1] = 0;
  vector_table_1074085544_value_24[2] = 0;
  vector_table_1074085544_value_24[3] = 1;
  state->vector_table_1074085544.write(24, vector_table_1074085544_value_24);
  // BDD node 103:vector_borrow(vector:(w64 1074102760), index:(w32 24), val_out:(w64 1074052808)[ -> (w64 1074117232)])
  // Module Ignore
  // BDD node 104:vector_return(vector:(w64 1074102760), index:(w32 24), value:(w64 1074117232)[(w16 25)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_24(2);
  vector_table_1074102760_value_24[0] = 0;
  vector_table_1074102760_value_24[1] = 25;
  state->vector_table_1074102760.write(24, vector_table_1074102760_value_24);
  // BDD node 105:vector_borrow(vector:(w64 1074085544), index:(w32 25), val_out:(w64 1074052744)[ -> (w64 1074100040)])
  // Module Ignore
  // BDD node 106:vector_return(vector:(w64 1074085544), index:(w32 25), value:(w64 1074100040)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_25(4);
  vector_table_1074085544_value_25[0] = 0;
  vector_table_1074085544_value_25[1] = 0;
  vector_table_1074085544_value_25[2] = 0;
  vector_table_1074085544_value_25[3] = 0;
  state->vector_table_1074085544.write(25, vector_table_1074085544_value_25);
  // BDD node 107:vector_borrow(vector:(w64 1074102760), index:(w32 25), val_out:(w64 1074052808)[ -> (w64 1074117256)])
  // Module Ignore
  // BDD node 108:vector_return(vector:(w64 1074102760), index:(w32 25), value:(w64 1074117256)[(w16 24)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_25(2);
  vector_table_1074102760_value_25[0] = 0;
  vector_table_1074102760_value_25[1] = 24;
  state->vector_table_1074102760.write(25, vector_table_1074102760_value_25);
  // BDD node 109:vector_borrow(vector:(w64 1074085544), index:(w32 26), val_out:(w64 1074052744)[ -> (w64 1074100064)])
  // Module Ignore
  // BDD node 110:vector_return(vector:(w64 1074085544), index:(w32 26), value:(w64 1074100064)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_26(4);
  vector_table_1074085544_value_26[0] = 0;
  vector_table_1074085544_value_26[1] = 0;
  vector_table_1074085544_value_26[2] = 0;
  vector_table_1074085544_value_26[3] = 1;
  state->vector_table_1074085544.write(26, vector_table_1074085544_value_26);
  // BDD node 111:vector_borrow(vector:(w64 1074102760), index:(w32 26), val_out:(w64 1074052808)[ -> (w64 1074117280)])
  // Module Ignore
  // BDD node 112:vector_return(vector:(w64 1074102760), index:(w32 26), value:(w64 1074117280)[(w16 27)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_26(2);
  vector_table_1074102760_value_26[0] = 0;
  vector_table_1074102760_value_26[1] = 27;
  state->vector_table_1074102760.write(26, vector_table_1074102760_value_26);
  // BDD node 113:vector_borrow(vector:(w64 1074085544), index:(w32 27), val_out:(w64 1074052744)[ -> (w64 1074100088)])
  // Module Ignore
  // BDD node 114:vector_return(vector:(w64 1074085544), index:(w32 27), value:(w64 1074100088)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_27(4);
  vector_table_1074085544_value_27[0] = 0;
  vector_table_1074085544_value_27[1] = 0;
  vector_table_1074085544_value_27[2] = 0;
  vector_table_1074085544_value_27[3] = 0;
  state->vector_table_1074085544.write(27, vector_table_1074085544_value_27);
  // BDD node 115:vector_borrow(vector:(w64 1074102760), index:(w32 27), val_out:(w64 1074052808)[ -> (w64 1074117304)])
  // Module Ignore
  // BDD node 116:vector_return(vector:(w64 1074102760), index:(w32 27), value:(w64 1074117304)[(w16 26)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_27(2);
  vector_table_1074102760_value_27[0] = 0;
  vector_table_1074102760_value_27[1] = 26;
  state->vector_table_1074102760.write(27, vector_table_1074102760_value_27);
  // BDD node 117:vector_borrow(vector:(w64 1074085544), index:(w32 28), val_out:(w64 1074052744)[ -> (w64 1074100112)])
  // Module Ignore
  // BDD node 118:vector_return(vector:(w64 1074085544), index:(w32 28), value:(w64 1074100112)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_28(4);
  vector_table_1074085544_value_28[0] = 0;
  vector_table_1074085544_value_28[1] = 0;
  vector_table_1074085544_value_28[2] = 0;
  vector_table_1074085544_value_28[3] = 1;
  state->vector_table_1074085544.write(28, vector_table_1074085544_value_28);
  // BDD node 119:vector_borrow(vector:(w64 1074102760), index:(w32 28), val_out:(w64 1074052808)[ -> (w64 1074117328)])
  // Module Ignore
  // BDD node 120:vector_return(vector:(w64 1074102760), index:(w32 28), value:(w64 1074117328)[(w16 29)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_28(2);
  vector_table_1074102760_value_28[0] = 0;
  vector_table_1074102760_value_28[1] = 29;
  state->vector_table_1074102760.write(28, vector_table_1074102760_value_28);
  // BDD node 121:vector_borrow(vector:(w64 1074085544), index:(w32 29), val_out:(w64 1074052744)[ -> (w64 1074100136)])
  // Module Ignore
  // BDD node 122:vector_return(vector:(w64 1074085544), index:(w32 29), value:(w64 1074100136)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_29(4);
  vector_table_1074085544_value_29[0] = 0;
  vector_table_1074085544_value_29[1] = 0;
  vector_table_1074085544_value_29[2] = 0;
  vector_table_1074085544_value_29[3] = 0;
  state->vector_table_1074085544.write(29, vector_table_1074085544_value_29);
  // BDD node 123:vector_borrow(vector:(w64 1074102760), index:(w32 29), val_out:(w64 1074052808)[ -> (w64 1074117352)])
  // Module Ignore
  // BDD node 124:vector_return(vector:(w64 1074102760), index:(w32 29), value:(w64 1074117352)[(w16 28)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_29(2);
  vector_table_1074102760_value_29[0] = 0;
  vector_table_1074102760_value_29[1] = 28;
  state->vector_table_1074102760.write(29, vector_table_1074102760_value_29);
  // BDD node 125:vector_borrow(vector:(w64 1074085544), index:(w32 30), val_out:(w64 1074052744)[ -> (w64 1074100160)])
  // Module Ignore
  // BDD node 126:vector_return(vector:(w64 1074085544), index:(w32 30), value:(w64 1074100160)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_30(4);
  vector_table_1074085544_value_30[0] = 0;
  vector_table_1074085544_value_30[1] = 0;
  vector_table_1074085544_value_30[2] = 0;
  vector_table_1074085544_value_30[3] = 1;
  state->vector_table_1074085544.write(30, vector_table_1074085544_value_30);
  // BDD node 127:vector_borrow(vector:(w64 1074102760), index:(w32 30), val_out:(w64 1074052808)[ -> (w64 1074117376)])
  // Module Ignore
  // BDD node 128:vector_return(vector:(w64 1074102760), index:(w32 30), value:(w64 1074117376)[(w16 31)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_30(2);
  vector_table_1074102760_value_30[0] = 0;
  vector_table_1074102760_value_30[1] = 31;
  state->vector_table_1074102760.write(30, vector_table_1074102760_value_30);
  // BDD node 129:vector_borrow(vector:(w64 1074085544), index:(w32 31), val_out:(w64 1074052744)[ -> (w64 1074100184)])
  // Module Ignore
  // BDD node 130:vector_return(vector:(w64 1074085544), index:(w32 31), value:(w64 1074100184)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085544_value_31(4);
  vector_table_1074085544_value_31[0] = 0;
  vector_table_1074085544_value_31[1] = 0;
  vector_table_1074085544_value_31[2] = 0;
  vector_table_1074085544_value_31[3] = 0;
  state->vector_table_1074085544.write(31, vector_table_1074085544_value_31);
  // BDD node 131:vector_borrow(vector:(w64 1074102760), index:(w32 31), val_out:(w64 1074052808)[ -> (w64 1074117400)])
  // Module Ignore
  // BDD node 132:vector_return(vector:(w64 1074102760), index:(w32 31), value:(w64 1074117400)[(w16 30)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102760_value_31(2);
  vector_table_1074102760_value_31[0] = 0;
  vector_table_1074102760_value_31[1] = 30;
  state->vector_table_1074102760.write(31, vector_table_1074102760_value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 cached_insert_success;
  u32 vector_data__139;
  u32 map_has_this_key__163;
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

  if (bswap16(cpu_hdr->code_path) == 6153) {
    // EP node  6136
    // BDD node 281:tofino_force_send_to_controller()
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  6137
    // BDD node 281:tofino_force_send_to_controller()
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  6138
    // BDD node 281:tofino_force_send_to_controller()
    u8* hdr_2 = packet_consume(pkt, 4);
    // EP node  6139
    // BDD node 281:tofino_force_send_to_controller()
    buffer_t value_0;
    state->vector_table_1074085544.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_0);
    // EP node  6140
    // BDD node 281:tofino_force_send_to_controller()
    buffer_t fcfs_ct_1074053136_key_0(12);
    fcfs_ct_1074053136_key_0[0] = *(u8*)(hdr_1 + 12);
    fcfs_ct_1074053136_key_0[1] = *(u8*)(hdr_1 + 13);
    fcfs_ct_1074053136_key_0[2] = *(u8*)(hdr_1 + 14);
    fcfs_ct_1074053136_key_0[3] = *(u8*)(hdr_1 + 15);
    fcfs_ct_1074053136_key_0[4] = *(u8*)(hdr_1 + 16);
    fcfs_ct_1074053136_key_0[5] = *(u8*)(hdr_1 + 17);
    fcfs_ct_1074053136_key_0[6] = *(u8*)(hdr_1 + 18);
    fcfs_ct_1074053136_key_0[7] = *(u8*)(hdr_1 + 19);
    fcfs_ct_1074053136_key_0[8] = *(u8*)(hdr_2 + 0);
    fcfs_ct_1074053136_key_0[9] = *(u8*)(hdr_2 + 1);
    fcfs_ct_1074053136_key_0[10] = *(u8*)(hdr_2 + 2);
    fcfs_ct_1074053136_key_0[11] = *(u8*)(hdr_2 + 3);
    u32 value_1;
    bool found_0 = state->fcfs_ct_1074053136.get(fcfs_ct_1074053136_key_0, value_1);
    // EP node  6141
    // BDD node 281:tofino_force_send_to_controller()
    if ((0) == ((u32)value_0.get(0, 4))) {
      // EP node  6142
      // BDD node 281:tofino_force_send_to_controller()
      // EP node  6144
      // BDD node 281:tofino_force_send_to_controller()
      result.abort_transaction = true;
      cpu_hdr->trigger_dataplane_execution = 1;
      return result;
    } else {
      // EP node  6143
      // BDD node 281:tofino_force_send_to_controller()
      // EP node  6145
      // BDD node 281:tofino_force_send_to_controller()
      if ((0) == (found_0)) {
        // EP node  6146
        // BDD node 281:tofino_force_send_to_controller()
        // EP node  6149
        // BDD node 281:tofino_force_send_to_controller()
        if ((bswap32(cpu_hdr_extra->cached_insert_success)) != (0)) {
          // EP node  6150
          // BDD node 281:tofino_force_send_to_controller()
          // EP node  6152
          // BDD node 281:tofino_force_send_to_controller()
          result.abort_transaction = true;
          cpu_hdr->trigger_dataplane_execution = 1;
          return result;
        } else {
          // EP node  6151
          // BDD node 281:tofino_force_send_to_controller()
          // EP node  8010
          // BDD node 265:dchain_allocate_new_index(chain:(w64 1074085120), index_out:(w64 1074223336)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__265)], time:(ReadLSB w64 (w32 0) next_time))
          buffer_t fcfs_ct_1074053136_key_1(12);
          fcfs_ct_1074053136_key_1[0] = *(u8*)(hdr_1 + 12);
          fcfs_ct_1074053136_key_1[1] = *(u8*)(hdr_1 + 13);
          fcfs_ct_1074053136_key_1[2] = *(u8*)(hdr_1 + 14);
          fcfs_ct_1074053136_key_1[3] = *(u8*)(hdr_1 + 15);
          fcfs_ct_1074053136_key_1[4] = *(u8*)(hdr_1 + 16);
          fcfs_ct_1074053136_key_1[5] = *(u8*)(hdr_1 + 17);
          fcfs_ct_1074053136_key_1[6] = *(u8*)(hdr_1 + 18);
          fcfs_ct_1074053136_key_1[7] = *(u8*)(hdr_1 + 19);
          fcfs_ct_1074053136_key_1[8] = *(u8*)(hdr_2 + 0);
          fcfs_ct_1074053136_key_1[9] = *(u8*)(hdr_2 + 1);
          fcfs_ct_1074053136_key_1[10] = *(u8*)(hdr_2 + 2);
          fcfs_ct_1074053136_key_1[11] = *(u8*)(hdr_2 + 3);
          u32 value_2;
          bool success_0 = state->fcfs_ct_1074053136.allocate_index_and_put(fcfs_ct_1074053136_key_1, value_2);
          // EP node  8088
          // BDD node 266:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__265))
          if ((0) == (success_0)) {
            // EP node  8089
            // BDD node 266:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__265))
            // EP node  9454
            // BDD node 270:DROP
            result.forward = false;
          } else {
            // EP node  8090
            // BDD node 266:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__265))
            // EP node  8250
            // BDD node 271:vector_borrow(vector:(w64 1074066960), index:(ReadLSB w32 (w32 0) new_index__265), val_out:(w64 1074223360)[ -> (w64 1074080856)])
            // EP node  8496
            // BDD node 273:vector_return(vector:(w64 1074066960), index:(ReadLSB w32 (w32 0) new_index__265), value:(w64 1074080856)[(Concat w96 (Read w8 (w32 515) packet_chunks) (Concat w88 (Read w8 (w32 514) packet_chunks) (Concat w80 (Read w8 (w32 513) packet_chunks) (Concat w72 (Read w8 (w32 512) packet_chunks) (ReadLSB w64 (w32 268) packet_chunks)))))])
            buffer_t vector_register_1074066960_value_0(12);
            vector_register_1074066960_value_0[0] = *(u8*)(hdr_1 + 12);
            vector_register_1074066960_value_0[1] = *(u8*)(hdr_1 + 13);
            vector_register_1074066960_value_0[2] = *(u8*)(hdr_1 + 14);
            vector_register_1074066960_value_0[3] = *(u8*)(hdr_1 + 15);
            vector_register_1074066960_value_0[4] = *(u8*)(hdr_1 + 16);
            vector_register_1074066960_value_0[5] = *(u8*)(hdr_1 + 17);
            vector_register_1074066960_value_0[6] = *(u8*)(hdr_1 + 18);
            vector_register_1074066960_value_0[7] = *(u8*)(hdr_1 + 19);
            vector_register_1074066960_value_0[8] = *(u8*)(hdr_2 + 0);
            vector_register_1074066960_value_0[9] = *(u8*)(hdr_2 + 1);
            vector_register_1074066960_value_0[10] = *(u8*)(hdr_2 + 2);
            vector_register_1074066960_value_0[11] = *(u8*)(hdr_2 + 3);
            state->vector_register_1074066960.put(value_2, vector_register_1074066960_value_0);
            // EP node  8663
            // BDD node 274:nf_set_rte_ipv4_udptcp_checksum(ip_header:(w64 1073763616), l4_header:(w64 1073763872), packet:(w64 1074155272))
            trigger_update_ipv4_tcpudp_checksums = true;
            l3_hdr = (void *)hdr_1;
            l4_hdr = (void *)hdr_2;
            // EP node  8664
            // BDD node 275:vector_borrow(vector:(w64 1074102760), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074225152)[ -> (w64 1074116656)])
            buffer_t value_3;
            state->vector_table_1074102760.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_3);
            // EP node  8749
            // BDD node 276:vector_return(vector:(w64 1074102760), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074116656)[(ReadLSB w16 (w32 0) vector_data__175)])
            // EP node  8921
            // BDD node 277:packet_return_chunk(p:(w64 1074206216), the_chunk:(w64 1073763872)[(Concat w32 (Read w8 (w32 515) packet_chunks) (Concat w24 (Read w8 (w32 514) packet_chunks) (ReadLSB w16 (w32 0) new_index__265)))])
            hdr_2[0] = value_2 & 255;
            hdr_2[1] = (value_2>>8) & 255;
            // EP node  9009
            // BDD node 278:packet_return_chunk(p:(w64 1074206216), the_chunk:(w64 1073763616)[(Concat w160 (Read w8 (w32 275) packet_chunks) (Concat w152 (Read w8 (w32 274) packet_chunks) (Concat w144 (Read w8 (w32 273) packet_chunks) (Concat w136 (Read w8 (w32 272) packet_chunks) (Concat w128 (w8 4) (Concat w120 (w8 3) (Concat w112 (w8 2) (Concat w104 (w8 1) (Concat w96 (Read w8 (w32 1) checksum__174) (Concat w88 (Read w8 (w32 0) checksum__174) (ReadLSB w80 (w32 256) packet_chunks)))))))))))])
            hdr_1[12] = 1;
            hdr_1[13] = 2;
            hdr_1[14] = 3;
            hdr_1[15] = 4;
            // EP node  9186
            // BDD node 280:FORWARD
            cpu_hdr->egress_dev = bswap16((u16)value_3.get(0, 2));
          }
        }
      } else {
        // EP node  6147
        // BDD node 281:tofino_force_send_to_controller()
        // EP node  6148
        // BDD node 281:tofino_force_send_to_controller()
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
