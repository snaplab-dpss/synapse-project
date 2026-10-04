#include <sycon/sycon.h>
#include <sycon/libnf.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  FCFSCachedSet fcfs_cs_1249835474944;
  CountMinSketch cms_1251982962688;
  VectorTable vector_table_1248761733120;
  VectorTable vector_table_1246614245376;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      fcfs_cs_1249835474944("fcfs_cs_1249835474944", {"Ingress.fcfs_cs_1249835474944_table_144", }, 1000LL),
      cms_1251982962688("cms_1251982962688",{"Ingress.cms_1251982962688_row_0", "Ingress.cms_1251982962688_row_1", "Ingress.cms_1251982962688_row_2", "Ingress.cms_1251982962688_row_3", }, 10000LL),
      vector_table_1248761733120("vector_table_1248761733120",{"Ingress.vector_table_1248761733120_141",}),
      vector_table_1246614245376("vector_table_1246614245376",{"Ingress.vector_table_1246614245376_177","Ingress.vector_table_1246614245376_171","Ingress.vector_table_1246614245376_160",})
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
  // Module DataplaneFCFSCachedSetAllocate
  // BDD node 2:dchain_allocate(index_range:(w32 65536), chain_out:(w64 1242453508112)[ -> (w64 1241073582080)])
  // Module Ignore
  // BDD node 3:cms_allocate(height:(w32 4), width:(w32 1024), key_size:(w16 8), cms_out:(w64 1242453508120)[(w64 0) -> (w64 1251982962688)], cleanup_interval:(w64 10000000000))
  // Module DataplaneCMSAllocate
  // BDD node 4:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1242453508128)[(w64 0) -> (w64 1248761733120)])
  // Module DataplaneVectorTableAllocate
  // BDD node 5:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1242453508136)[(w64 0) -> (w64 1246614245376)])
  // Module DataplaneVectorTableAllocate
  // BDD node 6:vector_borrow(vector:(w64 1248761733120), index:(w32 0), val_out:(w64 1649988861952)[ -> (w64 1239903371264)])
  // Module Ignore
  // BDD node 7:vector_return(vector:(w64 1248761733120), index:(w32 0), value:(w64 1239903371264)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_0(4);
  vector_table_1248761733120_value_0.set(0, 4, 1);
  state->vector_table_1248761733120.write(0, vector_table_1248761733120_value_0);
  // BDD node 8:vector_borrow(vector:(w64 1246614245376), index:(w32 0), val_out:(w64 1649812701184)[ -> (w64 1240134057984)])
  // Module Ignore
  // BDD node 9:vector_return(vector:(w64 1246614245376), index:(w32 0), value:(w64 1240134057984)[(w16 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_0(2);
  vector_table_1246614245376_value_0.set(0, 2, 1);
  state->vector_table_1246614245376.write(0, vector_table_1246614245376_value_0);
  // BDD node 10:vector_borrow(vector:(w64 1248761733120), index:(w32 1), val_out:(w64 1649988861952)[ -> (w64 1239299391488)])
  // Module Ignore
  // BDD node 11:vector_return(vector:(w64 1248761733120), index:(w32 1), value:(w64 1239299391488)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_1(4);
  vector_table_1248761733120_value_1.set(0, 4, 0);
  state->vector_table_1248761733120.write(1, vector_table_1248761733120_value_1);
  // BDD node 12:vector_borrow(vector:(w64 1246614245376), index:(w32 1), val_out:(w64 1649812701184)[ -> (w64 1239144202240)])
  // Module Ignore
  // BDD node 13:vector_return(vector:(w64 1246614245376), index:(w32 1), value:(w64 1239144202240)[(w16 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_1(2);
  vector_table_1246614245376_value_1.set(0, 2, 0);
  state->vector_table_1246614245376.write(1, vector_table_1246614245376_value_1);
  // BDD node 14:vector_borrow(vector:(w64 1248761733120), index:(w32 2), val_out:(w64 1649988861952)[ -> (w64 1239970480128)])
  // Module Ignore
  // BDD node 15:vector_return(vector:(w64 1248761733120), index:(w32 2), value:(w64 1239970480128)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_2(4);
  vector_table_1248761733120_value_2.set(0, 4, 1);
  state->vector_table_1248761733120.write(2, vector_table_1248761733120_value_2);
  // BDD node 16:vector_borrow(vector:(w64 1246614245376), index:(w32 2), val_out:(w64 1649812701184)[ -> (w64 1240125669376)])
  // Module Ignore
  // BDD node 17:vector_return(vector:(w64 1246614245376), index:(w32 2), value:(w64 1240125669376)[(w16 3)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_2(2);
  vector_table_1246614245376_value_2.set(0, 2, 3);
  state->vector_table_1246614245376.write(2, vector_table_1246614245376_value_2);
  // BDD node 18:vector_borrow(vector:(w64 1248761733120), index:(w32 3), val_out:(w64 1649988861952)[ -> (w64 1239433609216)])
  // Module Ignore
  // BDD node 19:vector_return(vector:(w64 1248761733120), index:(w32 3), value:(w64 1239433609216)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_3(4);
  vector_table_1248761733120_value_3.set(0, 4, 0);
  state->vector_table_1248761733120.write(3, vector_table_1248761733120_value_3);
  // BDD node 20:vector_borrow(vector:(w64 1246614245376), index:(w32 3), val_out:(w64 1649812701184)[ -> (w64 1239152590848)])
  // Module Ignore
  // BDD node 21:vector_return(vector:(w64 1246614245376), index:(w32 3), value:(w64 1239152590848)[(w16 2)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_3(2);
  vector_table_1246614245376_value_3.set(0, 2, 2);
  state->vector_table_1246614245376.write(3, vector_table_1246614245376_value_3);
  // BDD node 22:vector_borrow(vector:(w64 1248761733120), index:(w32 4), val_out:(w64 1649988861952)[ -> (w64 1239836262400)])
  // Module Ignore
  // BDD node 23:vector_return(vector:(w64 1248761733120), index:(w32 4), value:(w64 1239836262400)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_4(4);
  vector_table_1248761733120_value_4.set(0, 4, 1);
  state->vector_table_1248761733120.write(4, vector_table_1248761733120_value_4);
  // BDD node 24:vector_borrow(vector:(w64 1246614245376), index:(w32 4), val_out:(w64 1649812701184)[ -> (w64 1240117280768)])
  // Module Ignore
  // BDD node 25:vector_return(vector:(w64 1246614245376), index:(w32 4), value:(w64 1240117280768)[(w16 5)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_4(2);
  vector_table_1246614245376_value_4.set(0, 2, 5);
  state->vector_table_1246614245376.write(4, vector_table_1246614245376_value_4);
  // BDD node 26:vector_borrow(vector:(w64 1248761733120), index:(w32 5), val_out:(w64 1649988861952)[ -> (w64 1239567826944)])
  // Module Ignore
  // BDD node 27:vector_return(vector:(w64 1248761733120), index:(w32 5), value:(w64 1239567826944)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_5(4);
  vector_table_1248761733120_value_5.set(0, 4, 0);
  state->vector_table_1248761733120.write(5, vector_table_1248761733120_value_5);
  // BDD node 28:vector_borrow(vector:(w64 1246614245376), index:(w32 5), val_out:(w64 1649812701184)[ -> (w64 1239160979456)])
  // Module Ignore
  // BDD node 29:vector_return(vector:(w64 1246614245376), index:(w32 5), value:(w64 1239160979456)[(w16 4)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_5(2);
  vector_table_1246614245376_value_5.set(0, 2, 4);
  state->vector_table_1246614245376.write(5, vector_table_1246614245376_value_5);
  // BDD node 30:vector_borrow(vector:(w64 1248761733120), index:(w32 6), val_out:(w64 1649988861952)[ -> (w64 1239702044672)])
  // Module Ignore
  // BDD node 31:vector_return(vector:(w64 1248761733120), index:(w32 6), value:(w64 1239702044672)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_6(4);
  vector_table_1248761733120_value_6.set(0, 4, 1);
  state->vector_table_1248761733120.write(6, vector_table_1248761733120_value_6);
  // BDD node 32:vector_borrow(vector:(w64 1246614245376), index:(w32 6), val_out:(w64 1649812701184)[ -> (w64 1240108892160)])
  // Module Ignore
  // BDD node 33:vector_return(vector:(w64 1246614245376), index:(w32 6), value:(w64 1240108892160)[(w16 7)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_6(2);
  vector_table_1246614245376_value_6.set(0, 2, 7);
  state->vector_table_1246614245376.write(6, vector_table_1246614245376_value_6);
  // BDD node 34:vector_borrow(vector:(w64 1248761733120), index:(w32 7), val_out:(w64 1649988861952)[ -> (w64 1239131619328)])
  // Module Ignore
  // BDD node 35:vector_return(vector:(w64 1248761733120), index:(w32 7), value:(w64 1239131619328)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_7(4);
  vector_table_1248761733120_value_7.set(0, 4, 0);
  state->vector_table_1248761733120.write(7, vector_table_1248761733120_value_7);
  // BDD node 36:vector_borrow(vector:(w64 1246614245376), index:(w32 7), val_out:(w64 1649812701184)[ -> (w64 1239169368064)])
  // Module Ignore
  // BDD node 37:vector_return(vector:(w64 1246614245376), index:(w32 7), value:(w64 1239169368064)[(w16 6)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_7(2);
  vector_table_1246614245376_value_7.set(0, 2, 6);
  state->vector_table_1246614245376.write(7, vector_table_1246614245376_value_7);
  // BDD node 38:vector_borrow(vector:(w64 1248761733120), index:(w32 8), val_out:(w64 1649988861952)[ -> (w64 1240138252288)])
  // Module Ignore
  // BDD node 39:vector_return(vector:(w64 1248761733120), index:(w32 8), value:(w64 1240138252288)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_8(4);
  vector_table_1248761733120_value_8.set(0, 4, 1);
  state->vector_table_1248761733120.write(8, vector_table_1248761733120_value_8);
  // BDD node 40:vector_borrow(vector:(w64 1246614245376), index:(w32 8), val_out:(w64 1649812701184)[ -> (w64 1240100503552)])
  // Module Ignore
  // BDD node 41:vector_return(vector:(w64 1246614245376), index:(w32 8), value:(w64 1240100503552)[(w16 9)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_8(2);
  vector_table_1246614245376_value_8.set(0, 2, 9);
  state->vector_table_1246614245376.write(8, vector_table_1246614245376_value_8);
  // BDD node 42:vector_borrow(vector:(w64 1248761733120), index:(w32 9), val_out:(w64 1649988861952)[ -> (w64 1239198728192)])
  // Module Ignore
  // BDD node 43:vector_return(vector:(w64 1248761733120), index:(w32 9), value:(w64 1239198728192)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_9(4);
  vector_table_1248761733120_value_9.set(0, 4, 0);
  state->vector_table_1248761733120.write(9, vector_table_1248761733120_value_9);
  // BDD node 44:vector_borrow(vector:(w64 1246614245376), index:(w32 9), val_out:(w64 1649812701184)[ -> (w64 1239177756672)])
  // Module Ignore
  // BDD node 45:vector_return(vector:(w64 1246614245376), index:(w32 9), value:(w64 1239177756672)[(w16 8)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_9(2);
  vector_table_1246614245376_value_9.set(0, 2, 8);
  state->vector_table_1246614245376.write(9, vector_table_1246614245376_value_9);
  // BDD node 46:vector_borrow(vector:(w64 1248761733120), index:(w32 10), val_out:(w64 1649988861952)[ -> (w64 1240071143424)])
  // Module Ignore
  // BDD node 47:vector_return(vector:(w64 1248761733120), index:(w32 10), value:(w64 1240071143424)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_10(4);
  vector_table_1248761733120_value_10.set(0, 4, 1);
  state->vector_table_1248761733120.write(10, vector_table_1248761733120_value_10);
  // BDD node 48:vector_borrow(vector:(w64 1246614245376), index:(w32 10), val_out:(w64 1649812701184)[ -> (w64 1240092114944)])
  // Module Ignore
  // BDD node 49:vector_return(vector:(w64 1246614245376), index:(w32 10), value:(w64 1240092114944)[(w16 11)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_10(2);
  vector_table_1246614245376_value_10.set(0, 2, 11);
  state->vector_table_1246614245376.write(10, vector_table_1246614245376_value_10);
  // BDD node 50:vector_borrow(vector:(w64 1248761733120), index:(w32 11), val_out:(w64 1649988861952)[ -> (w64 1239265837056)])
  // Module Ignore
  // BDD node 51:vector_return(vector:(w64 1248761733120), index:(w32 11), value:(w64 1239265837056)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_11(4);
  vector_table_1248761733120_value_11.set(0, 4, 0);
  state->vector_table_1248761733120.write(11, vector_table_1248761733120_value_11);
  // BDD node 52:vector_borrow(vector:(w64 1246614245376), index:(w32 11), val_out:(w64 1649812701184)[ -> (w64 1239186145280)])
  // Module Ignore
  // BDD node 53:vector_return(vector:(w64 1246614245376), index:(w32 11), value:(w64 1239186145280)[(w16 10)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_11(2);
  vector_table_1246614245376_value_11.set(0, 2, 10);
  state->vector_table_1246614245376.write(11, vector_table_1246614245376_value_11);
  // BDD node 54:vector_borrow(vector:(w64 1248761733120), index:(w32 12), val_out:(w64 1649988861952)[ -> (w64 1240004034560)])
  // Module Ignore
  // BDD node 55:vector_return(vector:(w64 1248761733120), index:(w32 12), value:(w64 1240004034560)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_12(4);
  vector_table_1248761733120_value_12.set(0, 4, 1);
  state->vector_table_1248761733120.write(12, vector_table_1248761733120_value_12);
  // BDD node 56:vector_borrow(vector:(w64 1246614245376), index:(w32 12), val_out:(w64 1649812701184)[ -> (w64 1240083726336)])
  // Module Ignore
  // BDD node 57:vector_return(vector:(w64 1246614245376), index:(w32 12), value:(w64 1240083726336)[(w16 13)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_12(2);
  vector_table_1246614245376_value_12.set(0, 2, 13);
  state->vector_table_1246614245376.write(12, vector_table_1246614245376_value_12);
  // BDD node 58:vector_borrow(vector:(w64 1248761733120), index:(w32 13), val_out:(w64 1649988861952)[ -> (w64 1239332945920)])
  // Module Ignore
  // BDD node 59:vector_return(vector:(w64 1248761733120), index:(w32 13), value:(w64 1239332945920)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_13(4);
  vector_table_1248761733120_value_13.set(0, 4, 0);
  state->vector_table_1248761733120.write(13, vector_table_1248761733120_value_13);
  // BDD node 60:vector_borrow(vector:(w64 1246614245376), index:(w32 13), val_out:(w64 1649812701184)[ -> (w64 1239194533888)])
  // Module Ignore
  // BDD node 61:vector_return(vector:(w64 1246614245376), index:(w32 13), value:(w64 1239194533888)[(w16 12)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_13(2);
  vector_table_1246614245376_value_13.set(0, 2, 12);
  state->vector_table_1246614245376.write(13, vector_table_1246614245376_value_13);
  // BDD node 62:vector_borrow(vector:(w64 1248761733120), index:(w32 14), val_out:(w64 1649988861952)[ -> (w64 1239936925696)])
  // Module Ignore
  // BDD node 63:vector_return(vector:(w64 1248761733120), index:(w32 14), value:(w64 1239936925696)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_14(4);
  vector_table_1248761733120_value_14.set(0, 4, 1);
  state->vector_table_1248761733120.write(14, vector_table_1248761733120_value_14);
  // BDD node 64:vector_borrow(vector:(w64 1246614245376), index:(w32 14), val_out:(w64 1649812701184)[ -> (w64 1240075337728)])
  // Module Ignore
  // BDD node 65:vector_return(vector:(w64 1246614245376), index:(w32 14), value:(w64 1240075337728)[(w16 15)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_14(2);
  vector_table_1246614245376_value_14.set(0, 2, 15);
  state->vector_table_1246614245376.write(14, vector_table_1246614245376_value_14);
  // BDD node 66:vector_borrow(vector:(w64 1248761733120), index:(w32 15), val_out:(w64 1649988861952)[ -> (w64 1239400054784)])
  // Module Ignore
  // BDD node 67:vector_return(vector:(w64 1248761733120), index:(w32 15), value:(w64 1239400054784)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_15(4);
  vector_table_1248761733120_value_15.set(0, 4, 0);
  state->vector_table_1248761733120.write(15, vector_table_1248761733120_value_15);
  // BDD node 68:vector_borrow(vector:(w64 1246614245376), index:(w32 15), val_out:(w64 1649812701184)[ -> (w64 1239202922496)])
  // Module Ignore
  // BDD node 69:vector_return(vector:(w64 1246614245376), index:(w32 15), value:(w64 1239202922496)[(w16 14)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_15(2);
  vector_table_1246614245376_value_15.set(0, 2, 14);
  state->vector_table_1246614245376.write(15, vector_table_1246614245376_value_15);
  // BDD node 70:vector_borrow(vector:(w64 1248761733120), index:(w32 16), val_out:(w64 1649988861952)[ -> (w64 1239869816832)])
  // Module Ignore
  // BDD node 71:vector_return(vector:(w64 1248761733120), index:(w32 16), value:(w64 1239869816832)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_16(4);
  vector_table_1248761733120_value_16.set(0, 4, 1);
  state->vector_table_1248761733120.write(16, vector_table_1248761733120_value_16);
  // BDD node 72:vector_borrow(vector:(w64 1246614245376), index:(w32 16), val_out:(w64 1649812701184)[ -> (w64 1240066949120)])
  // Module Ignore
  // BDD node 73:vector_return(vector:(w64 1246614245376), index:(w32 16), value:(w64 1240066949120)[(w16 17)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_16(2);
  vector_table_1246614245376_value_16.set(0, 2, 17);
  state->vector_table_1246614245376.write(16, vector_table_1246614245376_value_16);
  // BDD node 74:vector_borrow(vector:(w64 1248761733120), index:(w32 17), val_out:(w64 1649988861952)[ -> (w64 1239467163648)])
  // Module Ignore
  // BDD node 75:vector_return(vector:(w64 1248761733120), index:(w32 17), value:(w64 1239467163648)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_17(4);
  vector_table_1248761733120_value_17.set(0, 4, 0);
  state->vector_table_1248761733120.write(17, vector_table_1248761733120_value_17);
  // BDD node 76:vector_borrow(vector:(w64 1246614245376), index:(w32 17), val_out:(w64 1649812701184)[ -> (w64 1239211311104)])
  // Module Ignore
  // BDD node 77:vector_return(vector:(w64 1246614245376), index:(w32 17), value:(w64 1239211311104)[(w16 16)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_17(2);
  vector_table_1246614245376_value_17.set(0, 2, 16);
  state->vector_table_1246614245376.write(17, vector_table_1246614245376_value_17);
  // BDD node 78:vector_borrow(vector:(w64 1248761733120), index:(w32 18), val_out:(w64 1649988861952)[ -> (w64 1239802707968)])
  // Module Ignore
  // BDD node 79:vector_return(vector:(w64 1248761733120), index:(w32 18), value:(w64 1239802707968)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_18(4);
  vector_table_1248761733120_value_18.set(0, 4, 1);
  state->vector_table_1248761733120.write(18, vector_table_1248761733120_value_18);
  // BDD node 80:vector_borrow(vector:(w64 1246614245376), index:(w32 18), val_out:(w64 1649812701184)[ -> (w64 1240058560512)])
  // Module Ignore
  // BDD node 81:vector_return(vector:(w64 1246614245376), index:(w32 18), value:(w64 1240058560512)[(w16 19)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_18(2);
  vector_table_1246614245376_value_18.set(0, 2, 19);
  state->vector_table_1246614245376.write(18, vector_table_1246614245376_value_18);
  // BDD node 82:vector_borrow(vector:(w64 1248761733120), index:(w32 19), val_out:(w64 1649988861952)[ -> (w64 1239534272512)])
  // Module Ignore
  // BDD node 83:vector_return(vector:(w64 1248761733120), index:(w32 19), value:(w64 1239534272512)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_19(4);
  vector_table_1248761733120_value_19.set(0, 4, 0);
  state->vector_table_1248761733120.write(19, vector_table_1248761733120_value_19);
  // BDD node 84:vector_borrow(vector:(w64 1246614245376), index:(w32 19), val_out:(w64 1649812701184)[ -> (w64 1239219699712)])
  // Module Ignore
  // BDD node 85:vector_return(vector:(w64 1246614245376), index:(w32 19), value:(w64 1239219699712)[(w16 18)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_19(2);
  vector_table_1246614245376_value_19.set(0, 2, 18);
  state->vector_table_1246614245376.write(19, vector_table_1246614245376_value_19);
  // BDD node 86:vector_borrow(vector:(w64 1248761733120), index:(w32 20), val_out:(w64 1649988861952)[ -> (w64 1239735599104)])
  // Module Ignore
  // BDD node 87:vector_return(vector:(w64 1248761733120), index:(w32 20), value:(w64 1239735599104)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_20(4);
  vector_table_1248761733120_value_20.set(0, 4, 1);
  state->vector_table_1248761733120.write(20, vector_table_1248761733120_value_20);
  // BDD node 88:vector_borrow(vector:(w64 1246614245376), index:(w32 20), val_out:(w64 1649812701184)[ -> (w64 1240050171904)])
  // Module Ignore
  // BDD node 89:vector_return(vector:(w64 1246614245376), index:(w32 20), value:(w64 1240050171904)[(w16 21)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_20(2);
  vector_table_1246614245376_value_20.set(0, 2, 21);
  state->vector_table_1246614245376.write(20, vector_table_1246614245376_value_20);
  // BDD node 90:vector_borrow(vector:(w64 1248761733120), index:(w32 21), val_out:(w64 1649988861952)[ -> (w64 1239601381376)])
  // Module Ignore
  // BDD node 91:vector_return(vector:(w64 1248761733120), index:(w32 21), value:(w64 1239601381376)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_21(4);
  vector_table_1248761733120_value_21.set(0, 4, 0);
  state->vector_table_1248761733120.write(21, vector_table_1248761733120_value_21);
  // BDD node 92:vector_borrow(vector:(w64 1246614245376), index:(w32 21), val_out:(w64 1649812701184)[ -> (w64 1239228088320)])
  // Module Ignore
  // BDD node 93:vector_return(vector:(w64 1246614245376), index:(w32 21), value:(w64 1239228088320)[(w16 20)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_21(2);
  vector_table_1246614245376_value_21.set(0, 2, 20);
  state->vector_table_1246614245376.write(21, vector_table_1246614245376_value_21);
  // BDD node 94:vector_borrow(vector:(w64 1248761733120), index:(w32 22), val_out:(w64 1649988861952)[ -> (w64 1239668490240)])
  // Module Ignore
  // BDD node 95:vector_return(vector:(w64 1248761733120), index:(w32 22), value:(w64 1239668490240)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_22(4);
  vector_table_1248761733120_value_22.set(0, 4, 1);
  state->vector_table_1248761733120.write(22, vector_table_1248761733120_value_22);
  // BDD node 96:vector_borrow(vector:(w64 1246614245376), index:(w32 22), val_out:(w64 1649812701184)[ -> (w64 1240041783296)])
  // Module Ignore
  // BDD node 97:vector_return(vector:(w64 1246614245376), index:(w32 22), value:(w64 1240041783296)[(w16 23)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_22(2);
  vector_table_1246614245376_value_22.set(0, 2, 23);
  state->vector_table_1246614245376.write(22, vector_table_1246614245376_value_22);
  // BDD node 98:vector_borrow(vector:(w64 1248761733120), index:(w32 23), val_out:(w64 1649988861952)[ -> (w64 1239114842112)])
  // Module Ignore
  // BDD node 99:vector_return(vector:(w64 1248761733120), index:(w32 23), value:(w64 1239114842112)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_23(4);
  vector_table_1248761733120_value_23.set(0, 4, 0);
  state->vector_table_1248761733120.write(23, vector_table_1248761733120_value_23);
  // BDD node 100:vector_borrow(vector:(w64 1246614245376), index:(w32 23), val_out:(w64 1649812701184)[ -> (w64 1239236476928)])
  // Module Ignore
  // BDD node 101:vector_return(vector:(w64 1246614245376), index:(w32 23), value:(w64 1239236476928)[(w16 22)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_23(2);
  vector_table_1246614245376_value_23.set(0, 2, 22);
  state->vector_table_1246614245376.write(23, vector_table_1246614245376_value_23);
  // BDD node 102:vector_borrow(vector:(w64 1248761733120), index:(w32 24), val_out:(w64 1649988861952)[ -> (w64 1240155029504)])
  // Module Ignore
  // BDD node 103:vector_return(vector:(w64 1248761733120), index:(w32 24), value:(w64 1240155029504)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_24(4);
  vector_table_1248761733120_value_24.set(0, 4, 1);
  state->vector_table_1248761733120.write(24, vector_table_1248761733120_value_24);
  // BDD node 104:vector_borrow(vector:(w64 1246614245376), index:(w32 24), val_out:(w64 1649812701184)[ -> (w64 1240033394688)])
  // Module Ignore
  // BDD node 105:vector_return(vector:(w64 1246614245376), index:(w32 24), value:(w64 1240033394688)[(w16 25)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_24(2);
  vector_table_1246614245376_value_24.set(0, 2, 25);
  state->vector_table_1246614245376.write(24, vector_table_1246614245376_value_24);
  // BDD node 106:vector_borrow(vector:(w64 1248761733120), index:(w32 25), val_out:(w64 1649988861952)[ -> (w64 1239148396544)])
  // Module Ignore
  // BDD node 107:vector_return(vector:(w64 1248761733120), index:(w32 25), value:(w64 1239148396544)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_25(4);
  vector_table_1248761733120_value_25.set(0, 4, 0);
  state->vector_table_1248761733120.write(25, vector_table_1248761733120_value_25);
  // BDD node 108:vector_borrow(vector:(w64 1246614245376), index:(w32 25), val_out:(w64 1649812701184)[ -> (w64 1239244865536)])
  // Module Ignore
  // BDD node 109:vector_return(vector:(w64 1246614245376), index:(w32 25), value:(w64 1239244865536)[(w16 24)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_25(2);
  vector_table_1246614245376_value_25.set(0, 2, 24);
  state->vector_table_1246614245376.write(25, vector_table_1246614245376_value_25);
  // BDD node 110:vector_borrow(vector:(w64 1248761733120), index:(w32 26), val_out:(w64 1649988861952)[ -> (w64 1240121475072)])
  // Module Ignore
  // BDD node 111:vector_return(vector:(w64 1248761733120), index:(w32 26), value:(w64 1240121475072)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_26(4);
  vector_table_1248761733120_value_26.set(0, 4, 1);
  state->vector_table_1248761733120.write(26, vector_table_1248761733120_value_26);
  // BDD node 112:vector_borrow(vector:(w64 1246614245376), index:(w32 26), val_out:(w64 1649812701184)[ -> (w64 1240025006080)])
  // Module Ignore
  // BDD node 113:vector_return(vector:(w64 1246614245376), index:(w32 26), value:(w64 1240025006080)[(w16 27)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_26(2);
  vector_table_1246614245376_value_26.set(0, 2, 27);
  state->vector_table_1246614245376.write(26, vector_table_1246614245376_value_26);
  // BDD node 114:vector_borrow(vector:(w64 1248761733120), index:(w32 27), val_out:(w64 1649988861952)[ -> (w64 1239181950976)])
  // Module Ignore
  // BDD node 115:vector_return(vector:(w64 1248761733120), index:(w32 27), value:(w64 1239181950976)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_27(4);
  vector_table_1248761733120_value_27.set(0, 4, 0);
  state->vector_table_1248761733120.write(27, vector_table_1248761733120_value_27);
  // BDD node 116:vector_borrow(vector:(w64 1246614245376), index:(w32 27), val_out:(w64 1649812701184)[ -> (w64 1239253254144)])
  // Module Ignore
  // BDD node 117:vector_return(vector:(w64 1246614245376), index:(w32 27), value:(w64 1239253254144)[(w16 26)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_27(2);
  vector_table_1246614245376_value_27.set(0, 2, 26);
  state->vector_table_1246614245376.write(27, vector_table_1246614245376_value_27);
  // BDD node 118:vector_borrow(vector:(w64 1248761733120), index:(w32 28), val_out:(w64 1649988861952)[ -> (w64 1240087920640)])
  // Module Ignore
  // BDD node 119:vector_return(vector:(w64 1248761733120), index:(w32 28), value:(w64 1240087920640)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_28(4);
  vector_table_1248761733120_value_28.set(0, 4, 1);
  state->vector_table_1248761733120.write(28, vector_table_1248761733120_value_28);
  // BDD node 120:vector_borrow(vector:(w64 1246614245376), index:(w32 28), val_out:(w64 1649812701184)[ -> (w64 1240016617472)])
  // Module Ignore
  // BDD node 121:vector_return(vector:(w64 1246614245376), index:(w32 28), value:(w64 1240016617472)[(w16 29)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_28(2);
  vector_table_1246614245376_value_28.set(0, 2, 29);
  state->vector_table_1246614245376.write(28, vector_table_1246614245376_value_28);
  // BDD node 122:vector_borrow(vector:(w64 1248761733120), index:(w32 29), val_out:(w64 1649988861952)[ -> (w64 1239215505408)])
  // Module Ignore
  // BDD node 123:vector_return(vector:(w64 1248761733120), index:(w32 29), value:(w64 1239215505408)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_29(4);
  vector_table_1248761733120_value_29.set(0, 4, 0);
  state->vector_table_1248761733120.write(29, vector_table_1248761733120_value_29);
  // BDD node 124:vector_borrow(vector:(w64 1246614245376), index:(w32 29), val_out:(w64 1649812701184)[ -> (w64 1239261642752)])
  // Module Ignore
  // BDD node 125:vector_return(vector:(w64 1246614245376), index:(w32 29), value:(w64 1239261642752)[(w16 28)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_29(2);
  vector_table_1246614245376_value_29.set(0, 2, 28);
  state->vector_table_1246614245376.write(29, vector_table_1246614245376_value_29);
  // BDD node 126:vector_borrow(vector:(w64 1248761733120), index:(w32 30), val_out:(w64 1649988861952)[ -> (w64 1240054366208)])
  // Module Ignore
  // BDD node 127:vector_return(vector:(w64 1248761733120), index:(w32 30), value:(w64 1240054366208)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_30(4);
  vector_table_1248761733120_value_30.set(0, 4, 1);
  state->vector_table_1248761733120.write(30, vector_table_1248761733120_value_30);
  // BDD node 128:vector_borrow(vector:(w64 1246614245376), index:(w32 30), val_out:(w64 1649812701184)[ -> (w64 1240008228864)])
  // Module Ignore
  // BDD node 129:vector_return(vector:(w64 1246614245376), index:(w32 30), value:(w64 1240008228864)[(w16 31)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_30(2);
  vector_table_1246614245376_value_30.set(0, 2, 31);
  state->vector_table_1246614245376.write(30, vector_table_1246614245376_value_30);
  // BDD node 130:vector_borrow(vector:(w64 1248761733120), index:(w32 31), val_out:(w64 1649988861952)[ -> (w64 1239249059840)])
  // Module Ignore
  // BDD node 131:vector_return(vector:(w64 1248761733120), index:(w32 31), value:(w64 1239249059840)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1248761733120_value_31(4);
  vector_table_1248761733120_value_31.set(0, 4, 0);
  state->vector_table_1248761733120.write(31, vector_table_1248761733120_value_31);
  // BDD node 132:vector_borrow(vector:(w64 1246614245376), index:(w32 31), val_out:(w64 1649812701184)[ -> (w64 1239270031360)])
  // Module Ignore
  // BDD node 133:vector_return(vector:(w64 1246614245376), index:(w32 31), value:(w64 1239270031360)[(w16 30)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246614245376_value_31(2);
  vector_table_1246614245376_value_31.set(0, 2, 30);
  state->vector_table_1246614245376.write(31, vector_table_1246614245376_value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 time; // The switch's clock at the hand-off, ingress_mac_tstamp[47:16].
  u32 cached_insert_success;
  u32 min_estimate__147;
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
    // EP node  3259
    // BDD node 266:tofino_force_send_to_controller()
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  3260
    // BDD node 266:tofino_force_send_to_controller()
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  3261
    // BDD node 266:tofino_force_send_to_controller()
    u8* hdr_2 = packet_consume(pkt, 4);
    // EP node  3262
    // BDD node 266:tofino_force_send_to_controller()
    buffer_t value_0;
    state->vector_table_1248761733120.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 0xffffull), value_0);
    // EP node  3263
    // BDD node 266:tofino_force_send_to_controller()
    if ((0) == ((u32)value_0.get(0, 4))) {
      // EP node  3264
      // BDD node 266:tofino_force_send_to_controller()
      // EP node  3267
      // BDD node 266:tofino_force_send_to_controller()
      buffer_t fcfs_cs_1249835474944_key_0(12);
      fcfs_cs_1249835474944_key_0[0] = *(u8*)(hdr_1 + 12);
      fcfs_cs_1249835474944_key_0[1] = *(u8*)(hdr_1 + 13);
      fcfs_cs_1249835474944_key_0[2] = *(u8*)(hdr_1 + 14);
      fcfs_cs_1249835474944_key_0[3] = *(u8*)(hdr_1 + 15);
      fcfs_cs_1249835474944_key_0[4] = *(u8*)(hdr_1 + 16);
      fcfs_cs_1249835474944_key_0[5] = *(u8*)(hdr_1 + 17);
      fcfs_cs_1249835474944_key_0[6] = *(u8*)(hdr_1 + 18);
      fcfs_cs_1249835474944_key_0[7] = *(u8*)(hdr_1 + 19);
      fcfs_cs_1249835474944_key_0[8] = *(u8*)(hdr_2 + 0);
      fcfs_cs_1249835474944_key_0[9] = *(u8*)(hdr_2 + 1);
      fcfs_cs_1249835474944_key_0[10] = *(u8*)(hdr_2 + 2);
      fcfs_cs_1249835474944_key_0[11] = *(u8*)(hdr_2 + 3);
      bool found_0 = state->fcfs_cs_1249835474944.get(fcfs_cs_1249835474944_key_0);
      // EP node  3268
      // BDD node 266:tofino_force_send_to_controller()
      if ((0) == (found_0)) {
        // EP node  3269
        // BDD node 266:tofino_force_send_to_controller()
        // EP node  3272
        // BDD node 266:tofino_force_send_to_controller()
        if ((bswap32(cpu_hdr_extra->min_estimate__147)) <= (131071)) {
          // EP node  3273
          // BDD node 266:tofino_force_send_to_controller()
          // EP node  3276
          // BDD node 266:tofino_force_send_to_controller()
          if ((bswap32(cpu_hdr_extra->cached_insert_success)) != (0)) {
            // EP node  3277
            // BDD node 266:tofino_force_send_to_controller()
            // EP node  3279
            // BDD node 266:tofino_force_send_to_controller()
            result.abort_transaction = true;
            cpu_hdr->trigger_dataplane_execution = 1;
            return result;
          } else {
            // EP node  3278
            // BDD node 266:tofino_force_send_to_controller()
            // EP node  4837
            // BDD node 251:dchain_allocate_new_index(chain:(w64 1241073582080), index_out:(w64 1649594597376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__251)], time:(ReadLSB w64 (w32 0) next_time))
            buffer_t fcfs_cs_1249835474944_key_1(12);
            fcfs_cs_1249835474944_key_1[0] = *(u8*)(hdr_1 + 12);
            fcfs_cs_1249835474944_key_1[1] = *(u8*)(hdr_1 + 13);
            fcfs_cs_1249835474944_key_1[2] = *(u8*)(hdr_1 + 14);
            fcfs_cs_1249835474944_key_1[3] = *(u8*)(hdr_1 + 15);
            fcfs_cs_1249835474944_key_1[4] = *(u8*)(hdr_1 + 16);
            fcfs_cs_1249835474944_key_1[5] = *(u8*)(hdr_1 + 17);
            fcfs_cs_1249835474944_key_1[6] = *(u8*)(hdr_1 + 18);
            fcfs_cs_1249835474944_key_1[7] = *(u8*)(hdr_1 + 19);
            fcfs_cs_1249835474944_key_1[8] = *(u8*)(hdr_2 + 0);
            fcfs_cs_1249835474944_key_1[9] = *(u8*)(hdr_2 + 1);
            fcfs_cs_1249835474944_key_1[10] = *(u8*)(hdr_2 + 2);
            fcfs_cs_1249835474944_key_1[11] = *(u8*)(hdr_2 + 3);
            bool success_0 = state->fcfs_cs_1249835474944.put(fcfs_cs_1249835474944_key_1);
            // EP node  4902
            // BDD node 252:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__251))
            if ((0) == (success_0)) {
              // EP node  4903
              // BDD node 252:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__251))
              // EP node  5390
              // BDD node 253:vector_borrow(vector:(w64 1246614245376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1650022416384)[ -> (w64 1240134057984)])
              buffer_t value_1;
              state->vector_table_1246614245376.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 0xffffull), value_1);
              // EP node  5462
              // BDD node 254:vector_return(vector:(w64 1246614245376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1240134057984)[(ReadLSB w16 (w32 0) vector_data__151)])
              // EP node  5827
              // BDD node 258:FORWARD
              cpu_hdr->egress_dev = bswap16((u16)value_1.get(0, 2));
            } else {
              // EP node  4904
              // BDD node 252:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__251))
              // EP node  4970
              // BDD node 260:vector_borrow(vector:(w64 1246614245376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1650022416384)[ -> (w64 1240134057984)])
              buffer_t value_2;
              state->vector_table_1246614245376.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 0xffffull), value_2);
              // EP node  5039
              // BDD node 261:vector_return(vector:(w64 1246614245376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1240134057984)[(ReadLSB w16 (w32 0) vector_data__160)])
              // EP node  5389
              // BDD node 265:FORWARD
              cpu_hdr->egress_dev = bswap16((u16)value_2.get(0, 2));
            }
          }
        } else {
          // EP node  3274
          // BDD node 266:tofino_force_send_to_controller()
          // EP node  3275
          // BDD node 266:tofino_force_send_to_controller()
          result.abort_transaction = true;
          cpu_hdr->trigger_dataplane_execution = 1;
          return result;
        }
      } else {
        // EP node  3270
        // BDD node 266:tofino_force_send_to_controller()
        // EP node  3271
        // BDD node 266:tofino_force_send_to_controller()
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      }
    } else {
      // EP node  3265
      // BDD node 266:tofino_force_send_to_controller()
      // EP node  3266
      // BDD node 266:tofino_force_send_to_controller()
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
