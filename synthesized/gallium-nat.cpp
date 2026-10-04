#include <sycon/sycon.h>
#include <sycon/libnf.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  GuardedMapTable guarded_map_table_1249835474944;
  VectorTable vector_table_1247687987200;
  DchainTable dchain_table_1240352161792;
  VectorTable vector_table_1251982962688;
  VectorRegister vector_register_1248761733120;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      guarded_map_table_1249835474944("guarded_map_table_1249835474944",{"Ingress.guarded_map_table_1249835474944_163",},"Ingress.guarded_map_table_1249835474944_guard", 1000LL),
      vector_table_1247687987200("vector_table_1247687987200",{"Ingress.vector_table_1247687987200_144",}),
      dchain_table_1240352161792("dchain_table_1240352161792",{"Ingress.dchain_table_1240352161792_142","Ingress.dchain_table_1240352161792_146","Ingress.dchain_table_1240352161792_181",}, 1000LL),
      vector_table_1251982962688("vector_table_1251982962688",{"Ingress.vector_table_1251982962688_139",}),
      vector_register_1248761733120("vector_register_1248761733120",{"Ingress.vector_register_1248761733120_0",})
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
  state->ingress_port_to_nf_dev.add_cpu_entry(asic_get_cpu_port());

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
  // BDD node 0:map_allocate(capacity:(w32 65536), key_size:(w32 12), map_out:(w64 1242453508096)[(w64 0) -> (w64 1249835474944)])
  // Module DataplaneGuardedMapTableAllocate
  // BDD node 1:vector_allocate(elem_size:(w32 12), capacity:(w32 65536), vector_out:(w64 1242453508104)[(w64 0) -> (w64 1247687987200)])
  // Module DataplaneVectorTableAllocate
  // BDD node 2:dchain_allocate(index_range:(w32 65536), chain_out:(w64 1242453508112)[ -> (w64 1240352161792)])
  // Module DataplaneDchainTableAllocate
  // BDD node 3:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1242453508120)[(w64 0) -> (w64 1251982962688)])
  // Module DataplaneVectorTableAllocate
  // BDD node 4:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1242453508128)[(w64 0) -> (w64 1248761733120)])
  // Module DataplaneVectorRegisterAllocate
  // BDD node 5:vector_borrow(vector:(w64 1251982962688), index:(w32 0), val_out:(w64 1649812701184)[ -> (w64 1239903371264)])
  // Module Ignore
  // BDD node 6:vector_return(vector:(w64 1251982962688), index:(w32 0), value:(w64 1239903371264)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_0(4);
  vector_table_1251982962688_value_0.set(0, 4, 1);
  state->vector_table_1251982962688.write(0, vector_table_1251982962688_value_0);
  // BDD node 7:vector_borrow(vector:(w64 1248761733120), index:(w32 0), val_out:(w64 1649829478400)[ -> (w64 1240134057984)])
  // Module Ignore
  // BDD node 8:vector_return(vector:(w64 1248761733120), index:(w32 0), value:(w64 1240134057984)[(w16 1)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_0(2);
  vector_register_1248761733120_value_0.set(0, 2, 1);
  state->vector_register_1248761733120.put(0, vector_register_1248761733120_value_0);
  // BDD node 9:vector_borrow(vector:(w64 1251982962688), index:(w32 1), val_out:(w64 1649812701184)[ -> (w64 1239299391488)])
  // Module Ignore
  // BDD node 10:vector_return(vector:(w64 1251982962688), index:(w32 1), value:(w64 1239299391488)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_1(4);
  vector_table_1251982962688_value_1.set(0, 4, 0);
  state->vector_table_1251982962688.write(1, vector_table_1251982962688_value_1);
  // BDD node 11:vector_borrow(vector:(w64 1248761733120), index:(w32 1), val_out:(w64 1649829478400)[ -> (w64 1239144202240)])
  // Module Ignore
  // BDD node 12:vector_return(vector:(w64 1248761733120), index:(w32 1), value:(w64 1239144202240)[(w16 0)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_1(2);
  vector_register_1248761733120_value_1.set(0, 2, 0);
  state->vector_register_1248761733120.put(1, vector_register_1248761733120_value_1);
  // BDD node 13:vector_borrow(vector:(w64 1251982962688), index:(w32 2), val_out:(w64 1649812701184)[ -> (w64 1239970480128)])
  // Module Ignore
  // BDD node 14:vector_return(vector:(w64 1251982962688), index:(w32 2), value:(w64 1239970480128)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_2(4);
  vector_table_1251982962688_value_2.set(0, 4, 1);
  state->vector_table_1251982962688.write(2, vector_table_1251982962688_value_2);
  // BDD node 15:vector_borrow(vector:(w64 1248761733120), index:(w32 2), val_out:(w64 1649829478400)[ -> (w64 1240125669376)])
  // Module Ignore
  // BDD node 16:vector_return(vector:(w64 1248761733120), index:(w32 2), value:(w64 1240125669376)[(w16 3)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_2(2);
  vector_register_1248761733120_value_2.set(0, 2, 3);
  state->vector_register_1248761733120.put(2, vector_register_1248761733120_value_2);
  // BDD node 17:vector_borrow(vector:(w64 1251982962688), index:(w32 3), val_out:(w64 1649812701184)[ -> (w64 1239433609216)])
  // Module Ignore
  // BDD node 18:vector_return(vector:(w64 1251982962688), index:(w32 3), value:(w64 1239433609216)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_3(4);
  vector_table_1251982962688_value_3.set(0, 4, 0);
  state->vector_table_1251982962688.write(3, vector_table_1251982962688_value_3);
  // BDD node 19:vector_borrow(vector:(w64 1248761733120), index:(w32 3), val_out:(w64 1649829478400)[ -> (w64 1239152590848)])
  // Module Ignore
  // BDD node 20:vector_return(vector:(w64 1248761733120), index:(w32 3), value:(w64 1239152590848)[(w16 2)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_3(2);
  vector_register_1248761733120_value_3.set(0, 2, 2);
  state->vector_register_1248761733120.put(3, vector_register_1248761733120_value_3);
  // BDD node 21:vector_borrow(vector:(w64 1251982962688), index:(w32 4), val_out:(w64 1649812701184)[ -> (w64 1239836262400)])
  // Module Ignore
  // BDD node 22:vector_return(vector:(w64 1251982962688), index:(w32 4), value:(w64 1239836262400)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_4(4);
  vector_table_1251982962688_value_4.set(0, 4, 1);
  state->vector_table_1251982962688.write(4, vector_table_1251982962688_value_4);
  // BDD node 23:vector_borrow(vector:(w64 1248761733120), index:(w32 4), val_out:(w64 1649829478400)[ -> (w64 1240117280768)])
  // Module Ignore
  // BDD node 24:vector_return(vector:(w64 1248761733120), index:(w32 4), value:(w64 1240117280768)[(w16 5)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_4(2);
  vector_register_1248761733120_value_4.set(0, 2, 5);
  state->vector_register_1248761733120.put(4, vector_register_1248761733120_value_4);
  // BDD node 25:vector_borrow(vector:(w64 1251982962688), index:(w32 5), val_out:(w64 1649812701184)[ -> (w64 1239567826944)])
  // Module Ignore
  // BDD node 26:vector_return(vector:(w64 1251982962688), index:(w32 5), value:(w64 1239567826944)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_5(4);
  vector_table_1251982962688_value_5.set(0, 4, 0);
  state->vector_table_1251982962688.write(5, vector_table_1251982962688_value_5);
  // BDD node 27:vector_borrow(vector:(w64 1248761733120), index:(w32 5), val_out:(w64 1649829478400)[ -> (w64 1239160979456)])
  // Module Ignore
  // BDD node 28:vector_return(vector:(w64 1248761733120), index:(w32 5), value:(w64 1239160979456)[(w16 4)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_5(2);
  vector_register_1248761733120_value_5.set(0, 2, 4);
  state->vector_register_1248761733120.put(5, vector_register_1248761733120_value_5);
  // BDD node 29:vector_borrow(vector:(w64 1251982962688), index:(w32 6), val_out:(w64 1649812701184)[ -> (w64 1239702044672)])
  // Module Ignore
  // BDD node 30:vector_return(vector:(w64 1251982962688), index:(w32 6), value:(w64 1239702044672)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_6(4);
  vector_table_1251982962688_value_6.set(0, 4, 1);
  state->vector_table_1251982962688.write(6, vector_table_1251982962688_value_6);
  // BDD node 31:vector_borrow(vector:(w64 1248761733120), index:(w32 6), val_out:(w64 1649829478400)[ -> (w64 1240108892160)])
  // Module Ignore
  // BDD node 32:vector_return(vector:(w64 1248761733120), index:(w32 6), value:(w64 1240108892160)[(w16 7)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_6(2);
  vector_register_1248761733120_value_6.set(0, 2, 7);
  state->vector_register_1248761733120.put(6, vector_register_1248761733120_value_6);
  // BDD node 33:vector_borrow(vector:(w64 1251982962688), index:(w32 7), val_out:(w64 1649812701184)[ -> (w64 1239131619328)])
  // Module Ignore
  // BDD node 34:vector_return(vector:(w64 1251982962688), index:(w32 7), value:(w64 1239131619328)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_7(4);
  vector_table_1251982962688_value_7.set(0, 4, 0);
  state->vector_table_1251982962688.write(7, vector_table_1251982962688_value_7);
  // BDD node 35:vector_borrow(vector:(w64 1248761733120), index:(w32 7), val_out:(w64 1649829478400)[ -> (w64 1239169368064)])
  // Module Ignore
  // BDD node 36:vector_return(vector:(w64 1248761733120), index:(w32 7), value:(w64 1239169368064)[(w16 6)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_7(2);
  vector_register_1248761733120_value_7.set(0, 2, 6);
  state->vector_register_1248761733120.put(7, vector_register_1248761733120_value_7);
  // BDD node 37:vector_borrow(vector:(w64 1251982962688), index:(w32 8), val_out:(w64 1649812701184)[ -> (w64 1240138252288)])
  // Module Ignore
  // BDD node 38:vector_return(vector:(w64 1251982962688), index:(w32 8), value:(w64 1240138252288)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_8(4);
  vector_table_1251982962688_value_8.set(0, 4, 1);
  state->vector_table_1251982962688.write(8, vector_table_1251982962688_value_8);
  // BDD node 39:vector_borrow(vector:(w64 1248761733120), index:(w32 8), val_out:(w64 1649829478400)[ -> (w64 1240100503552)])
  // Module Ignore
  // BDD node 40:vector_return(vector:(w64 1248761733120), index:(w32 8), value:(w64 1240100503552)[(w16 9)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_8(2);
  vector_register_1248761733120_value_8.set(0, 2, 9);
  state->vector_register_1248761733120.put(8, vector_register_1248761733120_value_8);
  // BDD node 41:vector_borrow(vector:(w64 1251982962688), index:(w32 9), val_out:(w64 1649812701184)[ -> (w64 1239198728192)])
  // Module Ignore
  // BDD node 42:vector_return(vector:(w64 1251982962688), index:(w32 9), value:(w64 1239198728192)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_9(4);
  vector_table_1251982962688_value_9.set(0, 4, 0);
  state->vector_table_1251982962688.write(9, vector_table_1251982962688_value_9);
  // BDD node 43:vector_borrow(vector:(w64 1248761733120), index:(w32 9), val_out:(w64 1649829478400)[ -> (w64 1239177756672)])
  // Module Ignore
  // BDD node 44:vector_return(vector:(w64 1248761733120), index:(w32 9), value:(w64 1239177756672)[(w16 8)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_9(2);
  vector_register_1248761733120_value_9.set(0, 2, 8);
  state->vector_register_1248761733120.put(9, vector_register_1248761733120_value_9);
  // BDD node 45:vector_borrow(vector:(w64 1251982962688), index:(w32 10), val_out:(w64 1649812701184)[ -> (w64 1240071143424)])
  // Module Ignore
  // BDD node 46:vector_return(vector:(w64 1251982962688), index:(w32 10), value:(w64 1240071143424)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_10(4);
  vector_table_1251982962688_value_10.set(0, 4, 1);
  state->vector_table_1251982962688.write(10, vector_table_1251982962688_value_10);
  // BDD node 47:vector_borrow(vector:(w64 1248761733120), index:(w32 10), val_out:(w64 1649829478400)[ -> (w64 1240092114944)])
  // Module Ignore
  // BDD node 48:vector_return(vector:(w64 1248761733120), index:(w32 10), value:(w64 1240092114944)[(w16 11)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_10(2);
  vector_register_1248761733120_value_10.set(0, 2, 11);
  state->vector_register_1248761733120.put(10, vector_register_1248761733120_value_10);
  // BDD node 49:vector_borrow(vector:(w64 1251982962688), index:(w32 11), val_out:(w64 1649812701184)[ -> (w64 1239265837056)])
  // Module Ignore
  // BDD node 50:vector_return(vector:(w64 1251982962688), index:(w32 11), value:(w64 1239265837056)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_11(4);
  vector_table_1251982962688_value_11.set(0, 4, 0);
  state->vector_table_1251982962688.write(11, vector_table_1251982962688_value_11);
  // BDD node 51:vector_borrow(vector:(w64 1248761733120), index:(w32 11), val_out:(w64 1649829478400)[ -> (w64 1239186145280)])
  // Module Ignore
  // BDD node 52:vector_return(vector:(w64 1248761733120), index:(w32 11), value:(w64 1239186145280)[(w16 10)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_11(2);
  vector_register_1248761733120_value_11.set(0, 2, 10);
  state->vector_register_1248761733120.put(11, vector_register_1248761733120_value_11);
  // BDD node 53:vector_borrow(vector:(w64 1251982962688), index:(w32 12), val_out:(w64 1649812701184)[ -> (w64 1240004034560)])
  // Module Ignore
  // BDD node 54:vector_return(vector:(w64 1251982962688), index:(w32 12), value:(w64 1240004034560)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_12(4);
  vector_table_1251982962688_value_12.set(0, 4, 1);
  state->vector_table_1251982962688.write(12, vector_table_1251982962688_value_12);
  // BDD node 55:vector_borrow(vector:(w64 1248761733120), index:(w32 12), val_out:(w64 1649829478400)[ -> (w64 1240083726336)])
  // Module Ignore
  // BDD node 56:vector_return(vector:(w64 1248761733120), index:(w32 12), value:(w64 1240083726336)[(w16 13)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_12(2);
  vector_register_1248761733120_value_12.set(0, 2, 13);
  state->vector_register_1248761733120.put(12, vector_register_1248761733120_value_12);
  // BDD node 57:vector_borrow(vector:(w64 1251982962688), index:(w32 13), val_out:(w64 1649812701184)[ -> (w64 1239332945920)])
  // Module Ignore
  // BDD node 58:vector_return(vector:(w64 1251982962688), index:(w32 13), value:(w64 1239332945920)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_13(4);
  vector_table_1251982962688_value_13.set(0, 4, 0);
  state->vector_table_1251982962688.write(13, vector_table_1251982962688_value_13);
  // BDD node 59:vector_borrow(vector:(w64 1248761733120), index:(w32 13), val_out:(w64 1649829478400)[ -> (w64 1239194533888)])
  // Module Ignore
  // BDD node 60:vector_return(vector:(w64 1248761733120), index:(w32 13), value:(w64 1239194533888)[(w16 12)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_13(2);
  vector_register_1248761733120_value_13.set(0, 2, 12);
  state->vector_register_1248761733120.put(13, vector_register_1248761733120_value_13);
  // BDD node 61:vector_borrow(vector:(w64 1251982962688), index:(w32 14), val_out:(w64 1649812701184)[ -> (w64 1239936925696)])
  // Module Ignore
  // BDD node 62:vector_return(vector:(w64 1251982962688), index:(w32 14), value:(w64 1239936925696)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_14(4);
  vector_table_1251982962688_value_14.set(0, 4, 1);
  state->vector_table_1251982962688.write(14, vector_table_1251982962688_value_14);
  // BDD node 63:vector_borrow(vector:(w64 1248761733120), index:(w32 14), val_out:(w64 1649829478400)[ -> (w64 1240075337728)])
  // Module Ignore
  // BDD node 64:vector_return(vector:(w64 1248761733120), index:(w32 14), value:(w64 1240075337728)[(w16 15)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_14(2);
  vector_register_1248761733120_value_14.set(0, 2, 15);
  state->vector_register_1248761733120.put(14, vector_register_1248761733120_value_14);
  // BDD node 65:vector_borrow(vector:(w64 1251982962688), index:(w32 15), val_out:(w64 1649812701184)[ -> (w64 1239400054784)])
  // Module Ignore
  // BDD node 66:vector_return(vector:(w64 1251982962688), index:(w32 15), value:(w64 1239400054784)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_15(4);
  vector_table_1251982962688_value_15.set(0, 4, 0);
  state->vector_table_1251982962688.write(15, vector_table_1251982962688_value_15);
  // BDD node 67:vector_borrow(vector:(w64 1248761733120), index:(w32 15), val_out:(w64 1649829478400)[ -> (w64 1239202922496)])
  // Module Ignore
  // BDD node 68:vector_return(vector:(w64 1248761733120), index:(w32 15), value:(w64 1239202922496)[(w16 14)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_15(2);
  vector_register_1248761733120_value_15.set(0, 2, 14);
  state->vector_register_1248761733120.put(15, vector_register_1248761733120_value_15);
  // BDD node 69:vector_borrow(vector:(w64 1251982962688), index:(w32 16), val_out:(w64 1649812701184)[ -> (w64 1239869816832)])
  // Module Ignore
  // BDD node 70:vector_return(vector:(w64 1251982962688), index:(w32 16), value:(w64 1239869816832)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_16(4);
  vector_table_1251982962688_value_16.set(0, 4, 1);
  state->vector_table_1251982962688.write(16, vector_table_1251982962688_value_16);
  // BDD node 71:vector_borrow(vector:(w64 1248761733120), index:(w32 16), val_out:(w64 1649829478400)[ -> (w64 1240066949120)])
  // Module Ignore
  // BDD node 72:vector_return(vector:(w64 1248761733120), index:(w32 16), value:(w64 1240066949120)[(w16 17)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_16(2);
  vector_register_1248761733120_value_16.set(0, 2, 17);
  state->vector_register_1248761733120.put(16, vector_register_1248761733120_value_16);
  // BDD node 73:vector_borrow(vector:(w64 1251982962688), index:(w32 17), val_out:(w64 1649812701184)[ -> (w64 1239467163648)])
  // Module Ignore
  // BDD node 74:vector_return(vector:(w64 1251982962688), index:(w32 17), value:(w64 1239467163648)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_17(4);
  vector_table_1251982962688_value_17.set(0, 4, 0);
  state->vector_table_1251982962688.write(17, vector_table_1251982962688_value_17);
  // BDD node 75:vector_borrow(vector:(w64 1248761733120), index:(w32 17), val_out:(w64 1649829478400)[ -> (w64 1239211311104)])
  // Module Ignore
  // BDD node 76:vector_return(vector:(w64 1248761733120), index:(w32 17), value:(w64 1239211311104)[(w16 16)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_17(2);
  vector_register_1248761733120_value_17.set(0, 2, 16);
  state->vector_register_1248761733120.put(17, vector_register_1248761733120_value_17);
  // BDD node 77:vector_borrow(vector:(w64 1251982962688), index:(w32 18), val_out:(w64 1649812701184)[ -> (w64 1239802707968)])
  // Module Ignore
  // BDD node 78:vector_return(vector:(w64 1251982962688), index:(w32 18), value:(w64 1239802707968)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_18(4);
  vector_table_1251982962688_value_18.set(0, 4, 1);
  state->vector_table_1251982962688.write(18, vector_table_1251982962688_value_18);
  // BDD node 79:vector_borrow(vector:(w64 1248761733120), index:(w32 18), val_out:(w64 1649829478400)[ -> (w64 1240058560512)])
  // Module Ignore
  // BDD node 80:vector_return(vector:(w64 1248761733120), index:(w32 18), value:(w64 1240058560512)[(w16 19)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_18(2);
  vector_register_1248761733120_value_18.set(0, 2, 19);
  state->vector_register_1248761733120.put(18, vector_register_1248761733120_value_18);
  // BDD node 81:vector_borrow(vector:(w64 1251982962688), index:(w32 19), val_out:(w64 1649812701184)[ -> (w64 1239534272512)])
  // Module Ignore
  // BDD node 82:vector_return(vector:(w64 1251982962688), index:(w32 19), value:(w64 1239534272512)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_19(4);
  vector_table_1251982962688_value_19.set(0, 4, 0);
  state->vector_table_1251982962688.write(19, vector_table_1251982962688_value_19);
  // BDD node 83:vector_borrow(vector:(w64 1248761733120), index:(w32 19), val_out:(w64 1649829478400)[ -> (w64 1239219699712)])
  // Module Ignore
  // BDD node 84:vector_return(vector:(w64 1248761733120), index:(w32 19), value:(w64 1239219699712)[(w16 18)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_19(2);
  vector_register_1248761733120_value_19.set(0, 2, 18);
  state->vector_register_1248761733120.put(19, vector_register_1248761733120_value_19);
  // BDD node 85:vector_borrow(vector:(w64 1251982962688), index:(w32 20), val_out:(w64 1649812701184)[ -> (w64 1239735599104)])
  // Module Ignore
  // BDD node 86:vector_return(vector:(w64 1251982962688), index:(w32 20), value:(w64 1239735599104)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_20(4);
  vector_table_1251982962688_value_20.set(0, 4, 1);
  state->vector_table_1251982962688.write(20, vector_table_1251982962688_value_20);
  // BDD node 87:vector_borrow(vector:(w64 1248761733120), index:(w32 20), val_out:(w64 1649829478400)[ -> (w64 1240050171904)])
  // Module Ignore
  // BDD node 88:vector_return(vector:(w64 1248761733120), index:(w32 20), value:(w64 1240050171904)[(w16 21)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_20(2);
  vector_register_1248761733120_value_20.set(0, 2, 21);
  state->vector_register_1248761733120.put(20, vector_register_1248761733120_value_20);
  // BDD node 89:vector_borrow(vector:(w64 1251982962688), index:(w32 21), val_out:(w64 1649812701184)[ -> (w64 1239601381376)])
  // Module Ignore
  // BDD node 90:vector_return(vector:(w64 1251982962688), index:(w32 21), value:(w64 1239601381376)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_21(4);
  vector_table_1251982962688_value_21.set(0, 4, 0);
  state->vector_table_1251982962688.write(21, vector_table_1251982962688_value_21);
  // BDD node 91:vector_borrow(vector:(w64 1248761733120), index:(w32 21), val_out:(w64 1649829478400)[ -> (w64 1239228088320)])
  // Module Ignore
  // BDD node 92:vector_return(vector:(w64 1248761733120), index:(w32 21), value:(w64 1239228088320)[(w16 20)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_21(2);
  vector_register_1248761733120_value_21.set(0, 2, 20);
  state->vector_register_1248761733120.put(21, vector_register_1248761733120_value_21);
  // BDD node 93:vector_borrow(vector:(w64 1251982962688), index:(w32 22), val_out:(w64 1649812701184)[ -> (w64 1239668490240)])
  // Module Ignore
  // BDD node 94:vector_return(vector:(w64 1251982962688), index:(w32 22), value:(w64 1239668490240)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_22(4);
  vector_table_1251982962688_value_22.set(0, 4, 1);
  state->vector_table_1251982962688.write(22, vector_table_1251982962688_value_22);
  // BDD node 95:vector_borrow(vector:(w64 1248761733120), index:(w32 22), val_out:(w64 1649829478400)[ -> (w64 1240041783296)])
  // Module Ignore
  // BDD node 96:vector_return(vector:(w64 1248761733120), index:(w32 22), value:(w64 1240041783296)[(w16 23)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_22(2);
  vector_register_1248761733120_value_22.set(0, 2, 23);
  state->vector_register_1248761733120.put(22, vector_register_1248761733120_value_22);
  // BDD node 97:vector_borrow(vector:(w64 1251982962688), index:(w32 23), val_out:(w64 1649812701184)[ -> (w64 1239114842112)])
  // Module Ignore
  // BDD node 98:vector_return(vector:(w64 1251982962688), index:(w32 23), value:(w64 1239114842112)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_23(4);
  vector_table_1251982962688_value_23.set(0, 4, 0);
  state->vector_table_1251982962688.write(23, vector_table_1251982962688_value_23);
  // BDD node 99:vector_borrow(vector:(w64 1248761733120), index:(w32 23), val_out:(w64 1649829478400)[ -> (w64 1239236476928)])
  // Module Ignore
  // BDD node 100:vector_return(vector:(w64 1248761733120), index:(w32 23), value:(w64 1239236476928)[(w16 22)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_23(2);
  vector_register_1248761733120_value_23.set(0, 2, 22);
  state->vector_register_1248761733120.put(23, vector_register_1248761733120_value_23);
  // BDD node 101:vector_borrow(vector:(w64 1251982962688), index:(w32 24), val_out:(w64 1649812701184)[ -> (w64 1240155029504)])
  // Module Ignore
  // BDD node 102:vector_return(vector:(w64 1251982962688), index:(w32 24), value:(w64 1240155029504)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_24(4);
  vector_table_1251982962688_value_24.set(0, 4, 1);
  state->vector_table_1251982962688.write(24, vector_table_1251982962688_value_24);
  // BDD node 103:vector_borrow(vector:(w64 1248761733120), index:(w32 24), val_out:(w64 1649829478400)[ -> (w64 1240033394688)])
  // Module Ignore
  // BDD node 104:vector_return(vector:(w64 1248761733120), index:(w32 24), value:(w64 1240033394688)[(w16 25)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_24(2);
  vector_register_1248761733120_value_24.set(0, 2, 25);
  state->vector_register_1248761733120.put(24, vector_register_1248761733120_value_24);
  // BDD node 105:vector_borrow(vector:(w64 1251982962688), index:(w32 25), val_out:(w64 1649812701184)[ -> (w64 1239148396544)])
  // Module Ignore
  // BDD node 106:vector_return(vector:(w64 1251982962688), index:(w32 25), value:(w64 1239148396544)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_25(4);
  vector_table_1251982962688_value_25.set(0, 4, 0);
  state->vector_table_1251982962688.write(25, vector_table_1251982962688_value_25);
  // BDD node 107:vector_borrow(vector:(w64 1248761733120), index:(w32 25), val_out:(w64 1649829478400)[ -> (w64 1239244865536)])
  // Module Ignore
  // BDD node 108:vector_return(vector:(w64 1248761733120), index:(w32 25), value:(w64 1239244865536)[(w16 24)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_25(2);
  vector_register_1248761733120_value_25.set(0, 2, 24);
  state->vector_register_1248761733120.put(25, vector_register_1248761733120_value_25);
  // BDD node 109:vector_borrow(vector:(w64 1251982962688), index:(w32 26), val_out:(w64 1649812701184)[ -> (w64 1240121475072)])
  // Module Ignore
  // BDD node 110:vector_return(vector:(w64 1251982962688), index:(w32 26), value:(w64 1240121475072)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_26(4);
  vector_table_1251982962688_value_26.set(0, 4, 1);
  state->vector_table_1251982962688.write(26, vector_table_1251982962688_value_26);
  // BDD node 111:vector_borrow(vector:(w64 1248761733120), index:(w32 26), val_out:(w64 1649829478400)[ -> (w64 1240025006080)])
  // Module Ignore
  // BDD node 112:vector_return(vector:(w64 1248761733120), index:(w32 26), value:(w64 1240025006080)[(w16 27)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_26(2);
  vector_register_1248761733120_value_26.set(0, 2, 27);
  state->vector_register_1248761733120.put(26, vector_register_1248761733120_value_26);
  // BDD node 113:vector_borrow(vector:(w64 1251982962688), index:(w32 27), val_out:(w64 1649812701184)[ -> (w64 1239181950976)])
  // Module Ignore
  // BDD node 114:vector_return(vector:(w64 1251982962688), index:(w32 27), value:(w64 1239181950976)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_27(4);
  vector_table_1251982962688_value_27.set(0, 4, 0);
  state->vector_table_1251982962688.write(27, vector_table_1251982962688_value_27);
  // BDD node 115:vector_borrow(vector:(w64 1248761733120), index:(w32 27), val_out:(w64 1649829478400)[ -> (w64 1239253254144)])
  // Module Ignore
  // BDD node 116:vector_return(vector:(w64 1248761733120), index:(w32 27), value:(w64 1239253254144)[(w16 26)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_27(2);
  vector_register_1248761733120_value_27.set(0, 2, 26);
  state->vector_register_1248761733120.put(27, vector_register_1248761733120_value_27);
  // BDD node 117:vector_borrow(vector:(w64 1251982962688), index:(w32 28), val_out:(w64 1649812701184)[ -> (w64 1240087920640)])
  // Module Ignore
  // BDD node 118:vector_return(vector:(w64 1251982962688), index:(w32 28), value:(w64 1240087920640)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_28(4);
  vector_table_1251982962688_value_28.set(0, 4, 1);
  state->vector_table_1251982962688.write(28, vector_table_1251982962688_value_28);
  // BDD node 119:vector_borrow(vector:(w64 1248761733120), index:(w32 28), val_out:(w64 1649829478400)[ -> (w64 1240016617472)])
  // Module Ignore
  // BDD node 120:vector_return(vector:(w64 1248761733120), index:(w32 28), value:(w64 1240016617472)[(w16 29)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_28(2);
  vector_register_1248761733120_value_28.set(0, 2, 29);
  state->vector_register_1248761733120.put(28, vector_register_1248761733120_value_28);
  // BDD node 121:vector_borrow(vector:(w64 1251982962688), index:(w32 29), val_out:(w64 1649812701184)[ -> (w64 1239215505408)])
  // Module Ignore
  // BDD node 122:vector_return(vector:(w64 1251982962688), index:(w32 29), value:(w64 1239215505408)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_29(4);
  vector_table_1251982962688_value_29.set(0, 4, 0);
  state->vector_table_1251982962688.write(29, vector_table_1251982962688_value_29);
  // BDD node 123:vector_borrow(vector:(w64 1248761733120), index:(w32 29), val_out:(w64 1649829478400)[ -> (w64 1239261642752)])
  // Module Ignore
  // BDD node 124:vector_return(vector:(w64 1248761733120), index:(w32 29), value:(w64 1239261642752)[(w16 28)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_29(2);
  vector_register_1248761733120_value_29.set(0, 2, 28);
  state->vector_register_1248761733120.put(29, vector_register_1248761733120_value_29);
  // BDD node 125:vector_borrow(vector:(w64 1251982962688), index:(w32 30), val_out:(w64 1649812701184)[ -> (w64 1240054366208)])
  // Module Ignore
  // BDD node 126:vector_return(vector:(w64 1251982962688), index:(w32 30), value:(w64 1240054366208)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_30(4);
  vector_table_1251982962688_value_30.set(0, 4, 1);
  state->vector_table_1251982962688.write(30, vector_table_1251982962688_value_30);
  // BDD node 127:vector_borrow(vector:(w64 1248761733120), index:(w32 30), val_out:(w64 1649829478400)[ -> (w64 1240008228864)])
  // Module Ignore
  // BDD node 128:vector_return(vector:(w64 1248761733120), index:(w32 30), value:(w64 1240008228864)[(w16 31)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_30(2);
  vector_register_1248761733120_value_30.set(0, 2, 31);
  state->vector_register_1248761733120.put(30, vector_register_1248761733120_value_30);
  // BDD node 129:vector_borrow(vector:(w64 1251982962688), index:(w32 31), val_out:(w64 1649812701184)[ -> (w64 1239249059840)])
  // Module Ignore
  // BDD node 130:vector_return(vector:(w64 1251982962688), index:(w32 31), value:(w64 1239249059840)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1251982962688_value_31(4);
  vector_table_1251982962688_value_31.set(0, 4, 0);
  state->vector_table_1251982962688.write(31, vector_table_1251982962688_value_31);
  // BDD node 131:vector_borrow(vector:(w64 1248761733120), index:(w32 31), val_out:(w64 1649829478400)[ -> (w64 1239270031360)])
  // Module Ignore
  // BDD node 132:vector_return(vector:(w64 1248761733120), index:(w32 31), value:(w64 1239270031360)[(w16 30)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1248761733120_value_31(2);
  vector_register_1248761733120_value_31.set(0, 2, 30);
  state->vector_register_1248761733120.put(31, vector_register_1248761733120_value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 time; // The switch's clock at the hand-off, ingress_mac_tstamp[47:16].
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

  // The packet as the NF sees it: without the cpu headers the data plane put in front of it.
  const u16 pkt_len = static_cast<u16>(size - packet_consumed);

  cpu_hdr->egress_dev = 0;
  cpu_hdr->trigger_dataplane_execution = 0;

  now = ((time_ns_t)bswap32(cpu_hdr_extra->time)) << 16;


  if (bswap16(cpu_hdr->code_path) == 0) {
    // EP node  4128
    // BDD node 165:dchain_allocate_new_index(chain:(w64 1240352161792), index_out:(w64 1649561042944)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__165)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  4129
    // BDD node 165:dchain_allocate_new_index(chain:(w64 1240352161792), index_out:(w64 1649561042944)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__165)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  4130
    // BDD node 165:dchain_allocate_new_index(chain:(w64 1240352161792), index_out:(w64 1649561042944)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__165)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_2 = packet_consume(pkt, 4);
    // EP node  4131
    // BDD node 165:dchain_allocate_new_index(chain:(w64 1240352161792), index_out:(w64 1649561042944)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__165)], time:(ReadLSB w64 (w32 0) next_time))
    buffer_t value_0;
    state->vector_table_1251982962688.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 0xffffull), value_0);
    // EP node  4132
    // BDD node 165:dchain_allocate_new_index(chain:(w64 1240352161792), index_out:(w64 1649561042944)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__165)], time:(ReadLSB w64 (w32 0) next_time))
    if ((0) == ((u32)value_0.get(0, 4))) {
      // EP node  4133
      // BDD node 165:dchain_allocate_new_index(chain:(w64 1240352161792), index_out:(w64 1649561042944)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__165)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  4135
      // BDD node 165:dchain_allocate_new_index(chain:(w64 1240352161792), index_out:(w64 1649561042944)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__165)], time:(ReadLSB w64 (w32 0) next_time))
      result.abort_transaction = true;
      cpu_hdr->trigger_dataplane_execution = 1;
      return result;
    } else {
      // EP node  4134
      // BDD node 165:dchain_allocate_new_index(chain:(w64 1240352161792), index_out:(w64 1649561042944)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__165)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  4136
      // BDD node 165:dchain_allocate_new_index(chain:(w64 1240352161792), index_out:(w64 1649561042944)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__165)], time:(ReadLSB w64 (w32 0) next_time))
      buffer_t guarded_map_table_1249835474944_key_0(12);
      guarded_map_table_1249835474944_key_0[0] = *(u8*)(hdr_1 + 12);
      guarded_map_table_1249835474944_key_0[1] = *(u8*)(hdr_1 + 13);
      guarded_map_table_1249835474944_key_0[2] = *(u8*)(hdr_1 + 14);
      guarded_map_table_1249835474944_key_0[3] = *(u8*)(hdr_1 + 15);
      guarded_map_table_1249835474944_key_0[4] = *(u8*)(hdr_1 + 16);
      guarded_map_table_1249835474944_key_0[5] = *(u8*)(hdr_1 + 17);
      guarded_map_table_1249835474944_key_0[6] = *(u8*)(hdr_1 + 18);
      guarded_map_table_1249835474944_key_0[7] = *(u8*)(hdr_1 + 19);
      guarded_map_table_1249835474944_key_0[8] = *(u8*)(hdr_2 + 0);
      guarded_map_table_1249835474944_key_0[9] = *(u8*)(hdr_2 + 1);
      guarded_map_table_1249835474944_key_0[10] = *(u8*)(hdr_2 + 2);
      guarded_map_table_1249835474944_key_0[11] = *(u8*)(hdr_2 + 3);
      u32 value_1;
      bool found_0 = state->guarded_map_table_1249835474944.get(guarded_map_table_1249835474944_key_0, value_1);
      // EP node  4137
      // BDD node 165:dchain_allocate_new_index(chain:(w64 1240352161792), index_out:(w64 1649561042944)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__165)], time:(ReadLSB w64 (w32 0) next_time))
      if ((0) == (found_0)) {
        // EP node  4138
        // BDD node 165:dchain_allocate_new_index(chain:(w64 1240352161792), index_out:(w64 1649561042944)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__165)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  6419
        // BDD node 165:dchain_allocate_new_index(chain:(w64 1240352161792), index_out:(w64 1649561042944)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__165)], time:(ReadLSB w64 (w32 0) next_time))
        u32 allocated_index_0;
        bool success_0 = state->dchain_table_1240352161792.allocate_new_index(allocated_index_0);
        // EP node  6485
        // BDD node 166:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__165))
        if ((0) == (success_0)) {
          // EP node  6486
          // BDD node 166:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__165))
          // EP node  7952
          // BDD node 170:DROP
          result.forward = false;
        } else {
          // EP node  6487
          // BDD node 166:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__165))
          // EP node  6692
          // BDD node 171:vector_borrow(vector:(w64 1247687987200), index:(ReadLSB w32 (w32 0) new_index__165), val_out:(w64 1649829478400)[ -> (w64 1240356356096)])
          // EP node  6902
          // BDD node 172:map_put(map:(w64 1249835474944), key:(w64 1240356356096)[(Concat w96 (Read w8 (w32 515) packet_chunks) (Concat w88 (Read w8 (w32 514) packet_chunks) (Concat w80 (Read w8 (w32 513) packet_chunks) (Concat w72 (Read w8 (w32 512) packet_chunks) (ReadLSB w64 (w32 268) packet_chunks))))) -> (Concat w96 (Read w8 (w32 515) packet_chunks) (Concat w88 (Read w8 (w32 514) packet_chunks) (Concat w80 (Read w8 (w32 513) packet_chunks) (Concat w72 (Read w8 (w32 512) packet_chunks) (ReadLSB w64 (w32 268) packet_chunks)))))], value:(ReadLSB w32 (w32 0) new_index__165))
          buffer_t guarded_map_table_1249835474944_key_1(12);
          guarded_map_table_1249835474944_key_1[0] = *(u8*)(hdr_1 + 12);
          guarded_map_table_1249835474944_key_1[1] = *(u8*)(hdr_1 + 13);
          guarded_map_table_1249835474944_key_1[2] = *(u8*)(hdr_1 + 14);
          guarded_map_table_1249835474944_key_1[3] = *(u8*)(hdr_1 + 15);
          guarded_map_table_1249835474944_key_1[4] = *(u8*)(hdr_1 + 16);
          guarded_map_table_1249835474944_key_1[5] = *(u8*)(hdr_1 + 17);
          guarded_map_table_1249835474944_key_1[6] = *(u8*)(hdr_1 + 18);
          guarded_map_table_1249835474944_key_1[7] = *(u8*)(hdr_1 + 19);
          guarded_map_table_1249835474944_key_1[8] = *(u8*)(hdr_2 + 0);
          guarded_map_table_1249835474944_key_1[9] = *(u8*)(hdr_2 + 1);
          guarded_map_table_1249835474944_key_1[10] = *(u8*)(hdr_2 + 2);
          guarded_map_table_1249835474944_key_1[11] = *(u8*)(hdr_2 + 3);
          state->guarded_map_table_1249835474944.put(guarded_map_table_1249835474944_key_1, allocated_index_0);
          // EP node  7044
          // BDD node 173:vector_return(vector:(w64 1247687987200), index:(ReadLSB w32 (w32 0) new_index__165), value:(w64 1240356356096)[(Concat w96 (Read w8 (w32 515) packet_chunks) (Concat w88 (Read w8 (w32 514) packet_chunks) (Concat w80 (Read w8 (w32 513) packet_chunks) (Concat w72 (Read w8 (w32 512) packet_chunks) (ReadLSB w64 (w32 268) packet_chunks)))))])
          buffer_t vector_table_1247687987200_value_0(12);
          vector_table_1247687987200_value_0[0] = *(u8*)(hdr_1 + 12);
          vector_table_1247687987200_value_0[1] = *(u8*)(hdr_1 + 13);
          vector_table_1247687987200_value_0[2] = *(u8*)(hdr_1 + 14);
          vector_table_1247687987200_value_0[3] = *(u8*)(hdr_1 + 15);
          vector_table_1247687987200_value_0[4] = *(u8*)(hdr_1 + 16);
          vector_table_1247687987200_value_0[5] = *(u8*)(hdr_1 + 17);
          vector_table_1247687987200_value_0[6] = *(u8*)(hdr_1 + 18);
          vector_table_1247687987200_value_0[7] = *(u8*)(hdr_1 + 19);
          vector_table_1247687987200_value_0[8] = *(u8*)(hdr_2 + 0);
          vector_table_1247687987200_value_0[9] = *(u8*)(hdr_2 + 1);
          vector_table_1247687987200_value_0[10] = *(u8*)(hdr_2 + 2);
          vector_table_1247687987200_value_0[11] = *(u8*)(hdr_2 + 3);
          state->vector_table_1247687987200.write(allocated_index_0, vector_table_1247687987200_value_0);
          // EP node  7260
          // BDD node 174:nf_set_rte_ipv4_udptcp_checksum(ip_header:(w64 1101625557248), l4_header:(w64 1101625557504), packet:(w64 1649972084736))
          trigger_update_ipv4_tcpudp_checksums = true;
          l3_hdr = (void *)hdr_1;
          l4_hdr = (void *)hdr_2;
          // EP node  7261
          // BDD node 175:vector_borrow(vector:(w64 1248761733120), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1649821089792)[ -> (w64 1240134057984)])
          buffer_t value_2;
          state->vector_register_1248761733120.get((u16)(bswap32(cpu_hdr_extra->DEVICE) & 0xffffull), value_2);
          // EP node  7335
          // BDD node 176:vector_return(vector:(w64 1248761733120), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1240134057984)[(ReadLSB w16 (w32 0) vector_data__175)])
          // EP node  7485
          // BDD node 177:packet_return_chunk(p:(w64 1246614249584), the_chunk:(w64 1101625557504)[(Concat w32 (Read w8 (w32 515) packet_chunks) (Concat w24 (Read w8 (w32 514) packet_chunks) (ReadLSB w16 (w32 0) new_index__165)))])
          const u8 hdr_2_7485_b1 = allocated_index_0 & 0xffull;
          const u8 hdr_2_7485_b0 = (allocated_index_0>>8) & 0xffull;
          hdr_2[1] = hdr_2_7485_b1;
          hdr_2[0] = hdr_2_7485_b0;
          // EP node  7562
          // BDD node 178:packet_return_chunk(p:(w64 1246614249584), the_chunk:(w64 1101625557248)[(Concat w160 (Read w8 (w32 275) packet_chunks) (Concat w152 (Read w8 (w32 274) packet_chunks) (Concat w144 (Read w8 (w32 273) packet_chunks) (Concat w136 (Read w8 (w32 272) packet_chunks) (Concat w128 (w8 4) (Concat w120 (w8 3) (Concat w112 (w8 2) (Concat w104 (w8 1) (Concat w96 (Read w8 (w32 1) checksum__174) (Concat w88 (Read w8 (w32 0) checksum__174) (ReadLSB w80 (w32 256) packet_chunks)))))))))))])
          const u8 hdr_1_7562_b12 = 1;
          const u8 hdr_1_7562_b13 = 2;
          const u8 hdr_1_7562_b14 = 3;
          const u8 hdr_1_7562_b15 = 4;
          hdr_1[12] = hdr_1_7562_b12;
          hdr_1[13] = hdr_1_7562_b13;
          hdr_1[14] = hdr_1_7562_b14;
          hdr_1[15] = hdr_1_7562_b15;
          // EP node  7717
          // BDD node 180:FORWARD
          cpu_hdr->egress_dev = bswap16((u16)value_2.get(0, 2));
        }
      } else {
        // EP node  4139
        // BDD node 165:dchain_allocate_new_index(chain:(w64 1240352161792), index_out:(w64 1649561042944)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__165)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  4140
        // BDD node 165:dchain_allocate_new_index(chain:(w64 1240352161792), index_out:(w64 1649561042944)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__165)], time:(ReadLSB w64 (w32 0) next_time))
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
