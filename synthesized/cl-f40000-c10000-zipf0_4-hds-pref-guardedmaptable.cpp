#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  GuardedMapTable guarded_map_table_1074048576;
  DchainTable dchain_table_1074080560;
  CountMinSketch cms_1074080976;
  VectorTable vector_table_1074093552;
  VectorTable vector_table_1074110768;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      guarded_map_table_1074048576("guarded_map_table_1074048576",{"Ingress.guarded_map_table_1074048576_144",},"Ingress.guarded_map_table_1074048576_guard", 100LL),
      dchain_table_1074080560("dchain_table_1074080560",{"Ingress.dchain_table_1074080560_174",}, 100LL),
      cms_1074080976("cms_1074080976",{"Ingress.cms_1074080976_row_0", "Ingress.cms_1074080976_row_1", "Ingress.cms_1074080976_row_2", "Ingress.cms_1074080976_row_3", }, 10LL),
      vector_table_1074093552("vector_table_1074093552",{"Ingress.vector_table_1074093552_141",}),
      vector_table_1074110768("vector_table_1074110768",{"Ingress.vector_table_1074110768_183","Ingress.vector_table_1074110768_175",})
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
  // BDD node 0:map_allocate(capacity:(w32 65536), key_size:(w32 13), map_out:(w64 1074048304)[(w64 0) -> (w64 1074048576)])
  // Module DataplaneGuardedMapTableAllocate
  // BDD node 2:dchain_allocate(index_range:(w32 65536), chain_out:(w64 1074048320)[ -> (w64 1074080560)])
  // Module DataplaneDchainTableAllocate
  // BDD node 3:cms_allocate(height:(w32 4), width:(w32 1024), key_size:(w16 8), cms_out:(w64 1074048328)[(w64 0) -> (w64 1074080976)], cleanup_interval:(w64 10000000))
  // Module DataplaneCMSAllocate
  // BDD node 4:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1074048336)[(w64 0) -> (w64 1074093552)])
  // Module DataplaneVectorTableAllocate
  // BDD node 5:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074048344)[(w64 0) -> (w64 1074110768)])
  // Module DataplaneVectorTableAllocate
  // BDD node 6:vector_borrow(vector:(w64 1074093552), index:(w32 0), val_out:(w64 1074048176)[ -> (w64 1074107448)])
  // Module Ignore
  // BDD node 7:vector_return(vector:(w64 1074093552), index:(w32 0), value:(w64 1074107448)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_0(4);
  vector_table_1074093552_value_0[0] = 0;
  vector_table_1074093552_value_0[1] = 0;
  vector_table_1074093552_value_0[2] = 0;
  vector_table_1074093552_value_0[3] = 1;
  state->vector_table_1074093552.write(0, vector_table_1074093552_value_0);
  // BDD node 8:vector_borrow(vector:(w64 1074110768), index:(w32 0), val_out:(w64 1074048240)[ -> (w64 1074124664)])
  // Module Ignore
  // BDD node 9:vector_return(vector:(w64 1074110768), index:(w32 0), value:(w64 1074124664)[(w16 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_0(2);
  vector_table_1074110768_value_0[0] = 0;
  vector_table_1074110768_value_0[1] = 1;
  state->vector_table_1074110768.write(0, vector_table_1074110768_value_0);
  // BDD node 10:vector_borrow(vector:(w64 1074093552), index:(w32 1), val_out:(w64 1074048176)[ -> (w64 1074107472)])
  // Module Ignore
  // BDD node 11:vector_return(vector:(w64 1074093552), index:(w32 1), value:(w64 1074107472)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_1(4);
  vector_table_1074093552_value_1[0] = 0;
  vector_table_1074093552_value_1[1] = 0;
  vector_table_1074093552_value_1[2] = 0;
  vector_table_1074093552_value_1[3] = 0;
  state->vector_table_1074093552.write(1, vector_table_1074093552_value_1);
  // BDD node 12:vector_borrow(vector:(w64 1074110768), index:(w32 1), val_out:(w64 1074048240)[ -> (w64 1074124688)])
  // Module Ignore
  // BDD node 13:vector_return(vector:(w64 1074110768), index:(w32 1), value:(w64 1074124688)[(w16 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_1(2);
  vector_table_1074110768_value_1[0] = 0;
  vector_table_1074110768_value_1[1] = 0;
  state->vector_table_1074110768.write(1, vector_table_1074110768_value_1);
  // BDD node 14:vector_borrow(vector:(w64 1074093552), index:(w32 2), val_out:(w64 1074048176)[ -> (w64 1074107496)])
  // Module Ignore
  // BDD node 15:vector_return(vector:(w64 1074093552), index:(w32 2), value:(w64 1074107496)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_2(4);
  vector_table_1074093552_value_2[0] = 0;
  vector_table_1074093552_value_2[1] = 0;
  vector_table_1074093552_value_2[2] = 0;
  vector_table_1074093552_value_2[3] = 1;
  state->vector_table_1074093552.write(2, vector_table_1074093552_value_2);
  // BDD node 16:vector_borrow(vector:(w64 1074110768), index:(w32 2), val_out:(w64 1074048240)[ -> (w64 1074124712)])
  // Module Ignore
  // BDD node 17:vector_return(vector:(w64 1074110768), index:(w32 2), value:(w64 1074124712)[(w16 3)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_2(2);
  vector_table_1074110768_value_2[0] = 0;
  vector_table_1074110768_value_2[1] = 3;
  state->vector_table_1074110768.write(2, vector_table_1074110768_value_2);
  // BDD node 18:vector_borrow(vector:(w64 1074093552), index:(w32 3), val_out:(w64 1074048176)[ -> (w64 1074107520)])
  // Module Ignore
  // BDD node 19:vector_return(vector:(w64 1074093552), index:(w32 3), value:(w64 1074107520)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_3(4);
  vector_table_1074093552_value_3[0] = 0;
  vector_table_1074093552_value_3[1] = 0;
  vector_table_1074093552_value_3[2] = 0;
  vector_table_1074093552_value_3[3] = 0;
  state->vector_table_1074093552.write(3, vector_table_1074093552_value_3);
  // BDD node 20:vector_borrow(vector:(w64 1074110768), index:(w32 3), val_out:(w64 1074048240)[ -> (w64 1074124736)])
  // Module Ignore
  // BDD node 21:vector_return(vector:(w64 1074110768), index:(w32 3), value:(w64 1074124736)[(w16 2)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_3(2);
  vector_table_1074110768_value_3[0] = 0;
  vector_table_1074110768_value_3[1] = 2;
  state->vector_table_1074110768.write(3, vector_table_1074110768_value_3);
  // BDD node 22:vector_borrow(vector:(w64 1074093552), index:(w32 4), val_out:(w64 1074048176)[ -> (w64 1074107544)])
  // Module Ignore
  // BDD node 23:vector_return(vector:(w64 1074093552), index:(w32 4), value:(w64 1074107544)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_4(4);
  vector_table_1074093552_value_4[0] = 0;
  vector_table_1074093552_value_4[1] = 0;
  vector_table_1074093552_value_4[2] = 0;
  vector_table_1074093552_value_4[3] = 1;
  state->vector_table_1074093552.write(4, vector_table_1074093552_value_4);
  // BDD node 24:vector_borrow(vector:(w64 1074110768), index:(w32 4), val_out:(w64 1074048240)[ -> (w64 1074124760)])
  // Module Ignore
  // BDD node 25:vector_return(vector:(w64 1074110768), index:(w32 4), value:(w64 1074124760)[(w16 5)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_4(2);
  vector_table_1074110768_value_4[0] = 0;
  vector_table_1074110768_value_4[1] = 5;
  state->vector_table_1074110768.write(4, vector_table_1074110768_value_4);
  // BDD node 26:vector_borrow(vector:(w64 1074093552), index:(w32 5), val_out:(w64 1074048176)[ -> (w64 1074107568)])
  // Module Ignore
  // BDD node 27:vector_return(vector:(w64 1074093552), index:(w32 5), value:(w64 1074107568)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_5(4);
  vector_table_1074093552_value_5[0] = 0;
  vector_table_1074093552_value_5[1] = 0;
  vector_table_1074093552_value_5[2] = 0;
  vector_table_1074093552_value_5[3] = 0;
  state->vector_table_1074093552.write(5, vector_table_1074093552_value_5);
  // BDD node 28:vector_borrow(vector:(w64 1074110768), index:(w32 5), val_out:(w64 1074048240)[ -> (w64 1074124784)])
  // Module Ignore
  // BDD node 29:vector_return(vector:(w64 1074110768), index:(w32 5), value:(w64 1074124784)[(w16 4)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_5(2);
  vector_table_1074110768_value_5[0] = 0;
  vector_table_1074110768_value_5[1] = 4;
  state->vector_table_1074110768.write(5, vector_table_1074110768_value_5);
  // BDD node 30:vector_borrow(vector:(w64 1074093552), index:(w32 6), val_out:(w64 1074048176)[ -> (w64 1074107592)])
  // Module Ignore
  // BDD node 31:vector_return(vector:(w64 1074093552), index:(w32 6), value:(w64 1074107592)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_6(4);
  vector_table_1074093552_value_6[0] = 0;
  vector_table_1074093552_value_6[1] = 0;
  vector_table_1074093552_value_6[2] = 0;
  vector_table_1074093552_value_6[3] = 1;
  state->vector_table_1074093552.write(6, vector_table_1074093552_value_6);
  // BDD node 32:vector_borrow(vector:(w64 1074110768), index:(w32 6), val_out:(w64 1074048240)[ -> (w64 1074124808)])
  // Module Ignore
  // BDD node 33:vector_return(vector:(w64 1074110768), index:(w32 6), value:(w64 1074124808)[(w16 7)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_6(2);
  vector_table_1074110768_value_6[0] = 0;
  vector_table_1074110768_value_6[1] = 7;
  state->vector_table_1074110768.write(6, vector_table_1074110768_value_6);
  // BDD node 34:vector_borrow(vector:(w64 1074093552), index:(w32 7), val_out:(w64 1074048176)[ -> (w64 1074107616)])
  // Module Ignore
  // BDD node 35:vector_return(vector:(w64 1074093552), index:(w32 7), value:(w64 1074107616)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_7(4);
  vector_table_1074093552_value_7[0] = 0;
  vector_table_1074093552_value_7[1] = 0;
  vector_table_1074093552_value_7[2] = 0;
  vector_table_1074093552_value_7[3] = 0;
  state->vector_table_1074093552.write(7, vector_table_1074093552_value_7);
  // BDD node 36:vector_borrow(vector:(w64 1074110768), index:(w32 7), val_out:(w64 1074048240)[ -> (w64 1074124832)])
  // Module Ignore
  // BDD node 37:vector_return(vector:(w64 1074110768), index:(w32 7), value:(w64 1074124832)[(w16 6)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_7(2);
  vector_table_1074110768_value_7[0] = 0;
  vector_table_1074110768_value_7[1] = 6;
  state->vector_table_1074110768.write(7, vector_table_1074110768_value_7);
  // BDD node 38:vector_borrow(vector:(w64 1074093552), index:(w32 8), val_out:(w64 1074048176)[ -> (w64 1074107640)])
  // Module Ignore
  // BDD node 39:vector_return(vector:(w64 1074093552), index:(w32 8), value:(w64 1074107640)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_8(4);
  vector_table_1074093552_value_8[0] = 0;
  vector_table_1074093552_value_8[1] = 0;
  vector_table_1074093552_value_8[2] = 0;
  vector_table_1074093552_value_8[3] = 1;
  state->vector_table_1074093552.write(8, vector_table_1074093552_value_8);
  // BDD node 40:vector_borrow(vector:(w64 1074110768), index:(w32 8), val_out:(w64 1074048240)[ -> (w64 1074124856)])
  // Module Ignore
  // BDD node 41:vector_return(vector:(w64 1074110768), index:(w32 8), value:(w64 1074124856)[(w16 9)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_8(2);
  vector_table_1074110768_value_8[0] = 0;
  vector_table_1074110768_value_8[1] = 9;
  state->vector_table_1074110768.write(8, vector_table_1074110768_value_8);
  // BDD node 42:vector_borrow(vector:(w64 1074093552), index:(w32 9), val_out:(w64 1074048176)[ -> (w64 1074107664)])
  // Module Ignore
  // BDD node 43:vector_return(vector:(w64 1074093552), index:(w32 9), value:(w64 1074107664)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_9(4);
  vector_table_1074093552_value_9[0] = 0;
  vector_table_1074093552_value_9[1] = 0;
  vector_table_1074093552_value_9[2] = 0;
  vector_table_1074093552_value_9[3] = 0;
  state->vector_table_1074093552.write(9, vector_table_1074093552_value_9);
  // BDD node 44:vector_borrow(vector:(w64 1074110768), index:(w32 9), val_out:(w64 1074048240)[ -> (w64 1074124880)])
  // Module Ignore
  // BDD node 45:vector_return(vector:(w64 1074110768), index:(w32 9), value:(w64 1074124880)[(w16 8)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_9(2);
  vector_table_1074110768_value_9[0] = 0;
  vector_table_1074110768_value_9[1] = 8;
  state->vector_table_1074110768.write(9, vector_table_1074110768_value_9);
  // BDD node 46:vector_borrow(vector:(w64 1074093552), index:(w32 10), val_out:(w64 1074048176)[ -> (w64 1074107688)])
  // Module Ignore
  // BDD node 47:vector_return(vector:(w64 1074093552), index:(w32 10), value:(w64 1074107688)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_10(4);
  vector_table_1074093552_value_10[0] = 0;
  vector_table_1074093552_value_10[1] = 0;
  vector_table_1074093552_value_10[2] = 0;
  vector_table_1074093552_value_10[3] = 1;
  state->vector_table_1074093552.write(10, vector_table_1074093552_value_10);
  // BDD node 48:vector_borrow(vector:(w64 1074110768), index:(w32 10), val_out:(w64 1074048240)[ -> (w64 1074124904)])
  // Module Ignore
  // BDD node 49:vector_return(vector:(w64 1074110768), index:(w32 10), value:(w64 1074124904)[(w16 11)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_10(2);
  vector_table_1074110768_value_10[0] = 0;
  vector_table_1074110768_value_10[1] = 11;
  state->vector_table_1074110768.write(10, vector_table_1074110768_value_10);
  // BDD node 50:vector_borrow(vector:(w64 1074093552), index:(w32 11), val_out:(w64 1074048176)[ -> (w64 1074107712)])
  // Module Ignore
  // BDD node 51:vector_return(vector:(w64 1074093552), index:(w32 11), value:(w64 1074107712)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_11(4);
  vector_table_1074093552_value_11[0] = 0;
  vector_table_1074093552_value_11[1] = 0;
  vector_table_1074093552_value_11[2] = 0;
  vector_table_1074093552_value_11[3] = 0;
  state->vector_table_1074093552.write(11, vector_table_1074093552_value_11);
  // BDD node 52:vector_borrow(vector:(w64 1074110768), index:(w32 11), val_out:(w64 1074048240)[ -> (w64 1074124928)])
  // Module Ignore
  // BDD node 53:vector_return(vector:(w64 1074110768), index:(w32 11), value:(w64 1074124928)[(w16 10)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_11(2);
  vector_table_1074110768_value_11[0] = 0;
  vector_table_1074110768_value_11[1] = 10;
  state->vector_table_1074110768.write(11, vector_table_1074110768_value_11);
  // BDD node 54:vector_borrow(vector:(w64 1074093552), index:(w32 12), val_out:(w64 1074048176)[ -> (w64 1074107736)])
  // Module Ignore
  // BDD node 55:vector_return(vector:(w64 1074093552), index:(w32 12), value:(w64 1074107736)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_12(4);
  vector_table_1074093552_value_12[0] = 0;
  vector_table_1074093552_value_12[1] = 0;
  vector_table_1074093552_value_12[2] = 0;
  vector_table_1074093552_value_12[3] = 1;
  state->vector_table_1074093552.write(12, vector_table_1074093552_value_12);
  // BDD node 56:vector_borrow(vector:(w64 1074110768), index:(w32 12), val_out:(w64 1074048240)[ -> (w64 1074124952)])
  // Module Ignore
  // BDD node 57:vector_return(vector:(w64 1074110768), index:(w32 12), value:(w64 1074124952)[(w16 13)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_12(2);
  vector_table_1074110768_value_12[0] = 0;
  vector_table_1074110768_value_12[1] = 13;
  state->vector_table_1074110768.write(12, vector_table_1074110768_value_12);
  // BDD node 58:vector_borrow(vector:(w64 1074093552), index:(w32 13), val_out:(w64 1074048176)[ -> (w64 1074107760)])
  // Module Ignore
  // BDD node 59:vector_return(vector:(w64 1074093552), index:(w32 13), value:(w64 1074107760)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_13(4);
  vector_table_1074093552_value_13[0] = 0;
  vector_table_1074093552_value_13[1] = 0;
  vector_table_1074093552_value_13[2] = 0;
  vector_table_1074093552_value_13[3] = 0;
  state->vector_table_1074093552.write(13, vector_table_1074093552_value_13);
  // BDD node 60:vector_borrow(vector:(w64 1074110768), index:(w32 13), val_out:(w64 1074048240)[ -> (w64 1074124976)])
  // Module Ignore
  // BDD node 61:vector_return(vector:(w64 1074110768), index:(w32 13), value:(w64 1074124976)[(w16 12)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_13(2);
  vector_table_1074110768_value_13[0] = 0;
  vector_table_1074110768_value_13[1] = 12;
  state->vector_table_1074110768.write(13, vector_table_1074110768_value_13);
  // BDD node 62:vector_borrow(vector:(w64 1074093552), index:(w32 14), val_out:(w64 1074048176)[ -> (w64 1074107784)])
  // Module Ignore
  // BDD node 63:vector_return(vector:(w64 1074093552), index:(w32 14), value:(w64 1074107784)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_14(4);
  vector_table_1074093552_value_14[0] = 0;
  vector_table_1074093552_value_14[1] = 0;
  vector_table_1074093552_value_14[2] = 0;
  vector_table_1074093552_value_14[3] = 1;
  state->vector_table_1074093552.write(14, vector_table_1074093552_value_14);
  // BDD node 64:vector_borrow(vector:(w64 1074110768), index:(w32 14), val_out:(w64 1074048240)[ -> (w64 1074125000)])
  // Module Ignore
  // BDD node 65:vector_return(vector:(w64 1074110768), index:(w32 14), value:(w64 1074125000)[(w16 15)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_14(2);
  vector_table_1074110768_value_14[0] = 0;
  vector_table_1074110768_value_14[1] = 15;
  state->vector_table_1074110768.write(14, vector_table_1074110768_value_14);
  // BDD node 66:vector_borrow(vector:(w64 1074093552), index:(w32 15), val_out:(w64 1074048176)[ -> (w64 1074107808)])
  // Module Ignore
  // BDD node 67:vector_return(vector:(w64 1074093552), index:(w32 15), value:(w64 1074107808)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_15(4);
  vector_table_1074093552_value_15[0] = 0;
  vector_table_1074093552_value_15[1] = 0;
  vector_table_1074093552_value_15[2] = 0;
  vector_table_1074093552_value_15[3] = 0;
  state->vector_table_1074093552.write(15, vector_table_1074093552_value_15);
  // BDD node 68:vector_borrow(vector:(w64 1074110768), index:(w32 15), val_out:(w64 1074048240)[ -> (w64 1074125024)])
  // Module Ignore
  // BDD node 69:vector_return(vector:(w64 1074110768), index:(w32 15), value:(w64 1074125024)[(w16 14)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_15(2);
  vector_table_1074110768_value_15[0] = 0;
  vector_table_1074110768_value_15[1] = 14;
  state->vector_table_1074110768.write(15, vector_table_1074110768_value_15);
  // BDD node 70:vector_borrow(vector:(w64 1074093552), index:(w32 16), val_out:(w64 1074048176)[ -> (w64 1074107832)])
  // Module Ignore
  // BDD node 71:vector_return(vector:(w64 1074093552), index:(w32 16), value:(w64 1074107832)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_16(4);
  vector_table_1074093552_value_16[0] = 0;
  vector_table_1074093552_value_16[1] = 0;
  vector_table_1074093552_value_16[2] = 0;
  vector_table_1074093552_value_16[3] = 1;
  state->vector_table_1074093552.write(16, vector_table_1074093552_value_16);
  // BDD node 72:vector_borrow(vector:(w64 1074110768), index:(w32 16), val_out:(w64 1074048240)[ -> (w64 1074125048)])
  // Module Ignore
  // BDD node 73:vector_return(vector:(w64 1074110768), index:(w32 16), value:(w64 1074125048)[(w16 17)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_16(2);
  vector_table_1074110768_value_16[0] = 0;
  vector_table_1074110768_value_16[1] = 17;
  state->vector_table_1074110768.write(16, vector_table_1074110768_value_16);
  // BDD node 74:vector_borrow(vector:(w64 1074093552), index:(w32 17), val_out:(w64 1074048176)[ -> (w64 1074107856)])
  // Module Ignore
  // BDD node 75:vector_return(vector:(w64 1074093552), index:(w32 17), value:(w64 1074107856)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_17(4);
  vector_table_1074093552_value_17[0] = 0;
  vector_table_1074093552_value_17[1] = 0;
  vector_table_1074093552_value_17[2] = 0;
  vector_table_1074093552_value_17[3] = 0;
  state->vector_table_1074093552.write(17, vector_table_1074093552_value_17);
  // BDD node 76:vector_borrow(vector:(w64 1074110768), index:(w32 17), val_out:(w64 1074048240)[ -> (w64 1074125072)])
  // Module Ignore
  // BDD node 77:vector_return(vector:(w64 1074110768), index:(w32 17), value:(w64 1074125072)[(w16 16)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_17(2);
  vector_table_1074110768_value_17[0] = 0;
  vector_table_1074110768_value_17[1] = 16;
  state->vector_table_1074110768.write(17, vector_table_1074110768_value_17);
  // BDD node 78:vector_borrow(vector:(w64 1074093552), index:(w32 18), val_out:(w64 1074048176)[ -> (w64 1074107880)])
  // Module Ignore
  // BDD node 79:vector_return(vector:(w64 1074093552), index:(w32 18), value:(w64 1074107880)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_18(4);
  vector_table_1074093552_value_18[0] = 0;
  vector_table_1074093552_value_18[1] = 0;
  vector_table_1074093552_value_18[2] = 0;
  vector_table_1074093552_value_18[3] = 1;
  state->vector_table_1074093552.write(18, vector_table_1074093552_value_18);
  // BDD node 80:vector_borrow(vector:(w64 1074110768), index:(w32 18), val_out:(w64 1074048240)[ -> (w64 1074125096)])
  // Module Ignore
  // BDD node 81:vector_return(vector:(w64 1074110768), index:(w32 18), value:(w64 1074125096)[(w16 19)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_18(2);
  vector_table_1074110768_value_18[0] = 0;
  vector_table_1074110768_value_18[1] = 19;
  state->vector_table_1074110768.write(18, vector_table_1074110768_value_18);
  // BDD node 82:vector_borrow(vector:(w64 1074093552), index:(w32 19), val_out:(w64 1074048176)[ -> (w64 1074107904)])
  // Module Ignore
  // BDD node 83:vector_return(vector:(w64 1074093552), index:(w32 19), value:(w64 1074107904)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_19(4);
  vector_table_1074093552_value_19[0] = 0;
  vector_table_1074093552_value_19[1] = 0;
  vector_table_1074093552_value_19[2] = 0;
  vector_table_1074093552_value_19[3] = 0;
  state->vector_table_1074093552.write(19, vector_table_1074093552_value_19);
  // BDD node 84:vector_borrow(vector:(w64 1074110768), index:(w32 19), val_out:(w64 1074048240)[ -> (w64 1074125120)])
  // Module Ignore
  // BDD node 85:vector_return(vector:(w64 1074110768), index:(w32 19), value:(w64 1074125120)[(w16 18)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_19(2);
  vector_table_1074110768_value_19[0] = 0;
  vector_table_1074110768_value_19[1] = 18;
  state->vector_table_1074110768.write(19, vector_table_1074110768_value_19);
  // BDD node 86:vector_borrow(vector:(w64 1074093552), index:(w32 20), val_out:(w64 1074048176)[ -> (w64 1074107928)])
  // Module Ignore
  // BDD node 87:vector_return(vector:(w64 1074093552), index:(w32 20), value:(w64 1074107928)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_20(4);
  vector_table_1074093552_value_20[0] = 0;
  vector_table_1074093552_value_20[1] = 0;
  vector_table_1074093552_value_20[2] = 0;
  vector_table_1074093552_value_20[3] = 1;
  state->vector_table_1074093552.write(20, vector_table_1074093552_value_20);
  // BDD node 88:vector_borrow(vector:(w64 1074110768), index:(w32 20), val_out:(w64 1074048240)[ -> (w64 1074125144)])
  // Module Ignore
  // BDD node 89:vector_return(vector:(w64 1074110768), index:(w32 20), value:(w64 1074125144)[(w16 21)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_20(2);
  vector_table_1074110768_value_20[0] = 0;
  vector_table_1074110768_value_20[1] = 21;
  state->vector_table_1074110768.write(20, vector_table_1074110768_value_20);
  // BDD node 90:vector_borrow(vector:(w64 1074093552), index:(w32 21), val_out:(w64 1074048176)[ -> (w64 1074107952)])
  // Module Ignore
  // BDD node 91:vector_return(vector:(w64 1074093552), index:(w32 21), value:(w64 1074107952)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_21(4);
  vector_table_1074093552_value_21[0] = 0;
  vector_table_1074093552_value_21[1] = 0;
  vector_table_1074093552_value_21[2] = 0;
  vector_table_1074093552_value_21[3] = 0;
  state->vector_table_1074093552.write(21, vector_table_1074093552_value_21);
  // BDD node 92:vector_borrow(vector:(w64 1074110768), index:(w32 21), val_out:(w64 1074048240)[ -> (w64 1074125168)])
  // Module Ignore
  // BDD node 93:vector_return(vector:(w64 1074110768), index:(w32 21), value:(w64 1074125168)[(w16 20)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_21(2);
  vector_table_1074110768_value_21[0] = 0;
  vector_table_1074110768_value_21[1] = 20;
  state->vector_table_1074110768.write(21, vector_table_1074110768_value_21);
  // BDD node 94:vector_borrow(vector:(w64 1074093552), index:(w32 22), val_out:(w64 1074048176)[ -> (w64 1074107976)])
  // Module Ignore
  // BDD node 95:vector_return(vector:(w64 1074093552), index:(w32 22), value:(w64 1074107976)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_22(4);
  vector_table_1074093552_value_22[0] = 0;
  vector_table_1074093552_value_22[1] = 0;
  vector_table_1074093552_value_22[2] = 0;
  vector_table_1074093552_value_22[3] = 1;
  state->vector_table_1074093552.write(22, vector_table_1074093552_value_22);
  // BDD node 96:vector_borrow(vector:(w64 1074110768), index:(w32 22), val_out:(w64 1074048240)[ -> (w64 1074125192)])
  // Module Ignore
  // BDD node 97:vector_return(vector:(w64 1074110768), index:(w32 22), value:(w64 1074125192)[(w16 23)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_22(2);
  vector_table_1074110768_value_22[0] = 0;
  vector_table_1074110768_value_22[1] = 23;
  state->vector_table_1074110768.write(22, vector_table_1074110768_value_22);
  // BDD node 98:vector_borrow(vector:(w64 1074093552), index:(w32 23), val_out:(w64 1074048176)[ -> (w64 1074108000)])
  // Module Ignore
  // BDD node 99:vector_return(vector:(w64 1074093552), index:(w32 23), value:(w64 1074108000)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_23(4);
  vector_table_1074093552_value_23[0] = 0;
  vector_table_1074093552_value_23[1] = 0;
  vector_table_1074093552_value_23[2] = 0;
  vector_table_1074093552_value_23[3] = 0;
  state->vector_table_1074093552.write(23, vector_table_1074093552_value_23);
  // BDD node 100:vector_borrow(vector:(w64 1074110768), index:(w32 23), val_out:(w64 1074048240)[ -> (w64 1074125216)])
  // Module Ignore
  // BDD node 101:vector_return(vector:(w64 1074110768), index:(w32 23), value:(w64 1074125216)[(w16 22)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_23(2);
  vector_table_1074110768_value_23[0] = 0;
  vector_table_1074110768_value_23[1] = 22;
  state->vector_table_1074110768.write(23, vector_table_1074110768_value_23);
  // BDD node 102:vector_borrow(vector:(w64 1074093552), index:(w32 24), val_out:(w64 1074048176)[ -> (w64 1074108024)])
  // Module Ignore
  // BDD node 103:vector_return(vector:(w64 1074093552), index:(w32 24), value:(w64 1074108024)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_24(4);
  vector_table_1074093552_value_24[0] = 0;
  vector_table_1074093552_value_24[1] = 0;
  vector_table_1074093552_value_24[2] = 0;
  vector_table_1074093552_value_24[3] = 1;
  state->vector_table_1074093552.write(24, vector_table_1074093552_value_24);
  // BDD node 104:vector_borrow(vector:(w64 1074110768), index:(w32 24), val_out:(w64 1074048240)[ -> (w64 1074125240)])
  // Module Ignore
  // BDD node 105:vector_return(vector:(w64 1074110768), index:(w32 24), value:(w64 1074125240)[(w16 25)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_24(2);
  vector_table_1074110768_value_24[0] = 0;
  vector_table_1074110768_value_24[1] = 25;
  state->vector_table_1074110768.write(24, vector_table_1074110768_value_24);
  // BDD node 106:vector_borrow(vector:(w64 1074093552), index:(w32 25), val_out:(w64 1074048176)[ -> (w64 1074108048)])
  // Module Ignore
  // BDD node 107:vector_return(vector:(w64 1074093552), index:(w32 25), value:(w64 1074108048)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_25(4);
  vector_table_1074093552_value_25[0] = 0;
  vector_table_1074093552_value_25[1] = 0;
  vector_table_1074093552_value_25[2] = 0;
  vector_table_1074093552_value_25[3] = 0;
  state->vector_table_1074093552.write(25, vector_table_1074093552_value_25);
  // BDD node 108:vector_borrow(vector:(w64 1074110768), index:(w32 25), val_out:(w64 1074048240)[ -> (w64 1074125264)])
  // Module Ignore
  // BDD node 109:vector_return(vector:(w64 1074110768), index:(w32 25), value:(w64 1074125264)[(w16 24)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_25(2);
  vector_table_1074110768_value_25[0] = 0;
  vector_table_1074110768_value_25[1] = 24;
  state->vector_table_1074110768.write(25, vector_table_1074110768_value_25);
  // BDD node 110:vector_borrow(vector:(w64 1074093552), index:(w32 26), val_out:(w64 1074048176)[ -> (w64 1074108072)])
  // Module Ignore
  // BDD node 111:vector_return(vector:(w64 1074093552), index:(w32 26), value:(w64 1074108072)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_26(4);
  vector_table_1074093552_value_26[0] = 0;
  vector_table_1074093552_value_26[1] = 0;
  vector_table_1074093552_value_26[2] = 0;
  vector_table_1074093552_value_26[3] = 1;
  state->vector_table_1074093552.write(26, vector_table_1074093552_value_26);
  // BDD node 112:vector_borrow(vector:(w64 1074110768), index:(w32 26), val_out:(w64 1074048240)[ -> (w64 1074125288)])
  // Module Ignore
  // BDD node 113:vector_return(vector:(w64 1074110768), index:(w32 26), value:(w64 1074125288)[(w16 27)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_26(2);
  vector_table_1074110768_value_26[0] = 0;
  vector_table_1074110768_value_26[1] = 27;
  state->vector_table_1074110768.write(26, vector_table_1074110768_value_26);
  // BDD node 114:vector_borrow(vector:(w64 1074093552), index:(w32 27), val_out:(w64 1074048176)[ -> (w64 1074108096)])
  // Module Ignore
  // BDD node 115:vector_return(vector:(w64 1074093552), index:(w32 27), value:(w64 1074108096)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_27(4);
  vector_table_1074093552_value_27[0] = 0;
  vector_table_1074093552_value_27[1] = 0;
  vector_table_1074093552_value_27[2] = 0;
  vector_table_1074093552_value_27[3] = 0;
  state->vector_table_1074093552.write(27, vector_table_1074093552_value_27);
  // BDD node 116:vector_borrow(vector:(w64 1074110768), index:(w32 27), val_out:(w64 1074048240)[ -> (w64 1074125312)])
  // Module Ignore
  // BDD node 117:vector_return(vector:(w64 1074110768), index:(w32 27), value:(w64 1074125312)[(w16 26)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_27(2);
  vector_table_1074110768_value_27[0] = 0;
  vector_table_1074110768_value_27[1] = 26;
  state->vector_table_1074110768.write(27, vector_table_1074110768_value_27);
  // BDD node 118:vector_borrow(vector:(w64 1074093552), index:(w32 28), val_out:(w64 1074048176)[ -> (w64 1074108120)])
  // Module Ignore
  // BDD node 119:vector_return(vector:(w64 1074093552), index:(w32 28), value:(w64 1074108120)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_28(4);
  vector_table_1074093552_value_28[0] = 0;
  vector_table_1074093552_value_28[1] = 0;
  vector_table_1074093552_value_28[2] = 0;
  vector_table_1074093552_value_28[3] = 1;
  state->vector_table_1074093552.write(28, vector_table_1074093552_value_28);
  // BDD node 120:vector_borrow(vector:(w64 1074110768), index:(w32 28), val_out:(w64 1074048240)[ -> (w64 1074125336)])
  // Module Ignore
  // BDD node 121:vector_return(vector:(w64 1074110768), index:(w32 28), value:(w64 1074125336)[(w16 29)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_28(2);
  vector_table_1074110768_value_28[0] = 0;
  vector_table_1074110768_value_28[1] = 29;
  state->vector_table_1074110768.write(28, vector_table_1074110768_value_28);
  // BDD node 122:vector_borrow(vector:(w64 1074093552), index:(w32 29), val_out:(w64 1074048176)[ -> (w64 1074108144)])
  // Module Ignore
  // BDD node 123:vector_return(vector:(w64 1074093552), index:(w32 29), value:(w64 1074108144)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_29(4);
  vector_table_1074093552_value_29[0] = 0;
  vector_table_1074093552_value_29[1] = 0;
  vector_table_1074093552_value_29[2] = 0;
  vector_table_1074093552_value_29[3] = 0;
  state->vector_table_1074093552.write(29, vector_table_1074093552_value_29);
  // BDD node 124:vector_borrow(vector:(w64 1074110768), index:(w32 29), val_out:(w64 1074048240)[ -> (w64 1074125360)])
  // Module Ignore
  // BDD node 125:vector_return(vector:(w64 1074110768), index:(w32 29), value:(w64 1074125360)[(w16 28)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_29(2);
  vector_table_1074110768_value_29[0] = 0;
  vector_table_1074110768_value_29[1] = 28;
  state->vector_table_1074110768.write(29, vector_table_1074110768_value_29);
  // BDD node 126:vector_borrow(vector:(w64 1074093552), index:(w32 30), val_out:(w64 1074048176)[ -> (w64 1074108168)])
  // Module Ignore
  // BDD node 127:vector_return(vector:(w64 1074093552), index:(w32 30), value:(w64 1074108168)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_30(4);
  vector_table_1074093552_value_30[0] = 0;
  vector_table_1074093552_value_30[1] = 0;
  vector_table_1074093552_value_30[2] = 0;
  vector_table_1074093552_value_30[3] = 1;
  state->vector_table_1074093552.write(30, vector_table_1074093552_value_30);
  // BDD node 128:vector_borrow(vector:(w64 1074110768), index:(w32 30), val_out:(w64 1074048240)[ -> (w64 1074125384)])
  // Module Ignore
  // BDD node 129:vector_return(vector:(w64 1074110768), index:(w32 30), value:(w64 1074125384)[(w16 31)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_30(2);
  vector_table_1074110768_value_30[0] = 0;
  vector_table_1074110768_value_30[1] = 31;
  state->vector_table_1074110768.write(30, vector_table_1074110768_value_30);
  // BDD node 130:vector_borrow(vector:(w64 1074093552), index:(w32 31), val_out:(w64 1074048176)[ -> (w64 1074108192)])
  // Module Ignore
  // BDD node 131:vector_return(vector:(w64 1074093552), index:(w32 31), value:(w64 1074108192)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093552_value_31(4);
  vector_table_1074093552_value_31[0] = 0;
  vector_table_1074093552_value_31[1] = 0;
  vector_table_1074093552_value_31[2] = 0;
  vector_table_1074093552_value_31[3] = 0;
  state->vector_table_1074093552.write(31, vector_table_1074093552_value_31);
  // BDD node 132:vector_borrow(vector:(w64 1074110768), index:(w32 31), val_out:(w64 1074048240)[ -> (w64 1074125408)])
  // Module Ignore
  // BDD node 133:vector_return(vector:(w64 1074110768), index:(w32 31), value:(w64 1074125408)[(w16 30)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110768_value_31(2);
  vector_table_1074110768_value_31[0] = 0;
  vector_table_1074110768_value_31[1] = 30;
  state->vector_table_1074110768.write(31, vector_table_1074110768_value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 vector_data_512;
  u32 map_has_this_key_r6;
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

  if (bswap16(cpu_hdr->code_path) == 2558) {
    // EP node  2545
    // BDD node 147:cms_count_min(cms:(w64 1074080976), key:(w64 1074226482)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  2546
    // BDD node 147:cms_count_min(cms:(w64 1074080976), key:(w64 1074226482)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  2547
    // BDD node 147:cms_count_min(cms:(w64 1074080976), key:(w64 1074226482)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
    u8* hdr_2 = packet_consume(pkt, 4);
    // EP node  2548
    // BDD node 147:cms_count_min(cms:(w64 1074080976), key:(w64 1074226482)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
    buffer_t value_0;
    state->vector_table_1074093552.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_0);
    // EP node  2549
    // BDD node 147:cms_count_min(cms:(w64 1074080976), key:(w64 1074226482)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
    buffer_t guarded_map_table_1074048576_key_0(13);
    guarded_map_table_1074048576_key_0[0] = *(u8*)(hdr_2 + 0);
    guarded_map_table_1074048576_key_0[1] = *(u8*)(hdr_2 + 1);
    guarded_map_table_1074048576_key_0[2] = *(u8*)(hdr_2 + 2);
    guarded_map_table_1074048576_key_0[3] = *(u8*)(hdr_2 + 3);
    guarded_map_table_1074048576_key_0[4] = *(u8*)(hdr_1 + 12);
    guarded_map_table_1074048576_key_0[5] = *(u8*)(hdr_1 + 13);
    guarded_map_table_1074048576_key_0[6] = *(u8*)(hdr_1 + 14);
    guarded_map_table_1074048576_key_0[7] = *(u8*)(hdr_1 + 15);
    guarded_map_table_1074048576_key_0[8] = *(u8*)(hdr_1 + 16);
    guarded_map_table_1074048576_key_0[9] = *(u8*)(hdr_1 + 17);
    guarded_map_table_1074048576_key_0[10] = *(u8*)(hdr_1 + 18);
    guarded_map_table_1074048576_key_0[11] = *(u8*)(hdr_1 + 19);
    guarded_map_table_1074048576_key_0[12] = *(u8*)(hdr_1 + 9);
    u32 value_1;
    bool found_0 = state->guarded_map_table_1074048576.get(guarded_map_table_1074048576_key_0, value_1);
    // EP node  2550
    // BDD node 147:cms_count_min(cms:(w64 1074080976), key:(w64 1074226482)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
    if ((0) == ((u32)value_0.get(0, 4))) {
      // EP node  2551
      // BDD node 147:cms_count_min(cms:(w64 1074080976), key:(w64 1074226482)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
      // EP node  2554
      // BDD node 147:cms_count_min(cms:(w64 1074080976), key:(w64 1074226482)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
      if ((0) == (found_0)) {
        // EP node  2555
        // BDD node 147:cms_count_min(cms:(w64 1074080976), key:(w64 1074226482)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
        // EP node  4305
        // BDD node 147:cms_count_min(cms:(w64 1074080976), key:(w64 1074226482)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
        buffer_t cms_1074080976_key_0(8);
        cms_1074080976_key_0[0] = *(u8*)(hdr_1 + 12);
        cms_1074080976_key_0[1] = *(u8*)(hdr_1 + 13);
        cms_1074080976_key_0[2] = *(u8*)(hdr_1 + 14);
        cms_1074080976_key_0[3] = *(u8*)(hdr_1 + 15);
        cms_1074080976_key_0[4] = *(u8*)(hdr_1 + 16);
        cms_1074080976_key_0[5] = *(u8*)(hdr_1 + 17);
        cms_1074080976_key_0[6] = *(u8*)(hdr_1 + 18);
        cms_1074080976_key_0[7] = *(u8*)(hdr_1 + 19);
        u32 min_estimate_0 = state->cms_1074080976.count_min(cms_1074080976_key_0);
        // EP node  4359
        // BDD node 148:if ((Sle (ReadLSB w32 (w32 0) min_estimate) (w32 64))
        if ((min_estimate_0) <= (64)) {
          // EP node  4360
          // BDD node 148:if ((Sle (ReadLSB w32 (w32 0) min_estimate) (w32 64))
          // EP node  4416
          // BDD node 149:dchain_allocate_new_index(chain:(w64 1074080560), index_out:(w64 1074227048)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
          u32 allocated_index_0;
          bool success_0 = state->dchain_table_1074080560.allocate_new_index(allocated_index_0);
          // EP node  4474
          // BDD node 150:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
          if ((0) == (success_0)) {
            // EP node  4475
            // BDD node 150:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
            // EP node  5254
            // BDD node 151:vector_borrow(vector:(w64 1074110768), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074227248)[ -> (w64 1074124664)])
            buffer_t value_2;
            state->vector_table_1074110768.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_2);
            // EP node  5533
            // BDD node 152:vector_return(vector:(w64 1074110768), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074124664)[(ReadLSB w16 (w32 0) vector_data_640)])
            // EP node  5817
            // BDD node 156:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
            if ((65535) != ((u16)value_2.get(0, 2))) {
              // EP node  5818
              // BDD node 156:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
              // EP node  6719
              // BDD node 157:FORWARD
              cpu_hdr->egress_dev = bswap16((u16)value_2.get(0, 2));
            } else {
              // EP node  5819
              // BDD node 156:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
              // EP node  6720
              // BDD node 158:DROP
              result.forward = false;
            }
          } else {
            // EP node  4476
            // BDD node 150:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
            // EP node  4596
            // BDD node 160:map_put(map:(w64 1074048576), key:(w64 1074076296)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks))))))))))], value:(ReadLSB w32 (w32 0) new_index))
            buffer_t guarded_map_table_1074048576_key_1(13);
            guarded_map_table_1074048576_key_1[0] = *(u8*)(hdr_2 + 0);
            guarded_map_table_1074048576_key_1[1] = *(u8*)(hdr_2 + 1);
            guarded_map_table_1074048576_key_1[2] = *(u8*)(hdr_2 + 2);
            guarded_map_table_1074048576_key_1[3] = *(u8*)(hdr_2 + 3);
            guarded_map_table_1074048576_key_1[4] = *(u8*)(hdr_1 + 12);
            guarded_map_table_1074048576_key_1[5] = *(u8*)(hdr_1 + 13);
            guarded_map_table_1074048576_key_1[6] = *(u8*)(hdr_1 + 14);
            guarded_map_table_1074048576_key_1[7] = *(u8*)(hdr_1 + 15);
            guarded_map_table_1074048576_key_1[8] = *(u8*)(hdr_1 + 16);
            guarded_map_table_1074048576_key_1[9] = *(u8*)(hdr_1 + 17);
            guarded_map_table_1074048576_key_1[10] = *(u8*)(hdr_1 + 18);
            guarded_map_table_1074048576_key_1[11] = *(u8*)(hdr_1 + 19);
            guarded_map_table_1074048576_key_1[12] = *(u8*)(hdr_1 + 9);
            state->guarded_map_table_1074048576.put(guarded_map_table_1074048576_key_1, allocated_index_0);
            // EP node  4658
            // BDD node 162:vector_borrow(vector:(w64 1074110768), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074229480)[ -> (w64 1074124664)])
            buffer_t value_3;
            state->vector_table_1074110768.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_3);
            // EP node  4784
            // BDD node 167:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
            if ((65535) != ((u16)value_3.get(0, 2))) {
              // EP node  4785
              // BDD node 167:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
              // EP node  4850
              // BDD node 163:vector_return(vector:(w64 1074110768), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074124664)[(ReadLSB w16 (w32 0) vector_data_640)])
              // EP node  5185
              // BDD node 168:FORWARD
              cpu_hdr->egress_dev = bswap16((u16)value_3.get(0, 2));
            } else {
              // EP node  4786
              // BDD node 167:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
              // EP node  5323
              // BDD node 259:vector_return(vector:(w64 1074110768), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074124664)[(ReadLSB w16 (w32 0) vector_data_640)])
              // EP node  6415
              // BDD node 169:DROP
              result.forward = false;
            }
          }
        } else {
          // EP node  4361
          // BDD node 148:if ((Sle (ReadLSB w32 (w32 0) min_estimate) (w32 64))
          // EP node  5965
          // BDD node 173:DROP
          result.forward = false;
        }
      } else {
        // EP node  2556
        // BDD node 147:cms_count_min(cms:(w64 1074080976), key:(w64 1074226482)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
        // EP node  2557
        // BDD node 147:cms_count_min(cms:(w64 1074080976), key:(w64 1074226482)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      }
    } else {
      // EP node  2552
      // BDD node 147:cms_count_min(cms:(w64 1074080976), key:(w64 1074226482)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
      // EP node  2553
      // BDD node 147:cms_count_min(cms:(w64 1074080976), key:(w64 1074226482)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
      result.abort_transaction = true;
      cpu_hdr->trigger_dataplane_execution = 1;
      return result;
    }
  }


  if (trigger_update_ipv4_tcpudp_checksums) {
    update_ipv4_tcpudp_checksums(l3_hdr, l4_hdr);
  }

  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
